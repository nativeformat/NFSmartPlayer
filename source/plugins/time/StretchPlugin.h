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
#include <elastique/elastiqueProV3DirectAPI.h>

#include <mutex>

namespace nativeformat {
namespace plugin {
namespace time {

/**
 * A plugin that stretches time
 */
class StretchPlugin : public Plugin {
 public:
  StretchPlugin(const nfgrapher::contract::StretchNodeInfo &stretch_node,
                int channels, double samplerate,
                const std::shared_ptr<plugin::Plugin> &child_plugin);
  virtual ~StretchPlugin();

  // Plugin
  void feed(std::map<std::string, std::shared_ptr<Content>> &content,
            long sample_index, long graph_sample_index,
            nfgrapher::LoadingPolicy loading_policy) override;

  void majorTimeChange(long sample_index, long graph_sample_index) override;
  std::string name() override;
  long timeDilation(long sample_index) override;
  std::vector<std::string> paramNames() override;
  std::shared_ptr<param::Param> paramForName(const std::string &name) override;
  bool finished(long sample_index, long sample_index_end) override;
  void notifyFinished(long sample_index, long graph_sample_index) override;
  PluginType type() const override;
  void run(long sample_index, const NodeTimes &node_times,
           long node_sample_index) override;
  void load(LOAD_CALLBACK callback) override;
  bool loaded() const override;
  bool shouldProcess(long sample_index_start, long sample_index_end) override;

 private:
  void createElastique();
  void destroyElastique();
  size_t copyResidualFrames(Content &output);
  size_t copyProcessedFrames(Content &output, int output_frames);
  size_t resizeContent(
      std::map<std::string, std::shared_ptr<Content>> &content);
  bool prefill(Content &child_content, Content &output_content);
  void saveOutput(long sample_index, Content &output_content);
  bool fillInput(long frames_to_process, long frames_available, float *samples);
  static bool almostEqual(float a, float b, float tolerance_factor = 0.001f);

  const int _channels;
  const double _samplerate;
  const std::shared_ptr<plugin::Plugin> _child_plugin;

  CElastiqueProV3DirectIf *_elastique;
  float **_input_buffers;
  float **_output_buffers;
  size_t _buffers_size;
  std::mutex _residual_buffer_mutex;
  std::vector<float> _prefill_buffer;
  std::vector<float> _residual_buffer;
  std::vector<float> _previous_output;
  std::atomic<long> _expected_feed_sample;
  std::atomic<long> _input_frame_offset;
  std::shared_ptr<param::Param> _stretch;
  std::shared_ptr<param::Param> _pitch;
  std::shared_ptr<param::Param> _formants;
  std::atomic<float> _previous_stretch_value;
  std::atomic<float> _previous_pitch_value;
  std::atomic<float> _previous_formant_value;
  std::atomic<bool> _prefilled;
  std::atomic<int> _frames_to_discard;
  std::atomic<int> _frames_to_preproc;
  std::atomic<long> _child_sample_offset;
  std::mutex _elastique_mutex;
  std::map<std::string, std::shared_ptr<Content>> _content;
  std::atomic<long> _child_sample_index;
};

}  // namespace time
}  // namespace plugin
}  // namespace nativeformat
