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
#ifndef NF_SMART_PLAYER_H
#define NF_SMART_PLAYER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdarg.h>
#include <stddef.h>

#define smartplayer_cast_message_type(message_type_a1, payload_a2, result_a3) \
  {                                                                           \
    int success = 0;                                                          \
    switch (message_type_a1) {                                                \
      case NF_SMART_PLAYER_MESSAGE_TYPE_GENERIC: {                            \
        result_a3 = (const char *)payload_a2;                                 \
        success = 1;                                                          \
      }                                                                       \
      case NF_SMART_PLAYER_MESSAGE_TYPE_NONE: {                               \
        result_a3 = NULL;                                                     \
        success = 1;                                                          \
      }                                                                       \
    }                                                                         \
    success;                                                                  \
  }

typedef enum {
  NF_SMART_PLAYER_MESSAGE_TYPE_GENERIC,
  NF_SMART_PLAYER_MESSAGE_TYPE_NONE
} NF_SMART_PLAYER_MESSAGE_TYPE;

typedef enum {
  NF_SMART_PLAYER_SCRIPT_SCOPE_NONE,
  NF_SMART_PLAYER_SCRIPT_SCOPE_GRAPH,
  NF_SMART_PLAYER_SCRIPT_SCOPE_SESSION
} NF_SMART_PLAYER_SCRIPT_SCOPE;

typedef enum {
  NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD,
  NF_SMART_PLAYER_DRIVER_TYPE_FILE
} NF_SMART_PLAYER_DRIVER_TYPE;

typedef void *NF_SMART_PLAYER_HANDLE;
typedef void *NF_SMART_PLAYER_SCRIPT_HANDLE;
typedef void *NF_SMART_PLAYER_GRAPH_HANDLE;
typedef void *NF_SMART_PLAYER_PARAM_HANDLE;
typedef void *NF_SMART_PLAYER_EDGE_HANDLE;
typedef void *NF_SMART_PLAYER_NODE_HANDLE;
typedef const char *(*NF_SMART_PLAYER_RESOLVE_CALLBACK)(
    void *context, const char *plugin_namespace,
    const char *variable_identifier);
typedef void (*NF_SMART_PLAYER_LOAD_CALLBACK)(void *context, int success,
                                              const char *error_message);
typedef void (*NF_SMART_PLAYER_MESSAGE_CALLBACK)(
    void *context, const char *message_identifier,
    const char *sender_identifier, NF_SMART_PLAYER_MESSAGE_TYPE message_type,
    const void *payload);
typedef struct NF_SMART_PLAYER_SETTINGS {
  int _osc_read_port;
  int _osc_write_port;
  const char *_osc_address;
  int _localhost_port;
  int _pump_manually;
} NF_SMART_PLAYER_SETTINGS;

extern const char *NF_SMART_PLAYER_OSC_ADDRESS;
extern const char *NF_SMART_PLAYER_DRIVER_TYPE_FILE_STRING;
extern const char *NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD_STRING;
extern const char *NF_SMART_PLAYER_SENDER_IDENTIFIER;
extern const char *NF_SMART_PLAYER_MESSAGE_IDENTIFIER_END;
extern const char *NF_SMART_PLAYER_VERSION;
extern const int NF_SMART_PLAYER_OSC_READ_PORT;
extern const int NF_SMART_PLAYER_OSC_WRITE_PORT;
extern const int NF_SMART_PLAYER_LOCALHOST_PORT;
extern const NF_SMART_PLAYER_SETTINGS NF_SMART_PLAYER_DEFAULT_SETTINGS;

// Player
extern NF_SMART_PLAYER_HANDLE smartplayer_open(
    NF_SMART_PLAYER_RESOLVE_CALLBACK resolve_callback, void *context,
    NF_SMART_PLAYER_SETTINGS settings, NF_SMART_PLAYER_DRIVER_TYPE driver_type,
    const char *output_destination);
extern void smartplayer_close(NF_SMART_PLAYER_HANDLE handle);
extern int smartplayer_get_graphs(NF_SMART_PLAYER_HANDLE handle);
extern NF_SMART_PLAYER_GRAPH_HANDLE smartplayer_get_graph(
    NF_SMART_PLAYER_HANDLE handle, const char *graph_identifier);
extern NF_SMART_PLAYER_GRAPH_HANDLE smartplayer_get_graph_at_index(
    NF_SMART_PLAYER_HANDLE handle, int graph_index);
extern void smartplayer_set_json(NF_SMART_PLAYER_HANDLE handle,
                                 const char *json,
                                 NF_SMART_PLAYER_LOAD_CALLBACK load_callback);
extern void smartplayer_add_graph(NF_SMART_PLAYER_HANDLE handle,
                                  NF_SMART_PLAYER_GRAPH_HANDLE graph);
extern void smartplayer_remove_graph(NF_SMART_PLAYER_HANDLE handle,
                                     const char *graph_identifier);
extern int smartplayer_is_playing(NF_SMART_PLAYER_HANDLE handle);
extern void smartplayer_set_playing(NF_SMART_PLAYER_HANDLE handle, int playing);
extern double smartplayer_render_time(NF_SMART_PLAYER_HANDLE handle);
extern void smartplayer_set_render_time(NF_SMART_PLAYER_HANDLE handle,
                                        double time);
extern void smartplayer_set_message_callback(
    NF_SMART_PLAYER_HANDLE handle,
    NF_SMART_PLAYER_MESSAGE_CALLBACK message_callback);
extern void smartplayer_send_message(NF_SMART_PLAYER_HANDLE handle,
                                     const char *message_identifier,
                                     NF_SMART_PLAYER_MESSAGE_TYPE message_type,
                                     const void *payload);
extern int smartplayer_get_scripts(NF_SMART_PLAYER_HANDLE handle);
extern NF_SMART_PLAYER_SCRIPT_HANDLE smartplayer_open_script(
    NF_SMART_PLAYER_HANDLE handle, const char *name, const char *script);
extern NF_SMART_PLAYER_SCRIPT_HANDLE smartplayer_get_script(
    NF_SMART_PLAYER_HANDLE handle, const char *name);
extern NF_SMART_PLAYER_SCRIPT_HANDLE smartplayer_get_script_at_index(
    NF_SMART_PLAYER_HANDLE handle, int script_index);
extern float smartplayer_get_value_for_path(NF_SMART_PLAYER_HANDLE handle,
                                            const char *path);
extern void smartplayer_set_value_for_path(NF_SMART_PLAYER_HANDLE handle,
                                           const char *path, ...);
extern void smartplayer_vset_value_for_path(NF_SMART_PLAYER_HANDLE handle,
                                            const char *path, va_list args);
extern void smartplayer_set_values_for_path(NF_SMART_PLAYER_HANDLE handle,
                                            const char *path, float *values,
                                            size_t values_length);
extern int smartplayer_is_loaded(NF_SMART_PLAYER_HANDLE handle);
extern void *smartplayer_get_context(NF_SMART_PLAYER_HANDLE handle);

// Graph
extern NF_SMART_PLAYER_GRAPH_HANDLE smartplayer_create_graph(
    const char *json, void *context,
    NF_SMART_PLAYER_LOAD_CALLBACK load_callback);
extern void smartplayer_graph_set_json(NF_SMART_PLAYER_GRAPH_HANDLE graph,
                                       const char *json);
extern const char *smartplayer_graph_get_identifier(
    NF_SMART_PLAYER_GRAPH_HANDLE graph);
extern double smartplayer_graph_get_render_time(
    NF_SMART_PLAYER_GRAPH_HANDLE graph);
extern void smartplayer_graph_set_render_time(
    NF_SMART_PLAYER_GRAPH_HANDLE graph, double render_time);
extern float smartplayer_graph_value_for_path(
    NF_SMART_PLAYER_GRAPH_HANDLE graph, const char *path);
extern void smartplayer_graph_set_value_for_path(
    NF_SMART_PLAYER_GRAPH_HANDLE graph, const char *path, ...);
extern const char *smartplayer_graph_type(NF_SMART_PLAYER_GRAPH_HANDLE graph);
extern int smartplayer_graph_get_nodes(NF_SMART_PLAYER_GRAPH_HANDLE graph);
extern NF_SMART_PLAYER_NODE_HANDLE smartplayer_graph_get_node_at_index(
    NF_SMART_PLAYER_GRAPH_HANDLE graph, int node_index);
extern NF_SMART_PLAYER_PARAM_HANDLE smartplayer_graph_get_parameter(
    NF_SMART_PLAYER_GRAPH_HANDLE graph, const char *path);
extern void smartplayer_graph_close(NF_SMART_PLAYER_GRAPH_HANDLE graph);
extern int smartplayer_graph_loaded(NF_SMART_PLAYER_GRAPH_HANDLE graph);

// Edge
extern NF_SMART_PLAYER_EDGE_HANDLE smartplayer_create_edge(const char *source,
                                                           const char *target);
extern NF_SMART_PLAYER_EDGE_HANDLE smartplayer_create_edge_json(
    const char *json);
extern const char *smartplayer_edge_get_source(
    NF_SMART_PLAYER_EDGE_HANDLE edge);
extern const char *smartplayer_edge_get_target(
    NF_SMART_PLAYER_EDGE_HANDLE edge);
extern const char *smartplayer_edge_get_json(NF_SMART_PLAYER_EDGE_HANDLE edge);
extern void smartplayer_edge_close(NF_SMART_PLAYER_EDGE_HANDLE edge);
extern const char *smartplayer_edge_get_identifier(
    NF_SMART_PLAYER_EDGE_HANDLE edge);

// Node
extern NF_SMART_PLAYER_NODE_HANDLE smartplayer_create_node(const char *json);
extern const char *smartplayer_node_identifier(
    NF_SMART_PLAYER_NODE_HANDLE node_handle);
extern const char *smartplayer_node_type(
    NF_SMART_PLAYER_NODE_HANDLE node_handle);
extern int smartplayer_node_is_output_node(
    NF_SMART_PLAYER_NODE_HANDLE node_handle);
extern const char *smartplayer_node_json(
    NF_SMART_PLAYER_NODE_HANDLE node_handle);
extern int smartplayer_node_get_params(NF_SMART_PLAYER_NODE_HANDLE node_handle);
extern NF_SMART_PLAYER_PARAM_HANDLE smartplayer_node_get_param(
    NF_SMART_PLAYER_NODE_HANDLE node_handle, const char *param_identifier);
extern NF_SMART_PLAYER_PARAM_HANDLE smartplayer_node_get_param_at_index(
    NF_SMART_PLAYER_NODE_HANDLE node_handle, int param_index);
extern void smartplayer_node_close(NF_SMART_PLAYER_NODE_HANDLE node_handle);

// Script
extern const char *smartplayer_script_name(
    NF_SMART_PLAYER_SCRIPT_HANDLE script);
extern NF_SMART_PLAYER_SCRIPT_SCOPE smartplayer_script_scope(
    NF_SMART_PLAYER_SCRIPT_HANDLE script);
extern void smartplayer_script_close(NF_SMART_PLAYER_SCRIPT_HANDLE script);

// Param
extern void smartplayer_param_close(NF_SMART_PLAYER_PARAM_HANDLE param);

// Utilities
extern const char *smartplayer_driver_type_to_string(
    NF_SMART_PLAYER_DRIVER_TYPE driver_type);
extern NF_SMART_PLAYER_DRIVER_TYPE smartplayer_driver_type_from_string(
    const char *driver_type_string);

#ifdef __cplusplus
}
#endif

#endif
