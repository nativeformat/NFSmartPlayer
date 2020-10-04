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
#include <NFSmartPlayer/Registry.h>

namespace nativeformat {
namespace plugin {

Registry::Registry(std::vector<std::shared_ptr<Factory>> factories,
                   const NF_PLUGIN_RESOLVE_CALLBACK &resolve_callback)
    : _resolve_callback(resolve_callback) {
  for (const auto &factory : factories) {
    registerPluginFactory(factory);
  }
}

Registry::~Registry() {}

bool Registry::registerPluginFactory(std::shared_ptr<Factory> plugin_factory) {
  if (!plugin_factory) {
    return false;
  }

  // Check whether we have a registration conflict
  for (const auto &identifier : plugin_factory->identifiers()) {
    if (containsPluginIdentifier(identifier)) {
      return false;
    }
  }

  // Register our plugins
  for (const auto &identifier : plugin_factory->identifiers()) {
    _plugin_factories[identifier] = plugin_factory;
  }

  return true;
}

std::shared_ptr<Plugin> Registry::createPlugin(
    const nfgrapher::Node &grapher_node, const std::string &graph_id,
    int channels, double samplerate,
    const std::shared_ptr<plugin::Plugin> &child_plugin,
    const std::string &session_id) const {
  const std::string &plugin_identifier = grapher_node.kind;
  if (containsPluginIdentifier(plugin_identifier)) {
    auto &factory = _plugin_factories.at(plugin_identifier);
    return factory->createPlugin(grapher_node, graph_id, _resolve_callback,
                                 channels, samplerate, child_plugin,
                                 session_id);
  }
  return nullptr;
}

bool Registry::containsPluginIdentifier(
    const std::string &plugin_identifier) const {
  return _plugin_factories.find(plugin_identifier) != _plugin_factories.end();
}

}  // namespace plugin
}  // namespace nativeformat
