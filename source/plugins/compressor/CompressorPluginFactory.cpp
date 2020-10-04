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
#include "CompressorPluginFactory.h"

#include "CompanderPlugin.h"
#include "CompressorPlugin.h"
#include "ExpanderPlugin.h"

namespace nativeformat {
namespace plugin {
namespace compressor {

CompressorPluginFactory::CompressorPluginFactory() {}

CompressorPluginFactory::~CompressorPluginFactory() {}

std::shared_ptr<Plugin> CompressorPluginFactory::createPlugin(
    const nfgrapher::Node &grapher_node, const std::string &graph_id,
    const NF_PLUGIN_RESOLVE_CALLBACK &resolve_callback, int channels,
    double samplerate, const std::shared_ptr<plugin::Plugin> &child_plugin,
    const std::string &session_id) {
  std::cout << grapher_node.kind << std::endl;
  if (grapher_node.kind == nfgrapher::contract::CompressorNodeInfo::kind()) {
    return std::make_shared<CompressorPlugin>(
        nfgrapher::contract::CompressorNodeInfo(grapher_node), channels,
        samplerate, child_plugin);
  }
  if (grapher_node.kind == nfgrapher::contract::ExpanderNodeInfo::kind()) {
    return std::make_shared<ExpanderPlugin>(
        nfgrapher::contract::ExpanderNodeInfo(grapher_node), channels,
        samplerate, child_plugin);
  }
  if (grapher_node.kind == nfgrapher::contract::CompanderNodeInfo::kind()) {
    return std::make_shared<CompanderPlugin>(
        nfgrapher::contract::CompanderNodeInfo(grapher_node), channels,
        samplerate, child_plugin);
  }
  std::string msg =
      "Could not instantiate plugin with graph_id " + graph_id +
      " please check the kind in your json definition to confirm that "
      "this either com.nativeformat.plugin.compressor.compressor, "
      "com.nativeformat.plugin.compressor.expander, or "
      "com.nativeformat.plugin.compressor.compander";
  // TODO: Repair with cmake
  // NF_ERROR(msg.c_str());
  std::cerr << msg << std::endl;
  return std::shared_ptr<Plugin>(nullptr);
}

std::vector<std::string> CompressorPluginFactory::identifiers() const {
  return {nfgrapher::contract::CompressorNodeInfo::kind(),
          nfgrapher::contract::ExpanderNodeInfo::kind(),
          nfgrapher::contract::CompanderNodeInfo::kind()};
}

}  // namespace compressor
}  // namespace plugin
}  // namespace nativeformat
