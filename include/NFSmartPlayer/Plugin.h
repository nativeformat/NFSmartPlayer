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

#include <NFGrapher/NFGrapher.h>
#include <NFGrapher/param/ParamUtil.h>
#include <NFSmartPlayer/CallbackTypes.h>
#include <NFSmartPlayer/Content.h>

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace nativeformat {
namespace plugin {

#define AudioContentTypeKey "audio"

typedef struct {
  long start_sample_index;
  long end_sample_index;
  long local_start_sample_index;
  long local_end_sample_index;
} NodeTime;
typedef std::map<std::string, NodeTime> NodeTimes;

inline bool rangesOverlap(long begin1, long end1, long begin2, long end2) {
  if (begin1 == end1) {
    return begin1 >= begin2 && begin1 < end2;
  }
  if (begin2 == end2) {
    return begin2 >= begin1 && begin2 < end1;
  }
  long w1 = end1 - begin1;
  long w2 = end2 - begin2;
  long max = std::max(end1, end2);
  long min = std::min(begin1, begin2);
  return max - min < w1 + w2;
}

inline long rangeSampleIndexBegin(long big_range_begin, long big_range_end,
                                  long small_range_begin,
                                  long small_range_end) {
  if (!rangesOverlap(big_range_begin, big_range_end, small_range_begin,
                     small_range_end)) {
    return 0;
  }
  return small_range_begin < big_range_begin
             ? big_range_begin - small_range_begin
             : small_range_begin - big_range_begin;
}

inline long rangeSampleIndexEnd(long big_range_begin, long big_range_end,
                                long small_range_begin, long small_range_end) {
  if (!rangesOverlap(big_range_begin, big_range_end, small_range_begin,
                     small_range_end)) {
    return 0;
  }
  if (small_range_begin == small_range_end) {
    return small_range_end;
  }
  return small_range_end > big_range_end ? big_range_end - small_range_begin
                                         : small_range_end - small_range_begin;
}

inline bool findBeginAndEndOfRange(long big_range_begin, long big_range_end,
                                   long small_range_begin, long small_range_end,
                                   long &begin_index, long &end_index) {
  if (!rangesOverlap(big_range_begin, big_range_end, small_range_begin,
                     small_range_end)) {
    return false;
  }
  begin_index = rangeSampleIndexBegin(big_range_begin, big_range_end,
                                      small_range_begin, small_range_end);
  end_index = rangeSampleIndexEnd(big_range_begin, big_range_end,
                                  small_range_begin, small_range_end);
  return true;
}

inline long clipIntervleavedSamplesToChannel(long sample_count, int channels) {
  return sample_count - (sample_count % channels);
}

inline long nanosToFrames(double samplerate, long ns) {
  return ns * 1.0e-9 * samplerate;
}

/**
 * This is the generic interface to a plugin for the smart player
 */
class Plugin {
 public:
  typedef enum : int {
    PluginTypeProducer = 1u << 0,
    PluginTypeConsumer = 1u << 1,
    PluginTypeProducerConsumer = PluginTypeProducer | PluginTypeConsumer
  } PluginType;

  /**
   * The routine executed when the player needs this plugin to manipulate
   * samples
   */
  virtual void feed(std::map<std::string, std::shared_ptr<Content>> &content,
                    long sample_index, long graph_sample_index,
                    nfgrapher::LoadingPolicy loading_policy) = 0;
  /**
   * This is called before the player begins rendering, the plugin must call the
   * callback to let the player know it is ready to load
   * @param callback A function to call when the plugin has finished loading
   */
  virtual void load(LOAD_CALLBACK callback) = 0;
  /**
   * Whether the plugin has loaded
   */
  virtual bool loaded() const = 0;
  /**
   * Called when the client may change the render time
   */
  virtual void potentialTimeChange(long sample_index, long graph_sample_index) {
  }
  /**
   * Called when the client changes the render time explicitly
   */
  virtual void majorTimeChange(long sample_index, long graph_sample_index) {}
  /**
   * The name of this plugin
   */
  virtual std::string name() = 0;
  /**
   * The routine executed when the player is periodically telling the plugins to
   * perform any operations they otherwise wouldn't run in the feed routine.
   * Good examples of this are fetching more content.
   */
  virtual void run(long sample_index, const NodeTimes &node_times,
                   long node_sample_index) = 0;
  /**
   * Fetch the names of the parameters
   */
  virtual std::vector<std::string> paramNames() { return {}; }
  /**
   * Fetch the audio param corresponding to a variable name
   * @param name The name of the param to fetch
   * @discussion This will result in a shared_ptr with a nullptr if nothing is
   * found, make sure to check for it.
   */
  virtual std::shared_ptr<param::Param> paramForName(const std::string &name) {
    return nullptr;
  }
  /**
   * Whether the player should proceed to process any children of the plugin
   */
  virtual bool shouldProcessChildren(long sample_index, long sample_index_end,
                                     const NodeTimes &node_times) {
    return type() != PluginTypeProducer;
  }
  /**
   * Allows time dilation for the nodes below the current node
   */
  virtual long timeDilation(long sample_index) { return sample_index; }
  /**
   * Whether the plugin has finished processing at a given time.
   */
  virtual bool finished(long sample_index, long sample_index_end) = 0;
  /**
   * Tells the plugin the final graph sample index once prcessing is finished
   */
  virtual void notifyFinished(long sample_index, long graph_sample_index) {}
  /**
   * The plugins producer/consumer type.
   */
  virtual PluginType type() const = 0;
  /**
   * The plugins local render time
   */
  virtual long localRenderSampleIndex(long sample_index) { return 0; }
  /**
   * Tells when the node time starts
   */
  virtual long startSampleIndex() { return 0; }
  virtual bool shouldProcess(long sample_index_start,
                             long sample_index_end) = 0;
};

}  // namespace plugin
}  // namespace nativeformat
