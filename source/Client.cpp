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
#include <NFSmartPlayer/Client.h>
#include <nf_generated_header.h>

#include "ClientImplementation.h"

namespace nativeformat {
namespace smartplayer {

const std::string PlayerErrorDomain("com.nativeformat.smartplayer");

const char *PlayerMessageIdentifierEnd = "com.nativeformat.player.end";
const char *PlayerSenderIdentifier = "com.nativeformat.client";
const char *Version = SMARTPLAYER_VERSION;

std::shared_ptr<Client> createClient(
    NF_SMART_PLAYER_RESOLVE_CALLBACK resolve_callback,
    NF_SMART_PLAYER_ERROR_CALLBACK error_callback, DriverType driver_type,
    std::string output_destination, int osc_read_port, int osc_write_port,
    const std::string osc_address, int localhost_write_port) {
  return std::make_shared<ClientImplementation>(
      resolve_callback, error_callback, driver_type, output_destination,
      osc_read_port, osc_write_port, osc_address, localhost_write_port);
}

}  // namespace smartplayer
}  // namespace nativeformat
