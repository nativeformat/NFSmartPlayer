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

#include <NFHTTP/Client.h>
#include <NFHTTP/Request.h>

#include <functional>
#include <memory>

namespace nativeformat {
namespace smartplayer {

class Authoriser {
 public:
  virtual bool requiresAuthorisation(
      const std::shared_ptr<http::Request> &request) = 0;
  virtual void authoriseRequest(
      std::function<void(const std::shared_ptr<http::Request> &request)>
          callback,
      const std::shared_ptr<http::Request> &request) = 0;
  virtual bool isAuthorised(const std::shared_ptr<http::Request> &request) = 0;
  virtual bool shouldRetryFailedAuthorisation(
      const std::shared_ptr<http::Request> &request) = 0;
};

}  // namespace smartplayer
}  // namespace nativeformat
