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
#include <NFSmartPlayerObjC/NFSmartPlayerObjC.h>

#include <NFGrapher/NFGrapher.h>
#include <boost/test/unit_test.hpp>

#include <thread>

#import "NFSmartPlayerObjCDelegateMock.h"

BOOST_AUTO_TEST_SUITE(NFSmartPlayerObjCTests)

static const nlohmann::json sine_json = {
    {"id", "node-01"},
    {"kind", nfgrapher::contract::SineNodeInfo::kind()},
    {"config", {{"when", 50E9}, {"duration", 367E9}, {"frequency", 1000.0}}}};

static const nlohmann::json gain_json = {
    {"id", "node-02"},
    {"kind", nfgrapher::contract::GainNodeInfo::kind()},
    {"params",
     {{"gain", {{{"name", "setValueAtTime"}, {"args", {{"value", 1}, {"startTime", 0}}}}}}}}};

static const nlohmann::json edge_json = {
    {"id", "edge-01"}, {"source", "node-01"}, {"target", "node-02"}};

BOOST_AUTO_TEST_CASE(testInit) {
  NFSmartPlayerObjC *player = [[NFSmartPlayerObjC alloc] init];
  BOOST_CHECK(player);
}

BOOST_AUTO_TEST_CASE(testSetJson) {
  nfgrapher::Score s;
  s.version = nfgrapher::version();
  s.graph.id = "com.nativeformat.graph:67352cd747ca928eccf57b5b29727068";
  s.graph.nodes = std::unique_ptr<std::vector<nfgrapher::Node>>(new std::vector<nfgrapher::Node>());
  s.graph.nodes->push_back((nfgrapher::Node)sine_json);
  s.graph.nodes->push_back((nfgrapher::Node)gain_json);
  s.graph.edges = std::unique_ptr<std::vector<nfgrapher::Edge>>(new std::vector<nfgrapher::Edge>());
  s.graph.edges->push_back((nfgrapher::Edge)edge_json);

  NFSmartPlayerObjC *player = [[NFSmartPlayerObjC alloc] init];
  NFSmartPlayerObjCDelegateMock *delegate = [NFSmartPlayerObjCDelegateMock new];
  player.delegate = delegate;
  std::mutex mutex;
  __block std::condition_variable *conditional_variable = new std::condition_variable();
  delegate.loadedBlock = ^(BOOL loaded) {
    BOOST_CHECK(loaded);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    conditional_variable->notify_one();
  };
  player.json = [NSString stringWithCString:((nlohmann::json)s).dump().c_str()
                                   encoding:[NSString defaultCStringEncoding]];
  std::unique_lock<std::mutex> lock(mutex);
  conditional_variable->wait(lock);
  BOOST_CHECK(player);
  delete conditional_variable;
}

BOOST_AUTO_TEST_CASE(testGraphForIdentifier) {
  nfgrapher::Score s;
  s.version = nfgrapher::version();
  s.graph.id = "com.nativeformat.graph:67352cd747ca928eccf57b5b29727068";
  s.graph.nodes = std::unique_ptr<std::vector<nfgrapher::Node>>(new std::vector<nfgrapher::Node>());
  s.graph.nodes->push_back((nfgrapher::Node)sine_json);
  s.graph.nodes->push_back((nfgrapher::Node)gain_json);
  s.graph.edges = std::unique_ptr<std::vector<nfgrapher::Edge>>(new std::vector<nfgrapher::Edge>());
  s.graph.edges->push_back((nfgrapher::Edge)edge_json);

  NFSmartPlayerObjC *player = [[NFSmartPlayerObjC alloc] init];
  NFSmartPlayerObjCDelegateMock *delegate = [NFSmartPlayerObjCDelegateMock new];
  player.delegate = delegate;
  std::mutex mutex;
  __block std::condition_variable *conditional_variable = new std::condition_variable();
  delegate.loadedBlock = ^(BOOL loaded) {
    BOOST_CHECK(loaded);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    conditional_variable->notify_one();
  };
  player.json = [NSString stringWithCString:((nlohmann::json)s).dump().c_str()
                                   encoding:[NSString defaultCStringEncoding]];

  std::unique_lock<std::mutex> lock(mutex);
  conditional_variable->wait(lock);

  NFSmartPlayerGraph *validGraph =
      [player graphForIdentifier:@"com.nativeformat.graph:67352cd747ca928eccf57b5b29727068"];
  BOOST_CHECK(validGraph);

  NFSmartPlayerGraph *invalidGraph = [player graphForIdentifier:@"invalid"];
  BOOST_CHECK(!invalidGraph);
}

BOOST_AUTO_TEST_CASE(testScriptForIdentifier) {
  NFSmartPlayerObjC *player = [[NFSmartPlayerObjC alloc] init];
  [player openScript:@"test" code:@""];

  NFSmartPlayerScript *validScript = [player scriptForIdentifier:@"test"];
  BOOST_CHECK(validScript);

  NFSmartPlayerScript *invalidScript = [player scriptForIdentifier:@"invalid"];
  BOOST_CHECK(!invalidScript);
}

BOOST_AUTO_TEST_CASE(testParamForPath) {
  nfgrapher::Score s;
  s.version = nfgrapher::version();
  s.graph.id = "com.nativeformat.graph:67352cd747ca928eccf57b5b29727068";
  s.graph.nodes = std::unique_ptr<std::vector<nfgrapher::Node>>(new std::vector<nfgrapher::Node>());
  s.graph.nodes->push_back((nfgrapher::Node)sine_json);
  s.graph.nodes->push_back((nfgrapher::Node)gain_json);
  s.graph.edges = std::unique_ptr<std::vector<nfgrapher::Edge>>(new std::vector<nfgrapher::Edge>());
  s.graph.edges->push_back((nfgrapher::Edge)edge_json);

  NFSmartPlayerObjC *player = [[NFSmartPlayerObjC alloc] init];
  NFSmartPlayerObjCDelegateMock *delegate = [NFSmartPlayerObjCDelegateMock new];
  player.delegate = delegate;
  std::mutex mutex;
  __block std::condition_variable *conditional_variable = new std::condition_variable();
  delegate.loadedBlock = ^(BOOL loaded) {
    BOOST_CHECK(loaded);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    conditional_variable->notify_one();
  };
  player.json = [NSString stringWithCString:((nlohmann::json)s).dump().c_str()
                                   encoding:[NSString defaultCStringEncoding]];

  std::unique_lock<std::mutex> lock(mutex);
  conditional_variable->wait(lock);

  NFSmartPlayerGraph *graph =
      [player graphForIdentifier:@"com.nativeformat.graph:67352cd747ca928eccf57b5b29727068"];

  NFSmartPlayerParam *validParam =
      [graph paramForPath:@"com.nativeformat.graph:67352cd747ca928eccf57b5b29727068/node-02/gain"];
  BOOST_CHECK(validParam);

  NFSmartPlayerParam *invalidParam =
      [graph paramForPath:@"com.nativeformat.graph:67352cd747ca928eccf57b5b29727068/invalid/gain"];
  BOOST_CHECK(!invalidParam);
}

BOOST_AUTO_TEST_SUITE_END()
