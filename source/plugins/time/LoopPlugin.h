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
#pragma once

#include <NFSmartPlayer/Plugin.h>

#include <atomic>

namespace nativeformat {
namespace plugin {
namespace time {

/**
 * A plugin that allows looping for a particular piece of time
 */
class LoopPlugin : public Plugin {
 public:
  LoopPlugin(const nfgrapher::contract::LoopNodeInfo &loop_node, int channels,
             double samplerate,
             const std::shared_ptr<plugin::Plugin> &child_plugin);
  virtual ~LoopPlugin();

  // Plugin
  void feed(std::map<std::string, std::shared_ptr<Content>> &content,
            long sample_index, long graph_sample_index,
            nfgrapher::LoadingPolicy loading_policy) override;

  std::string name() override;
  long timeDilation(long sample_index) override;
  bool finished(long sample_index, long sample_index_end) override;
  void notifyFinished(long sample_index, long graph_sample_index) override;
  PluginType type() const override;
  long localRenderSampleIndex(long sample_index) override;
  long startSampleIndex() override;
  void load(LOAD_CALLBACK callback) override;
  bool loaded() const override;
  void run(long sample_index, const NodeTimes &node_times,
           long node_sample_index) override;
  bool shouldProcess(long sample_index_start, long sample_index_end) override;

 private:
  const std::shared_ptr<plugin::Plugin> _child_plugin;

  const long _start_sample_index;
  const long _duration_samples;
  const long _loops;
};

}  // namespace time
}  // namespace plugin
}  // namespace nativeformat
