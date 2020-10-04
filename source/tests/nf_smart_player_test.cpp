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
#include <NFGrapher/NFGrapher.h>
#include <NFSmartPlayer/nf_smart_player.h>

#include <boost/test/unit_test.hpp>
#include <future>
#include <thread>

BOOST_AUTO_TEST_SUITE(nf_smart_player_tests)

static void fillTestScore(nfgrapher::Score &s) {
  s.version = nfgrapher::version();
  s.graph.id = "com.nativeformat.graph:789";
  s.graph.loading_policy =
      std::unique_ptr<nfgrapher::LoadingPolicy>(new nfgrapher::LoadingPolicy(
          nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH));
}

BOOST_AUTO_TEST_CASE(test_smartplayer_driver_type_to_string) {
  std::string driver_type =
      smartplayer_driver_type_to_string(NF_SMART_PLAYER_DRIVER_TYPE_FILE);
  BOOST_CHECK_EQUAL(driver_type, "file");
}

BOOST_AUTO_TEST_CASE(test_smartplayer_driver_type_from_string) {
  NF_SMART_PLAYER_DRIVER_TYPE driver_type =
      smartplayer_driver_type_from_string("file");
  BOOST_CHECK_EQUAL(driver_type, NF_SMART_PLAYER_DRIVER_TYPE_FILE);
}

BOOST_AUTO_TEST_CASE(test_smartplayer_set_json) {
  NF_SMART_PLAYER_HANDLE handle =
      smartplayer_open(nullptr, nullptr, NF_SMART_PLAYER_DEFAULT_SETTINGS,
                       NF_SMART_PLAYER_DRIVER_TYPE_FILE, "");
  std::mutex mutex;
  static std::condition_variable conditional_variable;
  nfgrapher::Score s;
  fillTestScore(s);
  smartplayer_set_json(
      handle, ((nlohmann::json)s).dump().c_str(),
      [](void *context, int success, const char *error_message) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        conditional_variable.notify_one();
      });
  std::unique_lock<std::mutex> lock(mutex);
  conditional_variable.wait(lock);
}

BOOST_AUTO_TEST_CASE(test_smartplayer_set_json_error_message_callback) {
  NF_SMART_PLAYER_HANDLE handle =
      smartplayer_open(nullptr, nullptr, NF_SMART_PLAYER_DEFAULT_SETTINGS,
                       NF_SMART_PLAYER_DRIVER_TYPE_FILE, "");
  std::mutex mutex;
  static std::condition_variable conditional_variable;
  smartplayer_set_json(
      handle, "", [](void *context, int success, const char *error_message) {
        BOOST_CHECK_EQUAL(
            error_message,
            "Error: com.nativeformat.smartplayer: Could not parse JSON");
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        conditional_variable.notify_one();
      });
  std::unique_lock<std::mutex> lock(mutex);
  conditional_variable.wait(lock);
}

BOOST_AUTO_TEST_CASE(test_smartplayer_create_graph_error_message_callback) {
  static bool callback_called = false;
  smartplayer_create_graph(
      "", nullptr, [](void *context, int success, const char *error_message) {
        BOOST_CHECK_EQUAL(
            strncmp(error_message,
                    "Error: com.nativeformat.graph.error: Invalid JSON", 48),
            0);
        callback_called = true;
      });
  BOOST_CHECK(callback_called);
}

BOOST_AUTO_TEST_SUITE_END()
