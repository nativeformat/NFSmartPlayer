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

#include <BandSplitter.h>
#include <NFSmartPlayer/Plugin.h>

#include "Drc.h"
#include "types.h"

namespace nativeformat {
namespace plugin {
namespace compressor {

extern const std::string CompressorPluginIdentifier;

/**
 * A plugin that can perform dynamic range compression
 */
class CompressorPlugin : public Plugin {
  using NodeInfo = nfgrapher::contract::CompressorNodeInfo;
  using DetectionMode = typename NodeInfo::DetectionMode;
  using Compressor = Drc<NodeInfo>;

 public:
  CompressorPlugin(const NodeInfo &grapher_node, int channels,
                   double samplerate,
                   const std::shared_ptr<plugin::Plugin> &child_plugin);
  virtual ~CompressorPlugin();

  // Plugin
  void feed(std::map<std::string, std::shared_ptr<Content>> &content,
            long sample_index, long graph_sample_index,
            nfgrapher::LoadingPolicy loading_policy) override;

  std::string name() override;
  bool finished(long sample_index, long sample_index_end) override;
  void notifyFinished(long sample_index, long graph_sample_index) override;
  PluginType type() const override;
  void load(LOAD_CALLBACK callback) override;
  bool loaded() const override;
  void run(long sample_index, const NodeTimes &node_times,
           long node_sample_index) override;
  bool shouldProcess(long sample_index_start, long sample_index_end) override;

 private:
  const int _channels;
  const double _samplerate;
  const std::shared_ptr<plugin::Plugin> _child_plugin;
  std::shared_ptr<param::Param> _attack;
  std::shared_ptr<param::Param> _release;

  Compressor::KneeParams _compressor_params;

  std::unique_ptr<util::BandSplitter> _splitter;
  std::vector<std::unique_ptr<plugin::Content>> _content;
  std::vector<std::unique_ptr<plugin::Content>> _sidechain_content;

  std::vector<Compressor> _compressors;
  size_t _payload_size;

  // helper functions to perform filtering and compression
  void split_bands(
      size_t sample_count, Content &content,
      std::vector<std::unique_ptr<plugin::Content>> &local_content);

  static constexpr char SidechainContentTypeKey[] = "sidechain";
};

}  // namespace compressor
}  // namespace plugin
}  // namespace nativeformat
