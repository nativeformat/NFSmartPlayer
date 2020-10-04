/*
 * Copyright (c) 2017 Spotify AB.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "CompanderPlugin.h"

#include "Knee.h"

namespace nativeformat {
namespace plugin {
namespace compressor {

const std::string CompanderPluginIdentifier(
    "com.nativeformat.plugin.compressor.compander");

constexpr char CompanderPlugin::SidechainContentTypeKey[];

CompanderPlugin::CompanderPlugin(
    const NodeInfo &grapher_node, int channels, double samplerate,
    const std::shared_ptr<plugin::Plugin> &child_plugin)
    : _channels(channels),
      _samplerate(samplerate),
      _child_plugin(child_plugin),
      _attack(param::createParam(grapher_node._attack.getInitialVal(), 1, 0,
                                 "attack")),
      _release(param::createParam(grapher_node._release.getInitialVal(), 1, 0,
                                  "release")),
      _compressor_params({
          param::createParam(
              grapher_node._compressor_threshold_db.getInitialVal(), 0, -100,
              "compressor_threshold_db"),
          param::createParam(grapher_node._compressor_knee_db.getInitialVal(),
                             40, 0, "compressor_knee_db"),
          param::createParam(grapher_node._compressor_ratio_db.getInitialVal(),
                             20, 1, "compressor_ratio_db"),
      }),
      _expander_params({
          param::createParam(
              grapher_node._expander_threshold_db.getInitialVal(), 0, -100,
              "expander_threshold_db"),
          param::createParam(grapher_node._expander_knee_db.getInitialVal(), 40,
                             0, "expander_knee_db"),
          param::createParam(grapher_node._expander_ratio_db.getInitialVal(),
                             20, 1, "expander_ratio_db"),
      }),
      _splitter(nullptr),
      _payload_size(0) {
  nfgrapher::param::addCommands(_attack, grapher_node._attack);
  nfgrapher::param::addCommands(_release, grapher_node._release);
  nfgrapher::param::addCommands(_compressor_params._threshold_db,
                                grapher_node._compressor_threshold_db);
  nfgrapher::param::addCommands(_compressor_params._knee_db,
                                grapher_node._compressor_knee_db);
  nfgrapher::param::addCommands(_compressor_params._ratio_db,
                                grapher_node._compressor_ratio_db);
  nfgrapher::param::addCommands(_expander_params._threshold_db,
                                grapher_node._expander_threshold_db);
  nfgrapher::param::addCommands(_expander_params._knee_db,
                                grapher_node._expander_knee_db);
  nfgrapher::param::addCommands(_expander_params._ratio_db,
                                grapher_node._expander_ratio_db);
  // TODO: Replace with unordered_map in c++14
  // OR extract hard and soft into classes and template this off of
  // those classes
  size_t bands = grapher_node._cutoffs.size() + 1;
  if (grapher_node._cutoffs.size()) {
    _splitter.reset(
        new util::BandSplitter(grapher_node._cutoffs, samplerate, channels));
    _content.resize(bands);
  }

  // create the compressor
  std::unique_ptr<Detector> compressor_detector, expander_detector;
  if (grapher_node._detection_mode == DetectionMode::rms) {
    compressor_detector.reset(new CompressorRmsDetector());
    expander_detector.reset(new ExpanderRmsDetector());
  } else {  // "max"
    compressor_detector.reset(new CompressorPeakDetector());
    expander_detector.reset(new ExpanderPeakDetector());
  }

  knee_fun compressor_knee, expander_knee;
  if (grapher_node._knee_mode == NodeInfo::KneeMode::soft) {
    compressor_knee = SoftKnee<CompressorType>();
    expander_knee = SoftKnee<ExpanderType>();
  } else {
    compressor_knee = HardKnee<CompressorType>();
    expander_knee = HardKnee<ExpanderType>();
  }

  Compander::Metadata compressor_meta = {_compressor_params, compressor_knee,
                                         std::move(compressor_detector)};
  Compander::Metadata expander_meta = {_expander_params, expander_knee,
                                       std::move(expander_detector)};
  std::vector<Compander::Metadata> chains;
  chains.push_back(compressor_meta);
  chains.push_back(expander_meta);

  _companders.resize(bands, Compander(grapher_node._detection_mode, chains,
                                      _channels, _samplerate));
}

CompanderPlugin::~CompanderPlugin() {}

void CompanderPlugin::feed(
    std::map<std::string, std::shared_ptr<Content>> &content, long sample_index,
    long graph_sample_index, nfgrapher::LoadingPolicy loading_policy) {
  _child_plugin->feed(content, sample_index, graph_sample_index,
                      loading_policy);
  Content &audio_content = *content[AudioContentTypeKey];
  size_t sample_count = audio_content.items();
  std::string sidechain_key =
      content.find(SidechainContentTypeKey) == content.end()
          ? AudioContentTypeKey
          : SidechainContentTypeKey;

  Content &sidechain_content = *content[sidechain_key];
  size_t sidechain_sample_count = sidechain_content.items();

  // Output zeros if sidechain signal starts before audio
  if (!sample_count) {
    if (sidechain_sample_count) {
      size_t zero_count =
          std::max(audio_content.requiredItems(), sidechain_sample_count);
      std::fill_n(audio_content.payload(), zero_count, 0.0f);
      audio_content.setItems(zero_count);
    }
    return;
  }

  size_t bands = 1;
  if (_splitter) {
    bands = _splitter->bands();

    split_bands(sample_count, audio_content, _content);
    if (AudioContentTypeKey != sidechain_key)
      split_bands(sidechain_sample_count, sidechain_content,
                  _sidechain_content);
  }

  for (size_t band_index = 0; band_index < bands; ++band_index) {
    sample_t *samples = nullptr;
    sample_t *sidechain_samples = nullptr;
    if (bands == 1) {
      samples = static_cast<sample_t *>(audio_content.payload());
      sidechain_samples = static_cast<sample_t *>(sidechain_content.payload());
    } else {
      samples = static_cast<sample_t *>(_content.at(band_index)->payload());
      sidechain_samples = static_cast<sample_t *>(
          (AudioContentTypeKey == sidechain_key ? _content : _sidechain_content)
              .at(band_index)
              ->payload());
    }
    size_t frame_count = sidechain_sample_count / _channels;
    double time = sample_index / (_samplerate * _channels);
    double end_time = time + (frame_count / _samplerate);
    float current_attack = _attack->smoothedValueForTimeRange(time, end_time);
    float current_release = _release->smoothedValueForTimeRange(time, end_time);
    _companders[band_index].compand(current_attack, current_release, time,
                                    end_time, samples, sample_count,
                                    sidechain_samples, sidechain_sample_count);
  }

  // Finally, if there are multiple bands, mix them back together
  // Don't bother mixing the sidechain back up together obviously
  for (size_t band_index = 1; band_index < bands; ++band_index) {
    _content[band_index]->mix(*_content[band_index - 1]);
  }
  if (bands > 1) {
    std::copy_n(_content[bands - 1]->payload(), _payload_size,
                audio_content.payload());
  }
}

std::string CompanderPlugin::name() { return CompanderPluginIdentifier; }

bool CompanderPlugin::finished(long sample_index, long sample_index_end) {
  return _child_plugin->finished(sample_index, sample_index_end);
}

void CompanderPlugin::notifyFinished(long sample_index,
                                     long graph_sample_index) {
  return _child_plugin->notifyFinished(sample_index, graph_sample_index);
}

Plugin::PluginType CompanderPlugin::type() const {
  return Plugin::PluginTypeConsumer;
}

void CompanderPlugin::load(LOAD_CALLBACK callback) {
  _child_plugin->load(callback);
}

bool CompanderPlugin::loaded() const { return _child_plugin->loaded(); }

void CompanderPlugin::run(long sample_index, const NodeTimes &node_times,
                          long node_sample_index) {
  _child_plugin->run(sample_index, node_times, node_sample_index);
}

bool CompanderPlugin::shouldProcess(long sample_index_start,
                                    long sample_index_end) {
  return _child_plugin->shouldProcess(sample_index_start, sample_index_end);
}

void CompanderPlugin::split_bands(
    size_t sample_count, Content &content,
    std::vector<std::unique_ptr<plugin::Content>> &local_content) {
  // Initialize multiband Content if necessary
  if (content.payloadSize() > _payload_size) {
    _payload_size = content.payloadSize();
    for (auto &cptr : local_content) {
      cptr.reset(new plugin::Content(_payload_size, 0, _samplerate, _channels,
                                     content.requiredItems(), content.type()));
    }
  }

  // Set up vector of sample pointers for band splitter
  std::vector<sample_t *> band_samples;
  sample_t *samples = content.payload();
  std::copy_n(samples, _payload_size, local_content[0]->payload());
  for (auto &cptr : local_content) {
    band_samples.push_back(static_cast<sample_t *>(cptr->payload()));
    cptr->setItems(content.items());
  }

  // Split!
  _splitter->filter(band_samples.data(), sample_count / _channels);
}

}  // namespace compressor
}  // namespace plugin
}  // namespace nativeformat
