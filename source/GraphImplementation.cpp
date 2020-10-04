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
#include "GraphImplementation.h"

#include <boost/algorithm/string.hpp>
#include <queue>
#include <string>

#include "EdgeImplementation.h"
#include "MixerPlugin.h"
#include "NodeImplementation.h"

namespace nativeformat {
namespace smartplayer {

struct PrioritisedNode {
  std::shared_ptr<Node> node;
  std::vector<std::shared_ptr<PrioritisedNode>> output;
  std::vector<std::shared_ptr<PrioritisedNode>> input;
  int priority;

  PrioritisedNode(std::shared_ptr<Node> node) : node(node), priority(0) {}
  PrioritisedNode() : node(nullptr), priority(0) {}
};

// the key is the pair defining the output node's id + input node's id
// This way we can collect all the edges between the two nodes easily
using EdgeMap = std::map<std::pair<std::string, std::string>,
                         std::vector<std::pair<std::string, std::string>>>;

const std::string GraphImplementationErrorDomain =
    "com.nativeformat.graph.error";
static const std::string GraphImplementationOutputDriverTarget =
    "output_driver";

static long PlayerContentExpectedRenderSamples(
    const std::map<std::string, std::shared_ptr<plugin::Content>> &content);
static void PrioritiseNodes(
    std::map<std::string, std::shared_ptr<PrioritisedNode>> &nodes,
    std::shared_ptr<PrioritisedNode> &output_node);
static int NodePriorityTraversal(std::shared_ptr<PrioritisedNode> &node);
static std::shared_ptr<plugin::Plugin> CreatePluginTraverse(
    std::shared_ptr<PrioritisedNode> &node, const EdgeMap &edge_map,
    const std::string &graph_id, int channels, double samplerate,
    std::shared_ptr<plugin::Registry> &plugin_registry,
    std::shared_ptr<PrioritisedNode> &output_node,
    const std::string &session_id);

GraphImplementation::GraphImplementation(int channels, double samplerate,
                                         const std::string &session_id)
    : _channels(channels),
      _samplerate(samplerate),
      _session_id(session_id),
      _sample_index(0),
      _finished(false) {}

GraphImplementation::~GraphImplementation() {}

void GraphImplementation::setJson(const std::string &json,
                                  LOAD_CALLBACK load_callback) {
  try {
    const nlohmann::json &parsed_json = nlohmann::json::parse(json);
    setJson(parsed_json, load_callback);
  } catch (const std::exception &exception) {
    std::stringstream error_ss;
    error_ss << "Invalid JSON: " << exception.what();
    load_callback(Load{false, GraphImplementationErrorDomain, error_ss.str()});
  }
}

std::string GraphImplementation::identifier() const { return _identifier; }

double GraphImplementation::renderTime() const {
  return (_sample_index / _channels) / _samplerate;
}

void GraphImplementation::setRenderTime(double render_time) {
  std::lock_guard<std::mutex> lock(_nodes_mutex);
  long sample_index = (render_time * _channels) * _samplerate;
  sample_index -= sample_index % 2;
  _sample_index = sample_index;
  _root_plugin->majorTimeChange(sample_index, sample_index);
}

float GraphImplementation::valueForPath(const std::string &path) {
  std::vector<std::string> major_split;
  boost::split(major_split, path, boost::is_any_of("/"));
  if (major_split.size() > NFSmartPlayerCommandComponentIndex) {
    return 0.0;
  }
  const std::string &graph_identifier =
      major_split[NFSmartPlayerGraphComponentIndex];
  if (graph_identifier != identifier()) {
    return 0.0;
  }
  auto param = parameterForPath(path);
  if (!param) {
    return 0.0f;
  }
  return param->valueForTime(renderTime());
}

void GraphImplementation::setValueForPath(const std::string path, ...) {
  va_list valist;
  va_start(valist, path);
  vSetValueForPath(path, valist);
  va_end(valist);
}

void GraphImplementation::vSetValueForPath(const std::string path,
                                           va_list valist) {
  std::vector<std::string> major_split;
  boost::split(major_split, path, boost::is_any_of("/"));
  if (major_split.size() >= NFSmartPlayerParamComponentIndex) {
    const std::string graph_identifier =
        major_split[NFSmartPlayerGraphComponentIndex];
    if (graph_identifier == identifier()) {
      std::shared_ptr<param::Param> param = parameterForPath(path);
      if (!param) {
        return;
      }
      const std::string &command =
          major_split[NFSmartPlayerCommandComponentIndex];
      if (command == "value" || command == "setValue") {
        double value = va_arg(valist, double);
        param->setValueAtTime(value, renderTime());
      } else if (command == "setValueAtTime") {
        param->setValueAtTime(va_arg(valist, double), va_arg(valist, double));
      } else if (command == "linearRampToValueAtTime") {
        param->linearRampToValueAtTime(va_arg(valist, double),
                                       va_arg(valist, double));
      } else if (command == "setTargetAtTime") {
        param->setTargetAtTime(va_arg(valist, double), va_arg(valist, double),
                               va_arg(valist, double));
      } else if (command == "exponentialRampToValueAtTime") {
        param->exponentialRampToValueAtTime(va_arg(valist, double),
                                            va_arg(valist, double));
      } else if (command == "setValueCurveAtTime") {
        float *values = va_arg(valist, float *);
        int number_of_values = va_arg(valist, int);
        double start_time = va_arg(valist, double);
        double duration = va_arg(valist, double);
        std::vector<float> vector_values(values, values + number_of_values);
        param->setValueCurveAtTime(vector_values, start_time, duration);
      }
    }
  }
}

void GraphImplementation::setValuesForPath(const std::string path,
                                           const std::vector<float> &values) {
  std::vector<std::string> major_split;
  boost::split(major_split, path, boost::is_any_of("/"));
  if (major_split.size() >= NFSmartPlayerParamComponentIndex) {
    const std::string graph_identifier =
        major_split[NFSmartPlayerGraphComponentIndex];
    if (graph_identifier == identifier()) {
      std::shared_ptr<param::Param> param = parameterForPath(path);
      if (!param) {
        return;
      }
      const std::string &command =
          major_split[NFSmartPlayerCommandComponentIndex];
      if (command == "value" || command == "setValue") {
        double value = values.front();
        param->setValueAtTime(value, renderTime());
      } else if (command == "setValueAtTime") {
        param->setValueAtTime(values[0], values[1]);
      } else if (command == "linearRampToValueAtTime") {
        param->linearRampToValueAtTime(values[0], values[1]);
      } else if (command == "setTargetAtTime") {
        param->setTargetAtTime(values[0], values[1], values[2]);
      } else if (command == "exponentialRampToValueAtTime") {
        param->exponentialRampToValueAtTime(values[0], values[1]);
      } else if (command == "setValueCurveAtTime") {
        param->setValueCurveAtTime(
            std::vector<float>(values.begin(),
                               values.begin() + values.size() - 2),
            values[values.size() - 2], values[values.size() - 1]);
      }
    }
  }
}

std::shared_ptr<Node> GraphImplementation::nodeForIdentifier(
    const std::string &identifier) {
  return _nodes[identifier];
}

std::string GraphImplementation::type() const { return _type; }

std::shared_ptr<param::Param> GraphImplementation::parameterForPath(
    const std::string &path) {
  std::vector<std::string> major_split;
  boost::split(major_split, path, boost::is_any_of("/"));
  if (major_split.size() < NFSmartPlayerParamComponentIndex) {
    return nullptr;
  }
  const std::string &graph_identifier =
      major_split[NFSmartPlayerGraphComponentIndex];
  if (graph_identifier != identifier()) {
    return nullptr;
  }
  const std::string &node_identifier =
      major_split[NFSmartPlayerNodeComponentIndex];
  std::shared_ptr<Node> node = nodeForIdentifier(node_identifier);
  if (!node) {
    return nullptr;
  }
  const std::string &param_identifier =
      major_split[NFSmartPlayerParamComponentIndex];
  return node->paramForIdentifier(param_identifier);
}

bool GraphImplementation::loaded() const { return _loaded; }

bool GraphImplementation::finished() {
  std::lock_guard<std::mutex> lock(_root_plugin_mutex);
  long sample_index = _sample_index;
  bool ret = _root_plugin->finished(sample_index, sample_index);
  if (ret) {
    _root_plugin->notifyFinished(sample_index, sample_index);
  }
  return ret;
}

void GraphImplementation::traverse(
    std::map<std::string, std::shared_ptr<plugin::Content>> &content,
    nfgrapher::LoadingPolicy loading_policy) {
  if (!loaded()) {
    return;
  }

  std::lock_guard<std::mutex> lock(_root_plugin_mutex);
  long sample_index = _sample_index;
  long maximum_expected_render_samples_step =
      PlayerContentExpectedRenderSamples(content);
  long end_sample_index = sample_index + maximum_expected_render_samples_step;
  if (end_sample_index < 0) {
    // Fill any content buffers with silence
    for (const auto &content_type : content) {
      switch (content_type.second->type()) {
        case plugin::ContentPayloadTypeNone:
        case plugin::ContentPayloadTypeHandle:
          break;
        case plugin::ContentPayloadTypeBuffer:
          std::fill_n(content_type.second->payload(),
                      content_type.second->payloadSize(), 0);
          content_type.second->setItems(content_type.second->requiredItems());
          break;
      }
    }
  } else {
    if (sample_index < 0) {
      // Cut the fed in content buffer
      for (const auto &content_pair : content) {
        auto it = _cut_content.find(content_pair.first);
        if (it == _cut_content.end()) {
          const auto &content_instance = content_pair.second;
          auto new_content = std::make_shared<plugin::Content>(
              content_instance->payloadSize(), 0,
              content_instance->sampleRate(), content_instance->channels(),
              content_instance->requiredItems(), content_instance->type());
          auto result = _cut_content.emplace(content_pair.first, new_content);
          if (result.second) {
            it = result.first;
          }
        }
        (*it).second->setItems(0);
      }
      for (auto &content_type : _cut_content) {
        switch (content_type.second->type()) {
          case plugin::ContentPayloadTypeNone:
          case plugin::ContentPayloadTypeHandle:
            break;
          case plugin::ContentPayloadTypeBuffer: {
            _cut_content[content_type.first] =
                std::make_shared<plugin::Content>(
                    end_sample_index, 0, content_type.second->sampleRate(),
                    content_type.second->channels(), end_sample_index,
                    content_type.second->type());
            break;
          }
        }
      }
      {
        std::lock_guard<std::mutex> node_times_lock(_node_times_mutex);
        _root_plugin->feed(_cut_content, sample_index, sample_index,
                           loading_policy);
      }
      // Merge the cut buffer back into the original
      for (auto &content_type : _cut_content) {
        switch (content_type.second->type()) {
          case plugin::ContentPayloadTypeNone:
          case plugin::ContentPayloadTypeHandle:
            break;
          case plugin::ContentPayloadTypeBuffer: {
            size_t silence_samples =
                content[content_type.first]->requiredItems() -
                content_type.second->items();
            content[content_type.first]->setItems(silence_samples);
            std::fill_n(content[content_type.first]->payload(), silence_samples,
                        0);
            content[content_type.first]->append(*content_type.second);
            break;
          }
        }
      }
    } else {
      {
        std::lock_guard<std::mutex> node_times_lock(_node_times_mutex);
        _root_plugin->feed(content, sample_index, sample_index, loading_policy);
      }
    }
  }

  long samples_step = 0;
  if (loading_policy == nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH) {
    samples_step = maximum_expected_render_samples_step;
  }
  for (const auto &content_type : content) {
    const std::shared_ptr<plugin::Content> &content_type_struct =
        content_type.second;
    switch (loading_policy) {
      case nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH: {
        samples_step = std::min(static_cast<long>(content_type_struct->items()),
                                samples_step);
        break;
      }
      case nfgrapher::LoadingPolicy::SOME_CONTENT_PLAYTHROUGH: {
        samples_step = std::max(static_cast<long>(content_type_struct->items()),
                                samples_step);
        break;
      }
    }
  }

  _sample_index = sample_index + samples_step;
  _buffer_size = maximum_expected_render_samples_step;
}

void GraphImplementation::forEachNode(
    NF_SMART_PLAYER_GRAPH_NODE_CALLBACK node_callback) {
  std::lock_guard<std::mutex> lock(_nodes_mutex);
  for (const auto &node : _nodes) {
    if (!node_callback(node.second)) {
      return;
    }
  }
}

void GraphImplementation::forEachScript(
    NF_SMART_PLAYER_GRAPH_SCRIPT_CALLBACK script_callback) {
  std::lock_guard<std::mutex> lock(_scripts_mutex);
  for (const auto &script : _scripts) {
    if (!script_callback(script.second)) {
      return;
    }
  }
}

void GraphImplementation::setPluginRegistry(
    std::shared_ptr<plugin::Registry> plugin_registry) {
  _plugin_registry = plugin_registry;
}

void GraphImplementation::run() {
  bool graph_finished = finished();
  std::lock_guard<std::mutex> node_times_lock(_node_times_mutex);
  long sample_index = _sample_index;
  _root_plugin->run(sample_index, _node_times, 0);
  if (graph_finished != _finished) {
    if (graph_finished) {
      if (auto delegate = _delegate.lock()) {
        delegate->graphSendMessage(
            shared_from_this(), GraphMessageIdentifierEnd,
            NFSmartPlayerMessageTypeGeneric, identifier().c_str());
      }
    }
    _finished = graph_finished;
  }
}

void GraphImplementation::setDelegate(std::weak_ptr<GraphDelegate> delegate) {
  _delegate = delegate;
}

void GraphImplementation::setJson(const nlohmann::json &json,
                                  LOAD_CALLBACK load_callback) {
  nfgrapher::Score score;
  try {
    score = json;
  } catch (const nlohmann::json::exception &e) {
    std::string msg = "Invalid NFGrapher Score: ";
    msg += e.what();
    load_callback({false, GraphImplementationErrorDomain, msg});
    return;
  }

  const nfgrapher::Graph &graph = score.graph;
  if (!_plugin_registry && graph.nodes && graph.nodes->size() > 0) {
    load_callback(Load{false, GraphImplementationErrorDomain,
                       "Plugin registry not loaded"});
    return;
  }

  // load_callback will be called by loadScore
  loadScore(score, load_callback);
  return;
}

void GraphImplementation::loadScore(const nfgrapher::Score &score,
                                    LOAD_CALLBACK load_callback) {
  const nfgrapher::Graph &graph = score.graph;
  nfgrapher::LoadingPolicy loading_policy =
      graph.loading_policy ? *graph.loading_policy.get()
                           : nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH;
  const std::string identifier = graph.id;
  const auto scripts = loadScripts(score);

  // Begin loading the graph now...
  if (auto delegate = _delegate.lock()) {
    delegate->graphSendMessage(shared_from_this(),
                               "com.nativeformat.graph.beginload",
                               NFSmartPlayerMessageTypeNone, nullptr);
  }

  // Wrap the callback message to send the necessary messages
  auto load_callback_message = [load_callback, this](const Load &load) {
    if (auto delegate = _delegate.lock()) {
      if (load._loaded) {
        delegate->graphSendMessage(shared_from_this(),
                                   "com.nativeformat.graph.loaded",
                                   NFSmartPlayerMessageTypeNone, nullptr);
      } else {
        delegate->graphSendMessage(shared_from_this(),
                                   "com.nativeformat.graph.loaderror",
                                   NFSmartPlayerMessageTypeError, nullptr);
      }
    }
    load_callback(load);
  };

  bool failed = false;

  std::vector<std::shared_ptr<Node>> raw_nodes =
      loadNodes(score, load_callback_message, failed);
  if (failed) {
    load_callback_message(
        {false, GraphImplementationErrorDomain, "loadNodes failed"});
    return;
  }
  // Load all the plugins
  std::map<std::string, std::shared_ptr<PrioritisedNode>> nodes;
  for (auto &raw_node : raw_nodes) {
    std::shared_ptr<PrioritisedNode> node =
        std::make_shared<PrioritisedNode>(raw_node);
    if (!node || !node->node) {
      load_callback_message({false, GraphImplementationErrorDomain,
                             "creating PrioritisedNode failed"});
      return;
    }
    nodes[node->node->identifier()] = node;
  }

  EdgeMap edge_map;

  if (graph.edges) {
    for (auto &e : *graph.edges.get()) {
      std::shared_ptr<PrioritisedNode> source = nodes[e.source];

      if (!source) {
        std::string error_msg = "Invalid edge could not find source with id: ";
        error_msg += e.source;

        load_callback_message(
            {false, GraphImplementationErrorDomain, error_msg});
        return;
      }

      std::shared_ptr<PrioritisedNode> target = nodes[e.target];

      if (!target) {
        std::string error_msg = "Invalid edge could not find target with id: ";
        error_msg += e.target;

        load_callback_message(
            {false, GraphImplementationErrorDomain, error_msg});
        return;
      }

      std::string source_output = "audio";
      if (e.source_port.get()) source_output = *(e.source_port.get());
      std::string target_input = "audio";
      if (e.target_port.get()) target_input = *(e.target_port.get());

      edge_map[{e.source, e.target}].push_back({source_output, target_input});
      source->output.push_back(target);
      target->input.push_back(source);
    }
  }

  // Connect all loose ends to the output node
  std::shared_ptr<PrioritisedNode> output_node =
      std::make_shared<PrioritisedNode>(std::make_shared<NodeImplementation>(
          true, nfgrapher::LoadingPolicy::ALL_CONTENT_PLAYTHROUGH,
          GraphImplementationOutputDriverTarget, "driver"));

  for (auto &node : nodes) {
    if (node.second->output.empty()) {
      node.second->output.push_back(output_node);
      output_node->input.push_back(node.second);

      edge_map[{node.second->node->identifier(),
                output_node->node->identifier()}]
          .push_back({"audio", "audio"});
    }
  }

  // Update the graph
  {
    std::lock_guard<std::mutex> nodes_lock(_nodes_mutex);
    std::lock_guard<std::mutex> node_times_lock(_node_times_mutex);
    std::lock_guard<std::mutex> scripts_lock(_scripts_mutex);
    _loading_policy = loading_policy;
    _loaded = true;
    _identifier = identifier;
    _type = "TODO: remove this";
    _scripts = scripts;
    _node_times.clear();
    _nodes.clear();
    for (const auto &node : nodes) {
      _nodes[node.first] = node.second->node;
    }
    PrioritiseNodes(nodes, output_node);

    // Try to intialize plugins from node descriptions
    std::shared_ptr<plugin::Plugin> root_plugin;
    try {
      root_plugin = CreatePluginTraverse(
          output_node, edge_map, _identifier, _channels, _samplerate,
          _plugin_registry, output_node, _session_id);
    } catch (std::logic_error e) {  // out_of_range or invalid_argument
      std::string error_msg = "Failed to parse command values. Exception: ";
      error_msg += std::string(e.what());
      load_callback_message({false, GraphImplementationErrorDomain, error_msg});
      return;
    }
    auto strong_this = shared_from_this();
    root_plugin->load(
        [load_callback_message, strong_this, root_plugin](Load load) {
          {
            std::lock_guard<std::mutex> lock(strong_this->_root_plugin_mutex);
            strong_this->_root_plugin = root_plugin;
          }
          load_callback_message(load);
        });
  }
}

std::map<std::string, std::shared_ptr<Script>> GraphImplementation::loadScripts(
    const nfgrapher::Score &score) {
  const nfgrapher::Graph &graph = score.graph;

  std::lock_guard<std::mutex> scripts_lock(_scripts_mutex);
  std::map<std::string, std::shared_ptr<Script>> scripts;

  // Check if we should load scripts
  if (graph.scripts) {
    for (const auto &script : *graph.scripts.get()) {
      const std::string name = script.name;
      const std::string code = script.code;
      if (_scripts.find(name) != _scripts.end() &&
          _scripts[name]->code() == code) {
        continue;
      }
      if (auto delegate = _delegate.lock()) {
        scripts[name] = delegate->createScript(name, code);
      }
    }
  }

  return scripts;
}

std::vector<std::shared_ptr<Node>> GraphImplementation::loadNodes(
    const nfgrapher::Score &score, LOAD_CALLBACK load_callback, bool &failed) {
  std::vector<std::shared_ptr<Node>> nodes;
  std::set<std::string> node_ids;

  const nfgrapher::Graph &graph = score.graph;
  if (graph.nodes) {
    for (auto &gn : *graph.nodes.get()) {
      nodes.push_back(std::make_shared<NodeImplementation>(gn, false));
    }
  }
  return nodes;
}

static long PlayerContentExpectedRenderSamples(
    const std::map<std::string, std::shared_ptr<plugin::Content>> &content) {
  long maximum_expected_render_samples_step = 0;
  for (const auto &content_type : content) {
    const std::shared_ptr<plugin::Content> &content_type_struct =
        content_type.second;
    maximum_expected_render_samples_step =
        std::max(static_cast<long>(content_type_struct->requiredItems()),
                 maximum_expected_render_samples_step);
  }
  return maximum_expected_render_samples_step;
}

static void PrioritiseNodes(
    std::map<std::string, std::shared_ptr<PrioritisedNode>> &nodes,
    std::shared_ptr<PrioritisedNode> &output_node) {
  // Give all the nodes an initial priority of zero
  std::vector<std::string> node_priority_list;
  for (auto &node : nodes) {
    node.second->priority = 0;
    node_priority_list.push_back(node.first);
  }

  // Find the priority of each node
  std::map<std::string, int> dependency_count;
  for (auto &node : nodes) {
    std::vector<std::string> dependencies = node.second->node->dependencies();
    if (dependencies.empty()) {
      continue;
    }
    std::set<std::string> sub_dependencies;
    for (const auto &dependency : dependencies) {
      sub_dependencies.insert(dependency);
    }
    std::vector<std::string> current_dependencies = dependencies;
    while (!current_dependencies.empty()) {
      std::vector<std::string> loop_dependencies = current_dependencies;
      current_dependencies.clear();
      for (const auto &loop_dependency : loop_dependencies) {
        sub_dependencies.insert(loop_dependency);
        std::vector<std::string> plugin_dependencies =
            nodes[loop_dependency]->node->dependencies();
        current_dependencies.insert(current_dependencies.end(),
                                    plugin_dependencies.begin(),
                                    plugin_dependencies.end());
      }
    }
    dependency_count[node.first] = sub_dependencies.size();
  }

  // Reorder the priority list accordingly
  for (auto &node : nodes) {
    node.second->priority = dependency_count[node.first];
  }
  std::sort(node_priority_list.begin(), node_priority_list.end(),
            [&nodes](const std::string &node1, const std::string &node2) {
              return nodes[node1]->priority < nodes[node2]->priority;
            });

  // Traverse the graph to find compounded priorities
  NodePriorityTraversal(output_node);
}

static int NodePriorityTraversal(std::shared_ptr<PrioritisedNode> &node) {
  for (auto &input_node : node->input) {
    node->priority += NodePriorityTraversal(input_node);
  }

  return node->priority;
}

static std::shared_ptr<plugin::Plugin> CreatePluginTraverse(
    std::shared_ptr<PrioritisedNode> &node, const EdgeMap &edge_map,
    const std::string &graph_id, int channels, double samplerate,
    std::shared_ptr<plugin::Registry> &plugin_registry,
    std::shared_ptr<PrioritisedNode> &output_node,
    const std::string &session_id) {
  // Traverse the leaves in order
  std::sort(node->input.begin(), node->input.end(),
            [](const std::shared_ptr<PrioritisedNode> &node1,
               const std::shared_ptr<PrioritisedNode> &node2) {
              return node1->priority > node2->priority;
            });

  std::vector<plugin::MixerPlugin::Metadata> mixer_metadata;
  for (auto &source_node : node->input) {
    plugin::MixerPlugin::Metadata metadata;
    metadata._plugin = CreatePluginTraverse(
        source_node, edge_map, graph_id, channels, samplerate, plugin_registry,
        output_node, session_id);
    EdgeMap::key_type edge_key{std::string(source_node->node->identifier()),
                               std::string(node->node->identifier())};
    metadata._edges = edge_map.at(edge_key);
    mixer_metadata.push_back(metadata);
  }

  std::shared_ptr<plugin::Plugin> mixer_plugin =
      std::make_shared<plugin::MixerPlugin>(mixer_metadata, channels,
                                            samplerate);
  if (output_node == node) {
    node->node->setPlugin(mixer_plugin);
    return mixer_plugin;
  }
  auto plugin = plugin_registry->createPlugin(node->node->grapherNode(),
                                              graph_id, channels, samplerate,
                                              mixer_plugin, session_id);
  node->node->setPlugin(plugin);
  return plugin;
}

}  // namespace smartplayer
}  // namespace nativeformat
