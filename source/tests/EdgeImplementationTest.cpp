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

#include "EdgeImplementation.h"

BOOST_AUTO_TEST_SUITE(EdgeImplementationTests)

static const std::string id = "123";
static const std::string source = "test";
static const std::string target = "test2";

static const nlohmann::json j = {{"id", id},
                                 {"source", source},
                                 {"target", target},
                                 {"sourcePort", nullptr},
                                 {"targetPort", nullptr}};

static const nfgrapher::Edge e = j;

BOOST_AUTO_TEST_CASE(testEdgeImplementationConstructor) {
  nativeformat::smartplayer::EdgeImplementation edge(e);
  BOOST_CHECK_EQUAL(edge.identifier(), id);
  BOOST_CHECK_EQUAL(edge.source(), source);
  BOOST_CHECK_EQUAL(edge.target(), target);
}

BOOST_AUTO_TEST_CASE(testEdgeImplementationJson) {
  const std::string source = "test";
  const std::string target = "test2";
  nativeformat::smartplayer::EdgeImplementation edge(e);
  BOOST_CHECK_EQUAL(edge.json(), j.dump());
}

BOOST_AUTO_TEST_SUITE_END()
