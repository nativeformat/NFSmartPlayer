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
#include <NFSmartPlayer/Graph.h>

#include <boost/test/unit_test.hpp>
#include <thread>

BOOST_AUTO_TEST_SUITE(GraphTests)

BOOST_AUTO_TEST_CASE(testCreateGraph) {
  std::mutex mutex;
  std::condition_variable conditional_variable;
  std::atomic<bool> callback_done;
  callback_done = false;

  nfgrapher::Score s;
  s.version = nfgrapher::version();
  s.graph.id = "com.nativeformat.graph:456";
  s.graph.loading_policy =
      std::unique_ptr<nfgrapher::LoadingPolicy>(new nfgrapher::LoadingPolicy(
          nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH));
  nlohmann::json j = s;

  std::shared_ptr<nativeformat::smartplayer::Graph> graph =
      nativeformat::smartplayer::createGraph(
          j.dump(),
          [&conditional_variable,
           &callback_done](const nativeformat::Load &load) {
            callback_done = true;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            conditional_variable.notify_one();
          },
          2, 44100.0);
  if (!callback_done) {
    std::unique_lock<std::mutex> lock(mutex);
    conditional_variable.wait(lock);
  }
  BOOST_CHECK(graph);
}

BOOST_AUTO_TEST_SUITE_END()
