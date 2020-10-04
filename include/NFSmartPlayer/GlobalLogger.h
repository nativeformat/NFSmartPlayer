/*
 * Copyright (c) 2018 Spotify AB.
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

#include <NFLogger/Logger.h>

#define NF_INFO(i) INFO(nativeformat::getGlobalLogger(), i)
#define NF_WARN(i) WARN(nativeformat::getGlobalLogger(), i)
#define NF_ERROR(i) ERROR(nativeformat::getGlobalLogger(), i)
#define NF_REPORT(i) REPORT(nativeformat::getGlobalLogger(), i)

namespace nativeformat {
using logger::Logger;
using logger::LogInfoHandler;

typedef Logger<LogInfoHandler<std::string>> NF_LOGGER_TYPE;

void configGlobalLogger(std::string token_type, std::string token);
NF_LOGGER_TYPE &getGlobalLogger();
}  // namespace nativeformat
