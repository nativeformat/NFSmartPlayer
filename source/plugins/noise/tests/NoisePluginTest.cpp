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
#include <boost/test/unit_test.hpp>

#include "NoisePlugin.h"

BOOST_AUTO_TEST_SUITE(NoisePluginTests)

static const nlohmann::json noise_json = {
    {"id", "noise-node-01"},
    {"kind", nfgrapher::contract::NoiseNodeInfo::kind()},
    {"config", {{"when", 50E9}, {"duration", 50E9}}}};

nfgrapher::contract::NoiseNodeInfo noise_node((nfgrapher::Node)noise_json);

BOOST_AUTO_TEST_CASE(testLocalRenderTimeIsZeroWhenTimeIsBelowStartTime) {
  nativeformat::plugin::noise::NoisePlugin plugin(noise_node, 2, 44100.0);
  BOOST_CHECK_EQUAL(plugin.localRenderSampleIndex(0), 0);
}

BOOST_AUTO_TEST_CASE(testLocalRenderTimeDurationWhenTimeIsAbovePlayTime) {
  nativeformat::plugin::noise::NoisePlugin plugin(noise_node, 2, 44100.0);
  BOOST_CHECK_EQUAL(plugin.localRenderSampleIndex(110 * 2 * 44100),
                    50 * 2 * 44100);
}

BOOST_AUTO_TEST_CASE(testLocalRenderTimeBetweenStartTimeAndDuration) {
  nativeformat::plugin::noise::NoisePlugin plugin(noise_node, 2, 44100.0);
  BOOST_CHECK_EQUAL(plugin.localRenderSampleIndex(60 * 2 * 44100),
                    10 * 2 * 44100);
}

BOOST_AUTO_TEST_SUITE_END()
