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
#include <NFSmartPlayer/CallbackTypes.h>
#include <NFSmartPlayer/ErrorCode.h>

namespace nativeformat {

Load::Load(const Load &l) : _loaded(l._loaded) {
  for (auto &info : l._info) {
    _info.push_back(info);
  }
}

void Load::operator=(const Load &l) {
  _loaded = l._loaded;
  for (auto &info : l._info) {
    _info.push_back(info);
  }
}

Load::Load(bool loaded, std::string domain, std::string msg) : _loaded(loaded) {
  // Forward domain and message string to LogInfo<string> ctor
  _info.emplace_back(msg, logger::Severity::ERROR, domain);
}

Load::Load(bool loaded, ERROR_INFO info /* = ERROR_INFO() */)
    : _loaded(loaded), _info(std::move(info)) {}

Load::Load(Load &&l) : _loaded(l._loaded), _info(std::move(l._info)) {}

std::string errorMessageFromLoad(const Load &load) {
  std::stringstream ss;
  if (!load._loaded) {
    for (auto &info : load._info) {
      ss << info.toString();
    }
  }
  return ss.str();
}

}  // namespace nativeformat
