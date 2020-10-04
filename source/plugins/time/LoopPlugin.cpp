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
#include "LoopPlugin.h"

#include <cmath>

namespace nativeformat {
namespace plugin {
namespace time {

LoopPlugin::LoopPlugin(const nfgrapher::contract::LoopNodeInfo &loop_node,
                       int channels, double samplerate,
                       const std::shared_ptr<plugin::Plugin> &child_plugin)
    : _child_plugin(child_plugin),
      _start_sample_index(nanosToFrames(samplerate, loop_node._when) *
                          channels),
      _duration_samples(nanosToFrames(samplerate, loop_node._duration) *
                        channels),
      _loops(loop_node._loop_count) {}

LoopPlugin::~LoopPlugin() {}

void LoopPlugin::feed(std::map<std::string, std::shared_ptr<Content>> &content,
                      long sample_index, long graph_sample_index,
                      nfgrapher::LoadingPolicy loading_policy) {
  _child_plugin->feed(content, timeDilation(sample_index), graph_sample_index,
                      loading_policy);
  Content &audio_content = *content[AudioContentTypeKey];
  size_t sample_count = audio_content.requiredItems();

  int loops = _loops;
  long start_sample_index = _start_sample_index;
  long duration_samples = _duration_samples;

  long dilated_sample_index = timeDilation(sample_index);
  long dilated_end_sample_index = dilated_sample_index + sample_count;
  long dilated_end_loop_index = start_sample_index + duration_samples;
  long undilated_end_loop_index = start_sample_index + duration_samples * loops;

  // If we're in the loop, make sure not to go past end_loop_index
  bool in_loop = rangesOverlap(sample_index, sample_index, start_sample_index,
                               undilated_end_loop_index);
  if (in_loop && dilated_end_sample_index > dilated_end_loop_index) {
    long extra_samples = dilated_end_sample_index - dilated_end_loop_index;
    long new_sample_count = sample_count - extra_samples;
    audio_content.setItems(new_sample_count);
  }
}

std::string LoopPlugin::name() {
  return nfgrapher::contract::LoopNodeInfo::kind();
}

long LoopPlugin::timeDilation(long sample_index) {
  long start_sample_index = _start_sample_index;

  // Before the loops start
  if (sample_index < start_sample_index) {
    return sample_index;
  }
  int loops = _loops;
  long duration_samples = _duration_samples;
  if (loops > 0) {
    long total_looped_samples = duration_samples * loops;
    long end_sample_index = start_sample_index + total_looped_samples;
    // After the loops end
    if (sample_index >= end_sample_index) {
      long extra_samples = total_looped_samples - duration_samples;
      return sample_index - extra_samples;
    }
  }
  return ((sample_index - start_sample_index) % duration_samples) +
         start_sample_index;
}

bool LoopPlugin::finished(long sample_index, long sample_index_end) {
  long start_sample_index = _start_sample_index;
  int loops = _loops;
  if (loops <= 0) {
    return sample_index_end < start_sample_index;
  }
  long end_sample_index = start_sample_index + (_duration_samples * loops);
  if (!rangesOverlap(sample_index, sample_index_end, start_sample_index,
                     end_sample_index)) {
    return _child_plugin->finished(timeDilation(sample_index),
                                   timeDilation(sample_index_end));
  }
  return false;
}

void LoopPlugin::notifyFinished(long sample_index, long graph_sample_index) {
  return _child_plugin->notifyFinished(timeDilation(sample_index),
                                       graph_sample_index);
}

Plugin::PluginType LoopPlugin::type() const {
  return Plugin::PluginTypeProducerConsumer;
}

long LoopPlugin::localRenderSampleIndex(long sample_index) {
  long start_sample_index = _start_sample_index;
  if (sample_index < start_sample_index) {
    return 0;
  }
  int loops = _loops;
  if (loops > 0) {
    long duration_samples = _duration_samples;
    long end_sample_index = start_sample_index + (loops * duration_samples);
    if (sample_index >= end_sample_index) {
      return end_sample_index - start_sample_index;
    }
  }
  return sample_index - start_sample_index;
}

long LoopPlugin::startSampleIndex() { return _start_sample_index; }

void LoopPlugin::load(LOAD_CALLBACK callback) { _child_plugin->load(callback); }

bool LoopPlugin::loaded() const { return _child_plugin->loaded(); }

void LoopPlugin::run(long sample_index, const NodeTimes &node_times,
                     long node_sample_index) {
  _child_plugin->run(timeDilation(sample_index), node_times, node_sample_index);
}

bool LoopPlugin::shouldProcess(long sample_index_start, long sample_index_end) {
  auto dilated_sample_index = timeDilation(sample_index_start);
  return _child_plugin->shouldProcess(
      dilated_sample_index,
      dilated_sample_index + (sample_index_end - sample_index_start));
}

}  // namespace time
}  // namespace plugin
}  // namespace nativeformat
