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
#include "BandSplitter.h"

#include <algorithm>

namespace nativeformat {
namespace plugin {
namespace util {

BandSplitter::BandSplitter(std::vector<double> freqs, double samplerate,
                           size_t channels)
    : _bands(freqs.size() + 1), _channels(channels) {
  for (auto f : freqs) {
    _crossovers.push_back(f / samplerate);
  }
  std::sort(_crossovers.begin(), _crossovers.end());

  // Set up the binary tree structure as a vector of filters per band
  build_filter_tree(_crossovers.data(), _crossovers.size());
}

BandSplitter::~BandSplitter() {}

void BandSplitter::filter(float **samples, size_t frames) {
  filter(samples, frames, _bands, _filter_tree.data());
}

void BandSplitter::filter(float **samples, size_t frames, size_t bands,
                          ButterSquared *filters) {
  if (bands < 2) {
    return;
  }
  size_t band_split = std::floor(bands / 2);
  size_t low_filters = filters_from_bands(band_split);

  // Copy input to high-pass half of output
  float *low_input = samples[0];
  float *high_input = samples[band_split];
  std::copy_n(low_input, frames * _channels, high_input);

  // Perform low and high filtering
  filters[0].first.filter(low_input, frames);
  filters[0].second.filter(low_input, frames);
  filters[1].first.filter(high_input, frames);
  filters[1].second.filter(high_input, frames);

  // Recursively move to the next level of the filter bank
  filter(samples, frames, band_split, filters + 2);
  filter(samples + band_split, frames, bands - band_split,
         filters + 2 + low_filters);
}

void BandSplitter::build_filter_tree(const double *crossovers, size_t len) {
  if (!len) {
    return;
  }
  size_t split = std::floor((len - 1) / 2);
  float crossover = crossovers[split];

  util::ButterFilter low(2, util::FilterType::LowPassFilter, _channels);
  low.compute(crossover);
  ButterSquared lows(low, low);
  _filter_tree.push_back(lows);

  util::ButterFilter high(2, util::FilterType::HighPassFilter, _channels);
  high.compute(crossover);
  ButterSquared highs(high, high);
  _filter_tree.push_back(highs);

  build_filter_tree(crossovers, split);
  build_filter_tree(crossovers + split + 1, len - split - 1);
}

size_t BandSplitter::stages(size_t band) {
  size_t stage_count = 0, bands_left = _bands;
  while (bands_left > 1) {
    ++stage_count;
    size_t band_split = std::floor(bands_left / 2);
    if (band >= band_split) {
      bands_left -= band_split;
      band -= band_split;
    } else {
      bands_left = band_split;
    }
  }
  return stage_count;
}

std::string BandSplitter::description() {
  std::stringstream ss;
  ss << "Filter chain for " << _bands << "-band splitter:" << std::endl;
  std::vector<std::stringstream> descs(_bands);
  for (size_t i = 0; i < _bands; ++i) {
    descs[i] << "\tBand " << i << std::endl;
  }
  description(descs.data(), descs.size(), _filter_tree.data());
  for (auto &d : descs) {
    ss << d.str();
  }
  return ss.str();
}

void BandSplitter::description(std::stringstream *descs, size_t len,
                               ButterSquared *filters) {
  if (len < 2) {
    return;
  }
  size_t band_split = std::floor(len / 2);
  size_t low_filters = filters_from_bands(band_split);

  // Describe filters
  std::string lo_desc = filters[0].first.description();
  std::string hi_desc = filters[1].first.description();

  for (size_t i = 0; i < band_split; ++i) {
    descs[i] << "\t" << lo_desc << std::endl;
  }
  for (size_t i = band_split; i < len; ++i) {
    descs[i] << "\t" << hi_desc << std::endl;
  }

  // Recursively move to the next level of the filter bank
  description(descs, band_split, filters + 2);
  description(descs + band_split, len - band_split, filters + 2 + low_filters);
}

}  // namespace util
}  // namespace plugin
}  // namespace nativeformat
