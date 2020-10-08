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
#include "ClientImplementation.h"

#include <NFSmartPlayer/Registry.h>

#include "NFOSCHandler.h"
#include "Player.h"
#include "plugins/channel/PluginFactory.h"
#include "plugins/compressor/CompressorPluginFactory.h"
#include "plugins/eq/EQPluginFactory.h"
#include "plugins/file/FilePluginFactory.h"
#include "plugins/noise/NoisePluginFactory.h"
#include "plugins/time/TimePluginFactory.h"
#include "plugins/waa/WAAPluginFactory.h"
#include "plugins/wave/WavePluginFactory.h"

namespace nativeformat {
namespace smartplayer {

static const boost::posix_time::milliseconds NF_SMART_PLAYER_PUMP_INTERVAL_MS(
    100);

const char *NFSmartPlayerOscAddress = "127.0.0.1";
const int NFSmartPlayerOscReadPort = 7003;
const int NFSmartPlayerOscWritePort = 7000;
const int NFSmartPlayerLocalhostPort = 64855;

ClientImplementation::ClientImplementation(
    NF_SMART_PLAYER_RESOLVE_CALLBACK resolve_callback,
    NF_SMART_PLAYER_ERROR_CALLBACK error_callback, DriverType driver_type,
    std::string output_destination, int osc_read_port, int osc_write_port,
    const std::string osc_address, int localhost_write_port)
    : _authorisers(),
      _client(http::createClient(
          http::standardCacheLocation(), "Native Format Player 0.1",
          std::bind(&ClientImplementation::onClientRequest, this,
                    std::placeholders::_1, std::placeholders::_2),
          std::bind(&ClientImplementation::onClientResponse, this,
                    std::placeholders::_1, std::placeholders::_2))),
      _smart_player(std::make_shared<Player>(
          std::make_shared<nativeformat::plugin::Registry>(
              std::vector<std::shared_ptr<nativeformat::plugin::Factory>>(
                  {std::make_shared<plugin::waa::WAAPluginFactory>(),
                   std::make_shared<plugin::noise::NoisePluginFactory>(),
                   std::make_shared<plugin::wave::WavePluginFactory>(),
                   std::make_shared<plugin::file::FilePluginFactory>(_client),
                   std::make_shared<
                       plugin::compressor::CompressorPluginFactory>(),
                   std::make_shared<plugin::channel::PluginFactory>(),
                   std::make_shared<plugin::eq::EQPluginFactory>(),
                   std::make_shared<plugin::time::TimePluginFactory>()}),
              resolve_callback),
          error_callback, localhost_write_port, driver_type, output_destination,
          _io_service)),
      _thread(&ClientImplementation::thread, &_io_service, _smart_player) {
  // TODO: Add this back someday
  /*
  try {
    _osc_handler = std::make_shared<NFOSCHandler>(_smart_player, osc_read_port,
                                                  osc_write_port, osc_address);
    _smart_player->setOSCHandler(_osc_handler);
  } catch (const std::exception &exception) {
    // Just so we don't crash
  }
  */
}

ClientImplementation::~ClientImplementation() {
  _io_service.stop();
  _thread.join();
}

void ClientImplementation::setJson(const std::string &score_json,
                                   LOAD_CALLBACK load_callback) {
  _smart_player->setJson(score_json)
      .then([load_callback](boost::future<Load> load) {
        load_callback(load.get());
      });
}

bool ClientImplementation::isPlaying() const {
  return _smart_player->isPlaying();
}

void ClientImplementation::setPlaying(bool playing) {
  _smart_player->setPlaying(playing);
}

double ClientImplementation::getRenderTime() const {
  return _smart_player->getRenderTime();
}

void ClientImplementation::setRenderTime(double time) {
  _smart_player->setRenderTime(time);
}

void ClientImplementation::setMessageCallback(
    NF_SMART_PLAYER_MESSAGE_CALLBACK callback) {
  _smart_player->setMessageCallback(callback);
}

void ClientImplementation::sendMessage(const std::string &message_identifier,
                                       NFSmartPlayerMessageType message_type,
                                       const void *payload) {
  _smart_player->sendMessage(message_identifier, PlayerSenderIdentifier,
                             message_type, payload);
}

int ClientImplementation::numberOfScripts() {
  return _smart_player->numberOfScripts();
}

std::shared_ptr<Script> ClientImplementation::openScript(
    const std::string &name, const std::string &script) {
  return _smart_player->openScript(name, script);
}

std::shared_ptr<Graph> ClientImplementation::graphForIdentifier(
    const std::string &identifier) {
  return _smart_player->graphForIdentifier(identifier);
}

void ClientImplementation::forEachGraph(
    NF_SMART_PLAYER_GRAPH_CALLBACK graph_callback) {
  _smart_player->forEachGraph(graph_callback);
}

float ClientImplementation::valueForPath(const std::string &path) {
  return _smart_player->valueForPath(path);
}

void ClientImplementation::setValueForPath(const std::string path, ...) {
  va_list args;
  va_start(args, path);
  vSetValueForPath(path, args);
  va_end(args);
}

void ClientImplementation::vSetValueForPath(const std::string path,
                                            va_list args) {
  _smart_player->vSetValueForPath(path, args);
}

void ClientImplementation::setValuesForPath(const std::string path,
                                            const std::vector<float> &values) {
  _smart_player->setValuesForPath(path, values);
}

void ClientImplementation::forEachScript(
    NF_SMART_PLAYER_SCRIPT_CALLBACK script_callback) {
  _smart_player->forEachScript(script_callback);
}

std::shared_ptr<Script> ClientImplementation::scriptForIdentifier(
    const std::string &identifier) {
  return _smart_player->scriptForIdentifier(identifier);
}

void ClientImplementation::addGraph(std::shared_ptr<Graph> graph) {
  _smart_player->addGraph(graph);
}

void ClientImplementation::removeGraph(const std::string &identifier) {
  _smart_player->removeGraph(identifier);
}

void ClientImplementation::setLoadingPolicy(
    nfgrapher::LoadingPolicy loading_policy) {
  _smart_player->setLoadingPolicy(loading_policy);
}

nfgrapher::LoadingPolicy ClientImplementation::loadingPolicy() const {
  return _smart_player->loadingPolicy();
}

bool ClientImplementation::isLoaded() { return _smart_player->isLoaded(); }

void ClientImplementation::thread(
    boost::asio::io_service *io_service,
    std::shared_ptr<smartplayer::Player> smart_player) {
  boost::asio::io_service::work work(*io_service);
  boost::asio::deadline_timer timer(*io_service,
                                    NF_SMART_PLAYER_PUMP_INTERVAL_MS);
  timer.async_wait(std::bind(&ClientImplementation::thread_pump,
                             std::placeholders::_1, smart_player, &timer));
  io_service->run();
}

void ClientImplementation::thread_pump(
    const boost::system::error_code &ec,
    std::shared_ptr<smartplayer::Player> smart_player,
    boost::asio::deadline_timer *timer) {
  smart_player->run();
  timer->expires_at(timer->expires_at() + NF_SMART_PLAYER_PUMP_INTERVAL_MS);
  timer->async_wait(std::bind(&ClientImplementation::thread_pump,
                              std::placeholders::_1, smart_player, timer));
}

void ClientImplementation::onClientRequest(
    std::function<void(const std::shared_ptr<http::Request> &request)> callback,
    const std::shared_ptr<http::Request> &request) {
  for (auto authoriser : _authorisers) {
    if (authoriser->requiresAuthorisation(request)) {
      authoriser->authoriseRequest(callback, request);
      return;
    }
  }
  callback(request);
}

void ClientImplementation::onClientResponse(
    std::function<void(const std::shared_ptr<http::Response> &response,
                       bool retry)>
        callback,
    const std::shared_ptr<http::Response> &response) {
  if (response->statusCode() == http::StatusCodeUnauthorised) {
    for (auto authoriser : _authorisers) {
      if (authoriser->isAuthorised(response->request())) {
        bool should_retry =
            authoriser->shouldRetryFailedAuthorisation(response->request());
        callback(response, should_retry);
        return;
      }
    }
  }
  callback(response, false);
}

}  // namespace smartplayer
}  // namespace nativeformat
