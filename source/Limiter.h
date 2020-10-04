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

#include <stdint.h>

#include <cmath>
#include <cstdlib>
#include <vector>

extern const float_t kAttack_coeff;
extern const float_t kRelease_coeff;
extern const float_t kThreshold;
extern const u_int16_t kDelay;

class Limiter {
 public:
  Limiter(int channels = 2, int delay = kDelay,
          double attack_coeff = kAttack_coeff,
          double release_coeff = kRelease_coeff, double threshold = kThreshold);
  ~Limiter();

  void limit(float *buffer_in, float *buffer_out, int buffer_size);

 private:
  int _channels;
  int _delay;
  double _attack_coeff;
  double _release_coeff;
  double _threshold;
  std::vector<std::vector<float>> _delay_line;
  u_int16_t _delay_index;
  std::vector<double> _envelope;
  std::vector<double> _gain;
};
