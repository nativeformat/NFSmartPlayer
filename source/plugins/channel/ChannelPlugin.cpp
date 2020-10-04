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
#include "ChannelPlugin.h"

namespace nativeformat {
namespace plugin {
namespace channel {

const std::string ChannelPluginIdentifier(
    "com.nativeformat.plugin.channel.channel");

ChannelPlugin::ChannelPlugin(
    const nfgrapher::Node &grapher_node, int channels, double samplerate,
    const std::shared_ptr<plugin::Plugin> &child_plugin)
    : _child_plugin(child_plugin) {}

ChannelPlugin::~ChannelPlugin() {}

void ChannelPlugin::feed(
    std::map<std::string, std::shared_ptr<Content>> &content, long sample_index,
    long graph_sample_index, nfgrapher::LoadingPolicy loading_policy) {
  _child_plugin->feed(content, sample_index, graph_sample_index,
                      loading_policy);
  Content &audio_content = *content[AudioContentTypeKey];
  float *samples = (float *)audio_content.payload();
  size_t channels = audio_content.channels();
  size_t sample_count = audio_content.items();
  size_t frame_count = sample_count / channels;
  for (int i = 0; i < channels; ++i) {
    auto channel_iterator = std::find(_channels.begin(), _channels.end(), i);
    if (channel_iterator != _channels.end()) {
      continue;
    }
    for (int j = 0; j < frame_count; ++j) {
      samples[(j * channels) + i] = 0.0f;
    }
  }
}

std::string ChannelPlugin::name() { return ChannelPluginIdentifier; }

bool ChannelPlugin::finished(long sample_index, long sample_index_end) {
  return _child_plugin->finished(sample_index, sample_index_end);
}

void ChannelPlugin::notifyFinished(long sample_index, long graph_sample_index) {
  return _child_plugin->notifyFinished(sample_index, graph_sample_index);
}

Plugin::PluginType ChannelPlugin::type() const {
  return Plugin::PluginTypeConsumer;
}

void ChannelPlugin::load(LOAD_CALLBACK callback) {
  _child_plugin->load(callback);
}

bool ChannelPlugin::loaded() const { return _child_plugin->loaded(); }

void ChannelPlugin::run(long sample_index, const NodeTimes &node_times,
                        long node_sample_index) {
  _child_plugin->run(sample_index, node_times, node_sample_index);
}

bool ChannelPlugin::shouldProcess(long sample_index_start,
                                  long sample_index_end) {
  return _child_plugin->shouldProcess(sample_index_start, sample_index_end);
}

}  // namespace channel
}  // namespace plugin
}  // namespace nativeformat
