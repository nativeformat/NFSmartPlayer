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
#include "Limiter.h"

#include <stdint.h>

#include <cmath>

const float_t kAttack_coeff = 0.9;
const float_t kRelease_coeff = 0.9995;
const float_t kThreshold = 0.95;
const u_int16_t kDelay = 32;

Limiter::Limiter(int channels, int delay, double attack_coeff,
                 double release_coeff, double threshold)
    : _channels(channels),
      _delay(delay),
      _attack_coeff(attack_coeff),
      _release_coeff(release_coeff),
      _threshold(threshold),
      _delay_line(_channels, std::vector<float>(_delay)),
      _delay_index(0),
      _envelope(_channels),
      _gain(_channels) {}

Limiter::~Limiter() {}

void Limiter::limit(float *buffer_in, float *buffer_out, int buffer_size) {
  for (int i = 0; i < buffer_size; i += _channels) {
    auto prev_delay_index = _delay_index;
    _delay_index = (_delay_index + 1) % _delay;
    for (int channel = 0; channel < _channels; ++channel) {
      auto sample_index = channel + i;

      _delay_line[channel][prev_delay_index] = buffer_in[sample_index];

      _envelope[channel] = fmaxf(fabs(buffer_in[sample_index]),
                                 _envelope[channel] * _release_coeff);

      double target_gain = 1.0;
      if (_envelope[channel] > _threshold) {
        target_gain = _threshold / _envelope[channel];
      }
      _gain[channel] =
          _gain[channel] * _attack_coeff + target_gain * (1.0 - _attack_coeff);
      buffer_out[sample_index] =
          _delay_line[channel][_delay_index] * _gain[channel];
    }
  }
}
