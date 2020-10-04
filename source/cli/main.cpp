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
#include <NFHTTP/Client.h>
#include <NFSmartPlayer/GlobalLogger.h>
#include <NFSmartPlayer/nf_smart_player.h>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>
#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <map>
#include <nlohmann/json.hpp>
#include <streambuf>
#include <string>
#include <thread>
#include <vector>

#if __APPLE__
#include <CoreFoundation/CFRunLoop.h>
#endif

static std::atomic<bool> alive;
static std::mutex m;
static std::condition_variable alive_condition;
static NF_SMART_PLAYER_HANDLE handle = nullptr;
static boost::program_options::variables_map options_map;
static std::string CliErrorDomain("com.nativeformat.smartplayer.cli");

using nativeformat::logger::LogInfo;

void stdin_help() {
  std::cout << "Interactive mode supported commands:" << std::endl;
  std::cout << "  play|pause|exit|help" << std::endl;
  std::cout << "  seek [target time (sec)]" << std::endl;
  std::cout << "  ffwd [time difference (sec)]" << std::endl;
  std::cout << "  rewind [time difference (sec)]" << std::endl;
}

void interactive_thread() {
  std::string command;
  while (alive) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    if (smartplayer_is_loaded(handle)) {
      std::cout << "Enter command (or ? for help): ";
      std::getline(std::cin, command);
      std::vector<std::string> tokens;
      boost::split(tokens, command, boost::is_any_of(" "),
                   boost::token_compress_on);
      double current_render_time = smartplayer_render_time(handle);
      if (tokens.size() == 0) continue;
      if (tokens[0] == "pause") {
        smartplayer_set_playing(handle, false);
      } else if (tokens[0] == "play") {
        smartplayer_set_playing(handle, true);
      } else if (tokens.size() > 0 && tokens[0] == "seek") {
        // seeks to 0 if invalid number
        smartplayer_set_render_time(handle, atof(tokens[1].c_str()));
      } else if (tokens.size() > 0 && tokens[0] == "ffwd") {
        smartplayer_set_render_time(
            handle, current_render_time + atof(tokens[1].c_str()));
      } else if (tokens.size() > 0 && tokens[0] == "rewind") {
        smartplayer_set_render_time(
            handle, current_render_time - atof(tokens[1].c_str()));
      } else if (tokens[0] == "exit") {
        smartplayer_set_playing(handle, false);
        alive = false;
      } else if (tokens[0] == "help" || tokens[0] == "?") {
        stdin_help();
      } else {
        std::cout << "Invalid command: " << command << std::endl;
        stdin_help();
      }
    }
  }
}

int main(int argc, char *argv[]) {
  static const char *help_option = "help";
  static const char *input_file_option = "input-file";
  static const char *resolved_variables = "resolved-variables";
  static const char *driver_type_option = "driver-type";
  static const char *driver_file_option = "driver-file";
  static const char *render_time_option = "render-time";
  static const char *interactive_option = "interactive";
  static const char *smartplayer_wave = "/nfsmartplayer.wav";

  {
    std::cout << "Native Format Command Line Interface "
              << NF_SMART_PLAYER_VERSION << std::endl;

    // Get the current working directory
    char current_working_directory[256];
    getcwd(current_working_directory, sizeof(current_working_directory));

    boost::program_options::options_description desc("Allowed options");
    desc.add_options()(help_option, "produce help message")(
        input_file_option, boost::program_options::value<std::string>(),
        "tells the player where to get the graph from")(
        resolved_variables,
        boost::program_options::value<std::string>()->default_value(""),
        "variables to resolve in the player, in the format of CSV")(
        driver_type_option,
        boost::program_options::value<std::string>()->default_value(
            NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD_STRING),
        "driver type to render to")(
        driver_file_option,
        boost::program_options::value<std::string>()->default_value(
            std::string(current_working_directory) + smartplayer_wave),
        "driver file to render to")(
        render_time_option,
        boost::program_options::value<double>()->default_value(0.0),
        "The time to start rendering at");
    boost::program_options::store(
        boost::program_options::parse_command_line(argc, argv, desc),
        options_map);

    if (options_map.count(help_option) > 0) {
      std::cout << desc << std::endl;
      return 1;
    }

    std::string json_graph;
    if (options_map.count(input_file_option) > 0) {
      std::string json_file = options_map[input_file_option].as<std::string>();
      std::ifstream file_stream(json_file);
      json_graph = std::string(std::istreambuf_iterator<char>(file_stream),
                               std::istreambuf_iterator<char>());
      if (json_graph.empty()) {
        std::cout << "Failed to read from file: " + json_file << std::endl;
        return 1;
      }
    }

    if (json_graph.empty()) {
      for (std::string line; std::getline(std::cin, line);) {
        json_graph.append(line);
      }
    }

    std::string access_token("");
    std::string token_type("");
    std::map<std::string, std::string> resolved_variable_map;
    if (options_map.count(resolved_variables) > 0) {
      std::vector<std::string> csv_separated_strings;
      boost::split(csv_separated_strings,
                   options_map[resolved_variables].as<std::string>(),
                   boost::is_any_of(","));
      for (int i = 0; i < csv_separated_strings.size() - 1; i += 2) {
        resolved_variable_map[csv_separated_strings[i]] =
            csv_separated_strings[i + 1];
      }
    }

    NF_SMART_PLAYER_DRIVER_TYPE driver_type =
        NF_SMART_PLAYER_DRIVER_TYPE_SOUNDCARD;
    if (options_map.count(driver_type_option) > 0) {
      driver_type = smartplayer_driver_type_from_string(
          options_map[driver_type_option].as<std::string>().c_str());
    }

    handle = smartplayer_open(
        [](void *context, const char *plugin_namespace,
           const char *variable_identifier) -> const char * {
          std::map<std::string, std::string> *resolved_variable_map =
              (std::map<std::string, std::string> *)context;
          return (*resolved_variable_map)[variable_identifier].c_str();
        },
        &resolved_variable_map, NF_SMART_PLAYER_DEFAULT_SETTINGS, driver_type,
        options_map[driver_file_option].as<std::string>().c_str());

    alive = true;
    smartplayer_set_message_callback(
        handle,
        [](void *context, const char *message_identifier,
           const char *sender_identifier,
           NF_SMART_PLAYER_MESSAGE_TYPE message_type, const void *payload) {
          if (strcmp(message_identifier,
                     NF_SMART_PLAYER_MESSAGE_IDENTIFIER_END) == 0 &&
              strcmp(sender_identifier, NF_SMART_PLAYER_SENDER_IDENTIFIER) ==
                  0) {
            {
              std::lock_guard<std::mutex> lg(m);
              alive = false;
            }
            alive_condition.notify_one();
          }
        });
    std::cout << "Loading JSON" << std::endl;
    smartplayer_set_json(
        handle, json_graph.c_str(),
        [](void *context, int success, const char *error_message) {
          if (!success) {
            std::string msg = "Failed to play the JSON file: ";
            msg += error_message;
            NF_ERROR(msg);
            std::exit(1);
          } else {
            NF_INFO("Initialised playback");
            smartplayer_set_render_time(
                handle, options_map[render_time_option].as<double>());
            smartplayer_set_playing(handle, true);
          }
        });

    if (options_map.count(interactive_option) > 0) {
      std::thread t1 = std::thread(interactive_thread);
      while (alive) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
#if __APPLE__
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
#endif
      }
      t1.join();
    } else {
      while (alive) {
#if __APPLE__
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0, true);
#endif
        std::unique_lock<std::mutex> ul(m);
        alive_condition.wait_for(ul, std::chrono::seconds(1));
        if (smartplayer_is_loaded(handle)) {
          std::cout << "Time: " << smartplayer_render_time(handle) << std::endl;
        }
      }
    }

    std::cout << "Final time: " << smartplayer_render_time(handle) << std::endl;
    std::cout << "Finished rendering..." << std::endl;

    smartplayer_close(handle);
  }
  std::this_thread::sleep_for(std::chrono::seconds(1));

  return 0;
}
