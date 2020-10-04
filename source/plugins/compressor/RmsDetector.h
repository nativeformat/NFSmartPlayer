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

#include <cmath>
#include <deque>

#include "Detector.h"
#include "types.h"

namespace nativeformat {
namespace plugin {
namespace compressor {

class RmsDetector : public Detector {
 private:
  sample_t last_level;

 public:
  RmsDetector() : last_level(0.0f) {}

  virtual sample_t compute(const sample_t sample, const float attack,
                           const float release) final {
    // this function implements a running RMS
    // it is approximated with a first order IIR filter
    float alpha = sample > last_level ? attack : release;
    return (last_level = (alpha * (last_level * last_level) +
                          (1 - alpha) * (sample * sample)));
  }
};

class CompressorRmsDetector : public RmsDetector {
 public:
  virtual sample_t gain(const sample_t signal_power,
                        const sample_t desired_signal_level, const float attack,
                        const float release) final {
    return -compute(signal_power - desired_signal_level, attack, release);
  }

  virtual CompressorRmsDetector* clone() const final {
    return new CompressorRmsDetector(*this);
  }
};

class ExpanderRmsDetector : public RmsDetector {
 public:
  virtual sample_t gain(const sample_t signal_power,
                        const sample_t desired_signal_level, const float attack,
                        const float release) final {
    return compute(desired_signal_level - signal_power, attack, release);
  }

  virtual ExpanderRmsDetector* clone() const final {
    return new ExpanderRmsDetector(*this);
  }
};

}  // namespace compressor
}  // namespace plugin
}  // namespace nativeformat
