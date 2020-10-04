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
#include <boost/test/floating_point_comparison.hpp>
#include <boost/test/unit_test.hpp>

#include "EQPlugin.h"
#include "FilterPlugin.h"

// Just for testing
#define private public

BOOST_AUTO_TEST_SUITE(EQPluginTests)

static nlohmann::json eq_json = {
    {"id", "eq-1"},
    {"kind", nfgrapher::contract::Eq3bandNodeInfo ::kind()},
    {"config", {}},
    {"params",
     {{"highGain",
       {{{"name", "setValueAtTime"},
         {"args", {{"value", 24}, {"startTime", 0}}}},
        {{"name", "linearRampToValueAtTime"},
         {"args", {{"value", 0}, {"endTime", 15E9}}}}}}}}};

/*
BOOST_AUTO_TEST_CASE(testLocalRenderTimeIsZero) {
  nfgrapher::contract::Eq3bandNodeInfo  eq_node((nfgrapher::Node)eq_json);
  nativeformat::plugin::eq::EQPlugin plugin(eq_node, 2, 44100.0, nullptr);
  BOOST_CHECK_EQUAL(plugin.localRenderSampleIndex(0), 0);
}
*/

BOOST_AUTO_TEST_CASE(testFilterSpecs) {
  nfgrapher::Node n = eq_json;
  nfgrapher::contract::Eq3bandNodeInfo eq_node((nfgrapher::Node)eq_json);
  nativeformat::plugin::eq::EQPlugin plugin(eq_node, 2, 44100.0, nullptr);
  size_t expected_filters = 2;
  std::vector<float> expected_freqs = {264.0f, 1000.0f, 3300.0f};
  std::vector<float> expected_gains = {0.0f, 0.0f, 24.0f};
  std::vector<float> expected_coefs = {
      8.60111f, -15.9084f, 7.34337f,  -0.986869f, 0.0229137f,
      1.0f,     -1.93182f, 0.951599f, -1.93182f,  0.951599f};

  size_t filters = plugin.get_filter_count();
  std::vector<float> freqs = plugin.get_freqs(0);
  std::vector<float> gains = plugin.get_gains(0);

  std::vector<float> coefs;
  for (int i = 0; i < filters; ++i) {
    std::vector<float> fc = plugin.get_coefs(i);
    coefs.insert(coefs.end(), fc.begin(), fc.end());
  }

  BOOST_TEST(filters == expected_filters);
  BOOST_TEST(freqs == expected_freqs);
  BOOST_TEST(gains == expected_gains);
  for (int i = 0; i < expected_coefs.size(); ++i) {
    BOOST_CHECK_CLOSE(coefs[i], expected_coefs[i], 0.01);
  }
}

BOOST_AUTO_TEST_CASE(testBandpassFilter) {
  nlohmann::json filter_json = {
      {"id", "filter-1"},
      {"kind", nfgrapher::contract::FilterNodeInfo::kind()},
      {"config", {}},
      {"params",
       {{"highCutoff",
         {{{"name", "setValueAtTime"},
           {"args", {{"value", 0.2}, {"startTime", 0}}}},
          {{"name", "linearRampToValueAtTime"},
           {"args", {{"value", 0.4}, {"endTime", 15E9}}}}}}}}};
  nfgrapher::Node n = filter_json;
  nfgrapher::contract::FilterNodeInfo fni((nfgrapher::Node)filter_json);
  nativeformat::plugin::eq::FilterPlugin plugin(fni, 2, 44100.0, nullptr);
}

BOOST_AUTO_TEST_SUITE_END()
