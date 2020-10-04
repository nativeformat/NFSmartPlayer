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
#pragma once

#include <NFSmartPlayer/CallbackTypes.h>
#include <NFSmartPlayer/Client.h>
#include <NFSmartPlayer/Graph.h>
#include <NFSmartPlayer/Script.h>

#include <future>
#include <memory>
#include <vector>

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#include <boost/thread.hpp>

namespace nativeformat {
namespace smartplayer {

class ScriptDelegate {
 public:
  virtual void scriptDidClose(std::shared_ptr<Script> script) = 0;
  virtual bool playing() const = 0;
  virtual void setPlaying(bool playing) = 0;
  virtual double renderTime() const = 0;
  virtual void setRenderTime(double render_time) = 0;
  virtual void sendMessage(const std::string &message_identifier,
                           const std::string &sender_identifier,
                           NFSmartPlayerMessageType message_type,
                           const void *payload) = 0;
  virtual boost::future<Load> setJson(const std::string &json) = 0;
  virtual void forEachScript(
      NF_SMART_PLAYER_SCRIPT_CALLBACK script_callback) = 0;
  virtual void forEachGraph(NF_SMART_PLAYER_GRAPH_CALLBACK graph_callback) = 0;
};

}  // namespace smartplayer
}  // namespace nativeformat
