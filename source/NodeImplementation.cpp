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
#include "NodeImplementation.h"

namespace nativeformat {
namespace smartplayer {

static const std::string NodeImplementationLoadingPolicyKey = "loading-policy";

const std::string NodeImplementationIdentifierKey = "id";
const std::string NodeImplementationCommandsKey = "commands";
const std::string NodeImplementationTypeKey = "type";

using nfgrapher::LoadingPolicy;

static nfgrapher::Node makeCustomNode(std::string id, std::string kind) {
  nfgrapher::Node n;
  n.id = id;
  n.kind = kind;
  return n;
}

NodeImplementation::NodeImplementation(nfgrapher::Node &grapher_node,
                                       bool output_node)
    : _output_node(output_node),
      _loading_policy(LoadingPolicy::ALL_CONTENT_PLAYTHROUGH),
      _identifier(grapher_node.id),
      _type(grapher_node.kind),
      _grapher_node(std::move(grapher_node)) {
  /*
  if (json.find(NodeImplementationLoadingPolicyKey) != json.end()) {
    _loading_policy =
        loadingPolicyFromString(json.at(NodeImplementationLoadingPolicyKey));
  }
  */
}

NodeImplementation::NodeImplementation(bool output_node,
                                       LoadingPolicy loading_policy,
                                       const std::string &identifier,
                                       const std::string &type)
    : _output_node(output_node),
      _loading_policy(loading_policy),
      _identifier(identifier),
      _type(type),
      _grapher_node(makeCustomNode(identifier, type)) {}

NodeImplementation::NodeImplementation(bool output_node,
                                       LoadingPolicy loading_policy,
                                       const std::string &identifier,
                                       const std::string &type,
                                       nfgrapher::Node &grapher_node)
    : _output_node(output_node),
      _loading_policy(loading_policy),
      _identifier(identifier),
      _type(type),
      _grapher_node(std::move(grapher_node)) {}

NodeImplementation::~NodeImplementation() {}

void NodeImplementation::run(double time, const plugin::NodeTimes &node_times,
                             double node_time) {
  _plugin->run(time, node_times, node_time);
}

void NodeImplementation::setPlugin(std::shared_ptr<plugin::Plugin> &plugin) {
  _plugin = plugin;
}

std::shared_ptr<plugin::Plugin> NodeImplementation::plugin() const {
  return _plugin;
}

const nfgrapher::Node &NodeImplementation::grapherNode() const {
  return _grapher_node;
}

std::string NodeImplementation::identifier() const { return _identifier; }

std::string NodeImplementation::type() const { return _type; }

bool NodeImplementation::isOutputNode() const { return _output_node; }

std::string NodeImplementation::toJSON() const {
  nlohmann::json j = _grapher_node;
  return j.dump();
}

void NodeImplementation::forEachParam(
    NF_SMART_PLAYER_NODE_PARAM_CALLBACK param_callback) const {
  std::vector<std::string> param_names = _plugin->paramNames();
  for (const auto &param_name : param_names) {
    if (!param_callback(_plugin->paramForName(param_name))) {
      break;
    }
  }
}

std::shared_ptr<param::Param> NodeImplementation::paramForIdentifier(
    const std::string &identifier) const {
  if (!_plugin) {
    return nullptr;
  }
  return _plugin->paramForName(identifier);
}

LoadingPolicy NodeImplementation::loadingPolicy() const {
  return _loading_policy;
}

std::vector<std::string> NodeImplementation::dependencies() const {
  // TODO: Merge node and plugin
  return {};
}

}  // namespace smartplayer
}  // namespace nativeformat
