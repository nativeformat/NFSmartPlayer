/*
 * Copyright (c) 2018 Spotify AB.
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
#include "FilePluginFactory.h"

#include <NFGrapher/NFGrapher.h>

#include "FilePlugin.h"

namespace nativeformat {
namespace plugin {
namespace file {

FilePluginFactory::FilePluginFactory(std::shared_ptr<http::Client> client)
    : _client(client),
      _manifest_factory(decoder::createManifestFactory(client)),
      _decrypter_factory(
          decoder::createDecrypterFactory(client, _manifest_factory)),
      _data_provider_factory(
          decoder::createDataProviderFactory(client, _manifest_factory)),
      _decoder_factory(decoder::createFactory(
          _data_provider_factory, _decrypter_factory, _manifest_factory)) {}

FilePluginFactory::~FilePluginFactory() {}

std::shared_ptr<Plugin> FilePluginFactory::createPlugin(
    const nfgrapher::Node &grapher_node, const std::string &graph_id,
    const NF_PLUGIN_RESOLVE_CALLBACK &resolve_callback, int channels,
    double samplerate, const std::shared_ptr<plugin::Plugin> &child_plugin,
    const std::string &session_id) {
  std::shared_ptr<Plugin> plugin =
      filePluginForIdentifier(grapher_node.kind, grapher_node, graph_id,
                              channels, samplerate, session_id);
  return plugin;
}

std::vector<std::string> FilePluginFactory::identifiers() const {
  return {nfgrapher::contract::FileNodeInfo::kind()};
}

std::shared_ptr<Plugin> FilePluginFactory::filePluginForIdentifier(
    const std::string &identifier, const nfgrapher::Node &grapher_node,
    const std::string &graph_id, int channels, double samplerate,
    const std::string &session_id) {
  nfgrapher::contract::FileNodeInfo grapher_file_node(grapher_node);
  return std::make_shared<FilePlugin>(_decoder_factory, grapher_node,
                                      identifier, channels, samplerate);
}

}  // namespace file
}  // namespace plugin
}  // namespace nativeformat
