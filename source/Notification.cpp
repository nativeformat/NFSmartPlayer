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
#include "Notification.h"

namespace nativeformat {
namespace smartplayer {

Notification::Notification(const std::string &message_identifier,
                           const std::string &sender_identifier,
                           const void *payload,
                           NFSmartPlayerMessageType message_type)
    : _message_identifier(message_identifier),
      _sender_identifier(sender_identifier),
      _payload(payload),
      _message_type(message_type) {}

Notification::~Notification() {}

std::string Notification::name() const { return _message_identifier; }

std::string Notification::senderIdentifier() const {
  return _sender_identifier;
}

const void *Notification::payload() const { return _payload; }

NFSmartPlayerMessageType Notification::messageType() const {
  return _message_type;
}

}  // namespace smartplayer
}  // namespace nativeformat
