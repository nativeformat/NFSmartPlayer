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

#include <NFSmartPlayer/Plugin.h>

#include <string>

#include "Knee.h"
#include "PeakDetector.h"
#include "RmsDetector.h"
#include "types.h"

namespace nativeformat {
namespace plugin {
namespace compressor {

template <typename NodeInfo>
class Drc {
  using DetectionMode = typename NodeInfo::DetectionMode;

 public:
  struct KneeParams {
    std::shared_ptr<param::Param> _threshold_db;
    std::shared_ptr<param::Param> _knee_db;
    std::shared_ptr<param::Param> _ratio_db;
  };

  struct Metadata {
    KneeParams _params;
    knee_fun _knee;
    std::unique_ptr<Detector> _level_detector;

    Metadata(const KneeParams params, const knee_fun knee,
             std::unique_ptr<Detector> &&level_detector)
        : _params(params),
          _knee(knee),
          _level_detector(std::move(level_detector)) {}

    Metadata(const Metadata &other)
        : _params(other._params),
          _knee(other._knee),
          _level_detector(other._level_detector->clone()) {}
  };

  Drc(const DetectionMode &finder_name, std::vector<Metadata> &meta,
      const int channels, const double samplerate)
      : _channels(channels), _samplerate(samplerate), _chains(meta) {}

  Drc(const Drc &other)
      : _channels(other._channels),
        _samplerate(other._samplerate),
        _chains(other._chains) {}

  ~Drc() {}

  void compand(const float attack, const float release, const double time,
               const double end_time, sample_t *audio_samples,
               const size_t audio_sample_count, sample_t *sidechain_samples,
               const size_t sidechain_sample_count) {
    if (sidechain_sample_count != audio_sample_count) {
      return;
    }

    size_t frame_count = sidechain_sample_count / _channels;
    // the attack and release are defined as the time required to
    // reach 1-(1/e) of the final value
    auto attack_coeff = std::exp(-1.0 / (_samplerate * attack));
    auto release_coeff = std::exp(-1.0 / (_samplerate * release));

    // convert to DB
    // perform channel-linking. In this case we'll just mix
    for (int frame = 0; frame < frame_count; ++frame) {
      int frame_start = _channels * frame;
      // get mixed sample
      float max_sample =
          std::accumulate(sidechain_samples + frame_start,
                          sidechain_samples + frame_start + _channels, 0.0f,
                          [](const sample_t &a, const sample_t &b) {
                            return std::max(a, std::fabs(b));
                          });

      // now get the gain for gc
      float signal_level =
          20 * std::log10(max_sample == 0
                              ? std::numeric_limits<sample_t>::epsilon()
                              : max_sample);

      float gain = 0.0;
      for (const auto &chain : _chains) {
        float desired_signal_level = signal_level;
        auto threshold_db =
            chain._params._threshold_db->smoothedValueForTimeRange(time,
                                                                   end_time);
        auto knee_db =
            chain._params._knee_db->smoothedValueForTimeRange(time, end_time);
        auto ratio_db =
            chain._params._ratio_db->smoothedValueForTimeRange(time, end_time);
        desired_signal_level =
            chain._knee(desired_signal_level, threshold_db, knee_db, ratio_db);
        float new_gain = chain._level_detector->gain(
            signal_level, desired_signal_level, release_coeff, attack_coeff);
        gain += new_gain;
        signal_level += new_gain;
      }

      // apply gain
      std::transform(
          audio_samples + frame_start, audio_samples + frame_start + _channels,
          audio_samples + frame_start, [&gain](const sample_t &sample) {
            return sample * std::pow(10, gain / 20.);
          });
    }
  }

 private:
  int _channels;
  double _samplerate;

  std::vector<Metadata> _chains;
};

}  // namespace compressor
}  // namespace plugin
}  // namespace nativeformat
