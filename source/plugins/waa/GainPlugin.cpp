/*
 * Copyright (c) 2016 Spotify AB.
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
#include "GainPlugin.h"

namespace nativeformat {
namespace plugin {
namespace waa {

static constexpr double AMP_MIN_GAIN = 0.0f;
static constexpr double AMP_MAX_GAIN = 16.0f;

GainPlugin::GainPlugin(const nfgrapher::contract::GainNodeInfo &gain_node,
                       int channels, double samplerate,
                       const std::shared_ptr<plugin::Plugin> &child_plugin)
    : _child_plugin(child_plugin),
      _gain(param::createParam(gain_node._gain._initial_val, AMP_MAX_GAIN,
                               AMP_MIN_GAIN, "gain")) {
  nfgrapher::param::addCommands(_gain, gain_node._gain);
}

GainPlugin::~GainPlugin() {}

void GainPlugin::feed(std::map<std::string, std::shared_ptr<Content>> &content,
                      long sample_index, long graph_sample_index,
                      nfgrapher::LoadingPolicy loading_policy) {
  _child_plugin->feed(content, sample_index, graph_sample_index,
                      loading_policy);
  Content &audio_content = *content[AudioContentTypeKey];
  float *samples = (float *)audio_content.payload();
  size_t sample_count = audio_content.items();
  double samplerate = audio_content.sampleRate();
  size_t channels = audio_content.channels();
  long frame_count = sample_count / channels;
  long frame_index = sample_index / channels;
  double time = frame_index / samplerate;
  double end_time = time + (frame_count / samplerate);
  if (_gain_values_buffer.size() < frame_count) {
    _gain_values_buffer.insert(_gain_values_buffer.end(),
                               frame_count - _gain_values_buffer.size(), 1.0f);
  }
  _gain->valuesForTimeRange(&_gain_values_buffer[0], frame_count, time,
                            end_time);
  for (long i = 0; i < frame_count; ++i) {
    float gain_value = _gain_values_buffer[i];
    for (size_t j = 0; j < channels; ++j) {
      samples[(i * channels) + j] *= gain_value;
    }
  }
}

std::string GainPlugin::name() {
  return nfgrapher::contract::GainNodeInfo::kind();
}

std::vector<std::string> GainPlugin::paramNames() { return {_gain->name()}; }

std::shared_ptr<param::Param> GainPlugin::paramForName(
    const std::string &name) {
  if (name == _gain->name()) {
    return _gain;
  }
  return nullptr;
}

bool GainPlugin::finished(long sample_index, long sample_index_end) {
  return _child_plugin->finished(sample_index, sample_index_end);
}

void GainPlugin::notifyFinished(long sample_index, long graph_sample_index) {
  return _child_plugin->notifyFinished(sample_index, graph_sample_index);
}

Plugin::PluginType GainPlugin::type() const {
  return PluginType::PluginTypeConsumer;
}

void GainPlugin::load(LOAD_CALLBACK callback) { _child_plugin->load(callback); }

bool GainPlugin::loaded() const { return _child_plugin->loaded(); }

void GainPlugin::run(long sample_index, const NodeTimes &node_times,
                     long node_sample_index) {
  _child_plugin->run(sample_index, node_times, node_sample_index);
}

bool GainPlugin::shouldProcess(long sample_index_start, long sample_index_end) {
  return _child_plugin->shouldProcess(sample_index_start, sample_index_end);
}

}  // namespace waa
}  // namespace plugin
}  // namespace nativeformat
