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
#pragma once

#include <NFSmartPlayer/Plugin.h>

#include <memory>

namespace nativeformat {
namespace plugin {

extern const std::string MixerPluginIdentifier;

class MixerPlugin : public Plugin,
                    public std::enable_shared_from_this<MixerPlugin> {
 public:
  using src_dst_t = std::vector<std::pair<std::string, std::string>>;

  struct Metadata {
    std::shared_ptr<Plugin> _plugin;
    std::map<std::string, std::shared_ptr<plugin::Content>> _content;
    src_dst_t _edges;
  };

  MixerPlugin(const std::vector<Metadata> &metadata, int channels,
              double samplerate);
  virtual ~MixerPlugin();

  // Plugin
  void majorTimeChange(long sample_index, long graph_sample_index) override;
  std::string name() override;
  std::vector<std::string> paramNames() override;
  std::shared_ptr<param::Param> paramForName(const std::string &name) override;
  long timeDilation(long sample_index) override;
  bool finished(long sample_index, long sample_index_end) override;
  void notifyFinished(long sample_index, long graph_sample_index) override;
  PluginType type() const override;
  void feed(std::map<std::string, std::shared_ptr<Content>> &content,
            long sample_index, long graph_sample_index,
            nfgrapher::LoadingPolicy loading_policy) override;
  void load(LOAD_CALLBACK callback) override;
  bool loaded() const override;
  void run(long sample_index, const NodeTimes &node_times,
           long node_sample_index) override;
  long startSampleIndex() override;
  bool shouldProcess(long sample_index_start, long sample_index_end) override;

 private:
  std::vector<Metadata>
      _child_metadata;  // vector that defines metadata of plugins to source
                        // content from. Remember the graph is traversed from
                        // end -> beginning.

  std::map<std::string, size_t>
      _maximum_content_items;  // Tracks the amount of items in the content that
                               // will be returned/output to the destination
                               // plugin
};

}  // namespace plugin
}  // namespace nativeformat
