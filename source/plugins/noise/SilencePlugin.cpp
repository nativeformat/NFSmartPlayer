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
#include "SilencePlugin.h"

namespace nativeformat {
namespace plugin {
namespace noise {

SilencePlugin::SilencePlugin(
    const nfgrapher::contract::SilenceNodeInfo &silence_node, int channels,
    double samplerate)
    : _start_sample_index(nanosToFrames(samplerate, silence_node._when) *
                          channels),
      _duration_samples(nanosToFrames(samplerate, silence_node._duration) *
                        channels) {}

SilencePlugin::~SilencePlugin() {}

void SilencePlugin::feed(
    std::map<std::string, std::shared_ptr<Content>> &content, long sample_index,
    long graph_sample_index, nfgrapher::LoadingPolicy loading_policy) {
  Content &audio_content = *content[AudioContentTypeKey];
  size_t sample_count = audio_content.requiredItems();
  float *samples = (float *)audio_content.payload();
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;
  long end_sample_index = start_sample_index + duration_samples;
  long sample_index_end = sample_index + sample_count;
  if (!rangesOverlap(start_sample_index, end_sample_index, sample_index,
                     sample_index_end)) {
    return;
  }
  size_t sample_begin = std::max(start_sample_index, sample_index);
  size_t sample_end = std::min(end_sample_index, sample_index_end);
  size_t write_samples = sample_end - sample_begin;
  std::fill_n(&samples[sample_begin - sample_index], write_samples, 0);

  audio_content.setItems(write_samples);
}

std::string SilencePlugin::name() {
  return nfgrapher::contract::SilenceNodeInfo::kind();
}

bool SilencePlugin::finished(long sample_index, long sample_index_end) {
  long start_sample_index = _start_sample_index;
  return !rangesOverlap(start_sample_index,
                        start_sample_index + _duration_samples, sample_index,
                        sample_index_end);
}

Plugin::PluginType SilencePlugin::type() const {
  return Plugin::PluginTypeProducer;
}

long SilencePlugin::localRenderSampleIndex(long sample_index) {
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;
  long end_sample_index = start_sample_index + duration_samples;
  if (rangesOverlap(sample_index, sample_index, start_sample_index,
                    end_sample_index)) {
    return sample_index - start_sample_index;
  }
  if (sample_index >= end_sample_index) {
    return duration_samples;
  }
  return 0;
}

long SilencePlugin::startSampleIndex() { return _start_sample_index; }

void SilencePlugin::load(LOAD_CALLBACK callback) { callback(Load{true}); }

bool SilencePlugin::loaded() const { return true; }

void SilencePlugin::run(long sample_index, const NodeTimes &node_times,
                        long node_sample_index) {}

bool SilencePlugin::shouldProcess(long sample_index_start,
                                  long sample_index_end) {
  long start_sample_index = _start_sample_index;
  return rangesOverlap(start_sample_index,
                       start_sample_index + _duration_samples,
                       sample_index_start, sample_index_end);
}

}  // namespace noise
}  // namespace plugin
}  // namespace nativeformat
