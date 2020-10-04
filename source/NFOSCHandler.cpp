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
#include "NFOSCHandler.h"

#include <NFSmartPlayer/Plugin.h>

#include <boost/algorithm/string.hpp>

#include "GraphImplementation.h"
#include "Player.h"

namespace nativeformat {
namespace smartplayer {

static const int NFSmartPlayerOSCHandlerInitialBufferSize = 1024;

NFOSCHandler::NFOSCHandler(std::weak_ptr<Player> smart_player,
                           int osc_read_port, int osc_write_port,
                           const std::string osc_address)
    : _receive_socket(
          IpEndpointName(IpEndpointName::ANY_ADDRESS, osc_read_port), this),
      _transmit_socket(IpEndpointName(osc_address.c_str(), osc_write_port)),
      _osc_thread(&NFOSCHandler::oscRun, &_receive_socket),
      _packet_buffer(NFSmartPlayerOSCHandlerInitialBufferSize),
      _command_callbacks(
          {{"value",
            std::bind(&NFOSCHandler::sendValue, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3)},
           {"setValue",
            std::bind(&NFOSCHandler::sendValue, this, std::placeholders::_1,
                      std::placeholders::_2, std::placeholders::_3)},
           {"setValueAtTime",
            std::bind(&NFOSCHandler::sendValueAtTime, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3)},
           {"linearRampToValueAtTime",
            std::bind(&NFOSCHandler::sendLinearRampToValueAtTime, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3)},
           {"setTargetAtTime",
            std::bind(&NFOSCHandler::sendTargetAtTime, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3)},
           {"exponentialRampToValueAtTime",
            std::bind(&NFOSCHandler::sendExponentialRampToValueAtTime, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3)},
           {"setValueCurveAtTime",
            std::bind(&NFOSCHandler::sendValueCurveAtTime, this,
                      std::placeholders::_1, std::placeholders::_2,
                      std::placeholders::_3)}}) {}

NFOSCHandler::~NFOSCHandler() {
  _receive_socket.AsynchronousBreak();
  _osc_thread.join();
}

void NFOSCHandler::oscRun(UdpListeningReceiveSocket *receive_socket) {
  receive_socket->Run();
}

void NFOSCHandler::broadcast() {
  static const int NFSmartPlayerOSCPacketBufferFactor = 2;

  if (auto player = _smart_player.lock()) {
    std::vector<std::string> variables;
    player->forEachGraph([&variables](const std::shared_ptr<Graph> &graph) {
      std::string variable_name = graph->identifier() + "/";
      graph->forEachNode(
          [&variables, variable_name](const std::shared_ptr<Node> &node) {
            std::string node_variable_name =
                variable_name + node->identifier() + "/";
            node->forEachParam([&variables, node_variable_name](
                                   const std::shared_ptr<param::Param> &param) {
              std::string param_variable_name =
                  node_variable_name + param->name() + "/value";
              variables.push_back(param_variable_name);
              return true;
            });
            return true;
          });
      return true;
    });
    if (variables.empty()) {
      return;
    }

    // Transmit the OSC packet
    size_t parameter_size = 0;
    for (const auto &variable : variables) {
      parameter_size += variable.length();
    }
    parameter_size *=
        NFSmartPlayerOSCPacketBufferFactor;  // Just to be on the safe side
    if (_packet_buffer.size() < parameter_size) {
      _packet_buffer.insert(_packet_buffer.end(),
                            parameter_size - _packet_buffer.size(), '\0');
    }
    osc::OutboundPacketStream p(&_packet_buffer[0], _packet_buffer.size());

    p << osc::BeginBundleImmediate;
    for (const auto &variable : variables) {
      p << osc::BeginMessage(variable.c_str()) << player->valueForPath(variable)
        << osc::EndMessage;
    }
    p << osc::EndBundle;

    _transmit_socket.Send(p.Data(), p.Size());
  }
}

void NFOSCHandler::sendValue(osc::ReceivedMessageArgumentStream &args,
                             std::shared_ptr<Player> &player,
                             const std::string &path) {
  float a1 = 0.0f;
  args >> a1 >> osc::EndMessage;
  player->setValueForPath(path, a1);
}

void NFOSCHandler::sendValueAtTime(osc::ReceivedMessageArgumentStream &args,
                                   std::shared_ptr<Player> &player,
                                   const std::string &path) {
  float value = 0.0f;
  double time = 0.0;
  args >> value >> time >> osc::EndMessage;
  player->setValueForPath(path, value, time);
}

void NFOSCHandler::sendLinearRampToValueAtTime(
    osc::ReceivedMessageArgumentStream &args, std::shared_ptr<Player> &player,
    const std::string &path) {
  float end_value = 0.0f;
  double end_time = 0.0;
  args >> end_value >> end_time >> osc::EndMessage;
  player->setValueForPath(path, end_value, end_time);
}

void NFOSCHandler::sendTargetAtTime(osc::ReceivedMessageArgumentStream &args,
                                    std::shared_ptr<Player> &player,
                                    const std::string &path) {
  float target = 0.0f;
  double start_time = 0.0;
  float time_constant = 0.0f;
  args >> target >> start_time >> time_constant >> osc::EndMessage;
  player->setValueForPath(path, target, start_time, time_constant);
}

void NFOSCHandler::sendExponentialRampToValueAtTime(
    osc::ReceivedMessageArgumentStream &args, std::shared_ptr<Player> &player,
    const std::string &path) {
  float value = 0.0f;
  double end_time = 0.0;
  args >> value >> end_time >> osc::EndMessage;
  player->setValueForPath(path, value, end_time);
}

void NFOSCHandler::sendValueCurveAtTime(
    osc::ReceivedMessageArgumentStream &args, std::shared_ptr<Player> &player,
    const std::string &path) {
  std::vector<float> values;
  try {
    while (true) {
      float value = 0.0f;
      args >> value;
      values.push_back(value);
    }
  } catch (osc::MissingArgumentException &exception) {
  }
  if (values.size() < 3) {
    return;
  }
  double start_time = static_cast<double>(*(values.end() - 2));
  double duration = static_cast<double>(*(values.end() - 1));
  values.erase(values.end() - 2, values.end());
  player->setValueForPath(path, &values[0], values.size(), start_time,
                          duration);
}

void NFOSCHandler::ProcessMessage(const osc::ReceivedMessage &m,
                                  const IpEndpointName &remoteEndpoint) {
  std::string address_pattern(m.AddressPattern());

  std::vector<std::string> major_split;
  boost::split(major_split, address_pattern, boost::is_any_of("/"));
  if (major_split.size() < NFSmartPlayerVariableComponentCount) {
    return;
  }

  osc::ReceivedMessageArgumentStream args = m.ArgumentStream();
  const std::string &command = major_split[NFSmartPlayerCommandComponentIndex];
  if (auto player = _smart_player.lock()) {
    const NF_OSC_HANDLER_COMMAND_CALLBACK command_callback =
        _command_callbacks[command];
    if (command_callback) {
      command_callback(args, player, command);
    }
  }
}

}  // namespace smartplayer
}  // namespace nativeformat
