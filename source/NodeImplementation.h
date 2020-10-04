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

#include <NFSmartPlayer/Node.h>

#include <memory>

namespace nativeformat {
namespace smartplayer {

extern const std::string NodeImplementationIdentifierKey;
extern const std::string NodeImplementationTypeKey;
extern const std::string NodeImplementationCommandsKey;

class NodeImplementation : public Node {
 public:
  NodeImplementation(nfgrapher::Node &grapher_node, bool output_node);
  NodeImplementation(bool output_node, nfgrapher::LoadingPolicy loading_policy,
                     const std::string &identifier, const std::string &type);
  NodeImplementation(bool output_node, nfgrapher::LoadingPolicy loading_policy,
                     const std::string &identifier, const std::string &type,
                     nfgrapher::Node &grapher_node);
  virtual ~NodeImplementation();

  // Node
  std::string identifier() const override;
  std::string type() const override;
  bool isOutputNode() const override;
  std::string toJSON() const override;
  void forEachParam(
      NF_SMART_PLAYER_NODE_PARAM_CALLBACK param_callback) const override;
  std::shared_ptr<param::Param> paramForIdentifier(
      const std::string &identifier) const override;
  const nfgrapher::Node &grapherNode() const override;
  std::shared_ptr<plugin::Plugin> plugin() const override;
  nfgrapher::LoadingPolicy loadingPolicy() const override;
  std::vector<std::string> dependencies() const override;

 protected:
  void run(double time, const plugin::NodeTimes &node_times,
           double node_time) override;
  void setPlugin(std::shared_ptr<plugin::Plugin> &plugin) override;

 private:
  const bool _output_node;
  nfgrapher::LoadingPolicy _loading_policy;
  const std::string _identifier;
  const std::string _type;

  const nfgrapher::Node _grapher_node;
  std::shared_ptr<plugin::Plugin> _plugin;
};

}  // namespace smartplayer
}  // namespace nativeformat
