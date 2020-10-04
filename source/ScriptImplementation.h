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

#include <NFSmartPlayer/Script.h>
#include <duktape.h>

#include <boost/asio.hpp>
#include <map>
#include <memory>
#include <mutex>
#include <regex>
#include <string>
#include <unordered_map>

#include "Notification.h"
#include "ScriptDelegate.h"

namespace nativeformat {
namespace smartplayer {

typedef struct ScriptNotificationKey {
  std::regex _message_regex;
  std::regex _sender_regex;
  size_t _handle;
  ScriptNotificationKey(std::string message_regex, std::string sender_regex,
                        size_t handle)
      : _message_regex(message_regex),
        _sender_regex(sender_regex),
        _handle(handle) {}
  inline bool operator<(const ScriptNotificationKey &rhs) const {
    return _handle < rhs._handle;
  }
} ScriptNotificationKey;

class ScriptImplementation
    : public Script,
      public std::enable_shared_from_this<ScriptImplementation> {
 public:
  ScriptImplementation(const std::string &name, const std::string &script,
                       ScriptScope scope,
                       std::weak_ptr<ScriptDelegate> delegate,
                       boost::asio::io_service &io_service,
                       const std::string &graph_identifier = "");
  virtual ~ScriptImplementation();

  // Script
  std::string name() const override;
  ScriptScope scope() const override;
  void run() override;
  void close() override;
  std::string code() const override;
  bool isRunning() const override;

  void processNotification(const Notification &notification);

 private:
  static duk_ret_t scriptGetPlayer(duk_context *context);
  static duk_ret_t scriptGetGraphs(duk_context *context);
  static duk_ret_t scriptSetGraphJson(duk_context *context);
  static duk_ret_t scriptGetGraphRenderTime(duk_context *context);
  static duk_ret_t scriptSetGraphRenderTime(duk_context *context);
  static duk_ret_t scriptGetGraphVariable(duk_context *context);
  static duk_ret_t scriptSetGraphVariable(duk_context *context);
  static duk_ret_t scriptGetGraphNodes(duk_context *context);
  static duk_ret_t scriptGetGraphNodeIdentifier(duk_context *context);
  static duk_ret_t scriptGetGraphNodeType(duk_context *context);
  static duk_ret_t scriptGetGraphNodeIsOutputNode(duk_context *context);
  static duk_ret_t scriptGraphNodeConstructor(duk_context *context);
  static duk_ret_t scriptGraphNodeDestructor(duk_context *context);
  static duk_ret_t scriptGraphConstructor(duk_context *context);
  static duk_ret_t scriptGraphDestructor(duk_context *context);
  static duk_ret_t scriptGraphEdgeConstructor(duk_context *context);
  static duk_ret_t scriptGraphEdgeDestructor(duk_context *context);
  static duk_ret_t scriptGetGraphEdgeSource(duk_context *context);
  static duk_ret_t scriptGetGraphEdgeTarget(duk_context *context);
  static duk_ret_t scriptGetGraphEdgeJSON(duk_context *context);
  static duk_ret_t scriptGetGraphType(duk_context *context);
  static duk_ret_t scriptGetGraphIdentifier(duk_context *context);
  static duk_ret_t scriptGetPlaying(duk_context *context);
  static duk_ret_t scriptSetPlaying(duk_context *context);
  static duk_ret_t scriptGetRenderTime(duk_context *context);
  static duk_ret_t scriptSetRenderTime(duk_context *context);
  static duk_ret_t scriptRegisterMessage(duk_context *context);
  static duk_ret_t scriptSendMessage(duk_context *context);
  static duk_ret_t scriptUnregisterMessage(duk_context *context);
  static duk_ret_t scriptSetJSON(duk_context *context);
  static duk_ret_t scriptGetScripts(duk_context *context);
  static duk_ret_t scriptGetScriptName(duk_context *context);
  static duk_ret_t scriptGetScriptScope(duk_context *context);
  static duk_ret_t scriptCloseScript(duk_context *context);
  static duk_ret_t scriptScriptDestructor(duk_context *context);
  static duk_ret_t scriptGetGraphNodeJSON(duk_context *context);

  // Event Loop
  static duk_ret_t scriptSetTimeout(duk_context *context);
  static duk_ret_t scriptSetInterval(duk_context *context);
  static duk_ret_t scriptClearTimeout(duk_context *context);
  static duk_ret_t scriptClearInterval(duk_context *context);

  static void createJSPlayerObject(duk_context *context, duk_idx_t object,
                                   ScriptImplementation *script);
  static void createJSNodeObject(duk_context *context, duk_idx_t object,
                                 std::shared_ptr<Node> node);
  static void createJSGraphObject(duk_context *context, duk_idx_t object,
                                  std::shared_ptr<Graph> graph,
                                  ScriptImplementation *script);
  static void createJSEdgeObject(duk_context *context, duk_idx_t object,
                                 std::shared_ptr<Edge> edge);
  static void createJSScriptObject(duk_context *context, duk_idx_t object,
                                   std::shared_ptr<Script> script);

  void intervalLoop(const boost::system::error_code &error,
                    boost::posix_time::milliseconds interval, void *callback,
                    std::shared_ptr<boost::asio::deadline_timer> timer,
                    bool call_callback, int timer_id);
  int findTimerID();

  const std::string _name;
  const std::string _script;
  const ScriptScope _scope;
  const std::weak_ptr<ScriptDelegate> _delegate;
  const std::string _graph_identifier;
  boost::asio::io_service &_io_service;

  duk_context *_js_context;
  std::map<ScriptNotificationKey, void *> _notifications;
  size_t _handle_count;
  std::map<int, std::shared_ptr<boost::asio::deadline_timer>> _timer_map;
  std::mutex _timer_mutex;
  std::mutex _run_mutex;
  std::atomic<bool> _running;
};

}  // namespace smartplayer
}  // namespace nativeformat
