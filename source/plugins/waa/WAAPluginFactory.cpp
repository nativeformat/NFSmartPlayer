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
#include "WAAPluginFactory.h"

#include <NFGrapher/NFGrapher.h>

#include "DelayPlugin.h"
#include "GainPlugin.h"

namespace nativeformat {
namespace plugin {
namespace waa {

WAAPluginFactory::WAAPluginFactory()
    : _plugin_factories(
          {{nfgrapher::contract::GainNodeInfo::kind(),
            [](const nfgrapher::Node &grapher_node, int channels,
               double samplerate,
               const std::shared_ptr<plugin::Plugin> &child_plugin) {
              nfgrapher::contract::GainNodeInfo gn(grapher_node);
              return std::make_shared<GainPlugin>(gn, channels, samplerate,
                                                  child_plugin);
            }},
           {nfgrapher::contract::DelayNodeInfo ::kind(),
            [](const nfgrapher::Node &grapher_node, int channels,
               double samplerate,
               const std::shared_ptr<plugin::Plugin> &child_plugin) {
              nfgrapher::contract::DelayNodeInfo dn(grapher_node);
              return std::make_shared<DelayPlugin>(dn, channels, samplerate,
                                                   child_plugin);
            }}}) {}

WAAPluginFactory::~WAAPluginFactory() {}

std::shared_ptr<Plugin> WAAPluginFactory::createPlugin(
    const nfgrapher::Node &grapher_node, const std::string &graph_id,
    const NF_PLUGIN_RESOLVE_CALLBACK &resolve_callback, int channels,
    double samplerate, const std::shared_ptr<plugin::Plugin> &child_plugin,
    const std::string &session_id) {
  return _plugin_factories[grapher_node.kind](grapher_node, channels,
                                              samplerate, child_plugin);
}

std::vector<std::string> WAAPluginFactory::identifiers() const {
  std::vector<std::string> identifiers;
  for (const auto &plugin_factory : _plugin_factories) {
    identifiers.push_back(plugin_factory.first);
  }
  return identifiers;
}

}  // namespace waa
}  // namespace plugin
}  // namespace nativeformat
