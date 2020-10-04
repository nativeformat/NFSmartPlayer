/*
 * Copyright (c) 2016 Spotify AB.
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

#include <NFDriver/NFDriver.h>
#include <NFSmartPlayer/Client.h>
#include <NFSmartPlayer/Graph.h>
#include <NFSmartPlayer/Node.h>
#include <NFSmartPlayer/Registry.h>

#include <atomic>
#include <mutex>

#include "GraphDelegate.h"
#include "NodeImplementation.h"
#include "Player.h"

namespace nativeformat {
namespace smartplayer {

typedef enum : int {
  GraphImplementationErrorCodeInvalidJSON,
  GraphImplementationErrorCodeFailedToLoadNode,
  GraphImplementationErrorCodeNoIdentifierInJSON,
  GraphImplementationErrorCodeNoTypeInJSON,
  GraphImplementationErrorCodeNoNodesInJSON,
  GraphImplementationErrorCodeNoEdgesInJSON,
  GraphImplementationErrorCodeNoCommandsInJSON,
  GraphImplementationErrorCodeNoNodeTypeInJSON,
  GraphImplementationErrorCodeDuplicateNodeIdentifiers
} GraphImplementationErrorCode;

extern const std::string GraphImplementationErrorDomain;
extern const std::string GraphImplementationIdentifierKey;

class GraphImplementation
    : public Graph,
      public std::enable_shared_from_this<GraphImplementation> {
  friend class Player;

 public:
  GraphImplementation(int channels, double sample_rate,
                      const std::string &session_id);
  virtual ~GraphImplementation();

  // Graph
  void setJson(const std::string &json, LOAD_CALLBACK load_callback) override;
  std::string identifier() const override;
  double renderTime() const override;
  void setRenderTime(double render_time) override;
  float valueForPath(const std::string &path) override;
  void setValueForPath(const std::string path, ...) override;
  void vSetValueForPath(const std::string path, va_list args) override;
  void setValuesForPath(const std::string path,
                        const std::vector<float> &values) override;
  std::string type() const override;
  void forEachNode(NF_SMART_PLAYER_GRAPH_NODE_CALLBACK node_callback) override;
  std::shared_ptr<param::Param> parameterForPath(
      const std::string &path) override;
  bool loaded() const override;
  bool finished() override;
  void forEachScript(
      NF_SMART_PLAYER_GRAPH_SCRIPT_CALLBACK script_callback) override;

  void setPluginRegistry(std::shared_ptr<plugin::Registry> plugin_registry);
  void setDelegate(std::weak_ptr<GraphDelegate> delegate);

 protected:
  void run() override;
  void traverse(
      std::map<std::string, std::shared_ptr<plugin::Content>> &content,
      nfgrapher::LoadingPolicy loading_policy) override;
  void setJson(const nlohmann::json &json,
               LOAD_CALLBACK load_callback) override;

 private:
  void loadScore(const nfgrapher::Score &score, LOAD_CALLBACK load_callback);
  std::map<std::string, std::shared_ptr<Script>> loadScripts(
      const nfgrapher::Score &score);
  std::vector<std::shared_ptr<Node>> loadNodes(const nfgrapher::Score &score,
                                               LOAD_CALLBACK load_callback,
                                               bool &failed);
  std::shared_ptr<Node> nodeForIdentifier(const std::string &identifier);

  std::shared_ptr<plugin::Registry> _plugin_registry;
  const int _channels;
  const double _samplerate;
  const std::string _session_id;

  std::atomic<bool> _loaded;
  std::mutex _nodes_mutex;
  std::atomic<long> _sample_index;
  std::weak_ptr<GraphDelegate> _delegate;
  std::mutex _scripts_mutex;
  std::map<std::string, std::shared_ptr<Script>> _scripts;
  std::atomic<nfgrapher::LoadingPolicy> _loading_policy;
  std::string _identifier;
  std::string _type;
  plugin::NodeTimes _node_times;
  std::mutex _node_times_mutex;
  std::atomic<bool> _finished;
  std::atomic<long> _buffer_size;
  std::mutex _root_plugin_mutex;
  std::shared_ptr<plugin::Plugin> _root_plugin;
  std::map<std::string, std::shared_ptr<plugin::Content>> _cut_content;
  std::map<std::string, std::shared_ptr<Node>> _nodes;
};

}  // namespace smartplayer
}  // namespace nativeformat
