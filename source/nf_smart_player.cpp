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
#include <NFLogger/Logger.h>
#include <NFSmartPlayer/Client.h>
#include <NFSmartPlayer/GlobalLogger.h>
#include <NFSmartPlayer/nf_smart_player.h>

#include <mutex>

using nativeformat::logger::Logger;
using nativeformat::logger::LogInfo;

typedef struct NF_SMART_PLAYER_HANDLE_STRUCT {
  std::shared_ptr<nativeformat::smartplayer::Client> _smart_player_client;
  void *_context;

  NF_SMART_PLAYER_HANDLE_STRUCT(
      NF_SMART_PLAYER_RESOLVE_CALLBACK resolve_callback, void *context,
      NF_SMART_PLAYER_SETTINGS &settings,
      NF_SMART_PLAYER_DRIVER_TYPE driver_type, const char *output_destination)
      : _smart_player_client(nativeformat::smartplayer::createClient(
            [resolve_callback, context](
                const std::string &plugin_namespace,
                const std::string &variable_name) -> std::string {
              static std::mutex resolve_mutex;
              std::lock_guard<std::mutex> resolve_lock(resolve_mutex);
              std::string resolve =
                  resolve_callback(context, plugin_namespace.c_str(),
                                   variable_name.c_str())
                      ?: "";
              return resolve;
            },
            [](nativeformat::Load::ERROR_INFO &info) {
              // TODO: Expose this from C
              for (auto &li : info) {
                NF_ERROR(&li);
              }
            },
            (nativeformat::smartplayer::DriverType)driver_type,
            output_destination, settings._osc_read_port,
            settings._osc_write_port, settings._osc_address,
            settings._pump_manually)),
        _context(context) {}
} NF_SMART_PLAYER_HANDLE_STRUCT;

typedef struct NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT {
  void *_context;
  std::shared_ptr<nativeformat::smartplayer::Graph> _graph;
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT(
      const char *json, void *context,
      NF_SMART_PLAYER_LOAD_CALLBACK load_callback)
      : _context(context),
        _graph(nativeformat::smartplayer::createGraph(
            json,
            [load_callback, context](const nativeformat::Load &load) {
              load_callback(context, load._loaded,
                            errorMessageFromLoad(load).c_str());
            },
            2, 44100.0)) {}
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT(
      std::shared_ptr<nativeformat::smartplayer::Graph> graph)
      : _context(nullptr), _graph(graph) {}
} NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT;

const char *NF_SMART_PLAYER_VERSION = nativeformat::smartplayer::Version;
const char *NF_SMART_PLAYER_OSC_ADDRESS =
    nativeformat::smartplayer::NFSmartPlayerOscAddress;
const char *NF_SMART_PLAYER_DRIVER_TYPE_FILE_STRING = "file";
const char *NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD_STRING = "sound";
const char *NF_SMART_PLAYER_SENDER_IDENTIFIER =
    nativeformat::smartplayer::PlayerSenderIdentifier;
const char *NF_SMART_PLAYER_MESSAGE_IDENTIFIER_END =
    nativeformat::smartplayer::PlayerMessageIdentifierEnd;
const int NF_SMART_PLAYER_OSC_READ_PORT =
    nativeformat::smartplayer::NFSmartPlayerOscReadPort;
const int NF_SMART_PLAYER_OSC_WRITE_PORT =
    nativeformat::smartplayer::NFSmartPlayerOscWritePort;
const int NF_SMART_PLAYER_LOCALHOST_PORT =
    nativeformat::smartplayer::NFSmartPlayerLocalhostPort;
const NF_SMART_PLAYER_SETTINGS NF_SMART_PLAYER_DEFAULT_SETTINGS = {
    NF_SMART_PLAYER_OSC_READ_PORT, NF_SMART_PLAYER_OSC_WRITE_PORT,
    NF_SMART_PLAYER_OSC_ADDRESS, NF_SMART_PLAYER_LOCALHOST_PORT, false};

NF_SMART_PLAYER_HANDLE smartplayer_open(
    NF_SMART_PLAYER_RESOLVE_CALLBACK resolve_callback, void *context,
    NF_SMART_PLAYER_SETTINGS settings, NF_SMART_PLAYER_DRIVER_TYPE driver_type,
    const char *output_destination) {
  return new NF_SMART_PLAYER_HANDLE_STRUCT(resolve_callback, context, settings,
                                           driver_type, output_destination);
}

void smartplayer_close(NF_SMART_PLAYER_HANDLE handle) {
  delete (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
}

int smartplayer_get_graphs(NF_SMART_PLAYER_HANDLE handle) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  int graph_count = 0;
  player_handle->_smart_player_client->forEachGraph(
      [&graph_count](
          const std::shared_ptr<nativeformat::smartplayer::Graph> &graph) {
        graph_count++;
        return true;
      });
  return graph_count;
}

NF_SMART_PLAYER_GRAPH_HANDLE smartplayer_get_graph(
    NF_SMART_PLAYER_HANDLE handle, const char *graph_identifier) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  if (auto graph = player_handle->_smart_player_client->graphForIdentifier(
          graph_identifier)) {
    return new NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT(graph);
  }

  return nullptr;
}

NF_SMART_PLAYER_GRAPH_HANDLE smartplayer_get_graph_at_index(
    NF_SMART_PLAYER_HANDLE handle, int graph_index) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  int graph_count = 0;
  NF_SMART_PLAYER_GRAPH_HANDLE graph = nullptr;
  player_handle->_smart_player_client->forEachGraph(
      [&graph_count, graph_index,
       &graph](const std::shared_ptr<nativeformat::smartplayer::Graph>
                   &player_graph) {
        if (graph_count == graph_index) {
          graph = new NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT(player_graph);
          return false;
        }
        return true;
      });
  return graph;
}

void smartplayer_set_json(NF_SMART_PLAYER_HANDLE handle, const char *json,
                          NF_SMART_PLAYER_LOAD_CALLBACK load_callback) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  player_handle->_smart_player_client->setJson(
      json, [player_handle, load_callback](const nativeformat::Load &load) {
        load_callback(player_handle->_context, load._loaded,
                      errorMessageFromLoad(load).c_str());
      });
}

void smartplayer_add_graph(NF_SMART_PLAYER_HANDLE handle,
                           NF_SMART_PLAYER_GRAPH_HANDLE graph) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *player_graph =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  player_handle->_smart_player_client->addGraph(player_graph->_graph);
}

void smartplayer_remove_graph(NF_SMART_PLAYER_HANDLE handle,
                              const char *graph_identifier) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  player_handle->_smart_player_client->removeGraph(graph_identifier);
}

int smartplayer_is_playing(NF_SMART_PLAYER_HANDLE handle) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  return player_handle->_smart_player_client->isPlaying();
}

void smartplayer_set_playing(NF_SMART_PLAYER_HANDLE handle, int playing) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  player_handle->_smart_player_client->setPlaying(playing);
}

double smartplayer_render_time(NF_SMART_PLAYER_HANDLE handle) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  return player_handle->_smart_player_client->getRenderTime();
}

void smartplayer_set_render_time(NF_SMART_PLAYER_HANDLE handle, double time) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  player_handle->_smart_player_client->setRenderTime(time);
}

void smartplayer_set_message_callback(
    NF_SMART_PLAYER_HANDLE handle,
    NF_SMART_PLAYER_MESSAGE_CALLBACK message_callback) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  player_handle->_smart_player_client->setMessageCallback(
      [player_handle, message_callback](
          const std::string &message_identifier,
          const std::string &sender_identifier,
          nativeformat::smartplayer::NFSmartPlayerMessageType message_type,
          const void *payload) {
        message_callback(player_handle->_context, message_identifier.c_str(),
                         sender_identifier.c_str(),
                         (NF_SMART_PLAYER_MESSAGE_TYPE)message_type, payload);
      });
}

void smartplayer_send_message(NF_SMART_PLAYER_HANDLE handle,
                              const char *message_identifier,
                              NF_SMART_PLAYER_MESSAGE_TYPE message_type,
                              const void *payload) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  player_handle->_smart_player_client->sendMessage(
      message_identifier,
      (nativeformat::smartplayer::NFSmartPlayerMessageType)message_type,
      payload);
}

int smartplayer_get_scripts(NF_SMART_PLAYER_HANDLE handle) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  int script_count = 0;
  player_handle->_smart_player_client->forEachScript(
      [&script_count](
          const std::shared_ptr<nativeformat::smartplayer::Script> &script) {
        script_count++;
        return true;
      });
  return script_count;
}

NF_SMART_PLAYER_SCRIPT_HANDLE smartplayer_open_script(
    NF_SMART_PLAYER_HANDLE handle, const char *name, const char *script) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  std::shared_ptr<nativeformat::smartplayer::Script> player_script =
      player_handle->_smart_player_client->openScript(name, script);
  return (NF_SMART_PLAYER_SCRIPT_HANDLE) new std::weak_ptr<
      nativeformat::smartplayer::Script>(player_script);
}

NF_SMART_PLAYER_SCRIPT_HANDLE smartplayer_get_script(
    NF_SMART_PLAYER_HANDLE handle, const char *name) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;

  if (auto player_script =
          player_handle->_smart_player_client->scriptForIdentifier(name)) {
    return (NF_SMART_PLAYER_SCRIPT_HANDLE) new std::weak_ptr<
        nativeformat::smartplayer::Script>(player_script);
  }

  return nullptr;
}

NF_SMART_PLAYER_SCRIPT_HANDLE smartplayer_get_script_at_index(
    NF_SMART_PLAYER_HANDLE handle, int script_index) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  std::shared_ptr<nativeformat::smartplayer::Script> player_script(nullptr);
  int script_count = 0;
  player_handle->_smart_player_client->forEachScript(
      [&script_count, script_index, &player_script](
          const std::shared_ptr<nativeformat::smartplayer::Script> &script) {
        if (script_count++ == script_index) {
          player_script = script;
        }
        return true;
      });
  return (NF_SMART_PLAYER_SCRIPT_HANDLE) new std::weak_ptr<
      nativeformat::smartplayer::Script>(player_script);
}

float smartplayer_get_value_for_path(NF_SMART_PLAYER_HANDLE handle,
                                     const char *path) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  return player_handle->_smart_player_client->valueForPath(path);
}

void smartplayer_set_value_for_path(NF_SMART_PLAYER_HANDLE handle,
                                    const char *path, ...) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  va_list args;
  va_start(args, path);
  player_handle->_smart_player_client->vSetValueForPath(path, args);
  va_end(args);
}

void smartplayer_vset_value_for_path(NF_SMART_PLAYER_HANDLE handle,
                                     const char *path, va_list args) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  player_handle->_smart_player_client->vSetValueForPath(path, args);
}

void smartplayer_set_values_for_path(NF_SMART_PLAYER_HANDLE handle,
                                     const char *path, float *values,
                                     size_t values_length) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  std::vector<float> vector_values(values, values + values_length);
  player_handle->_smart_player_client->setValuesForPath(path, vector_values);
}

int smartplayer_is_loaded(NF_SMART_PLAYER_HANDLE handle) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  return player_handle->_smart_player_client->isLoaded();
}

void *smartplayer_get_context(NF_SMART_PLAYER_HANDLE handle) {
  NF_SMART_PLAYER_HANDLE_STRUCT *player_handle =
      (NF_SMART_PLAYER_HANDLE_STRUCT *)handle;
  return player_handle->_context;
}

NF_SMART_PLAYER_GRAPH_HANDLE smartplayer_create_graph(
    const char *json, void *context,
    NF_SMART_PLAYER_LOAD_CALLBACK load_callback) {
  return new NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT(json, context, load_callback);
}

void smartplayer_graph_set_json(NF_SMART_PLAYER_GRAPH_HANDLE graph,
                                const char *json) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  handle->_graph->setJson(std::string(json),
                          [](const nativeformat::Load &load) {});
}

const char *smartplayer_graph_get_identifier(
    NF_SMART_PLAYER_GRAPH_HANDLE graph) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  return handle->_graph->identifier().c_str();
}

double smartplayer_graph_get_render_time(NF_SMART_PLAYER_GRAPH_HANDLE graph) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  return handle->_graph->renderTime();
}

void smartplayer_graph_set_render_time(NF_SMART_PLAYER_GRAPH_HANDLE graph,
                                       double render_time) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  handle->_graph->setRenderTime(render_time);
}

float smartplayer_graph_value_for_path(NF_SMART_PLAYER_GRAPH_HANDLE graph,
                                       const char *path) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  return handle->_graph->valueForPath(path);
}

void smartplayer_graph_set_value_for_path(NF_SMART_PLAYER_GRAPH_HANDLE graph,
                                          const char *path, ...) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  va_list args;
  va_start(args, path);
  handle->_graph->setValueForPath(path, args);
  va_end(args);
}

const char *smartplayer_graph_type(NF_SMART_PLAYER_GRAPH_HANDLE graph) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  return handle->_graph->type().c_str();
}

int smartplayer_graph_get_nodes(NF_SMART_PLAYER_GRAPH_HANDLE graph) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  int node_count = 0;
  handle->_graph->forEachNode(
      [&node_count](
          const std::shared_ptr<nativeformat::smartplayer::Node> &node) {
        node_count++;
        return true;
      });
  return node_count;
}

NF_SMART_PLAYER_NODE_HANDLE smartplayer_graph_get_node_at_index(
    NF_SMART_PLAYER_GRAPH_HANDLE graph, int node_index) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  std::shared_ptr<nativeformat::smartplayer::Node> player_node(nullptr);
  int node_count = 0;
  handle->_graph->forEachNode(
      [&node_count, node_index, &player_node](
          const std::shared_ptr<nativeformat::smartplayer::Node> &node) {
        if (node_count++ == node_index) {
          player_node = node;
          return false;
        }
        return true;
      });
  return (NF_SMART_PLAYER_NODE_HANDLE) new std::shared_ptr<
      nativeformat::smartplayer::Node>(player_node);
}

NF_SMART_PLAYER_PARAM_HANDLE smartplayer_graph_get_parameter(
    NF_SMART_PLAYER_GRAPH_HANDLE graph, const char *path) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;

  if (auto param = handle->_graph->parameterForPath(path)) {
    return (NF_SMART_PLAYER_PARAM_HANDLE) new std::weak_ptr<
        nativeformat::param::Param>(param);
  }

  return nullptr;
}

void smartplayer_graph_close(NF_SMART_PLAYER_GRAPH_HANDLE graph) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  delete handle;
}

int smartplayer_graph_loaded(NF_SMART_PLAYER_GRAPH_HANDLE graph) {
  NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *handle =
      (NF_SMART_PLAYER_GRAPH_HANDLE_STRUCT *)graph;
  return handle->_graph->loaded();
}

NF_SMART_PLAYER_EDGE_HANDLE smartplayer_create_edge(const char *source,
                                                    const char *target) {
  nfgrapher::Edge ge;
  ge.source = source;
  ge.target = target;
  ge.id = std::string(source) + std::string(target);
  std::shared_ptr<nativeformat::smartplayer::Edge> *player_edge =
      new std::shared_ptr<nativeformat::smartplayer::Edge>(
          nativeformat::smartplayer::createEdge(ge));
  return (NF_SMART_PLAYER_EDGE_HANDLE)player_edge;
}

NF_SMART_PLAYER_EDGE_HANDLE smartplayer_create_edge_json(const char *json) {
  std::shared_ptr<nativeformat::smartplayer::Edge> *player_edge =
      new std::shared_ptr<nativeformat::smartplayer::Edge>(
          nativeformat::smartplayer::createEdge(json));
  return (NF_SMART_PLAYER_EDGE_HANDLE)player_edge;
}

const char *smartplayer_edge_get_source(NF_SMART_PLAYER_EDGE_HANDLE edge) {
  std::shared_ptr<nativeformat::smartplayer::Edge> *player_edge =
      (std::shared_ptr<nativeformat::smartplayer::Edge> *)edge;
  return (*player_edge)->source().c_str();
}

const char *smartplayer_edge_get_target(NF_SMART_PLAYER_EDGE_HANDLE edge) {
  std::shared_ptr<nativeformat::smartplayer::Edge> *player_edge =
      (std::shared_ptr<nativeformat::smartplayer::Edge> *)edge;
  return (*player_edge)->target().c_str();
}

const char *smartplayer_edge_get_json(NF_SMART_PLAYER_EDGE_HANDLE edge) {
  std::shared_ptr<nativeformat::smartplayer::Edge> *player_edge =
      (std::shared_ptr<nativeformat::smartplayer::Edge> *)edge;
  return (*player_edge)->json().c_str();
}

void smartplayer_edge_close(NF_SMART_PLAYER_EDGE_HANDLE edge) {
  std::shared_ptr<nativeformat::smartplayer::Edge> *player_edge =
      (std::shared_ptr<nativeformat::smartplayer::Edge> *)edge;
  delete player_edge;
}

const char *smartplayer_edge_get_identifier(NF_SMART_PLAYER_EDGE_HANDLE edge) {
  std::shared_ptr<nativeformat::smartplayer::Edge> *player_edge =
      (std::shared_ptr<nativeformat::smartplayer::Edge> *)edge;
  return (*player_edge)->identifier().c_str();
}

NF_SMART_PLAYER_NODE_HANDLE smartplayer_create_node(const char *json) {
  std::shared_ptr<nativeformat::smartplayer::Node> *player_node =
      new std::shared_ptr<nativeformat::smartplayer::Node>(
          nativeformat::smartplayer::createNode(json));
  return (NF_SMART_PLAYER_NODE_HANDLE)player_node;
}

const char *smartplayer_node_identifier(
    NF_SMART_PLAYER_NODE_HANDLE node_handle) {
  std::shared_ptr<nativeformat::smartplayer::Node> *player_node =
      (std::shared_ptr<nativeformat::smartplayer::Node> *)node_handle;
  return (*player_node)->identifier().c_str();
}

const char *smartplayer_node_type(NF_SMART_PLAYER_NODE_HANDLE node_handle) {
  std::shared_ptr<nativeformat::smartplayer::Node> *player_node =
      (std::shared_ptr<nativeformat::smartplayer::Node> *)node_handle;
  return (*player_node)->type().c_str();
}

int smartplayer_node_is_output_node(NF_SMART_PLAYER_NODE_HANDLE node_handle) {
  std::shared_ptr<nativeformat::smartplayer::Node> *player_node =
      (std::shared_ptr<nativeformat::smartplayer::Node> *)node_handle;
  return (*player_node)->isOutputNode();
}

const char *smartplayer_node_json(NF_SMART_PLAYER_NODE_HANDLE node_handle) {
  std::shared_ptr<nativeformat::smartplayer::Node> *player_node =
      (std::shared_ptr<nativeformat::smartplayer::Node> *)node_handle;
  return (*player_node)->toJSON().c_str();
}

int smartplayer_node_get_params(NF_SMART_PLAYER_NODE_HANDLE node_handle) {
  std::shared_ptr<nativeformat::smartplayer::Node> *player_node =
      (std::shared_ptr<nativeformat::smartplayer::Node> *)node_handle;
  int param_count = 0;
  (*player_node)
      ->forEachParam(
          [&param_count](
              const std::shared_ptr<nativeformat::param::Param> &param) {
            param_count++;
            return true;
          });
  return param_count;
}

NF_SMART_PLAYER_PARAM_HANDLE smartplayer_node_get_param(
    NF_SMART_PLAYER_NODE_HANDLE node_handle, const char *param_identifier) {
  std::shared_ptr<nativeformat::smartplayer::Node> *player_node =
      (std::shared_ptr<nativeformat::smartplayer::Node> *)node_handle;

  if (auto param = (*player_node)->paramForIdentifier(param_identifier)) {
    return (NF_SMART_PLAYER_PARAM_HANDLE) new std::weak_ptr<
        nativeformat::param::Param>(param);
  }

  return nullptr;
}

NF_SMART_PLAYER_PARAM_HANDLE smartplayer_node_get_param_at_index(
    NF_SMART_PLAYER_NODE_HANDLE node_handle, int param_index) {
  std::shared_ptr<nativeformat::smartplayer::Node> *player_node =
      (std::shared_ptr<nativeformat::smartplayer::Node> *)node_handle;
  std::shared_ptr<nativeformat::param::Param> player_param(nullptr);
  int param_count = 0;
  (*player_node)
      ->forEachParam(
          [&param_count, param_index, &player_param](
              const std::shared_ptr<nativeformat::param::Param> &param) {
            if (param_count++ == param_index) {
              player_param = param;
              return false;
            }
            return true;
          });
  return (NF_SMART_PLAYER_PARAM_HANDLE) new std::weak_ptr<
      nativeformat::param::Param>(player_param);
}

void smartplayer_node_close(NF_SMART_PLAYER_NODE_HANDLE node_handle) {
  std::shared_ptr<nativeformat::smartplayer::Node> *player_node =
      (std::shared_ptr<nativeformat::smartplayer::Node> *)node_handle;
  delete player_node;
}

const char *smartplayer_script_name(NF_SMART_PLAYER_SCRIPT_HANDLE script) {
  std::weak_ptr<nativeformat::smartplayer::Script> *player_script =
      (std::weak_ptr<nativeformat::smartplayer::Script> *)script;
  if (auto player_script_strong = (*player_script).lock()) {
    return player_script_strong->name().c_str();
  }
  return "";
}

NF_SMART_PLAYER_SCRIPT_SCOPE smartplayer_script_scope(
    NF_SMART_PLAYER_SCRIPT_HANDLE script) {
  std::weak_ptr<nativeformat::smartplayer::Script> *player_script =
      (std::weak_ptr<nativeformat::smartplayer::Script> *)script;
  if (auto player_script_strong = (*player_script).lock()) {
    return (NF_SMART_PLAYER_SCRIPT_SCOPE)player_script_strong->scope();
  }
  return NF_SMART_PLAYER_SCRIPT_SCOPE_NONE;
}

void smartplayer_script_close(NF_SMART_PLAYER_SCRIPT_HANDLE script) {
  std::weak_ptr<nativeformat::smartplayer::Script> *player_script =
      (std::weak_ptr<nativeformat::smartplayer::Script> *)script;
  delete player_script;
}

void smartplayer_param_close(NF_SMART_PLAYER_PARAM_HANDLE param) {
  std::weak_ptr<nativeformat::param::Param> *player_param =
      (std::weak_ptr<nativeformat::param::Param> *)param;
  delete player_param;
}

const char *smartplayer_driver_type_to_string(
    NF_SMART_PLAYER_DRIVER_TYPE driver_type) {
  switch (driver_type) {
    case NF_SMART_PLAYER_DRIVER_TYPE_FILE:
      return NF_SMART_PLAYER_DRIVER_TYPE_FILE_STRING;
    case NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD:
      return NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD_STRING;
  }
}

NF_SMART_PLAYER_DRIVER_TYPE smartplayer_driver_type_from_string(
    const char *driver_type_string) {
  if (strcmp(driver_type_string, NF_SMART_PLAYER_DRIVER_TYPE_FILE_STRING) ==
      0) {
    return NF_SMART_PLAYER_DRIVER_TYPE_FILE;
  }
  return NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD;
}

static_assert(
    nativeformat::smartplayer::NFSmartPlayerMessageType::
            NFSmartPlayerMessageTypeGeneric ==
        NF_SMART_PLAYER_MESSAGE_TYPE_GENERIC,
    "The generic message type must match in the C and C++ enumerations");
static_assert(
    nativeformat::smartplayer::NFSmartPlayerMessageType::
            NFSmartPlayerMessageTypeNone == NF_SMART_PLAYER_MESSAGE_TYPE_NONE,
    "The generic message type must match in the C and C++ enumerations");
static_assert(nativeformat::smartplayer::ScriptScope::ScriptScopeNone ==
                  NF_SMART_PLAYER_SCRIPT_SCOPE_NONE,
              "The none scope type must match in C and C++ enumerations");
static_assert(nativeformat::smartplayer::ScriptScope::ScriptScopeGraph ==
                  NF_SMART_PLAYER_SCRIPT_SCOPE_GRAPH,
              "The graph scope type must match in C and C++ enumerations");
static_assert(nativeformat::smartplayer::ScriptScope::ScriptScopeSession ==
                  NF_SMART_PLAYER_SCRIPT_SCOPE_SESSION,
              "The session scope type must match in C and C++ enumerations");
static_assert(nativeformat::smartplayer::DriverType::DriverTypeFile ==
                  NF_SMART_PLAYER_DRIVER_TYPE_FILE,
              "The driver type file must match in C and C++ enumerations");
static_assert(
    nativeformat::smartplayer::DriverType::DriverTypeSoundCard ==
        NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD,
    "The driver type sound card must match in C and C++ enumerations");
