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

#include <NFDriver/NFDriver.h>
#include <NFSmartPlayer/Client.h>
#include <NFSmartPlayer/Registry.h>

#include <atomic>
#include <boost/asio.hpp>
#include <boost/graph/directed_graph.hpp>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#include <boost/thread.hpp>

#include "GraphDelegate.h"
#include "ScriptDelegate.h"
#include "ScriptImplementation.h"

class Limiter;

namespace nativeformat {
namespace smartplayer {

class NFOSCHandler;
class Graph;

typedef std::function<bool(const std::shared_ptr<Graph> &)>
    NF_SMART_PLAYER_GRAPH_CALLBACK;

extern const int NFSmartPlayerVariableComponentCount;
extern const int NFSmartPlayerGraphComponentIndex;
extern const int NFSmartPlayerNodeComponentIndex;
extern const int NFSmartPlayerParamComponentIndex;
extern const int NFSmartPlayerCommandComponentIndex;

class Player : public std::enable_shared_from_this<Player>,
               public ScriptDelegate,
               public GraphDelegate {
 public:
  Player(std::shared_ptr<plugin::Registry> plugin_registry,
         NF_SMART_PLAYER_ERROR_CALLBACK error_callback,
         int localhost_write_port, DriverType driver_type,
         std::string output_destination, boost::asio::io_service &io_service);
  virtual ~Player();

  // ScriptDelegate
  void scriptDidClose(std::shared_ptr<Script> script) override;
  bool playing() const override;
  double renderTime() const override;
  boost::future<Load> setJson(const std::string &json) override;
  void forEachScript(NF_SMART_PLAYER_SCRIPT_CALLBACK script_callback) override;
  void forEachGraph(NF_SMART_PLAYER_GRAPH_CALLBACK graph_callback) override;

  // GraphDelegate
  std::shared_ptr<Script> createScript(const std::string &script_name,
                                       const std::string &script) override;
  void graphSendMessage(std::shared_ptr<Graph> graph,
                        const std::string &message_identifier,
                        NFSmartPlayerMessageType message_type,
                        const void *payload) override;

  std::shared_ptr<plugin::Registry> pluginRegistry() const;
  bool isPlaying() const;
  void setPlaying(bool playing) override;
  bool canPlayThrough() const;
  void setPlayThrough(bool play_through);
  double getRenderTime() const;
  void setRenderTime(double time) override;
  void run();
  void setOSCHandler(std::shared_ptr<NFOSCHandler> osc_handler);
  void setMessageCallback(NF_SMART_PLAYER_MESSAGE_CALLBACK callback);
  void sendMessage(const std::string &message_identifier,
                   const std::string &sender_identifier,
                   NFSmartPlayerMessageType message_type,
                   const void *payload) override;
  int numberOfScripts();
  std::shared_ptr<Script> scriptForIndex(int index);
  std::shared_ptr<Script> openScript(const std::string &name,
                                     const std::string &script);
  std::shared_ptr<Graph> graphForIdentifier(const std::string &identifier);
  size_t numberOfGraphs() const;
  float valueForPath(const std::string &path);
  void setValueForPath(const std::string path, ...);
  void vSetValueForPath(const std::string path, va_list args);
  void setValuesForPath(const std::string path,
                        const std::vector<float> &values);
  std::shared_ptr<Script> scriptForIdentifier(const std::string &identifier);
  void addGraph(std::shared_ptr<Graph> graph);
  void removeGraph(const std::string &identifier);
  void setLoadingPolicy(nfgrapher::LoadingPolicy loading_policy);
  nfgrapher::LoadingPolicy loadingPolicy() const;
  bool finished();
  bool isLoaded();

 private:
  static void driverDidStutter(void *clientdata);
  static int driverRender(void *clientdata, float *samples, int sample_count);
  static void driverDidError(void *clientdata, const char *domain, int error);
  static void driverWillRender(void *clientdata);
  static void driverDidRender(void *clientdata);
  void receivedNotification(const Notification &notification);
  void recalculateRenderVariables();

  const int _channels;
  const double _samplerate;
  std::shared_ptr<plugin::Registry> _plugin_registry;
  NF_SMART_PLAYER_ERROR_CALLBACK _error_callback;
  boost::asio::io_service &_io_service;

  std::shared_ptr<driver::NFDriver> _driver;

  std::atomic<bool> _playing;
  std::atomic<bool> _play_through;
  std::atomic<double> _render_time;
  std::shared_ptr<Limiter> _limiter;
  std::shared_ptr<NFOSCHandler> _osc_handler;
  std::map<std::string, std::shared_ptr<Graph>> _graphs;
  std::mutex _graphs_mutex;
  std::mutex _render_mutex;
  NF_SMART_PLAYER_MESSAGE_CALLBACK _message_callback;
  std::map<std::string, std::shared_ptr<ScriptImplementation>> _scripts;
  std::atomic<nfgrapher::LoadingPolicy> _loading_policy;
  bool _finished;

  // Render variables
  std::map<std::string, std::map<std::string, std::shared_ptr<plugin::Content>>>
      _graph_content_maps;
  std::shared_ptr<plugin::Content> _final_content;
};

}  // namespace smartplayer
}  // namespace nativeformat
