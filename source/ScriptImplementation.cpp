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
#include "ScriptImplementation.h"

#include <boost/algorithm/string.hpp>

#include "Player.h"

static duk_ret_t native_print(duk_context *ctx) {
  duk_push_string(ctx, " ");
  duk_insert(ctx, 0);
  duk_join(ctx, duk_get_top(ctx) - 1);
  printf("%s\n", duk_safe_to_string(ctx, -1));
  return 0;
}

namespace nativeformat {
namespace smartplayer {

static const std::string ScriptImplementationContextProperty(
    "\xFF"
    "com.nativeformat.smartplayer.script");
static const std::string ScriptImplementationGraphProperty(
    "\xFF"
    "com.nativeformat.smartplayer.graph");
static const std::string ScriptImplementationNodeProperty(
    "\xFF"
    "com.nativeformat.smartplayer.graph.node");
static const std::string ScriptImplementationEdgeProperty(
    "\xFF"
    "com.nativeformat.smartplayer.graph.edge");
static const std::string ScriptImplementationScriptProperty(
    "\xFF"
    "com.nativeformat.smartplayer.graph.script");

static ScriptImplementation *ScriptImplementationFromContext(
    duk_context *context);
static std::shared_ptr<Graph> *ScriptImplementationGraphFromContext(
    duk_context *context);
static std::shared_ptr<Node> *ScriptImplementationNodeFromContext(
    duk_context *context);
static std::shared_ptr<Edge> *ScriptImplementationEdgeFromContext(
    duk_context *context);
static std::shared_ptr<Script> *ScriptImplementationScriptFromContext(
    duk_context *context);

ScriptImplementation::ScriptImplementation(
    const std::string &name, const std::string &script, ScriptScope scope,
    std::weak_ptr<ScriptDelegate> delegate, boost::asio::io_service &io_service,
    const std::string &graph_identifier)
    : _name(name),
      _script(script),
      _scope(scope),
      _delegate(delegate),
      _graph_identifier(graph_identifier),
      _io_service(io_service),
      _js_context(duk_create_heap_default()),
      _handle_count(0) {
  const duk_function_list_entry nf_module_funcs[] = {
      {"getPlayer", &ScriptImplementation::scriptGetPlayer, 0},
      {NULL, NULL, 0}};

  // Store the pointer to this class in the global stash
  duk_push_global_stash(_js_context);
  duk_push_pointer(_js_context, (void *)this);
  duk_put_prop_string(_js_context, -2,
                      ScriptImplementationContextProperty.c_str());

  // Add the global NF module
  duk_push_global_object(_js_context);
  duk_push_object(_js_context);
  duk_put_function_list(_js_context, -1, nf_module_funcs);
  duk_put_prop_string(_js_context, -2, "NF");
  duk_pop(_js_context);

  // Add the constructor for nodes
  duk_push_c_function(_js_context,
                      &ScriptImplementation::scriptGraphNodeConstructor, 1);
  duk_push_object(_js_context);
  duk_put_prop_string(_js_context, -2, "prototype");
  duk_put_global_string(_js_context, "Node");

  // Add the constructor for edges
  duk_push_c_function(_js_context,
                      &ScriptImplementation::scriptGraphEdgeConstructor, 3);
  duk_push_object(_js_context);
  duk_put_prop_string(_js_context, -2, "prototype");
  duk_put_global_string(_js_context, "Edge");

  // Add the constructor for graphs
  duk_push_c_function(_js_context,
                      &ScriptImplementation::scriptGraphConstructor, 1);
  duk_push_object(_js_context);
  duk_put_prop_string(_js_context, -2, "prototype");
  duk_put_global_string(_js_context, "Graph");

  // Add the event functions
  duk_push_c_function(_js_context, &ScriptImplementation::scriptSetTimeout, 2);
  duk_put_global_string(_js_context, "setTimeout");
  duk_push_c_function(_js_context, &ScriptImplementation::scriptSetInterval, 2);
  duk_put_global_string(_js_context, "setInterval");
  duk_push_c_function(_js_context, &ScriptImplementation::scriptClearTimeout,
                      1);
  duk_put_global_string(_js_context, "clearTimeout");
  duk_push_c_function(_js_context, &ScriptImplementation::scriptClearInterval,
                      1);
  duk_put_global_string(_js_context, "clearInterval");

  // Add some miscellaneous debug tools
  duk_push_c_function(_js_context, native_print, DUK_VARARGS);
  duk_put_global_string(_js_context, "print");
}

ScriptImplementation::~ScriptImplementation() {
  std::lock_guard<std::mutex> timer_map_lock(_timer_mutex);
  duk_destroy_heap(_js_context);
  for (auto &timer : _timer_map) {
    timer.second->cancel();
  }
}

std::string ScriptImplementation::name() const { return _name; }

ScriptScope ScriptImplementation::scope() const { return _scope; }

void ScriptImplementation::close() {
  if (auto delegate = _delegate.lock()) {
    delegate->scriptDidClose(shared_from_this());
  }
}

std::string ScriptImplementation::code() const { return _script; }

void ScriptImplementation::run() {
  std::lock_guard<std::mutex> run_lock(_run_mutex);
  if (duk_peval_string(_js_context, _script.c_str()) != 0) {
    printf("Eval Failed: %s\n", duk_safe_to_string(_js_context, -1));
  } else {
    _running = true;
  }
}

bool ScriptImplementation::isRunning() const { return _running; }

void ScriptImplementation::processNotification(
    const Notification &notification) {
#ifndef __clang_analyzer__
  // We have a bug in the clang static analyzer when it comes to regex_match
  std::lock_guard<std::mutex> run_lock(_run_mutex);
  for (const auto &internal_notification : _notifications) {
    if (!std::regex_match(notification.name(),
                          internal_notification.first._message_regex) ||
        !std::regex_match(notification.senderIdentifier(),
                          internal_notification.first._sender_regex)) {
      continue;
    }
    duk_push_heapptr(_js_context, internal_notification.second);
    duk_push_string(_js_context, notification.name().c_str());
    duk_push_string(_js_context, notification.senderIdentifier().c_str());
    duk_push_int(_js_context, notification.messageType());
    switch (notification.messageType()) {
      case NFSmartPlayerMessageTypeGeneric: {
        duk_push_string(_js_context, (const char *)notification.payload());
        break;
      }
      case NFSmartPlayerMessageTypeNone:
      case NFSmartPlayerMessageTypeError:
        break;
    }
    duk_call(_js_context, 4);
  }
#endif
}

duk_ret_t ScriptImplementation::scriptGetPlayer(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);

  // Create a player object
  duk_idx_t obj = duk_push_object(context);
  createJSPlayerObject(context, obj, script);

  return 1;
}

duk_ret_t ScriptImplementation::scriptGetGraphs(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);

  duk_idx_t arr_idx = duk_push_array(context);

  // Create the graph objects
  if (auto delegate = script->_delegate.lock()) {
    size_t i = 0;
    delegate->forEachGraph(
        [context, arr_idx, &i, script](const std::shared_ptr<Graph> &graph) {
          duk_idx_t obj = duk_push_object(context);
          createJSGraphObject(context, obj, graph, script);
          duk_put_prop_index(context, arr_idx, i++);
          return true;
        });
  }

  return 1;
}

duk_ret_t ScriptImplementation::scriptSetGraphJson(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  std::shared_ptr<Graph> *graph = ScriptImplementationGraphFromContext(context);
  if (script->scope() == ScriptScopeGraph &&
      script->_graph_identifier != (*graph)->identifier()) {
    return 0;
  }
  (*graph)->setJson(std::string(duk_get_string(context, 0)),
                    [](const Load &load) {});
  return 0;
}

duk_ret_t ScriptImplementation::scriptGetGraphRenderTime(duk_context *context) {
  std::shared_ptr<Graph> *graph = ScriptImplementationGraphFromContext(context);
  duk_push_number(context, (*graph)->renderTime());
  return 1;
}

duk_ret_t ScriptImplementation::scriptSetGraphRenderTime(duk_context *context) {
  std::shared_ptr<Graph> *graph = ScriptImplementationGraphFromContext(context);
  (*graph)->setRenderTime(duk_get_number(context, 0));
  return 0;
}

duk_ret_t ScriptImplementation::scriptGetGraphVariable(duk_context *context) {
  std::shared_ptr<Graph> *graph = ScriptImplementationGraphFromContext(context);
  duk_push_number(context, (*graph)->valueForPath(duk_get_string(context, 0)));
  return 1;
}

duk_ret_t ScriptImplementation::scriptSetGraphVariable(duk_context *context) {
  std::shared_ptr<Graph> *graph = ScriptImplementationGraphFromContext(context);
  std::vector<std::string> major_split;
  const std::string path = duk_get_string(context, 0);
  boost::split(major_split, path, boost::is_any_of("/"));
  if (major_split.size() < NFSmartPlayerVariableComponentCount) {
    return 0;
  }
  const std::string &command = major_split[NFSmartPlayerCommandComponentIndex];
  if (command == "value" || command == "setValue") {
    (*graph)->setValueForPath(path, duk_get_number(context, 1));
  } else if (command == "setValueAtTime") {
    (*graph)->setValueForPath(path, duk_get_number(context, 1),
                              duk_get_number(context, 2));
  } else if (command == "linearRampToValueAtTime") {
    (*graph)->setValueForPath(path, duk_get_number(context, 1),
                              duk_get_number(context, 2));
  } else if (command == "setTargetAtTime") {
    (*graph)->setValueForPath(path, duk_get_number(context, 1),
                              duk_get_number(context, 2),
                              duk_get_number(context, 3));
  } else if (command == "exponentialRampToValueAtTime") {
    (*graph)->setValueForPath(path, duk_get_number(context, 1),
                              duk_get_number(context, 2));
  } else if (command == "setValueCurveAtTime") {
    std::vector<float> values;
    if (duk_is_array(context, 0)) {
      duk_enum(context, 0, DUK_ENUM_ARRAY_INDICES_ONLY);
      while (duk_next(context, 0, 0)) {
        values.push_back(duk_get_number(context, 0));
        duk_pop(context);
      }
    }
    (*graph)->setValueForPath(path, &values[0], values.size(),
                              duk_get_number(context, 1),
                              duk_get_number(context, 2));
  }
  return 0;
}

duk_ret_t ScriptImplementation::scriptGetGraphNodes(duk_context *context) {
  std::shared_ptr<Graph> *graph = ScriptImplementationGraphFromContext(context);
  duk_idx_t arr_idx = duk_push_array(context);
  size_t i = 0;
  (*graph)->forEachNode(
      [context, arr_idx, &i](const std::shared_ptr<Node> &node) {
        duk_idx_t obj = duk_push_object(context);
        createJSNodeObject(context, obj, node);
        duk_put_prop_index(context, arr_idx, i++);
        return true;
      });
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetGraphNodeIdentifier(
    duk_context *context) {
  std::shared_ptr<Node> *node = ScriptImplementationNodeFromContext(context);
  duk_push_string(context, (*node)->identifier().c_str());
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetGraphNodeType(duk_context *context) {
  std::shared_ptr<Node> *node = ScriptImplementationNodeFromContext(context);
  duk_push_string(context, (*node)->type().c_str());
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetGraphNodeIsOutputNode(
    duk_context *context) {
  std::shared_ptr<Node> *node = ScriptImplementationNodeFromContext(context);
  duk_push_boolean(context, (*node)->isOutputNode());
  return 1;
}

duk_ret_t ScriptImplementation::scriptGraphNodeConstructor(
    duk_context *context) {
  if (!duk_is_constructor_call(context)) {
    return DUK_RET_TYPE_ERROR;
  }
  std::shared_ptr<Node> node = createNode(duk_get_string(context, 0));
  duk_push_this(context);
  duk_dup(context, 0);
  createJSNodeObject(context, -2, node);
  return 0;
}

duk_ret_t ScriptImplementation::scriptGraphNodeDestructor(
    duk_context *context) {
  std::shared_ptr<Node> *node = ScriptImplementationNodeFromContext(context);
  delete node;
  return 0;
}

duk_ret_t ScriptImplementation::scriptGraphConstructor(duk_context *context) {
  if (!duk_is_constructor_call(context)) {
    return DUK_RET_TYPE_ERROR;
  }
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  std::shared_ptr<Graph> graph = createGraph(
      duk_get_string(context, 0), [](const Load &load) {}, 2, 44100.0);
  duk_push_this(context);
  duk_dup(context, 0);
  createJSGraphObject(context, -2, graph, script);
  return 0;
}

duk_ret_t ScriptImplementation::scriptGraphDestructor(duk_context *context) {
  std::shared_ptr<Graph> *graph = ScriptImplementationGraphFromContext(context);
  delete graph;
  return 0;
}

duk_ret_t ScriptImplementation::scriptGraphEdgeConstructor(
    duk_context *context) {
  if (!duk_is_constructor_call(context)) {
    return DUK_RET_TYPE_ERROR;
  }
  nfgrapher::Edge ge;
  ge.source = duk_get_string(context, 0);
  ge.target = duk_get_string(context, 1);
  ge.id = ge.source + ge.target;
  std::shared_ptr<Edge> edge = createEdge(ge);
  duk_push_this(context);
  duk_dup(context, 0);
  createJSEdgeObject(context, -2, edge);
  return 0;
}

duk_ret_t ScriptImplementation::scriptGraphEdgeDestructor(
    duk_context *context) {
  std::shared_ptr<Edge> *edge = ScriptImplementationEdgeFromContext(context);
  delete edge;
  return 0;
}

duk_ret_t ScriptImplementation::scriptGetGraphEdgeSource(duk_context *context) {
  std::shared_ptr<Edge> *edge = ScriptImplementationEdgeFromContext(context);
  duk_push_string(context, (*edge)->source().c_str());
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetGraphEdgeTarget(duk_context *context) {
  std::shared_ptr<Edge> *edge = ScriptImplementationEdgeFromContext(context);
  duk_push_string(context, (*edge)->target().c_str());
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetGraphEdgeJSON(duk_context *context) {
  std::shared_ptr<Edge> *edge = ScriptImplementationEdgeFromContext(context);
  duk_push_string(context, (*edge)->json().c_str());
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetGraphType(duk_context *context) {
  std::shared_ptr<Graph> *graph = ScriptImplementationGraphFromContext(context);
  duk_push_string(context, (*graph)->type().c_str());
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetGraphIdentifier(duk_context *context) {
  std::shared_ptr<Graph> *graph = ScriptImplementationGraphFromContext(context);
  duk_push_string(context, (*graph)->identifier().c_str());
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetPlaying(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  if (auto delegate = script->_delegate.lock()) {
    duk_push_boolean(context, delegate->playing());
  } else {
    duk_push_boolean(context, false);
  }
  return 1;
}

duk_ret_t ScriptImplementation::scriptSetPlaying(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  if (script->scope() == ScriptScopeGraph) {
    return 0;
  }
  if (auto delegate = script->_delegate.lock()) {
    delegate->setPlaying(duk_get_boolean(context, 0));
  }
  return 0;
}

duk_ret_t ScriptImplementation::scriptGetRenderTime(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  if (auto delegate = script->_delegate.lock()) {
    duk_push_number(context, delegate->renderTime());
  } else {
    duk_push_number(context, 0.0);
  }
  return 1;
}

duk_ret_t ScriptImplementation::scriptSetRenderTime(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  if (script->scope() == ScriptScopeGraph) {
    return 0;
  }
  if (auto delegate = script->_delegate.lock()) {
    delegate->setRenderTime(duk_get_number(context, 0));
  }
  return 0;
}

duk_ret_t ScriptImplementation::scriptRegisterMessage(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  ScriptNotificationKey key(duk_get_string(context, 0),
                            duk_get_string(context, 1),
                            script->_handle_count++);
  script->_notifications[key] = duk_get_heapptr(context, 2);
  duk_push_int(context, key._handle);
  return 1;
}

duk_ret_t ScriptImplementation::scriptSendMessage(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  if (auto delegate = script->_delegate.lock()) {
    NFSmartPlayerMessageType message_type =
        (NFSmartPlayerMessageType)duk_get_int(context, 1);
    const void *payload = nullptr;
    switch (message_type) {
      case NFSmartPlayerMessageTypeGeneric: {
        payload = duk_get_string(context, 2);
        break;
      }
      case NFSmartPlayerMessageTypeError:
      case NFSmartPlayerMessageTypeNone:
        break;
    }
    delegate->sendMessage(duk_get_string(context, 0), script->name(),
                          message_type, payload);
  }
  return 0;
}

duk_ret_t ScriptImplementation::scriptUnregisterMessage(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  int handle = duk_get_int(context, 0);
  for (auto it = script->_notifications.begin();
       it != script->_notifications.end(); ++it) {
    if ((*it).first._handle == handle) {
      script->_notifications.erase(it);
      break;
    }
  }
  return 0;
}

duk_ret_t ScriptImplementation::scriptSetJSON(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  if (script->scope() == ScriptScopeGraph) {
    return 0;
  }
  if (auto delegate = script->_delegate.lock()) {
    delegate->setJson(duk_get_string(context, 0));
  }
  return 0;
}

duk_ret_t ScriptImplementation::scriptGetScripts(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  duk_idx_t arr_idx = duk_push_array(context);
  if (auto delegate = script->_delegate.lock()) {
    size_t i = 0;
    delegate->forEachScript(
        [context, arr_idx, &i](const std::shared_ptr<Script> &player_script) {
          duk_idx_t obj = duk_push_object(context);
          createJSScriptObject(context, obj, player_script);
          duk_put_prop_index(context, arr_idx, i++);
          return true;
        });
  }
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetScriptName(duk_context *context) {
  std::shared_ptr<Script> *script =
      ScriptImplementationScriptFromContext(context);
  duk_push_string(context, (*script)->name().c_str());
  return 1;
}

duk_ret_t ScriptImplementation::scriptGetScriptScope(duk_context *context) {
  std::shared_ptr<Script> *script =
      ScriptImplementationScriptFromContext(context);
  duk_push_int(context, (*script)->scope());
  return 1;
}

duk_ret_t ScriptImplementation::scriptCloseScript(duk_context *context) {
  ScriptImplementation *script = ScriptImplementationFromContext(context);
  std::shared_ptr<Script> *player_script =
      ScriptImplementationScriptFromContext(context);
  if (script->scope() == ScriptScopeGraph) {
    return 0;
  }
  (*player_script)->close();
  return 0;
}

duk_ret_t ScriptImplementation::scriptScriptDestructor(duk_context *context) {
  std::shared_ptr<Script> *script =
      ScriptImplementationScriptFromContext(context);
  delete script;
  return 0;
}

duk_ret_t ScriptImplementation::scriptGetGraphNodeJSON(duk_context *context) {
  std::shared_ptr<Node> *node = ScriptImplementationNodeFromContext(context);
  duk_push_string(context, (*node)->toJSON().c_str());
  return 1;
}

duk_ret_t ScriptImplementation::scriptSetTimeout(duk_context *context) {
  ScriptImplementation *script_implementation =
      ScriptImplementationFromContext(context);
  std::lock_guard<std::mutex> timer_map_lock(
      script_implementation->_timer_mutex);
  void *callback = duk_get_heapptr(context, 0);
  int ms = duk_get_int(context, 1);
  boost::posix_time::milliseconds interval(ms);
  std::shared_ptr<boost::asio::deadline_timer> timer =
      std::make_shared<boost::asio::deadline_timer>(
          script_implementation->_io_service);
  int timeout_id = script_implementation->findTimerID();
  script_implementation->_timer_map[timeout_id] = timer;
  timer->expires_from_now(interval);
  timer->async_wait([timer, callback, script_implementation,
                     timeout_id](const boost::system::error_code &error) {
    std::lock_guard<std::mutex> timer_map_lock(
        script_implementation->_timer_mutex);
    std::lock_guard<std::mutex> run_lock(script_implementation->_run_mutex);
    duk_push_heapptr(script_implementation->_js_context, callback);
    duk_call(script_implementation->_js_context, 0);
    timer->cancel();
    script_implementation->_timer_map.erase(timeout_id);
  });
  duk_push_int(context, timeout_id);
  return 1;
}

duk_ret_t ScriptImplementation::scriptSetInterval(duk_context *context) {
  ScriptImplementation *script_implementation =
      ScriptImplementationFromContext(context);
  std::lock_guard<std::mutex> timer_map_lock(
      script_implementation->_timer_mutex);
  void *callback = duk_get_heapptr(context, 0);
  int ms = duk_get_int(context, 1);
  boost::posix_time::milliseconds interval(ms);
  std::shared_ptr<boost::asio::deadline_timer> timer =
      std::make_shared<boost::asio::deadline_timer>(
          script_implementation->_io_service);
  int timeout_id = script_implementation->findTimerID();
  script_implementation->_timer_map[timeout_id] = timer;
  script_implementation->intervalLoop(boost::system::error_code(), interval,
                                      callback, timer, false, timeout_id);
  duk_push_int(context, timeout_id);
  return 1;
}

duk_ret_t ScriptImplementation::scriptClearTimeout(duk_context *context) {
  ScriptImplementation *script_implementation =
      ScriptImplementationFromContext(context);
  std::lock_guard<std::mutex> timer_map_lock(
      script_implementation->_timer_mutex);
  int timeout_id = duk_get_int(context, 0);
  auto iterator = script_implementation->_timer_map.find(timeout_id);
  if (iterator != script_implementation->_timer_map.end()) {
    (*iterator).second->cancel();
    script_implementation->_timer_map.erase(iterator);
  }
  return 0;
}

duk_ret_t ScriptImplementation::scriptClearInterval(duk_context *context) {
  return scriptClearTimeout(context);
}

void ScriptImplementation::createJSPlayerObject(duk_context *context,
                                                duk_idx_t object,
                                                ScriptImplementation *script) {
  const duk_function_list_entry player_funcs[] = {
      {"registerMessage", &ScriptImplementation::scriptRegisterMessage, 3},
      {"sendMessage", &ScriptImplementation::scriptSendMessage, 3},
      {"unregisterMessage", &ScriptImplementation::scriptUnregisterMessage, 1},
      {"setJSON", &ScriptImplementation::scriptSetJSON, 1},
      {NULL, NULL, 0}};

  // Add the graphs property
  duk_push_string(context, "graphs");
  duk_push_c_function(context, &ScriptImplementation::scriptGetGraphs, 0);
  duk_def_prop(context, object, DUK_DEFPROP_HAVE_GETTER);

  // Add the playing property
  duk_push_string(context, "playing");
  duk_push_c_function(context, &ScriptImplementation::scriptGetPlaying, 0);
  duk_uint_t flags = DUK_DEFPROP_HAVE_GETTER;
  if (script->scope() == ScriptScopeSession) {
    duk_push_c_function(context, &ScriptImplementation::scriptSetPlaying, 1);
    flags |= DUK_DEFPROP_HAVE_SETTER;
  }
  duk_def_prop(context, object, flags);

  // Add the renderTime property
  duk_push_string(context, "renderTime");
  duk_push_c_function(context, &ScriptImplementation::scriptGetRenderTime, 0);
  flags = DUK_DEFPROP_HAVE_GETTER;
  if (script->scope() == ScriptScopeSession) {
    duk_push_c_function(context, &ScriptImplementation::scriptSetRenderTime, 1);
    flags |= DUK_DEFPROP_HAVE_SETTER;
  }
  duk_def_prop(context, object, flags);

  duk_put_function_list(context, object, player_funcs);
}

void ScriptImplementation::createJSNodeObject(duk_context *context,
                                              duk_idx_t object,
                                              std::shared_ptr<Node> node) {
  const duk_function_list_entry node_functions[] = {
      {"toJSON", &ScriptImplementation::scriptGetGraphNodeJSON, 0},
      {NULL, NULL, 0}};

  // Attach the C++ node to our JS node
  duk_push_pointer(context, (void *)new std::shared_ptr<Node>(node));
  duk_put_prop_string(context, object,
                      ScriptImplementationNodeProperty.c_str());

  // Add the "identifier" property
  duk_push_string(context, "identifier");
  duk_push_c_function(context,
                      &ScriptImplementation::scriptGetGraphNodeIdentifier, 0);
  duk_uint_t flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the "type" property
  duk_push_string(context, "type");
  duk_push_c_function(context, &ScriptImplementation::scriptGetGraphNodeType,
                      0);
  flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the "isOutputNode" property
  duk_push_string(context, "isOutputNode");
  duk_push_c_function(context,
                      &ScriptImplementation::scriptGetGraphNodeIsOutputNode, 0);
  flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the methods
  duk_put_function_list(context, object, node_functions);

  // Add the destructor
  duk_push_c_function(context, &ScriptImplementation::scriptGraphNodeDestructor,
                      1);
  duk_set_finalizer(context, object);
}

void ScriptImplementation::createJSGraphObject(duk_context *context,
                                               duk_idx_t object,
                                               std::shared_ptr<Graph> graph,
                                               ScriptImplementation *script) {
  const duk_function_list_entry graph_functions[] = {
      {"valueAtPath", &ScriptImplementation::scriptGetGraphVariable, 1},
      {"setValueAtPath", &ScriptImplementation::scriptSetGraphVariable,
       DUK_VARARGS},
      {"setJSON", &ScriptImplementation::scriptSetGraphJson, 1},
      {NULL, NULL, 0}};

  // Attach the C++ graph to our JS graph
  duk_push_pointer(context, (void *)new std::shared_ptr<Graph>(graph));
  duk_put_prop_string(context, object,
                      ScriptImplementationGraphProperty.c_str());

  // Add the "renderTime" property
  duk_push_string(context, "renderTime");
  duk_push_c_function(context, &ScriptImplementation::scriptGetGraphRenderTime,
                      0);
  duk_uint_t flags = DUK_DEFPROP_HAVE_GETTER;
  if (script->scope() == ScriptScopeSession ||
      (script->scope() == ScriptScopeGraph &&
       script->_graph_identifier == graph->identifier())) {
    duk_push_c_function(context,
                        &ScriptImplementation::scriptSetGraphRenderTime, 1);
    flags |= DUK_DEFPROP_HAVE_SETTER;
  }
  duk_def_prop(context, object, flags);

  // Add the "nodes" property
  duk_push_string(context, "nodes");
  duk_push_c_function(context, &ScriptImplementation::scriptGetGraphNodes, 0);
  flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the "type" property
  duk_push_string(context, "type");
  duk_push_c_function(context, &ScriptImplementation::scriptGetGraphType, 0);
  flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the "identifier" property
  duk_push_string(context, "identifier");
  duk_push_c_function(context, &ScriptImplementation::scriptGetGraphIdentifier,
                      0);
  flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the methods to the class
  duk_put_function_list(context, object, graph_functions);

  // Add the destructor
  duk_push_c_function(context, &ScriptImplementation::scriptGraphDestructor, 1);
  duk_set_finalizer(context, object);
}

void ScriptImplementation::createJSEdgeObject(duk_context *context,
                                              duk_idx_t object,
                                              std::shared_ptr<Edge> edge) {
  const duk_function_list_entry edge_functions[] = {
      {"toJSON", &ScriptImplementation::scriptGetGraphEdgeJSON, 0},
      {NULL, NULL, 0}};

  // Attach the C++ edge to our JS edge
  duk_push_pointer(context, (void *)new std::shared_ptr<Edge>(edge));
  duk_put_prop_string(context, object,
                      ScriptImplementationEdgeProperty.c_str());

  // Add the "source" property
  duk_push_string(context, "source");
  duk_push_c_function(context, &ScriptImplementation::scriptGetGraphEdgeSource,
                      0);
  duk_uint_t flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the "target" property
  duk_push_string(context, "target");
  duk_push_c_function(context, &ScriptImplementation::scriptGetGraphEdgeTarget,
                      0);
  flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the methods to the class
  duk_put_function_list(context, object, edge_functions);

  // Add the destructor
  duk_push_c_function(context, &ScriptImplementation::scriptGraphEdgeDestructor,
                      1);
  duk_set_finalizer(context, object);
}

void ScriptImplementation::createJSScriptObject(
    duk_context *context, duk_idx_t object, std::shared_ptr<Script> script) {
  const duk_function_list_entry script_functions[] = {
      {"close", &ScriptImplementation::scriptGetGraphVariable, 0},
      {NULL, NULL, 0}};

  // Attach the C++ script to our JS script
  duk_push_pointer(context, (void *)new std::shared_ptr<Script>(script));
  duk_put_prop_string(context, object,
                      ScriptImplementationScriptProperty.c_str());

  // Add the "name" property
  duk_push_string(context, "name");
  duk_push_c_function(context, &ScriptImplementation::scriptGetScriptName, 0);
  duk_uint_t flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the "scope" property
  duk_push_string(context, "scope");
  duk_push_c_function(context, &ScriptImplementation::scriptGetScriptScope, 0);
  flags = DUK_DEFPROP_HAVE_GETTER;
  duk_def_prop(context, object, flags);

  // Add the methods
  duk_put_function_list(context, object, script_functions);

  // Add the destructor
  duk_push_c_function(context, &ScriptImplementation::scriptScriptDestructor,
                      1);
  duk_set_finalizer(context, object);
}

void ScriptImplementation::intervalLoop(
    const boost::system::error_code &error,
    boost::posix_time::milliseconds interval, void *callback,
    std::shared_ptr<boost::asio::deadline_timer> timer, bool call_callback,
    int timer_id) {
  if (call_callback) {
    std::lock_guard<std::mutex> timer_map_lock(_timer_mutex);
    // This indicates we have cancelled the timer
    if (_timer_map.find(timer_id) == _timer_map.end()) {
      return;
    }
    std::lock_guard<std::mutex> run_lock(_run_mutex);
    duk_push_heapptr(_js_context, callback);
    duk_call(_js_context, 0);
  }
  timer->expires_from_now(interval);
  timer->async_wait(std::bind(&ScriptImplementation::intervalLoop, this,
                              std::placeholders::_1, interval, callback, timer,
                              true, timer_id));
}

int ScriptImplementation::findTimerID() {
  for (int i = 0; i < INT_MAX; ++i) {
    if (_timer_map.find(i) == _timer_map.end()) {
      return i;
    }
  }
  return -1;
}

static ScriptImplementation *ScriptImplementationFromContext(
    duk_context *context) {
  duk_push_global_stash(context);
  duk_get_prop_string(context, -1, ScriptImplementationContextProperty.c_str());
  ScriptImplementation *script =
      (ScriptImplementation *)duk_to_pointer(context, -1);
  duk_pop(context);
  return script;
}

static std::shared_ptr<Graph> *ScriptImplementationGraphFromContext(
    duk_context *context) {
  duk_push_this(context);
  duk_get_prop_string(context, -1, ScriptImplementationGraphProperty.c_str());
  std::shared_ptr<Graph> *graph =
      (std::shared_ptr<Graph> *)duk_to_pointer(context, -1);
  duk_pop(context);
  return graph;
}

static std::shared_ptr<Node> *ScriptImplementationNodeFromContext(
    duk_context *context) {
  duk_push_this(context);
  duk_get_prop_string(context, -1, ScriptImplementationNodeProperty.c_str());
  std::shared_ptr<Node> *node =
      (std::shared_ptr<Node> *)duk_to_pointer(context, -1);
  duk_pop(context);
  return node;
}

static std::shared_ptr<Edge> *ScriptImplementationEdgeFromContext(
    duk_context *context) {
  duk_push_this(context);
  duk_get_prop_string(context, -1, ScriptImplementationEdgeProperty.c_str());
  std::shared_ptr<Edge> *edge =
      (std::shared_ptr<Edge> *)duk_to_pointer(context, -1);
  duk_pop(context);
  return edge;
}

static std::shared_ptr<Script> *ScriptImplementationScriptFromContext(
    duk_context *context) {
  duk_push_this(context);
  duk_get_prop_string(context, -1, ScriptImplementationScriptProperty.c_str());
  std::shared_ptr<Script> *edge =
      (std::shared_ptr<Script> *)duk_to_pointer(context, -1);
  duk_pop(context);
  return edge;
}

}  // namespace smartplayer
}  // namespace nativeformat
