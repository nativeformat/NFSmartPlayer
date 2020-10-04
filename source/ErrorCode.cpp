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
#include <NFSmartPlayer/ErrorCode.h>

namespace nativeformat {
namespace smartplayer {

const std::string ErrorCodeNoneString("none");
const std::string ErrorCodePluginNotFoundString("plugin_not_found");
const std::string ErrorCodeJSONDecodeFailedString("json_decode_failed");
const std::string ErrorCodeGraphIsSameString("graph_is_same");
const std::string ErrorCodeNoGraphsFoundString("no_graphs_found");
const std::string ErrorCodeCommandsFailedToParseString(
    "commands_failed_to_parse");
const std::string ErrorCodeFailedToLoadPluginFactoriesString(
    "failed_to_load_plugin_factories");
const std::string ErrorCodeReloadCalledBeforeLoadedString(
    "reload_called_before_loaded");
const std::string ErrorCodePluginFailedToLoadString("plugin_failed_to_load");

ErrorCode errorCodeFromString(const std::string &error_code) {
  if (error_code == ErrorCodePluginNotFoundString) {
    return ErrorCodePluginNotFound;
  } else if (error_code == ErrorCodeJSONDecodeFailedString) {
    return ErrorCodeJSONDecodeFailed;
  } else if (error_code == ErrorCodeGraphIsSameString) {
    return ErrorCodeGraphIsSame;
  } else if (error_code == ErrorCodeNoGraphsFoundString) {
    return ErrorCodeNoGraphsFound;
  } else if (error_code == ErrorCodeCommandsFailedToParseString) {
    return ErrorCodeCommandsFailedToParse;
  } else if (error_code == ErrorCodeFailedToLoadPluginFactoriesString) {
    return ErrorCodeFailedToLoadPluginFactories;
  } else if (error_code == ErrorCodeReloadCalledBeforeLoadedString) {
    return ErrorCodeReloadCalledBeforeLoaded;
  } else if (error_code == ErrorCodePluginFailedToLoadString) {
    return ErrorCodePluginFailedToLoad;
  }
  return ErrorCodeNone;
}

const std::string stringFromErrorCode(ErrorCode error_code) {
  switch (error_code) {
    case ErrorCodeNone:
      return "";
    case ErrorCodeJSONDecodeFailed:
      return ErrorCodeJSONDecodeFailedString;
    case ErrorCodeGraphIsSame:
      return ErrorCodeGraphIsSameString;
    case ErrorCodeNoGraphsFound:
      return ErrorCodeNoGraphsFoundString;
    case ErrorCodeCommandsFailedToParse:
      return ErrorCodeCommandsFailedToParseString;
    case ErrorCodeFailedToLoadPluginFactories:
      return ErrorCodeFailedToLoadPluginFactoriesString;
    case ErrorCodeReloadCalledBeforeLoaded:
      return ErrorCodeReloadCalledBeforeLoadedString;
    case ErrorCodePluginFailedToLoad:
      return ErrorCodePluginFailedToLoadString;
    case ErrorCodePluginNotFound:
      return ErrorCodePluginNotFoundString;
  }
}

}  // namespace smartplayer
}  // namespace nativeformat
