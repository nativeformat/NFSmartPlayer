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

#include <ip/UdpSocket.h>
#include <osc/OscOutboundPacketStream.h>
#include <osc/OscPacketListener.h>

#include <functional>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

namespace nativeformat {
namespace smartplayer {

class Player;

typedef std::function<void(osc::ReceivedMessageArgumentStream &,
                           std::shared_ptr<Player> &, const std::string &)>
    NF_OSC_HANDLER_COMMAND_CALLBACK;

class NFOSCHandler : public osc::OscPacketListener {
 public:
  NFOSCHandler(std::weak_ptr<Player> smart_player, int osc_read_port,
               int osc_write_port, const std::string osc_address);
  virtual ~NFOSCHandler();

  void broadcast();
  void sendValue(osc::ReceivedMessageArgumentStream &args,
                 std::shared_ptr<Player> &player, const std::string &path);
  void sendValueAtTime(osc::ReceivedMessageArgumentStream &args,
                       std::shared_ptr<Player> &player,
                       const std::string &path);
  void sendLinearRampToValueAtTime(osc::ReceivedMessageArgumentStream &args,
                                   std::shared_ptr<Player> &player,
                                   const std::string &path);
  void sendTargetAtTime(osc::ReceivedMessageArgumentStream &args,
                        std::shared_ptr<Player> &player,
                        const std::string &path);
  void sendExponentialRampToValueAtTime(
      osc::ReceivedMessageArgumentStream &args, std::shared_ptr<Player> &player,
      const std::string &path);
  void sendValueCurveAtTime(osc::ReceivedMessageArgumentStream &args,
                            std::shared_ptr<Player> &player,
                            const std::string &path);

  // OscPacketListener
  void ProcessMessage(const osc::ReceivedMessage &m,
                      const IpEndpointName &remoteEndpoint) override;

 private:
  static void oscRun(UdpListeningReceiveSocket *receive_socket);

  std::weak_ptr<Player> _smart_player;

  UdpListeningReceiveSocket _receive_socket;
  UdpTransmitSocket _transmit_socket;
  std::thread _osc_thread;
  std::vector<char> _packet_buffer;
  std::unordered_map<std::string, NF_OSC_HANDLER_COMMAND_CALLBACK>
      _command_callbacks;
};

}  // namespace smartplayer
}  // namespace nativeformat
