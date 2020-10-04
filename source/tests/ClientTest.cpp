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
#include <NFSmartPlayer/Client.h>

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ClientTests)

BOOST_AUTO_TEST_CASE(testCreateClient) {
  std::shared_ptr<nativeformat::smartplayer::Client> client =
      nativeformat::smartplayer::createClient(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "");
  BOOST_CHECK(client);
}

BOOST_AUTO_TEST_SUITE_END()
