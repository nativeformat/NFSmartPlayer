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
#include "DelayPlugin.h"

namespace nativeformat {
namespace plugin {
namespace waa {

DelayPlugin::DelayPlugin(const nfgrapher::contract::DelayNodeInfo &delay_node,
                         int channels, double samplerate,
                         const std::shared_ptr<plugin::Plugin> &child_plugin)
    : _channels(channels),
      _samplerate(samplerate),
      _child_plugin(child_plugin),
      _delay_time(param::createParam(delay_node._delay_time._initial_val,
                                     100.0f, 0.0f, "delayTime")) {
  nfgrapher::param::addCommands(_delay_time, delay_node._delay_time);
}

DelayPlugin::~DelayPlugin() {}

std::string DelayPlugin::name() {
  return nfgrapher::contract::DelayNodeInfo ::kind();
}

std::vector<std::string> DelayPlugin::paramNames() {
  return {_delay_time->name()};
}

std::shared_ptr<param::Param> DelayPlugin::paramForName(
    const std::string &name) {
  if (name == _delay_time->name()) {
    return _delay_time;
  }
  return nullptr;
}

long DelayPlugin::timeDilation(long sample_index) {
  double delay_value =
      _delay_time->valueForTime((sample_index / _samplerate) / _channels);
  return sample_index + clipIntervleavedSamplesToChannel(
                            delay_value * _samplerate * _channels, _channels);
}

bool DelayPlugin::finished(long sample_index, long sample_index_end) {
  return _child_plugin->finished(sample_index, sample_index_end);
}

void DelayPlugin::notifyFinished(long sample_index, long graph_sample_index) {
  return _child_plugin->notifyFinished(timeDilation(sample_index),
                                       graph_sample_index);
}

Plugin::PluginType DelayPlugin::type() const {
  return Plugin::PluginTypeProducerConsumer;
}

void DelayPlugin::feed(std::map<std::string, std::shared_ptr<Content>> &content,
                       long sample_index, long graph_sample_index,
                       nfgrapher::LoadingPolicy loading_policy) {
  _child_plugin->feed(content, timeDilation(sample_index), graph_sample_index,
                      loading_policy);
}

void DelayPlugin::load(LOAD_CALLBACK callback) {
  _child_plugin->load(callback);
}

bool DelayPlugin::loaded() const { return _child_plugin->loaded(); }

void DelayPlugin::run(long sample_index, const NodeTimes &node_times,
                      long node_sample_index) {
  _child_plugin->run(sample_index, node_times, node_sample_index);
}

bool DelayPlugin::shouldProcess(long sample_index_start,
                                long sample_index_end) {
  return _child_plugin->shouldProcess(sample_index_start, sample_index_end);
}

}  // namespace waa
}  // namespace plugin
}  // namespace nativeformat
