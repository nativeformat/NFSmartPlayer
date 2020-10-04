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

// Just for testing
#define private public

#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>

#include "BandSplitter.h"

BOOST_AUTO_TEST_SUITE(PluginUtilTests)
using namespace nativeformat::plugin::util;

BOOST_AUTO_TEST_CASE(testBandSplitterProperties4band) {
  std::vector<double> freqs{400, 800, 1600};
  BandSplitter splitter(freqs, 44100.0, 2);
  const size_t expected_bands = freqs.size() + 1;
  size_t actual_bands = splitter.bands();
  BOOST_CHECK_EQUAL(actual_bands, expected_bands);

  for (size_t i = 0; i < freqs.size(); ++i) {
    const float actual_freq = splitter.crossover(i);
    const float expected_freq = freqs[i] / 44100.0;
    BOOST_CHECK_EQUAL(actual_freq, expected_freq);
  }

  const size_t expected_stages = std::ceil(std::log2(expected_bands));
  for (size_t i = 0; i < actual_bands; ++i) {
    const size_t actual_stages = splitter.stages(i);
    BOOST_CHECK_EQUAL(actual_stages, expected_stages);
  }

  const size_t expected_filter_count = 2 * freqs.size();
  const size_t actual_filter_count = splitter._filter_tree.size();
  BOOST_CHECK_EQUAL(actual_filter_count, expected_filter_count);
}

BOOST_AUTO_TEST_CASE(testBandSplitterProperties3band) {
  std::vector<double> freqs{400, 1200};
  BandSplitter splitter(freqs, 44100.0, 2);

  const size_t expected_bands = freqs.size() + 1;
  size_t actual_bands = splitter.bands();
  BOOST_CHECK_EQUAL(actual_bands, expected_bands);

  for (size_t i = 0; i < freqs.size(); ++i) {
    const float actual_freq = splitter.crossover(i);
    const float expected_freq = freqs[i] / 44100.0;
    BOOST_CHECK_EQUAL(actual_freq, expected_freq);
  }

  const std::vector<size_t> expected_stages{1, 2, 2};
  for (size_t i = 0; i < actual_bands; ++i) {
    const size_t actual_stages = splitter.stages(i);
    BOOST_CHECK_EQUAL(actual_stages, expected_stages[i]);
  }
  const size_t expected_filter_count = 2 * freqs.size();
  const size_t actual_filter_count = splitter._filter_tree.size();
  BOOST_CHECK_EQUAL(actual_filter_count, expected_filter_count);
}

BOOST_AUTO_TEST_CASE(testBandSplitterPropertiesNband) {
  std::vector<double> freqs{3.125};
  for (size_t i = 0; i < 13; ++i) {
    BandSplitter splitter(freqs, 44100.0, 2);
    const size_t expected_filter_count = 2 * freqs.size();
    const size_t actual_filter_count = splitter._filter_tree.size();
    BOOST_CHECK_EQUAL(actual_filter_count, expected_filter_count);
    freqs.push_back(freqs.back() * 2);
  }
}

BOOST_AUTO_TEST_CASE(testButterFilterBandpassCoefficients) {
  ButterFilter filter(3, FilterType::BandPassFilter, 2);
  filter.compute(0.04666, 0.06666);

  const size_t expected_poles = 6;
  const size_t expected_zeros = 6;
  const double expected_gain = 4.553604437e+03;
  const std::vector<double> expected_x_coeffs{-1.0, 0.0, 3.0, 0.0,
                                              -3.0, 0.0, 1.0};
  const std::vector<double> expected_y_coeffs{
      -0.7776385602, 4.5653450765,   -11.4701594530,
      15.7555532150, -12.4737077230, 5.3990179269,
      -1.0};

  BOOST_CHECK_EQUAL(filter._zplane.numpoles, expected_poles);
  BOOST_CHECK_EQUAL(filter._zplane.numzeros, expected_zeros);
  BOOST_CHECK_CLOSE(filter._gain, expected_gain, 1.0e-06);
  for (size_t i = 0; i <= expected_zeros; ++i) {
    BOOST_CHECK_CLOSE(filter._x_coeffs[i], expected_x_coeffs[i], 1.0e-04);
  }
  for (size_t i = 0; i <= expected_poles; ++i) {
    BOOST_CHECK_CLOSE(filter._y_coeffs[i], expected_y_coeffs[i], 1.0e-04);
  }
}

BOOST_AUTO_TEST_SUITE_END()
