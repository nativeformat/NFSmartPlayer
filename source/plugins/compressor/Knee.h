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

#include <cmath>

#include "Detector.h"
#include "types.h"

namespace nativeformat {
namespace plugin {
namespace compressor {

// Functions in here conform to the type signature of knee_fun in types.h

template <typename DrcType>
struct Knee {
  using drc_type = typename DrcType::drc_type;
  virtual sample_t operator()(const sample_t &signal_power,
                              const float &threshold_db, const float &knee_db,
                              const float &ratio_db) = 0;
};

template <typename DrcType>
struct HardKnee : Knee<DrcType> {};

template <>
struct HardKnee<CompressorType> : Knee<CompressorType> {
  sample_t operator()(const sample_t &signal_power, const float &threshold_db,
                      const float &knee_db, const float &ratio_db) {
    return signal_power < threshold_db
               ? signal_power
               : threshold_db + (signal_power - threshold_db) / ratio_db;
  }
};

template <>
struct HardKnee<ExpanderType> : Knee<ExpanderType> {
  sample_t operator()(const sample_t &signal_power, const float &threshold_db,
                      const float &knee_db, const float &ratio_db) {
    return signal_power < threshold_db
               ? threshold_db + (signal_power - threshold_db) * ratio_db
               : signal_power;
  }
};

template <>
struct HardKnee<LimiterType> : Knee<LimiterType> {
  sample_t operator()(const sample_t &signal_power, const float &threshold_db,
                      const float &knee_db, const float &ratio_db) {
    return signal_power < threshold_db ? signal_power : threshold_db;
  }
};

template <>
struct HardKnee<NoiseGateType> : Knee<NoiseGateType> {
  sample_t operator()(const sample_t &signal_power, const float &threshold_db,
                      const float &knee_db, const float &ratio_db) {
    return signal_power < threshold_db
               ? -std::numeric_limits<sample_t>::max()  // -inf dB
               : 0;                                     // 0 dB
  }
};

template <typename DrcType>
struct SoftKnee : Knee<DrcType> {};

template <>
struct SoftKnee<CompressorType> : Knee<CompressorType> {
  sample_t operator()(const sample_t &signal_power, const float &threshold_db,
                      const float &knee_db, const float &ratio_db) {
    if (2 * (signal_power - threshold_db) < -knee_db)
      return signal_power;
    else if (2. * std::fabs(signal_power - threshold_db) <= knee_db)
      return signal_power +
             (1. / ratio_db - 1) *
                 std::pow(signal_power - threshold_db + knee_db / 2., 2) /
                 (2. * knee_db);
    // 2 * (signal_power - threshold_db) > width_db
    return threshold_db + (signal_power - threshold_db) / ratio_db;
  }
};

template <>
struct SoftKnee<ExpanderType> : Knee<ExpanderType> {
  sample_t operator()(const sample_t &signal_power, const float &threshold_db,
                      const float &knee_db, const float &ratio_db) {
    if (2. * (signal_power - threshold_db) < -knee_db)
      return threshold_db + (signal_power - threshold_db) * ratio_db;
    else if (signal_power <= std::fabs(threshold_db - knee_db / 2.))
      return signal_power +
             (1. - ratio_db) *
                 std::pow(signal_power - threshold_db - knee_db / 2., 2) /
                 (2. * knee_db);
    return threshold_db;
  }
};

template <>
struct SoftKnee<LimiterType> : Knee<LimiterType> {
  sample_t operator()(const sample_t &signal_power, const float &threshold_db,
                      const float &knee_db, const float &ratio_db) {
    if (signal_power < (threshold_db - knee_db / 2.)) {
      return signal_power;
    } else if (signal_power <= std::fabs(threshold_db - knee_db / 2.)) {
      return signal_power -
             std::pow((signal_power - threshold_db + knee_db / 2.), 2) /
                 (2 * knee_db);
    } else {
      return threshold_db;
    }
  }
};

}  // namespace compressor
}  // namespace plugin
}  // namespace nativeformat
