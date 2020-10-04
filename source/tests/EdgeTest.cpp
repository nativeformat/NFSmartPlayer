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
#include <NFSmartPlayer/Edge.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(EdgeTests)

BOOST_AUTO_TEST_CASE(testCreateEdge) {
  nfgrapher::Edge ge;
  ge.id = "123";
  ge.source = "test";
  ge.target = "test2";
  std::shared_ptr<nativeformat::smartplayer::Edge> edge =
      nativeformat::smartplayer::createEdge(ge);
  BOOST_CHECK(edge);
}

BOOST_AUTO_TEST_CASE(testCreateEdgeJson) {
  std::shared_ptr<nativeformat::smartplayer::Edge> edge =
      nativeformat::smartplayer::createEdge(
          "{\"id\":\"123\", \"source\":\"test\", \"target\":\"test2\"}");
  BOOST_CHECK(edge);
}

BOOST_AUTO_TEST_SUITE_END()
