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

#include "LoopPlugin.h"

BOOST_AUTO_TEST_SUITE(LoopPluginTests)

static const nlohmann::json loop_json = {
    {"id", "loop-node-01"},
    {"kind", nfgrapher::contract::LoopNodeInfo::kind()},
    {"config", {{"when", 50E9}, {"duration", 10E9}}}};

static const nlohmann::json loop_json_x2 = {
    {"id", "loop-node-01"},
    {"kind", nfgrapher::contract::LoopNodeInfo::kind()},
    {"config", {{"when", 50E9}, {"duration", 10E9}, {"loopCount", 2}}}};

nfgrapher::contract::LoopNodeInfo loop_node((nfgrapher::Node)loop_json);
nfgrapher::contract::LoopNodeInfo loop_node_x2((nfgrapher::Node)loop_json_x2);

BOOST_AUTO_TEST_CASE(testLocalRenderTimeIsZeroWhenTimeIsBelowStartTime) {
  nativeformat::plugin::time::LoopPlugin plugin(loop_node, 2, 44100.0, nullptr);
  BOOST_CHECK_EQUAL(plugin.localRenderSampleIndex(0), 0);
}

BOOST_AUTO_TEST_CASE(testLocalRenderTimeAnytimePastStartTimeWithInfiniteLoops) {
  nativeformat::plugin::time::LoopPlugin plugin(loop_node, 2, 44100.0, nullptr);
  BOOST_CHECK_EQUAL(plugin.localRenderSampleIndex(5000 * 2 * 44100),
                    4950 * 2 * 44100);
}

BOOST_AUTO_TEST_CASE(testLocalRenderTimeDurationPastEndTime) {
  nativeformat::plugin::time::LoopPlugin plugin(loop_node_x2, 2, 44100.0,
                                                nullptr);
  BOOST_CHECK_EQUAL(plugin.localRenderSampleIndex(80 * 2 * 44100),
                    20 * 2 * 44100);
}

BOOST_AUTO_TEST_CASE(testLocalRenderTimeWhileInLoop) {
  nativeformat::plugin::time::LoopPlugin plugin(loop_node_x2, 2, 44100.0,
                                                nullptr);
  BOOST_CHECK_EQUAL(plugin.localRenderSampleIndex(60 * 2 * 44100),
                    10 * 2 * 44100);
}

BOOST_AUTO_TEST_SUITE_END()
