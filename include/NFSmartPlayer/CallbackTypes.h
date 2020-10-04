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

#include <functional>
#include <list>

namespace nativeformat {

typedef struct Load {
  typedef std::list<logger::LogInfo<std::string>> ERROR_INFO;
  bool _loaded;
  ERROR_INFO _info;

  /*
   * since boost futures are not compatible with stdlib rvalue
   * references, allow copy
   */
  Load(const Load &l);
  void operator=(const Load &l);

  Load(bool loaded, std::string domain, std::string msg);
  Load(bool loaded, ERROR_INFO info = ERROR_INFO());
  Load(Load &&l);
} Load;

typedef std::function<void(Load)> LOAD_CALLBACK;

extern std::string errorMessageFromLoad(const Load &load);

}  // namespace nativeformat
