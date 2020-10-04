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
#import <NFSmartPlayerObjC/NFSmartPlayerGraph.h>

#include <thread>

#include <boost/test/unit_test.hpp>

#import "NFSmartPlayerGraphDelegateMock.h"

BOOST_AUTO_TEST_SUITE(NFSmartPlayerGraphTests)

BOOST_AUTO_TEST_CASE(testInitWithJson) {
  NFSmartPlayerGraph *graph = [[NFSmartPlayerGraph alloc]
      initWithJSON:@"{\"id\":\"com.nativeformat.graph:67352cd747ca928eccf57b5b29727068\",\"type\":"
                   @"\"com.nativeformat.graph\",\"nodes\":[{\"id\":\"node-0\",\"type\":\"com."
                   @"nativeformat.plugin.spotify.spotify\",\"label\":\"A New "
                   @"Error\",\"commands\":[[\"setTrackUri\",\"spotify:track:"
                   @"6OGRM4MAOlyOdhHuX0OJ6P\"],[\"setStartTime\",\"time:0\"],[\"setDuration\","
                   @"\"rtime:367.306\"]],\"metadata\":{\"editor.x\":65,\"editor.y\":104}},{\"id\":"
                   @"\"node-1\",\"type\":\"com.nativeformat.plugin.waa.gain\",\"label\":\"Node "
                   @"1\",\"commands\":[[\"gain.value\",1]],\"metadata\":{\"editor.x\":433,"
                   @"\"editor.y\":112}}],\"edges\":[{\"relation\":\"edge "
                   @"relationship\",\"directed\":true,\"metadata\":{},\"label\":\"\",\"source\":"
                   @"\"node-0\",\"target\":\"node-1\"}]}"
         loadBlock:nil];
  BOOST_CHECK(graph);
}

BOOST_AUTO_TEST_SUITE_END()
