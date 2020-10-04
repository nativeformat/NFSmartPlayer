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
#include "SineWavePlugin.h"

#include <cmath>

namespace nativeformat {
namespace plugin {
namespace wave {

SineWavePlugin::SineWavePlugin(
    const nfgrapher::contract::SineNodeInfo &sine_node, int channels,
    double samplerate)
    : _frequency(sine_node._frequency),
      _start_sample_index(nanosToFrames(samplerate, sine_node._when) *
                          channels),
      _duration_samples(nanosToFrames(samplerate, sine_node._duration) *
                        channels) {}

SineWavePlugin::~SineWavePlugin() {}

void SineWavePlugin::feed(
    std::map<std::string, std::shared_ptr<Content>> &content, long sample_index,
    long graph_sample_index, nfgrapher::LoadingPolicy loading_policy) {
  Content &audio_content = *content[AudioContentTypeKey];
  size_t sample_count = audio_content.requiredItems();
  size_t channels = audio_content.channels();
  double sample_rate = audio_content.sampleRate();
  float *samples = (float *)audio_content.payload();
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;
  long end_sample_index = start_sample_index + duration_samples;
  long sample_index_end = sample_index + sample_count;
  if (!rangesOverlap(start_sample_index, end_sample_index, sample_index,
                     sample_index_end)) {
    return;
  }

  size_t start_sample =
      std::max(start_sample_index, sample_index) - sample_index;
  size_t end_sample =
      std::min(end_sample_index, sample_index_end) - sample_index;
  size_t write_samples = end_sample - start_sample;
  size_t samples_rendered = std::max(sample_index - start_sample_index, 0l);
  double t = samples_rendered / (sample_rate * channels);
  const double inc = 1.0 / sample_rate;
  const double two_pi_f = 2.0 * M_PI * _frequency;
  for (size_t i = start_sample; i < end_sample; t += inc, i += channels) {
    float sample = sinf(two_pi_f * t);
    for (size_t j = 0; j < channels; ++j) {
      samples[i + j] = sample;
    }
  }
  audio_content.setItems(write_samples);
  return;
}

std::string SineWavePlugin::name() {
  return nfgrapher::contract::SineNodeInfo::kind();
}

bool SineWavePlugin::finished(long sample_index, long sample_index_end) {
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;
  long end_sample_index = start_sample_index + duration_samples;
  return !rangesOverlap(start_sample_index, end_sample_index, sample_index,
                        sample_index_end);
}

Plugin::PluginType SineWavePlugin::type() const {
  return Plugin::PluginTypeProducer;
}

long SineWavePlugin::localRenderSampleIndex(long sample_index) {
  long start_sample_index = _start_sample_index;
  if (sample_index < start_sample_index) {
    return 0;
  }
  long duration_samples = _duration_samples;
  long end_sample_index = start_sample_index + duration_samples;
  if (sample_index >= end_sample_index) {
    return duration_samples;
  }
  return sample_index - start_sample_index;
}

long SineWavePlugin::startSampleIndex() { return _start_sample_index; }

void SineWavePlugin::load(LOAD_CALLBACK callback) { callback(Load{true}); }

bool SineWavePlugin::loaded() const { return true; }

void SineWavePlugin::run(long sample_index, const NodeTimes &node_times,
                         long node_sample_index) {}

bool SineWavePlugin::shouldProcess(long sample_index_start,
                                   long sample_index_end) {
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;
  long end_sample_index = start_sample_index + duration_samples;
  return rangesOverlap(start_sample_index, end_sample_index, sample_index_start,
                       sample_index_end);
}

}  // namespace wave
}  // namespace plugin
}  // namespace nativeformat
