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
#include "NoisePlugin.h"

namespace nativeformat {
namespace plugin {
namespace noise {

NoisePlugin::NoisePlugin(const nfgrapher::contract::NoiseNodeInfo &noise_node,
                         int channels, double samplerate)
    : _distribution(-1.0f, 1.0f),
      _start_sample_index(nanosToFrames(samplerate, noise_node._when) *
                          channels),
      _duration_samples(nanosToFrames(samplerate, noise_node._duration) *
                        channels) {}

NoisePlugin::~NoisePlugin() {}

void NoisePlugin::feed(std::map<std::string, std::shared_ptr<Content>> &content,
                       long sample_index, long graph_sample_index,
                       nfgrapher::LoadingPolicy loading_policy) {
  Content &audio_content = *content[AudioContentTypeKey];
  size_t sample_count = audio_content.requiredItems();
  float *samples = (float *)audio_content.payload();
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;
  long end_sample_index = start_sample_index + duration_samples;
  long sample_index_end = sample_index + sample_count;
  long sample_count_index_begin = 0;
  long sample_count_index_end = 0;
  if (!findBeginAndEndOfRange(
          start_sample_index, end_sample_index, sample_index, sample_index_end,
          sample_count_index_begin, sample_count_index_end)) {
    return;
  }
  sample_count_index_begin -= sample_index;
  for (long i = sample_count_index_begin; i < sample_count_index_end; ++i) {
    float sample = _distribution(_generator);
    samples[i] = std::min(std::max(sample, -1.0f), 1.0f);
  }
  audio_content.setItems(sample_count_index_end - sample_count_index_begin);
}

std::string NoisePlugin::name() {
  return nfgrapher::contract::NoiseNodeInfo::kind();
}

bool NoisePlugin::finished(long sample_index, long sample_index_end) {
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;
  return !rangesOverlap(sample_index, sample_index_end, start_sample_index,
                        start_sample_index + duration_samples);
}

Plugin::PluginType NoisePlugin::type() const {
  return Plugin::PluginTypeProducer;
}

long NoisePlugin::localRenderSampleIndex(long sample_index) {
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;
  long end_sample_index = start_sample_index + duration_samples;
  if (rangesOverlap(start_sample_index, end_sample_index, sample_index,
                    sample_index)) {
    return sample_index - start_sample_index;
  }
  if (sample_index >= end_sample_index) {
    return duration_samples;
  }
  return 0;
}

long NoisePlugin::startSampleIndex() { return _start_sample_index; }

void NoisePlugin::load(LOAD_CALLBACK callback) { callback(Load{true}); }

bool NoisePlugin::loaded() const { return true; }

void NoisePlugin::run(long sample_index, const NodeTimes &node_times,
                      long node_sample_index) {}

bool NoisePlugin::shouldProcess(long sample_index_start,
                                long sample_index_end) {
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;
  return rangesOverlap(sample_index_start, sample_index_end, start_sample_index,
                       start_sample_index + duration_samples);
}

}  // namespace noise
}  // namespace plugin
}  // namespace nativeformat
