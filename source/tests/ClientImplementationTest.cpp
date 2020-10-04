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

#include "ClientImplementation.h"

BOOST_AUTO_TEST_SUITE(ClientImplementationTests)

static void fillTestScore(nfgrapher::Score &s, bool withScript = false) {
  s.version = nfgrapher::version();
  s.graph.id = "test-graph-id";
  s.graph.loading_policy =
      std::unique_ptr<nfgrapher::LoadingPolicy>(new nfgrapher::LoadingPolicy(
          nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH));
  if (withScript) {
    s.graph.scripts = std::unique_ptr<std::vector<nfgrapher::Script>>(
        new std::vector<nfgrapher::Script>());
    s.graph.scripts->push_back(
        nfgrapher::Script{"thing", "\"print('Test Script Print')\""});
  }
}

static std::string testScoreString(bool withScript = false) {
  nfgrapher::Score s;
  fillTestScore(s, withScript);
  return ((nlohmann::json)s).dump();
}

BOOST_AUTO_TEST_CASE(testIsPlaying) {
  std::shared_ptr<nativeformat::smartplayer::ClientImplementation> client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  BOOST_CHECK(!client->isPlaying());
}

BOOST_AUTO_TEST_CASE(testSendMessage) {
  std::shared_ptr<nativeformat::smartplayer::ClientImplementation> client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  std::string message_id = "com.some.thing";
  bool called = false;
  client->setMessageCallback(
      [&message_id, &called](
          const std::string &message_identifier,
          const std::string &sender_identifier,
          nativeformat::smartplayer::NFSmartPlayerMessageType message_type,
          const void *payload) {
        BOOST_CHECK_EQUAL(message_id, message_identifier);
        called = true;
      });
  client->sendMessage(message_id,
                      nativeformat::smartplayer::NFSmartPlayerMessageTypeNone,
                      nullptr);
  BOOST_CHECK(called);
}

BOOST_AUTO_TEST_CASE(testNumberOfScripts) {
  std::shared_ptr<nativeformat::smartplayer::ClientImplementation> client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  std::mutex mutex;
  std::condition_variable conditional_variable;
  client->setJson(testScoreString(true),
                  [&conditional_variable](const nativeformat::Load &load) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    conditional_variable.notify_one();
                  });
  std::unique_lock<std::mutex> lock(mutex);
  conditional_variable.wait(lock);
  BOOST_CHECK_EQUAL(client->numberOfScripts(), 1);
}

BOOST_AUTO_TEST_CASE(testOpenScript) {
  std::shared_ptr<nativeformat::smartplayer::ClientImplementation> client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  client->openScript("thing", "print('Test Script Print')");
  BOOST_CHECK_EQUAL(client->numberOfScripts(), 1);
}

BOOST_AUTO_TEST_CASE(testGraphForIdentifier) {
  auto client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  std::mutex mutex;
  std::condition_variable conditional_variable;
  std::string json = testScoreString();
  client->setJson(json,
                  [&conditional_variable](const nativeformat::Load &load) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    conditional_variable.notify_one();
                  });
  std::unique_lock<std::mutex> lock(mutex);
  conditional_variable.wait(lock);
  auto graph = client->graphForIdentifier("test-graph-id");
  BOOST_CHECK(graph);
}

BOOST_AUTO_TEST_CASE(testGetAndSetValueForPath) {
  auto client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  std::mutex mutex;
  std::condition_variable conditional_variable;
  nlohmann::json gain_node = {
      {"id", "node-1"},
      {"kind", "com.nativeformat.plugin.waa.gain"},
      {"config", {}},
      {"params",
       {{"gain",
         {{{"name", "setValueAtTime"},
           {"args", {{"value", 0.25}, {"startTime", 0}}}}}}}},
  };
  nfgrapher::Score s;
  fillTestScore(s);
  s.graph.nodes = std::unique_ptr<std::vector<nfgrapher::Node>>(
      new std::vector<nfgrapher::Node>());
  s.graph.nodes->push_back((nfgrapher::Node)gain_node);
  client->setJson(((nlohmann::json)s).dump(),
                  [&conditional_variable](const nativeformat::Load &load) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    conditional_variable.notify_one();
                  });
  std::unique_lock<std::mutex> lock(mutex);
  conditional_variable.wait(lock);

  std::string path("test-graph-id/node-1/gain");
  float value = client->valueForPath(path);
  BOOST_CHECK_EQUAL(value, 0.25f);

  client->setValueForPath(path + "/value", -1.0f);
  value = client->valueForPath(path);
  BOOST_CHECK_EQUAL(value, 0.0f);

  client->setValueForPath(path + "/value", 25.0f);
  value = client->valueForPath(path);
  BOOST_CHECK_EQUAL(value, 16.0f);
}

BOOST_AUTO_TEST_CASE(testForEachScript) {
  auto client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  client->openScript("thing1", "print('Hello from Script 1')");
  client->openScript("thing2", "print('Hello from Script 2')");
  int i = 0;
  client->forEachScript(
      [&i](const std::shared_ptr<nativeformat::smartplayer::Script> &script) {
        i++;
        return true;
      });
  BOOST_CHECK_EQUAL(i, 2);
}

BOOST_AUTO_TEST_CASE(testScriptForIdentifier) {
  auto client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  client->openScript("thing", "print('Hello from Script')");
  auto script = client->scriptForIdentifier("thing");
  BOOST_CHECK(script);
}

BOOST_AUTO_TEST_CASE(testAddGraph) {
  auto client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  std::mutex mutex;
  std::condition_variable conditional_variable;
  std::atomic<bool> callback_done;
  callback_done = false;
  auto graph = nativeformat::smartplayer::createGraph(
      testScoreString(),
      [&conditional_variable, &callback_done](const nativeformat::Load &load) {
        callback_done = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        conditional_variable.notify_one();
      },
      2, 44100.0);
  if (!callback_done) {
    std::unique_lock<std::mutex> lock(mutex);
    conditional_variable.wait(lock);
  }
  client->addGraph(graph);
  int graph_count = 0;
  client->forEachGraph(
      [&graph_count](
          const std::shared_ptr<nativeformat::smartplayer::Graph> &graph) {
        graph_count++;
        return true;
      });
  BOOST_CHECK_EQUAL(graph_count, 1);
}

BOOST_AUTO_TEST_CASE(testRemoveGraph) {
  auto client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  std::mutex mutex;
  std::condition_variable conditional_variable;
  std::atomic<bool> callback_done;
  callback_done = false;
  auto graph = nativeformat::smartplayer::createGraph(
      testScoreString(),
      [&conditional_variable, &callback_done](const nativeformat::Load &load) {
        callback_done = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        conditional_variable.notify_one();
      },
      2, 44100.0);
  if (!callback_done) {
    std::unique_lock<std::mutex> lock(mutex);
    conditional_variable.wait(lock);
  }
  client->addGraph(graph);
  client->removeGraph("test-graph-id");
  int graph_count = 0;
  client->forEachGraph(
      [&graph_count](
          const std::shared_ptr<nativeformat::smartplayer::Graph> &graph) {
        graph_count++;
        return true;
      });
  BOOST_CHECK_EQUAL(graph_count, 0);
}

BOOST_AUTO_TEST_CASE(testSetLoadingPolicy) {
  auto client =
      std::make_shared<nativeformat::smartplayer::ClientImplementation>(
          [](const std::string &plugin_namespace,
             const std::string &variable_name) { return ""; },
          [](nativeformat::Load::ERROR_INFO &info) {},
          nativeformat::smartplayer::DriverTypeSoundCard, "", 0, 0, "", 0);
  client->setLoadingPolicy(nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH);
  BOOST_CHECK_EQUAL((int)client->loadingPolicy(),
                    (int)nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH);
}

BOOST_AUTO_TEST_SUITE_END()
