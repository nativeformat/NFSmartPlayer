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
#include <utility>
#include <vector>

#include "ButterFilter.h"

namespace nativeformat {
namespace plugin {
namespace util {

class BandSplitter {
 public:
  BandSplitter(std::vector<double> freqs, double samplerate, size_t channels);
  virtual ~BandSplitter();

  // given N arrays of frames with the input at index 0,
  // generate the output for all N bands using tree structure
  void filter(float **samples, size_t frames);

  inline size_t bands() const { return _bands; }
  inline float crossover(size_t index) { return _crossovers[index]; }
  size_t stages(size_t band = 0);
  std::string description();

 private:
  using ButterSquared = std::pair<util::ButterFilter, util::ButterFilter>;

  void filter(float **samples, size_t frames, size_t bands,
              ButterSquared *filters);
  void build_filter_tree(const double *crossovers, size_t len);
  void description(std::stringstream *descs, size_t len,
                   ButterSquared *filters);

  inline size_t filters_from_bands(size_t bands) { return 2 * (bands - 1); }

  const size_t _bands;
  const size_t _channels;

  // Normalised from 0 to 0.5 (Hz / sample rate)
  std::vector<double> _crossovers;

  // Flattened filter tree
  std::vector<ButterSquared> _filter_tree;
};

}  // namespace util
}  // namespace plugin
}  // namespace nativeformat
