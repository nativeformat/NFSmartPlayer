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
#include "Player.h"

#include <boost/algorithm/string.hpp>
#include <ctime>
#include <functional>
#include <map>
#include <sstream>
#include <string>
#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#include <boost/thread.hpp>

#include "GraphImplementation.h"
#include "Limiter.h"
#include "NFOSCHandler.h"
#include "Notification.h"
#include "ScriptImplementation.h"

namespace nativeformat {
namespace smartplayer {

const int NFSmartPlayerVariableComponentCount = 4;
const int NFSmartPlayerGraphComponentIndex = 0;
const int NFSmartPlayerNodeComponentIndex = 1;
const int NFSmartPlayerParamComponentIndex = 2;
const int NFSmartPlayerCommandComponentIndex = 3;

Player::Player(std::shared_ptr<plugin::Registry> plugin_registry,
               NF_SMART_PLAYER_ERROR_CALLBACK error_callback,
               int localhost_write_port, DriverType driver_type,
               std::string output_destination,
               boost::asio::io_service &io_service)
    : _channels(2),
      _samplerate(44100.0),
      _plugin_registry(plugin_registry),
      _error_callback(error_callback),
      _io_service(io_service),
      _driver(driver::NFDriver::createNFDriver(
          this, &Player::driverDidStutter, &Player::driverRender,
          &Player::driverDidError, &Player::driverWillRender,
          &Player::driverDidRender, (driver::OutputType)driver_type,
          output_destination.c_str())),
      _playing(false),
      _play_through(false),
      _render_time(0.0),
      _limiter(std::make_shared<Limiter>(_channels, kDelay)),
      _loading_policy(nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH),
      _finished(false),
      _final_content(std::make_shared<plugin::Content>(
          NF_DRIVER_SAMPLE_BLOCK_SIZE, 0, _samplerate, _channels,
          NF_DRIVER_SAMPLE_BLOCK_SIZE, plugin::ContentPayloadTypeBuffer)) {}

Player::~Player() { _driver->setPlaying(false); }

std::shared_ptr<plugin::Registry> Player::pluginRegistry() const {
  return _plugin_registry;
}

void Player::scriptDidClose(std::shared_ptr<Script> script) {
  _scripts.erase(script->name());
}

bool Player::playing() const { return isPlaying(); }

double Player::renderTime() const { return getRenderTime(); }

boost::future<Load> Player::setJson(const std::string &json) {
  std::shared_ptr<boost::promise<Load>> load_promise =
      std::make_shared<boost::promise<Load>>();
  boost::future<Load> load_future = load_promise->get_future();

  // Decode JSON
  nlohmann::json j;
  nfgrapher::Score score;
  try {
    j = nlohmann::json::parse(json);
  } catch (const nlohmann::json::exception &exception) {
    load_promise->set_value(
        Load{false, PlayerErrorDomain, "Could not parse JSON"});
    return load_future;
  }

  try {
    score = j;
  } catch (const nlohmann::json::exception &exception) {
    std::string msg = "JSON is not a valid NFGrapher Score: ";
    msg += exception.what();
    load_promise->set_value(Load{false, PlayerErrorDomain, msg});
    return load_future;
  }

  // Build graph
  // TODO replace _graphs with single _graph
  std::shared_ptr<Graph> player_graph = nullptr;
  std::vector<nlohmann::json> graphs;

  std::shared_ptr<boost::promise<Load>> promise =
      std::make_shared<boost::promise<Load>>();
  boost::future<Load> graph_future = promise->get_future();
  nfgrapher::Graph &graph = score.graph;
  std::string graph_id = graph.id;

  std::lock_guard<std::mutex> graphs_lock(_graphs_mutex);
  if (_graphs.size() && _graphs.count(graph.id)) {
    player_graph = _graphs[graph.id];
  } else {
    std::shared_ptr<GraphImplementation> player_graph_implementation =
        std::make_shared<GraphImplementation>(_channels, _samplerate,
                                              createGraphSessionId());
    player_graph_implementation->setPluginRegistry(_plugin_registry);
    player_graph_implementation->setDelegate(shared_from_this());
    player_graph = player_graph_implementation;
  }

  nlohmann::json graph_json = graph;
  player_graph->setJson(
      j, [promise](const Load &load) { promise->set_value(load); });

  graph_future.then([this, load_promise, graph_id,
                     player_graph](boost::future<Load> future) {
    Load load = future.get();
    if (load._loaded == false) {
      load_promise->set_value(load);
      return;
    }
    {
      // Assign the finished graph
      std::lock_guard<std::mutex> lock(_graphs_mutex);
      _graphs[graph_id] = player_graph;
      recalculateRenderVariables();
      _render_time = 0.0;
    }

    // Load our scripts
    std::vector<std::shared_ptr<Script>> scripts;
    forEachGraph([&scripts](const std::shared_ptr<Graph> &graph) {
      graph->forEachScript([&scripts](const std::shared_ptr<Script> &script) {
        scripts.push_back(script);
        return true;
      });
      return true;
    });
    for (const auto &script : scripts) {
      if (!script->isRunning()) {
        script->run();
      }
    }
    load_promise->set_value(Load{true});
  });

  return load_future;
}

bool Player::isPlaying() const { return _driver->isPlaying(); }

void Player::setPlaying(bool playing) { _driver->setPlaying(playing); }

bool Player::canPlayThrough() const { return _play_through; }

void Player::setPlayThrough(bool play_through) { _play_through = play_through; }

double Player::getRenderTime() const { return _render_time; }

void Player::setRenderTime(double time) {
  std::lock_guard<std::mutex> lock(_render_mutex);
  double render_time = _render_time;
  double time_diff = time - render_time;
  forEachGraph([time_diff](const std::shared_ptr<Graph> &graph) {
    double render_time = graph->renderTime();
    double new_render_time = render_time + time_diff;
    if (render_time == new_render_time) {
      return true;
    }
    graph->setRenderTime(render_time + time_diff);
    return true;
  });
  _render_time = time;
}

void Player::run() {
  if (_osc_handler) {
    _osc_handler->broadcast();
  }
  forEachGraph([](const std::shared_ptr<Graph> &graph) {
    graph->run();
    return true;
  });
  bool graphs_finished = finished();
  if (graphs_finished != _finished) {
    sendMessage(PlayerMessageIdentifierEnd, PlayerSenderIdentifier,
                NFSmartPlayerMessageTypeNone, nullptr);
    _finished = true;
  }
}

void Player::forEachGraph(NF_SMART_PLAYER_GRAPH_CALLBACK graph_callback) {
  std::lock_guard<std::mutex> lock(_graphs_mutex);
  for (const auto &graph : _graphs) {
    if (!graph_callback(graph.second)) {
      break;
    }
  }
}

std::shared_ptr<Script> Player::createScript(const std::string &script_name,
                                             const std::string &script) {
  return std::make_shared<ScriptImplementation>(
      script_name, script, ScriptScopeGraph, shared_from_this(), _io_service);
}

void Player::graphSendMessage(std::shared_ptr<Graph> graph,
                              const std::string &message_identifier,
                              NFSmartPlayerMessageType message_type,
                              const void *payload) {
  sendMessage(message_identifier,
              "com.nativeformat.graph:" + graph->identifier(), message_type,
              payload);
}

void Player::setOSCHandler(std::shared_ptr<NFOSCHandler> osc_handler) {
  _osc_handler = osc_handler;
}

void Player::setMessageCallback(NF_SMART_PLAYER_MESSAGE_CALLBACK callback) {
  _message_callback = callback;
}

void Player::sendMessage(const std::string &message_identifier,
                         const std::string &sender_identifier,
                         NFSmartPlayerMessageType message_type,
                         const void *payload) {
  Notification notification(message_identifier, sender_identifier, payload,
                            message_type);
  receivedNotification(notification);
}

int Player::numberOfScripts() {
  int scripts = _scripts.size();
  forEachGraph([&scripts](const std::shared_ptr<Graph> &graph) {
    graph->forEachScript([&scripts](const std::shared_ptr<Script> &script) {
      scripts++;
      return true;
    });
    return true;
  });
  return scripts;
}

std::shared_ptr<Script> Player::openScript(const std::string &name,
                                           const std::string &script) {
  std::shared_ptr<ScriptImplementation> player_script =
      std::make_shared<ScriptImplementation>(name, script, ScriptScopeSession,
                                             shared_from_this(), _io_service);
  _scripts[name] = player_script;
  player_script->run();
  return player_script;
}

std::shared_ptr<Graph> Player::graphForIdentifier(
    const std::string &identifier) {
  std::lock_guard<std::mutex> lock(_graphs_mutex);
  auto graph_iterator = _graphs.find(identifier);
  if (graph_iterator == _graphs.end()) {
    return nullptr;
  }
  return (*graph_iterator).second;
}

float Player::valueForPath(const std::string &path) {
  std::vector<std::string> major_split;
  std::istringstream iss(path);
  std::string s;
  while (getline(iss, s, '/')) {
    major_split.push_back(s);
  }
  if (major_split.size() > NFSmartPlayerCommandComponentIndex) {
    return 0.0;
  }
  std::shared_ptr<Graph> graph =
      graphForIdentifier(major_split[NFSmartPlayerGraphComponentIndex]);
  if (!graph) {
    return 0.0;
  }
  return graph->valueForPath(path);
}

void Player::setValueForPath(const std::string path, ...) {
  va_list args;
  va_start(args, path);
  vSetValueForPath(path, args);
  va_end(args);
}

void Player::vSetValueForPath(const std::string path, va_list args) {
  std::vector<std::string> major_split;
  std::istringstream iss(path);
  std::string s;
  while (getline(iss, s, '/')) {
    major_split.push_back(s);
  }
  std::shared_ptr<Graph> graph =
      graphForIdentifier(major_split[NFSmartPlayerGraphComponentIndex]);
  if (!graph) {
    return;
  }
  graph->vSetValueForPath(path, args);
}

void Player::setValuesForPath(const std::string path,
                              const std::vector<float> &values) {
  std::vector<std::string> major_split;
  boost::split(major_split, path, boost::is_any_of("/"));
  if (major_split.size() <= NFSmartPlayerParamComponentIndex) {
    return;
  }
  std::shared_ptr<Graph> graph =
      graphForIdentifier(major_split[NFSmartPlayerGraphComponentIndex]);
  if (!graph) {
    return;
  }
  graph->setValuesForPath(path, values);
}

void Player::forEachScript(NF_SMART_PLAYER_SCRIPT_CALLBACK script_callback) {
  for (const auto &script : _scripts) {
    if (!script_callback(script.second)) {
      break;
    }
  }
}

std::shared_ptr<Script> Player::scriptForIdentifier(
    const std::string &identifier) {
  return _scripts[identifier];
}

void Player::addGraph(std::shared_ptr<Graph> graph) {
  std::lock_guard<std::mutex> lock(_graphs_mutex);
  _graphs[graph->identifier()] = graph;
  recalculateRenderVariables();
}

void Player::removeGraph(const std::string &identifier) {
  std::lock_guard<std::mutex> lock(_graphs_mutex);
  _graphs.erase(identifier);
  recalculateRenderVariables();
  sendMessage("com.nativeformat.graph.remove", "com.nativeformat.host",
              NFSmartPlayerMessageTypeGeneric, identifier.c_str());
}

void Player::setLoadingPolicy(nfgrapher::LoadingPolicy loading_policy) {
  _loading_policy = loading_policy;
}

nfgrapher::LoadingPolicy Player::loadingPolicy() const {
  return _loading_policy;
}

bool Player::finished() {
  bool graphs_finished = true;
  double render_time = getRenderTime();
  int graphs_processed = 0;
  forEachGraph([render_time, &graphs_finished,
                &graphs_processed](const std::shared_ptr<Graph> &graph) {
    graphs_processed++;
    if (!graph->finished()) {
      graphs_finished = false;
      return false;
    }
    return true;
  });
  return graphs_finished && graphs_processed > 0;
}

bool Player::isLoaded() {
  bool loaded = true;
  int graphs = 0;
  forEachGraph([&loaded, &graphs](const std::shared_ptr<Graph> &graph) {
    graphs++;
    if (!graph->loaded()) {
      loaded = false;
      return false;
    }
    return true;
  });
  return loaded && graphs > 0;
}

void Player::driverDidStutter(void *clientdata) {}

int Player::driverRender(void *clientdata, float *samples, int frame_count) {
  int sample_count = frame_count * NF_DRIVER_CHANNELS;
  Player *player = (Player *)clientdata;
  std::lock_guard<std::mutex> render_lock(player->_render_mutex);
  size_t channels = NF_DRIVER_CHANNELS;
  double sample_rate = static_cast<double>(NF_DRIVER_SAMPLERATE);
  double render_time = player->_render_time;
  std::fill_n(samples, sample_count, 0);
  nfgrapher::LoadingPolicy loading_policy = player->_loading_policy;

  // Render the graphs
  std::lock_guard<std::mutex> lock(player->_graphs_mutex);
  for (const auto &graph : player->_graphs) {
    for (auto &content_pair : player->_graph_content_maps[graph.first]) {
      content_pair.second->erase();
    }
    graph.second->traverse(player->_graph_content_maps[graph.first],
                           loading_policy);
  }

  // Mix them together
  player->_final_content->setItems(sample_count);
  std::fill_n(player->_final_content->payload(),
              player->_final_content->payloadSize(), 0);
  size_t maximum_samples = 0;
  for (const auto &graph : player->_graphs) {
    maximum_samples = std::max(
        player->_final_content->mix(
            *player->_graph_content_maps[graph.first][AudioContentTypeKey]),
        maximum_samples);
  }
  std::copy_n(player->_final_content->payload(), maximum_samples, samples);
  double maximum_time_step =
      static_cast<double>(maximum_samples / channels) / sample_rate;

  // Limit the signal so no clipping occurs
  player->_limiter->limit(samples, samples, maximum_samples);

  // TODO: Delete this eventually, its just for my sanity while debugging
  for (size_t i = 0; i < sample_count; ++i) {
    samples[i] = std::max(std::min(1.0f, samples[i]), -1.0f);
  }

  // Calculate the next render time
  player->_render_time = render_time + maximum_time_step;

  return maximum_samples / NF_DRIVER_CHANNELS;
}

void Player::driverDidError(void *clientdata, const char *domain, int error) {}

void Player::driverWillRender(void *clientdata) {}

void Player::driverDidRender(void *clientdata) {}

void Player::receivedNotification(const Notification &notification) {
  for (auto &script : _scripts) {
    script.second->processNotification(notification);
  }

  if (!_message_callback) {
    return;
  }

  _message_callback(notification.name(), notification.senderIdentifier(),
                    notification.messageType(), notification.payload());
}

void Player::recalculateRenderVariables() {
  _graph_content_maps.clear();
  for (const auto &graph : _graphs) {
    const size_t payload_size = NF_DRIVER_SAMPLE_BLOCK_SIZE * _channels;
    _graph_content_maps[graph.first][AudioContentTypeKey] =
        std::make_shared<plugin::Content>(
            payload_size, 0, _samplerate, _channels,
            NF_DRIVER_SAMPLE_BLOCK_SIZE, plugin::ContentPayloadTypeBuffer);
  }
}

static_assert(DriverTypeFile == driver::OutputTypeFile,
              "OutputTypeFile must match DriverTypeFile");
static_assert(DriverTypeSoundCard == driver::OutputTypeSoundCard,
              "OutputTypeSoundCard must match DriverTypeSoundCard");

}  // namespace smartplayer
}  // namespace nativeformat
