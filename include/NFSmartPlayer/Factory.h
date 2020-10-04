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

#include <NFSmartPlayer/Client.h>
#include <NFSmartPlayer/Plugin.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nativeformat {
namespace plugin {

typedef std::function<void(std::string domain, int error)>
    NF_PLUGIN_ERROR_CALLBACK;
typedef std::function<std::string(const std::string &plugin_namespace,
                                  const std::string &variable_name)>
    NF_PLUGIN_RESOLVE_CALLBACK;

/**
 * The class that represents the basic interface of a plugin factory
 */
class Factory {
 public:
  /**
   * Creates a plugin from the factory
   * @param greapher_node The grapher object describing the behaviour of the
   * plugin
   * @param identifier The identifier describing the plugin to invoke
   * @param resolve_callback The callback to invoke when a plugin wants to
   * resolve a variable
   * @param load_callback The callback to invoke when the plugin has been
   * created
   */
  virtual std::shared_ptr<Plugin> createPlugin(
      const nfgrapher::Node &grapher_node, const std::string &graph_id,
      const NF_PLUGIN_RESOLVE_CALLBACK &resolve_callback, int channels,
      double samplerate, const std::shared_ptr<plugin::Plugin> &child_plugin,
      const std::string &session_id) = 0;
  /**
   * The identifiers represented by this factory
   * @warning This should not change.
   */
  virtual std::vector<std::string> identifiers() const = 0;
};

}  // namespace plugin
}  // namespace nativeformat
