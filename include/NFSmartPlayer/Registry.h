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

#include <NFSmartPlayer/Factory.h>
#include <NFSmartPlayer/Plugin.h>

#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nativeformat {
namespace plugin {

/**
 * A registry for associating plugins with identifiers
 */
class Registry {
 public:
  /**
   * Constructor
   * @param register_common_plugins Whether to register the common plugins
   * associated with this package
   * @param resolve_callback The callback to call when a plugin wants to resolve
   * a variable
   */
  Registry(std::vector<std::shared_ptr<Factory>> factories,
           const NF_PLUGIN_RESOLVE_CALLBACK &resolve_callback);
  /**
   * Destructor
   */
  virtual ~Registry();

  /**
   * Registers a function that can be used to produce plugins given an
   * identifier
   * @param plugin_factory The function that produces plugin instances
   * @param plugin_identifier The identifier of the plugin to produce
   */
  bool registerPluginFactory(std::shared_ptr<Factory> plugin_factory);
  /**
   * Provides a plugin factory
   * @param plugin_identifier The type of plugin to create
   * @param commands The commands to create the plugin with
   */
  std::shared_ptr<Plugin> createPlugin(
      const nfgrapher::Node &grapher_node, const std::string &graph_id,
      int channels, double samplerate,
      const std::shared_ptr<plugin::Plugin> &child_plugin,
      const std::string &session_id) const;
  /**
   * Whether the plugin registry contains a plugin
   * @param plugin_identifier The plugin identifier to check for
   */
  bool containsPluginIdentifier(const std::string &plugin_identifier) const;

 private:
  const NF_PLUGIN_RESOLVE_CALLBACK _resolve_callback;
  std::unordered_map<std::string, std::shared_ptr<Factory>> _plugin_factories;
};

}  // namespace plugin
}  // namespace nativeformat
