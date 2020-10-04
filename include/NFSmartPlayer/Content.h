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

#include <algorithm>
#include <cstdlib>
#include <functional>
#include <vector>

namespace nativeformat {
namespace smartplayer {
class GraphImplementation;
class Player;
}  // namespace smartplayer
namespace plugin {

typedef enum : int {
  ContentPayloadTypeBuffer,
  ContentPayloadTypeHandle,
  ContentPayloadTypeNone
} ContentPayloadType;

class Content {
  friend class smartplayer::GraphImplementation;
  friend class smartplayer::Player;

 public:
  Content()
      : _payload(),
        _items(0),
        _sample_rate(0.0),
        _channels(0),
        _required_items(0),
        _payload_type(ContentPayloadTypeNone) {}
  Content(size_t payload_size, size_t items, double sample_rate,
          size_t channels, size_t required_items,
          ContentPayloadType payload_type)
      : _payload(payload_size),
        _items(items),
        _sample_rate(sample_rate),
        _channels(channels),
        _required_items(required_items),
        _payload_type(payload_type) {}

  float *payload() const { return const_cast<float *>(_payload.data()); }
  size_t channels() const { return _channels; }
  size_t requiredItems() const { return _required_items; }
  double sampleRate() const { return _sample_rate; }
  void setItems(size_t items) { _items = std::min(items, _required_items); }
  size_t items() const { return _items; }
  ContentPayloadType type() const { return _payload_type; }
  size_t payloadSize() const { return _payload.size(); }

  void erase() {
    _items = 0;
    std::fill(_payload.begin(), _payload.end(), 0);
  }

  Content &operator=(const Content &rhs) {
    if (&rhs == this) {
      return *this;
    }
    _payload = rhs._payload;
    _items = rhs._items;
    _sample_rate = rhs._sample_rate;
    _channels = rhs._channels;
    _required_items = rhs._required_items;
    _payload_type = rhs._payload_type;
    return *this;
  }

  size_t mix(const Content &content) {
    if (_payload_type != content.type() ||
        _payload_type != ContentPayloadTypeBuffer) {
      return 0;
    }
    size_t items_to_mix = std::min(_items, content.items());
    for (size_t i = 0; i < items_to_mix; ++i) {
      _payload[i] += content._payload[i];
    }
    return items_to_mix;
  }

  size_t append(const Content &content) {
    if (_payload_type != content.type() ||
        _payload_type != ContentPayloadTypeBuffer) {
      return 0;
    }
    size_t items_to_append =
        std::min(_items + content.items(), _required_items) - _items;
    _payload.resize(_payload.size() + items_to_append);
    std::copy_n(&content._payload[0], items_to_append, &_payload[_items]);
    _items += items_to_append;
    return items_to_append;
  }

  // resize content preserving data
  void resize(const size_t &new_size) {
    if (new_size > _payload.size()) {
      _payload.resize(new_size);
    }
  }

 protected:
  std::vector<float> _payload;
  size_t _items;
  double _sample_rate;
  size_t _channels;
  size_t _required_items;
  ContentPayloadType _payload_type;
};

}  // namespace plugin
}  // namespace nativeformat
