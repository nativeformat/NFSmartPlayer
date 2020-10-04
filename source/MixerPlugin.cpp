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
#include "MixerPlugin.h"

#define BOOST_THREAD_PROVIDES_FUTURE
#define BOOST_THREAD_PROVIDES_FUTURE_CONTINUATION
#define BOOST_THREAD_PROVIDES_FUTURE_WHEN_ALL_WHEN_ANY
#include <boost/thread.hpp>

namespace nativeformat {
namespace plugin {

const std::string MixerPluginIdentifier("com.nativeformat.plugin.mixer");

MixerPlugin::MixerPlugin(const std::vector<Metadata> &metadata, int channels,
                         double samplerate)
    : _child_metadata(metadata) {}

MixerPlugin::~MixerPlugin() {}

void MixerPlugin::majorTimeChange(long sample_index, long graph_sample_index) {
  for (const auto &metadata : _child_metadata) {
    metadata._plugin->majorTimeChange(sample_index, graph_sample_index);
  }
}

std::string MixerPlugin::name() { return MixerPluginIdentifier; }

std::vector<std::string> MixerPlugin::paramNames() { return {}; }

std::shared_ptr<param::Param> MixerPlugin::paramForName(
    const std::string &name) {
  return nullptr;
}

long MixerPlugin::timeDilation(long sample_index) { return sample_index; }

bool MixerPlugin::finished(long sample_index, long sample_index_end) {
  for (const auto &metadata : _child_metadata) {
    if (!metadata._plugin->finished(sample_index, sample_index_end)) {
      return false;
    }
  }
  return true;
}

void MixerPlugin::notifyFinished(long sample_index, long graph_sample_index) {
  for (const auto &metadata : _child_metadata) {
    metadata._plugin->notifyFinished(sample_index, graph_sample_index);
  }
}

Plugin::PluginType MixerPlugin::type() const {
  return Plugin::PluginTypeProducerConsumer;
}

void MixerPlugin::feed(std::map<std::string, std::shared_ptr<Content>> &content,
                       long sample_index, long graph_sample_index,
                       nfgrapher::LoadingPolicy loading_policy) {
  // if we have only 1 plugin then we don't have to mix
  // NOTE THIS HAS A SHORT CIRCUIT RETURN
  if (_child_metadata.size() == 1) {
    auto &metadata = _child_metadata.front();
    for (const auto &edge : metadata._edges) {
      // edge.first -> child (source)
      // edge.second -> parent (dest)
      metadata._content[edge.first] = content[edge.second];
    }
    metadata._plugin->feed(metadata._content, sample_index, graph_sample_index,
                           loading_policy);
    return;
  }

  // lets handle the mixing...
  for (auto &content_item_pair : _maximum_content_items) {
    content_item_pair.second = 0;
  }
  std::map<std::string, bool> first_processed;
  for (auto &metadata : _child_metadata) {
    auto &plugin = metadata._plugin;
    auto &edges = metadata._edges;
    auto &tmp_content = metadata._content;

    size_t max_items = 0;
    // Allocate content buffers for source plugins
    // if they dont already exist
    // TODO: Do we consider this auto-abuse?
    // edge is std::pair<std::string, std::string>
    for (const auto &edge : edges) {
      auto source = edge.first;
      auto dest = edge.second;

      /* THIS IS A HACK until we support not-audio
       or until we have a plugin that doesnt take
       a content labelled audio.
       Basically we will pull the information of the
       audio content to create a new buffer.
       At some point I want to push this logic into
      the plugins
      */
      if (content.find(dest) == content.end()) {
        auto new_content = std::make_shared<plugin::Content>(
            content[AudioContentTypeKey]->payloadSize(), 0,
            content[AudioContentTypeKey]->sampleRate(),
            content[AudioContentTypeKey]->channels(),
            content[AudioContentTypeKey]->requiredItems(),
            content[AudioContentTypeKey]->type());
        content.emplace(dest, new_content);
      }
      /* Now resume processing like normal... */
      auto source_it = tmp_content.find(source);
      auto dest_it = content.find(dest);
      // if we dont have a content buffer created for the child plugin
      // then allocate one
      if (source_it == tmp_content.end()) {
        auto new_content = std::make_shared<plugin::Content>(
            dest_it->second->payloadSize(), 0, dest_it->second->sampleRate(),
            dest_it->second->channels(), dest_it->second->requiredItems(),
            dest_it->second->type());
        auto result = tmp_content.emplace(source, new_content);
        if (result.second) {
          source_it = result.first;
        }
      }

      source_it->second->erase();
      max_items = std::max(source_it->second->requiredItems(), max_items);
    }

    // Process the plugin
    if (!plugin->shouldProcess(sample_index, sample_index + max_items)) {
      continue;
    }
    plugin->feed(tmp_content, sample_index, graph_sample_index, loading_policy);

    // Now mix!
    for (const auto &edge : edges) {
      auto dest = edge.second;
      auto source = edge.first;
      if (!first_processed.count(dest)) first_processed[dest] = false;
      if (!first_processed[dest]) {
        _maximum_content_items[dest] = tmp_content[source]->items();
        content[dest]->setItems(_maximum_content_items[dest]);
        // Copy instead of mixing since there's nothing in content[dest] yet
        std::copy_n(tmp_content[source]->payload(), content[dest]->items(),
                    content[dest]->payload());
        first_processed[dest] = true;
      } else {
        switch (loading_policy) {
          case nfgrapher::LoadingPolicy::SOME_CONTENT_PLAYTHROUGH:
            content[dest]->setItems(
                std::max(tmp_content[source]->items(), content[dest]->items()));
            break;
          case nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH:
            content[dest]->setItems(
                std::min(tmp_content[source]->items(), content[dest]->items()));
            break;
        }
        content[dest]->mix(*(tmp_content[source]));
      }
    }
  }
}

void MixerPlugin::load(LOAD_CALLBACK callback) {
  if (_child_metadata.empty()) {
    callback(Load{true});
  } else if (_child_metadata.size() == 1) {
    auto metadata = _child_metadata.front();
    // If plugin is null, something went wrong. Likely that the plugin is not
    // registered.
    if (!metadata._plugin) {
      callback(Load{false});
      return;
    }
    metadata._plugin->load(callback);
  } else {
    std::vector<boost::future<Load>> load_futures;
    for (auto &metadata : _child_metadata) {
      auto plugin = metadata._plugin;
      std::shared_ptr<boost::promise<Load>> promise =
          std::make_shared<boost::promise<Load>>();
      plugin->load([promise](const Load &load) { promise->set_value(load); });
      load_futures.push_back(promise->get_future());
    }
    boost::when_all(load_futures.begin(), load_futures.end())
        .then([callback](
                  boost::future<std::vector<boost::future<Load>>> futures) {
          std::vector<boost::future<Load>> results = futures.get();
          for (auto &future : results) {
            Load load = future.get();
            if (load._loaded == false) {
              callback(std::move(load));
              return;
            }
          }
          callback(Load{true});
        });
  }
}

bool MixerPlugin::loaded() const {
  if (_child_metadata.empty()) {
    return true;
  }
  for (const auto &metadata : _child_metadata) {
    if (!metadata._plugin->loaded()) {
      return false;
    }
  }
  return true;
}

void MixerPlugin::run(long sample_index, const NodeTimes &node_times,
                      long node_sample_index) {
  for (auto &metadata : _child_metadata) {
    metadata._plugin->run(sample_index, node_times, node_sample_index);
  }
}

long MixerPlugin::startSampleIndex() {
  long start_sample = LONG_MAX;
  for (auto &metadata : _child_metadata) {
    long tmp = metadata._plugin->startSampleIndex();
    start_sample = std::min(start_sample, tmp);
  }
  return start_sample;
}

bool MixerPlugin::shouldProcess(long sample_index_start,
                                long sample_index_end) {
  for (auto &metadata : _child_metadata) {
    if (metadata._plugin->shouldProcess(sample_index_start, sample_index_end)) {
      return true;
    }
  }
  return false;
}

}  // namespace plugin
}  // namespace nativeformat
