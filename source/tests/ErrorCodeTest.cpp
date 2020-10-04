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

#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(ErrorCodeTests)

BOOST_AUTO_TEST_CASE(testErrorCodePluginNotFoundString) {
  nativeformat::smartplayer::ErrorCode code =
      nativeformat::smartplayer::errorCodeFromString("plugin_not_found");
  BOOST_CHECK_EQUAL(code, nativeformat::smartplayer::ErrorCodePluginNotFound);
}

BOOST_AUTO_TEST_CASE(testErrorCodeJSONDecodeFailedString) {
  nativeformat::smartplayer::ErrorCode code =
      nativeformat::smartplayer::errorCodeFromString("json_decode_failed");
  BOOST_CHECK_EQUAL(code, nativeformat::smartplayer::ErrorCodeJSONDecodeFailed);
}

BOOST_AUTO_TEST_CASE(testErrorCodeGraphIsSameString) {
  nativeformat::smartplayer::ErrorCode code =
      nativeformat::smartplayer::errorCodeFromString("graph_is_same");
  BOOST_CHECK_EQUAL(code, nativeformat::smartplayer::ErrorCodeGraphIsSame);
}

BOOST_AUTO_TEST_CASE(testErrorCodeNoGraphsFoundString) {
  nativeformat::smartplayer::ErrorCode code =
      nativeformat::smartplayer::errorCodeFromString("no_graphs_found");
  BOOST_CHECK_EQUAL(code, nativeformat::smartplayer::ErrorCodeNoGraphsFound);
}

BOOST_AUTO_TEST_CASE(testErrorCodeCommandsFailedToParseString) {
  nativeformat::smartplayer::ErrorCode code =
      nativeformat::smartplayer::errorCodeFromString(
          "commands_failed_to_parse");
  BOOST_CHECK_EQUAL(code,
                    nativeformat::smartplayer::ErrorCodeCommandsFailedToParse);
}

BOOST_AUTO_TEST_CASE(testErrorCodeFailedToLoadPluginFactoriesString) {
  nativeformat::smartplayer::ErrorCode code =
      nativeformat::smartplayer::errorCodeFromString(
          "failed_to_load_plugin_factories");
  BOOST_CHECK_EQUAL(
      code, nativeformat::smartplayer::ErrorCodeFailedToLoadPluginFactories);
}

BOOST_AUTO_TEST_CASE(testErrorCodeReloadCalledBeforeLoadedString) {
  nativeformat::smartplayer::ErrorCode code =
      nativeformat::smartplayer::errorCodeFromString(
          "reload_called_before_loaded");
  BOOST_CHECK_EQUAL(
      code, nativeformat::smartplayer::ErrorCodeReloadCalledBeforeLoaded);
}

BOOST_AUTO_TEST_CASE(testErrorCodePluginFailedToLoadString) {
  nativeformat::smartplayer::ErrorCode code =
      nativeformat::smartplayer::errorCodeFromString("plugin_failed_to_load");
  BOOST_CHECK_EQUAL(code,
                    nativeformat::smartplayer::ErrorCodePluginFailedToLoad);
}

BOOST_AUTO_TEST_CASE(testErrorCodeNoneString) {
  nativeformat::smartplayer::ErrorCode code =
      nativeformat::smartplayer::errorCodeFromString("thing");
  BOOST_CHECK_EQUAL(code, nativeformat::smartplayer::ErrorCodeNone);
}

BOOST_AUTO_TEST_CASE(testErrorCodeGraphIsSameToString) {
  std::string error_string = nativeformat::smartplayer::stringFromErrorCode(
      nativeformat::smartplayer::ErrorCodeGraphIsSame);
  BOOST_CHECK_EQUAL(error_string, "graph_is_same");
}

BOOST_AUTO_TEST_CASE(testErrorCodeNoneToString) {
  std::string error_string = nativeformat::smartplayer::stringFromErrorCode(
      nativeformat::smartplayer::ErrorCodeNone);
  BOOST_CHECK_EQUAL(error_string, "");
}

BOOST_AUTO_TEST_CASE(testErrorCodeNoGraphsFoundToString) {
  std::string error_string = nativeformat::smartplayer::stringFromErrorCode(
      nativeformat::smartplayer::ErrorCodeNoGraphsFound);
  BOOST_CHECK_EQUAL(error_string, "no_graphs_found");
}

BOOST_AUTO_TEST_CASE(testErrorCodeCommandsFailedToParseToString) {
  std::string error_string = nativeformat::smartplayer::stringFromErrorCode(
      nativeformat::smartplayer::ErrorCodeCommandsFailedToParse);
  BOOST_CHECK_EQUAL(error_string, "commands_failed_to_parse");
}

BOOST_AUTO_TEST_CASE(testErrorCodeFailedToLoadPluginFactoriesToString) {
  std::string error_string = nativeformat::smartplayer::stringFromErrorCode(
      nativeformat::smartplayer::ErrorCodeFailedToLoadPluginFactories);
  BOOST_CHECK_EQUAL(error_string, "failed_to_load_plugin_factories");
}

BOOST_AUTO_TEST_CASE(testErrorCodeReloadCalledBeforeLoadedToString) {
  std::string error_string = nativeformat::smartplayer::stringFromErrorCode(
      nativeformat::smartplayer::ErrorCodeReloadCalledBeforeLoaded);
  BOOST_CHECK_EQUAL(error_string, "reload_called_before_loaded");
}

BOOST_AUTO_TEST_CASE(testErrorCodePluginFailedToLoadToString) {
  std::string error_string = nativeformat::smartplayer::stringFromErrorCode(
      nativeformat::smartplayer::ErrorCodePluginFailedToLoad);
  BOOST_CHECK_EQUAL(error_string, "plugin_failed_to_load");
}

BOOST_AUTO_TEST_CASE(testErrorCodePluginNotFoundToString) {
  std::string error_string = nativeformat::smartplayer::stringFromErrorCode(
      nativeformat::smartplayer::ErrorCodePluginNotFound);
  BOOST_CHECK_EQUAL(error_string, "plugin_not_found");
}

BOOST_AUTO_TEST_SUITE_END()
