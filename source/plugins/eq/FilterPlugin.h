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
#pragma once

#include <ButterFilter.h>
#include <NFSmartPlayer/Plugin.h>

#include <mutex>

namespace nativeformat {
namespace plugin {
namespace eq {

class FilterPlugin : public Plugin {
 public:
  FilterPlugin(const nfgrapher::contract::FilterNodeInfo &node, int channels,
               double samplerate,
               const std::shared_ptr<plugin::Plugin> &child_plugin);
  virtual ~FilterPlugin();

  // Plugin
  void feed(std::map<std::string, std::shared_ptr<Content>> &content,
            long sample_index, long graph_sample_index,
            nfgrapher::LoadingPolicy loading_policy) override;

  std::string name() override;
  std::vector<std::string> paramNames() override;
  std::shared_ptr<param::Param> paramForName(const std::string &name) override;
  bool finished(long sample_index, long sample_index_end) override;
  void notifyFinished(long sample_index, long graph_sample_index) override;
  PluginType type() const override;
  void load(LOAD_CALLBACK callback) override;
  bool loaded() const override;
  void run(long sample_index, const NodeTimes &node_times,
           long node_sample_index) override;
  bool shouldProcess(long sample_index_start, long sample_index_end) override;

 private:
  // convert a frequency in hz to a 0-1 scale
  double normalisedFreq(float freq_hz);

  const std::shared_ptr<plugin::Plugin> _child_plugin;
  const double _samplerate;
  std::shared_ptr<param::Param> _low_cutoff;
  std::shared_ptr<param::Param> _high_cutoff;
  float _last_low_cutoff;
  float _last_high_cutoff;
  util::ButterFilter _filter;
};

}  // namespace eq
}  // namespace plugin
}  // namespace nativeformat
