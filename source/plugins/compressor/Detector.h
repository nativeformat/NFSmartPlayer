
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

#include "types.h"

namespace nativeformat {
namespace plugin {
namespace compressor {

class Detector {
 public:
  Detector() {}

  /* \brief Compute the next value from a level detector
   * \param sample The newest sample
   * \param alpha The attack or release time
   * \return The currently detected level on a linear scale
   */
  virtual sample_t compute(const sample_t sample, const float attack,
                           const float release) = 0;
  virtual Detector* clone() const = 0;
  /* \brief Compute the gain based on the level detector output */
  virtual sample_t gain(const sample_t signal_power,
                        const sample_t desired_signal_level, const float attack,
                        const float release) = 0;
};

}  // namespace compressor
}  // namespace plugin
}  // namespace nativeformat
