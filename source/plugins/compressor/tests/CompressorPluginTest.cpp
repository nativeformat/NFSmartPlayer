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

#include "Knee.h"
#include "types.h"

namespace nativeformat {
namespace plugin {
namespace compressor {

BOOST_AUTO_TEST_SUITE(CompressorPluginTests)

BOOST_AUTO_TEST_CASE(compressor_test_hard_knee) {
  nativeformat::plugin::compressor::HardKnee<CompressorType> knee;
  // tests for <= threshold
  BOOST_CHECK_CLOSE(knee(-30, -15, 0, 10), -30, .001);
  BOOST_CHECK_CLOSE(knee(-15, -15, 0, 10), -15, .001);
  // tests for > threshold
  BOOST_CHECK_CLOSE(knee(-10, -15, 0, 10), -14.5, .001);
  BOOST_CHECK_CLOSE(knee(0, -15, 0, 10), -13.5, .001);
}

BOOST_AUTO_TEST_CASE(compressor_test_soft_knee) {
  nativeformat::plugin::compressor::SoftKnee<CompressorType> knee;
  BOOST_CHECK_CLOSE(knee(-30, -15, 5, 10), -30, .001);
  BOOST_CHECK_CLOSE(knee(-20, -15, 5, 10), -20, .001);
  BOOST_CHECK_CLOSE(knee(-13, -15, 5, 10), -14.8225, .001);
  BOOST_CHECK_CLOSE(knee(0, -15, 5, 10), -13.5, .001);
}

BOOST_AUTO_TEST_CASE(expander_test_hard_knee) {
  nativeformat::plugin::compressor::HardKnee<ExpanderType> knee;
  // tests for <= threshold
  BOOST_CHECK_CLOSE(knee(-30, -15, 0, 10), -165, .001);
  BOOST_CHECK_CLOSE(knee(-15, -15, 0, 10), -15, .001);
  // tests for > threshold
  BOOST_CHECK_CLOSE(knee(-10, -15, 0, 10), -10, .001);
  BOOST_CHECK_CLOSE(knee(0, -15, 0, 10), -0, .001);
}
BOOST_AUTO_TEST_SUITE_END()

}  // namespace compressor
}  // namespace plugin
}  // namespace nativeformat
