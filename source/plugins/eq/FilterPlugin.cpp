/*
 * Copyright (c) 2018 Spotify AB.
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
#include "FilterPlugin.h"

namespace nativeformat {
namespace plugin {
namespace eq {

FilterPlugin::FilterPlugin(const nfgrapher::contract::FilterNodeInfo &node,
                           int channels, double samplerate,
                           const std::shared_ptr<plugin::Plugin> &child_plugin)
    : _child_plugin(child_plugin),
      _samplerate(samplerate),
      _low_cutoff(param::createParam(node._low_cutoff._initial_val,
                                     samplerate / 2.0, 0.0f, "lowCutoff")),
      _high_cutoff(param::createParam(node._high_cutoff._initial_val,
                                      samplerate / 2.0, 0.0f, "highCutoff")),
      _last_low_cutoff(0),
      _last_high_cutoff(0),
      _filter(2, util::FilterType::BandPassFilter, channels) {
  nfgrapher::param::addCommands(_low_cutoff, node._low_cutoff);
  nfgrapher::param::addCommands(_high_cutoff, node._high_cutoff);
  _filter.compute(normalisedFreq(_low_cutoff->valueForTime(0)),
                  normalisedFreq(_high_cutoff->valueForTime(0)));
}

FilterPlugin::~FilterPlugin() {}

void FilterPlugin::feed(
    std::map<std::string, std::shared_ptr<Content>> &content, long sample_index,
    long graph_sample_index, nfgrapher::LoadingPolicy loading_policy) {
  _child_plugin->feed(content, sample_index, graph_sample_index,
                      loading_policy);
  Content &audio_content = *content[AudioContentTypeKey];
  size_t sample_count = audio_content.items();
  size_t channels = audio_content.channels();
  size_t frame_count = sample_count / channels;
  double sample_rate = audio_content.sampleRate();
  float *samples = (float *)audio_content.payload();
  double time = (sample_index / sample_rate) / static_cast<double>(channels);
  double end_time = time + ((static_cast<float>(sample_count) / sample_rate) /
                            static_cast<double>(channels));

  // get the new values for the filter based on the current time
  float low_cutoff = _low_cutoff->smoothedValueForTimeRange(time, end_time);
  float high_cutoff = _high_cutoff->smoothedValueForTimeRange(time, end_time);

  // recalculate filter coefficients if necessary
  if (low_cutoff != _last_low_cutoff || high_cutoff != _last_high_cutoff) {
    _filter.compute(normalisedFreq(low_cutoff), normalisedFreq(high_cutoff));
    _last_low_cutoff = low_cutoff;
    _last_high_cutoff = high_cutoff;
  }

  // perform filtering
  _filter.filter(samples, frame_count);
}

std::string FilterPlugin::name() {
  return nfgrapher::contract::FilterNodeInfo::kind();
}

std::vector<std::string> FilterPlugin::paramNames() {
  return {_low_cutoff->name(), _high_cutoff->name()};
}

std::shared_ptr<param::Param> FilterPlugin::paramForName(
    const std::string &name) {
  if (name == _low_cutoff->name()) {
    return _low_cutoff;
  }
  if (name == _high_cutoff->name()) {
    return _high_cutoff;
  }
  return nullptr;
}

bool FilterPlugin::finished(long sample_index, long sample_index_end) {
  return _child_plugin->finished(sample_index, sample_index_end);
}

void FilterPlugin::notifyFinished(long sample_index, long graph_sample_index) {
  return _child_plugin->notifyFinished(sample_index, graph_sample_index);
}

Plugin::PluginType FilterPlugin::type() const { return PluginTypeConsumer; }

void FilterPlugin::load(LOAD_CALLBACK callback) {
  _child_plugin->load(callback);
}

bool FilterPlugin::loaded() const { return _child_plugin->loaded(); }

void FilterPlugin::run(long sample_index, const NodeTimes &node_times,
                       long node_sample_index) {
  _child_plugin->run(sample_index, node_times, node_sample_index);
}

bool FilterPlugin::shouldProcess(long sample_index_start,
                                 long sample_index_end) {
  return _child_plugin->shouldProcess(sample_index_start, sample_index_end);
}

double FilterPlugin::normalisedFreq(float freq_hz) {
  return freq_hz / _samplerate;
}

}  // namespace eq
}  // namespace plugin
}  // namespace nativeformat
