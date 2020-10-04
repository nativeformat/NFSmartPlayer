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

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "GraphImplementation.h"

namespace nativeformat {
namespace smartplayer {

const char *GraphMessageIdentifierEnd = "com.nativeformat.graph.end";

std::shared_ptr<Graph> createGraph(const std::string &json,
                                   LOAD_CALLBACK load_callback, int channels,
                                   double samplerate) {
  const auto session_id = createGraphSessionId();
  std::shared_ptr<Graph> graph =
      std::make_shared<GraphImplementation>(channels, samplerate, session_id);
  graph->setJson(json, load_callback);
  return graph;
}

std::string createGraphSessionId() {
  static boost::uuids::random_generator uuid_generator;
  static std::mutex uuid_generator_mutex;
  std::lock_guard<std::mutex> uuid_generator_lock(uuid_generator_mutex);
  auto uuid = uuid_generator();
  return boost::uuids::to_string(uuid);
}

}  // namespace smartplayer
}  // namespace nativeformat
