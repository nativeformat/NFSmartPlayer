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

#include <NFHTTP/Client.h>
#include <NFSmartPlayer/Client.h>

#include <boost/asio/deadline_timer.hpp>
#include <boost/asio/io_service.hpp>
#include <boost/asio/system_timer.hpp>
#include <functional>
#include <future>
#include <thread>
#include <vector>

#include "Authoriser.h"

namespace nativeformat {
namespace smartplayer {

class Player;
class NFOSCHandler;

class ClientImplementation : public Client {
 public:
  ClientImplementation(NF_SMART_PLAYER_RESOLVE_CALLBACK resolve_callback,
                       NF_SMART_PLAYER_ERROR_CALLBACK error_callback,
                       DriverType driver_type, std::string output_destination,
                       int osc_read_port, int osc_write_port,
                       const std::string osc_address, int localhost_write_port);
  virtual ~ClientImplementation();

  // Client
  void setJson(const std::string &score_json,
               LOAD_CALLBACK load_callback) override;
  bool isPlaying() const override;
  void setPlaying(bool playing) override;
  double getRenderTime() const override;
  void setRenderTime(double time) override;
  void setMessageCallback(NF_SMART_PLAYER_MESSAGE_CALLBACK callback) override;
  void sendMessage(const std::string &message_identifier,
                   NFSmartPlayerMessageType message_type,
                   const void *payload) override;
  int numberOfScripts() override;
  std::shared_ptr<Script> openScript(const std::string &name,
                                     const std::string &script) override;
  std::shared_ptr<Graph> graphForIdentifier(
      const std::string &identifier) override;
  void forEachGraph(NF_SMART_PLAYER_GRAPH_CALLBACK graph_callback) override;
  float valueForPath(const std::string &path) override;
  void setValueForPath(const std::string path, ...) override;
  void vSetValueForPath(const std::string path, va_list args) override;
  void setValuesForPath(const std::string path,
                        const std::vector<float> &values) override;
  void forEachScript(NF_SMART_PLAYER_SCRIPT_CALLBACK script_callback) override;
  std::shared_ptr<Script> scriptForIdentifier(
      const std::string &identifier) override;
  void addGraph(std::shared_ptr<Graph> graph) override;
  void removeGraph(const std::string &identifier) override;
  void setLoadingPolicy(nfgrapher::LoadingPolicy loading_policy) override;
  nfgrapher::LoadingPolicy loadingPolicy() const override;
  bool isLoaded() override;

 private:
  static void thread(boost::asio::io_service *io_service,
                     std::shared_ptr<smartplayer::Player> smart_player);
  static void thread_pump(const boost::system::error_code &ec,
                          std::shared_ptr<smartplayer::Player> smart_player,
                          boost::asio::deadline_timer *timer);

  void onClientRequest(
      std::function<void(const std::shared_ptr<http::Request> &request)>
          callback,
      const std::shared_ptr<http::Request> &request);
  void onClientResponse(
      std::function<void(const std::shared_ptr<http::Response> &response,
                         bool retry)>
          callback,
      const std::shared_ptr<http::Response> &response);

  boost::asio::io_service _io_service;
  std::vector<std::shared_ptr<Authoriser>> _authorisers;
  std::shared_ptr<http::Client> _client;
  std::shared_ptr<Player> _smart_player;
  std::thread _thread;
  std::shared_ptr<NFOSCHandler> _osc_handler;
};

}  // namespace smartplayer
}  // namespace nativeformat
