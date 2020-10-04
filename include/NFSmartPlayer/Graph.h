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
#include <NFSmartPlayer/Content.h>
#include <NFSmartPlayer/Edge.h>
#include <NFSmartPlayer/ErrorCode.h>
#include <NFSmartPlayer/Node.h>
#include <NFSmartPlayer/Script.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace nativeformat {
namespace smartplayer {

typedef std::function<bool(const std::shared_ptr<Node> &)>
    NF_SMART_PLAYER_GRAPH_NODE_CALLBACK;
typedef std::function<bool(const std::shared_ptr<Edge> &)>
    NF_SMART_PLAYER_GRAPH_EDGE_CALLBACK;
typedef std::function<bool(const std::shared_ptr<Script> &)>
    NF_SMART_PLAYER_GRAPH_SCRIPT_CALLBACK;

extern const char *GraphMessageIdentifierEnd;

class Graph {
  friend class Player;

 public:
  virtual void setJson(const std::string &json,
                       LOAD_CALLBACK load_callback) = 0;
  virtual std::string identifier() const = 0;
  virtual double renderTime() const = 0;
  virtual void setRenderTime(double render_time) = 0;
  virtual float valueForPath(const std::string &path) = 0;
  virtual void setValueForPath(const std::string path, ...) = 0;
  virtual void vSetValueForPath(const std::string path, va_list args) = 0;
  virtual void setValuesForPath(const std::string path,
                                const std::vector<float> &values) = 0;
  virtual std::string type() const = 0;
  virtual void forEachNode(
      NF_SMART_PLAYER_GRAPH_NODE_CALLBACK node_callback) = 0;
  virtual std::shared_ptr<param::Param> parameterForPath(
      const std::string &path) = 0;
  virtual bool loaded() const = 0;
  virtual bool finished() = 0;
  virtual void forEachScript(
      NF_SMART_PLAYER_GRAPH_SCRIPT_CALLBACK script_callback) = 0;

 protected:
  virtual void run() = 0;
  virtual void traverse(
      std::map<std::string, std::shared_ptr<plugin::Content>> &content,
      nfgrapher::LoadingPolicy loading_policy) = 0;
  virtual void setJson(const nlohmann::json &json,
                       LOAD_CALLBACK load_callback) = 0;
};

extern std::shared_ptr<Graph> createGraph(const std::string &json,
                                          LOAD_CALLBACK load_callback,
                                          int channels, double samplerate);
extern std::string createGraphSessionId();

}  // namespace smartplayer
}  // namespace nativeformat
