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

#include <NFDecoder/Factory.h>
#include <NFSmartPlayer/Plugin.h>

#include <atomic>
#include <memory>

namespace nativeformat {
namespace plugin {
namespace file {

class FilePlugin : public Plugin,
                   public std::enable_shared_from_this<FilePlugin> {
 public:
  FilePlugin(const std::shared_ptr<decoder::Factory> &factory,
             const nfgrapher::contract::FileNodeInfo &grapher_node,
             const std::string &name, int channels, double samplerate);
  virtual ~FilePlugin();

  // Plugin
  void feed(std::map<std::string, std::shared_ptr<Content>> &content,
            long sample_index, long graph_sample_index,
            nfgrapher::LoadingPolicy loading_policy) override;

  void load(LOAD_CALLBACK callback) override;
  bool loaded() const override;
  std::string name() override;
  std::string path();
  void run(long sample_index, const NodeTimes &node_times,
           long node_sample_index) override;
  bool finished(long sample_index, long sample_index_end) override;
  PluginType type() const override;
  long localRenderSampleIndex(long sample_index) override;
  long startSampleIndex() override;
  bool shouldProcess(long sample_index_start, long sample_index_end) override;

 private:
  void initialise(LOAD_CALLBACK callback);
  void decode(long frame_index, long frames, LOAD_CALLBACK callback);

  const std::shared_ptr<decoder::Factory> _factory;
  const std::string _name;
  const int _channels;
  const double _samplerate;

  // grapher config
  const std::string _path;
  const long _start_time_frame_index;
  const long _duration_frames;
  const long _track_start_frame_index;

  std::atomic<bool> _loaded;
  std::shared_ptr<decoder::Decoder> _decoder;
  std::mutex _decoder_mutex;
  std::atomic<bool> _initialised;

  std::atomic<long> _cached_samples_start_frame_index;
  std::vector<float> _cached_samples;
  std::mutex _cached_samples_mutex;
  std::atomic<bool> _initialising;
  std::atomic<bool> _decoding;
};

}  // namespace file
}  // namespace plugin
}  // namespace nativeformat
