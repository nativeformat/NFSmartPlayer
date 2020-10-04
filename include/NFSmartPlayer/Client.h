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

#include <NFLogger/LogInfo.h>
#include <NFSmartPlayer/CallbackTypes.h>
#include <NFSmartPlayer/ErrorCode.h>
#include <NFSmartPlayer/Graph.h>
#include <NFSmartPlayer/Script.h>

#include <functional>
#include <future>
#include <memory>
#include <string>
#include <unordered_map>

namespace nativeformat {
namespace smartplayer {

typedef enum : int {
  NFSmartPlayerMessageTypeGeneric,
  NFSmartPlayerMessageTypeNone,
  NFSmartPlayerMessageTypeError
} NFSmartPlayerMessageType;

typedef enum : int { DriverTypeSoundCard, DriverTypeFile } DriverType;

typedef std::function<void(
    const std::string &message_identifier, const std::string &sender_identifier,
    NFSmartPlayerMessageType message_type, const void *payload)>
    NF_SMART_PLAYER_MESSAGE_CALLBACK;
typedef std::function<void(Load::ERROR_INFO &info)>
    NF_SMART_PLAYER_ERROR_CALLBACK;
typedef std::function<std::string(const std::string &plugin_namespace,
                                  const std::string &variable_name)>
    NF_SMART_PLAYER_RESOLVE_CALLBACK;
typedef std::function<bool(const std::shared_ptr<Graph> &)>
    NF_SMART_PLAYER_GRAPH_CALLBACK;
typedef std::function<bool(const std::shared_ptr<Script> &)>
    NF_SMART_PLAYER_SCRIPT_CALLBACK;

extern const std::string PlayerErrorDomain;

extern const char *NFSmartPlayerOscAddress;
extern const char *PlayerMessageIdentifierEnd;
extern const char *PlayerSenderIdentifier;
extern const char *Version;

extern const int NFSmartPlayerOscReadPort;
extern const int NFSmartPlayerOscWritePort;
extern const int NFSmartPlayerLocalhostPort;

class Client {
 public:
  virtual void setJson(const std::string &score_json,
                       LOAD_CALLBACK load_callback) = 0;
  virtual bool isPlaying() const = 0;
  virtual void setPlaying(bool playing) = 0;
  virtual double getRenderTime() const = 0;
  virtual void setRenderTime(double time) = 0;
  virtual void setMessageCallback(
      NF_SMART_PLAYER_MESSAGE_CALLBACK callback) = 0;
  virtual void sendMessage(const std::string &message_identifier,
                           NFSmartPlayerMessageType message_type,
                           const void *payload) = 0;
  virtual int numberOfScripts() = 0;
  virtual std::shared_ptr<Script> openScript(const std::string &name,
                                             const std::string &script) = 0;
  virtual std::shared_ptr<Graph> graphForIdentifier(
      const std::string &identifier) = 0;
  virtual void forEachGraph(NF_SMART_PLAYER_GRAPH_CALLBACK graph_callback) = 0;
  virtual float valueForPath(const std::string &path) = 0;
  virtual void setValueForPath(const std::string path, ...) = 0;
  virtual void vSetValueForPath(const std::string path, va_list args) = 0;
  virtual void setValuesForPath(const std::string path,
                                const std::vector<float> &values) = 0;
  virtual void forEachScript(
      NF_SMART_PLAYER_SCRIPT_CALLBACK script_callback) = 0;
  virtual std::shared_ptr<Script> scriptForIdentifier(
      const std::string &identifier) = 0;
  virtual void addGraph(std::shared_ptr<Graph> graph) = 0;
  virtual void removeGraph(const std::string &identifier) = 0;
  virtual void setLoadingPolicy(nfgrapher::LoadingPolicy loading_policy) = 0;
  virtual nfgrapher::LoadingPolicy loadingPolicy() const = 0;
  virtual bool isLoaded() = 0;
};

extern std::shared_ptr<Client> createClient(
    NF_SMART_PLAYER_RESOLVE_CALLBACK resolve_callback,
    NF_SMART_PLAYER_ERROR_CALLBACK error_callback, DriverType driver_type,
    std::string output_destination,
    int osc_read_port = NFSmartPlayerOscReadPort,
    int osc_write_port = NFSmartPlayerOscWritePort,
    const std::string osc_address = NFSmartPlayerOscAddress,
    int localhost_write_port = NFSmartPlayerLocalhostPort);

}  // namespace smartplayer
}  // namespace nativeformat
