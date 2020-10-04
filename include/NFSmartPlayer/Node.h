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

#include <NFSmartPlayer/Plugin.h>

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace nativeformat {
namespace smartplayer {

typedef std::function<bool(const std::shared_ptr<param::Param> &)>
    NF_SMART_PLAYER_NODE_PARAM_CALLBACK;

class Node {
  friend class GraphImplementation;

 public:
  virtual std::string identifier() const = 0;
  virtual std::string type() const = 0;
  virtual bool isOutputNode() const = 0;
  virtual std::string toJSON() const = 0;
  virtual void forEachParam(
      NF_SMART_PLAYER_NODE_PARAM_CALLBACK param_callback) const = 0;
  virtual std::shared_ptr<param::Param> paramForIdentifier(
      const std::string &identifier) const = 0;
  virtual const nfgrapher::Node &grapherNode() const = 0;
  virtual std::shared_ptr<plugin::Plugin> plugin() const = 0;
  virtual nfgrapher::LoadingPolicy loadingPolicy() const = 0;
  virtual std::vector<std::string> dependencies() const = 0;
  virtual void setPlugin(std::shared_ptr<plugin::Plugin> &plugin) = 0;

 protected:
  virtual void run(double time, const plugin::NodeTimes &node_times,
                   double node_time) = 0;
};

extern std::shared_ptr<Node> createNode(const std::string &json);

}  // namespace smartplayer
}  // namespace nativeformat
