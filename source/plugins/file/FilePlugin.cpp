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
#include "FilePlugin.h"

#include <stdexcept>

namespace nativeformat {
namespace plugin {
namespace file {

static const double FILE_PLUGIN_CACHE_TIME = 10.0;
static const long INVALID_DURATION_FRAMES = -1;

FilePlugin::FilePlugin(const std::shared_ptr<decoder::Factory> &factory,
                       const nfgrapher::contract::FileNodeInfo &grapher_node,
                       const std::string &name, int channels, double samplerate)
    : _factory(factory),
      _name(name),
      _channels(channels),
      _samplerate(samplerate),
      _path(grapher_node._file),
      _start_time_frame_index(nanosToFrames(samplerate, grapher_node._when)),
      _duration_frames(nanosToFrames(samplerate, grapher_node._duration)),
      _track_start_frame_index(nanosToFrames(samplerate, grapher_node._offset)),
      _loaded(false),
      _initialised(false),
      _cached_samples_start_frame_index(0),
      _initialising(false),
      _decoding(false) {}

FilePlugin::~FilePlugin() {}

void FilePlugin::feed(std::map<std::string, std::shared_ptr<Content>> &content,
                      long sample_index, long graph_sample_index,
                      nfgrapher::LoadingPolicy loading_policy) {
  Content &audio_content = *content[AudioContentTypeKey];
  size_t sample_count = audio_content.requiredItems();
  auto channels = audio_content.channels();
  long frame_index = sample_index / audio_content.channels();
  long frames = sample_count / audio_content.channels();
  long end_frame_index = frame_index + frames;
  long start_time_frame_index = _start_time_frame_index;
  long duration_frames = _duration_frames;
  long end_time_frame_index = start_time_frame_index + duration_frames;
  long frame_begin = 0;
  long frame_end = 0;
  if (!findBeginAndEndOfRange(frame_index, end_frame_index,
                              start_time_frame_index, end_time_frame_index,
                              frame_begin, frame_end)) {
    return;
  }
  frame_begin += start_time_frame_index;
  frame_end += start_time_frame_index;
  long output_samples = (frame_begin - frame_index) * channels;
  audio_content.setItems(output_samples);
  {
    std::lock_guard<std::mutex> cached_samples_lock(_cached_samples_mutex);
    long cached_samples_start_frame_index = _cached_samples_start_frame_index;
    long cached_samples_duration_frames = _cached_samples.size() / channels;
    if (cached_samples_duration_frames == 0) {
      return;
    }
    long cached_samples_end_frame_index =
        cached_samples_start_frame_index + cached_samples_duration_frames;
    long track_start_frame_index = _track_start_frame_index;
    long current_relative_frame_index_begin =
        frame_begin - start_time_frame_index + track_start_frame_index;
    long current_relative_frame_index_end =
        frame_end - start_time_frame_index + track_start_frame_index;
    long current_relative_frame_begin = 0;
    long current_relative_frame_end = 0;
    if (!findBeginAndEndOfRange(
            current_relative_frame_index_begin,
            current_relative_frame_index_end, cached_samples_start_frame_index,
            cached_samples_end_frame_index, current_relative_frame_begin,
            current_relative_frame_end)) {
      return;
    }
    float *samples = (float *)audio_content.payload();
    long copy_samples =
        (current_relative_frame_end - current_relative_frame_begin) * channels;
    if (copy_samples <= 0) {
      return;
    }
    std::copy_n(&_cached_samples[current_relative_frame_begin * channels],
                copy_samples, &samples[output_samples]);

    output_samples += copy_samples;
  }
  audio_content.setItems(output_samples);
}

void FilePlugin::load(LOAD_CALLBACK callback) {
  if (_start_time_frame_index / _samplerate < FILE_PLUGIN_CACHE_TIME) {
    initialise(callback);
  } else {
    _loaded = true;
    callback(Load{true});
  }
}

bool FilePlugin::loaded() const { return _loaded; }

std::string FilePlugin::name() { return _name; }

std::string FilePlugin::path() { return _path; }

void FilePlugin::run(long sample_index, const NodeTimes &node_times,
                     long node_sample_index) {
  long frame_index = sample_index / _channels;
  long start_time_frame_index = _start_time_frame_index;
  long duration_frames = _duration_frames;
  long end_time_frame_index = start_time_frame_index + duration_frames;
  if (frame_index < start_time_frame_index) {
    if ((start_time_frame_index - frame_index) / _samplerate <
        FILE_PLUGIN_CACHE_TIME) {
      initialise(nullptr);
    }
  } else if (frame_index > end_time_frame_index) {
    if ((frame_index - end_time_frame_index) / _samplerate >
        FILE_PLUGIN_CACHE_TIME) {
      // De-initialise our plugin
      std::lock_guard<std::mutex> lock(_decoder_mutex);
      _initialised = false;
      _initialising = false;
      _decoder = nullptr;
    }
  } else {
    // If we aren't initialised yet we definitely need to initialise now
    long relative_frame_index =
        frame_index - start_time_frame_index + _track_start_frame_index;
    if (!_initialised) {
      initialise(nullptr);
    } else {
      long cached_samples_frames = 0;
      {
        std::lock_guard<std::mutex> cached_samples_lock(_cached_samples_mutex);
        cached_samples_frames = _cached_samples.size() / _channels;
      }
      long cached_samples_start_frame_index = _cached_samples_start_frame_index;
      long cached_samples_end_frame_index =
          cached_samples_start_frame_index + cached_samples_frames;
      long cached_frames_required = (FILE_PLUGIN_CACHE_TIME * _samplerate) / 2;
      if (!cached_samples_frames ||
          !rangesOverlap(cached_samples_start_frame_index,
                         cached_samples_end_frame_index - 1,
                         relative_frame_index, relative_frame_index)) {
        long frames_required =
            relative_frame_index < cached_samples_start_frame_index
                ? std::min(
                      cached_frames_required,
                      cached_samples_start_frame_index - relative_frame_index)
                : std::min(
                      cached_frames_required,
                      duration_frames - (frame_index - start_time_frame_index));
        decode(relative_frame_index, frames_required, nullptr);
      } else {
        long frames_required =
            std::min(cached_frames_required,
                     duration_frames - (frame_index - start_time_frame_index));
        if (frames_required < 0) {
          return;
        }
        long frame_begin = 0, frame_end = 0;
        findBeginAndEndOfRange(relative_frame_index, relative_frame_index,
                               cached_samples_start_frame_index,
                               cached_samples_end_frame_index, frame_begin,
                               frame_end);
        long cached_samples_frames_left = cached_samples_frames - frame_end;
        if (cached_samples_frames_left < frames_required) {
          // Let's get some more samples
          decode(cached_samples_end_frame_index, frames_required, nullptr);
        } else {
          // Let's see if we can trim some samples
          long trimmable_frames =
              frame_begin - cached_samples_start_frame_index;
          if (trimmable_frames > cached_frames_required) {
            // Erase more than we are required to (to prevent reallocations
            // every time run is called)
            std::lock_guard<std::mutex> cached_samples_lock(
                _cached_samples_mutex);
            _cached_samples.erase(
                _cached_samples.begin(),
                _cached_samples.begin() + (cached_frames_required * 2));
            _cached_samples_start_frame_index =
                cached_samples_start_frame_index + cached_frames_required;
          }
        }
      }
    }
  }
}

bool FilePlugin::finished(long sample_index, long sample_index_end) {
  return localRenderSampleIndex(sample_index) ==
         ((_track_start_frame_index + _duration_frames) * _channels);
}

Plugin::PluginType FilePlugin::type() const { return PluginTypeProducer; }

long FilePlugin::localRenderSampleIndex(long sample_index) {
  long frame_index = sample_index / _channels;
  long start_time_frame_index = _start_time_frame_index;
  if (frame_index < start_time_frame_index) {
    return 0;
  }
  long duration_frames = _duration_frames;
  long end_time_frame_index = start_time_frame_index + duration_frames;
  long track_start_frame_index = _track_start_frame_index;
  if (frame_index > end_time_frame_index) {
    return (duration_frames + track_start_frame_index) * _channels;
  }
  return (frame_index - start_time_frame_index + track_start_frame_index) *
         _channels;
}

long FilePlugin::startSampleIndex() {
  return _start_time_frame_index * _channels;
}

bool FilePlugin::shouldProcess(long sample_index_start, long sample_index_end) {
  long start_time_frame_index = _start_time_frame_index;
  long duration_frames = _duration_frames;
  long end_time_frame_index = start_time_frame_index + duration_frames;
  long frame_index_start = sample_index_start / _channels;
  if (end_time_frame_index == frame_index_start) {
    return false;
  }
  if (frame_index_start >= start_time_frame_index &&
      !finished(sample_index_start, sample_index_end)) {
    return true;
  }
  long frame_index_end =
      frame_index_start + ((sample_index_end - sample_index_start) / _channels);
  if (frame_index_end >= start_time_frame_index &&
      !finished(sample_index_start, sample_index_end)) {
    return true;
  }
  return false;
}

void FilePlugin::initialise(LOAD_CALLBACK callback) {
  if (_initialising || _initialised) {
    return;
  }
  _initialising = true;

  auto strong_this = shared_from_this();
  _factory->createDecoder(
      _path, "",
      [callback,
       strong_this](const std::shared_ptr<decoder::Decoder> &decoder) {
        if (!decoder) {
          return;
        }
        {
          std::lock_guard<std::mutex> lock(strong_this->_decoder_mutex);
          strong_this->_decoder = decoder;
        }
        long duration_frames = strong_this->_duration_frames;
        if (duration_frames == INVALID_DURATION_FRAMES) {
          duration_frames = decoder->frames();
        }
        long frames = std::min(
            static_cast<long>(FILE_PLUGIN_CACHE_TIME * decoder->sampleRate()),
            duration_frames);
        strong_this->decode(strong_this->_track_start_frame_index, frames,
                            [callback, strong_this](const Load &load) {
                              strong_this->_loaded = true;
                              strong_this->_initialised = true;
                              strong_this->_initialising = false;
                              if (callback) {
                                callback(Load{true});
                              }
                            });
      },
      [callback, strong_this](const std::string &domain, int error_code) {
        strong_this->_loaded = false;
        strong_this->_initialising = false;
        std::string error_message =
            "Failed to create decoder for " + strong_this->_path;
        if (callback) {
          callback(Load{false, domain, error_message});
        } else {
          throw std::runtime_error(error_message);
        }
      });
}

void FilePlugin::decode(long frame_index, long frames, LOAD_CALLBACK callback) {
  if (_decoding) {
    return;
  }

  _decoding = true;
  std::lock_guard<std::mutex> lock(_decoder_mutex);
  long decoder_frame_index = _decoder->currentFrameIndex();
  if (decoder_frame_index != frame_index) {
    _decoder->seek(frame_index);
  }
  auto strong_this = shared_from_this();
  auto decoder = _decoder;
  decoder->decode(frames, [callback, strong_this, decoder](
                              long frame_index, long frames, float *samples) {
    // Load samples here
    {
      std::lock_guard<std::mutex> cached_samples_lock(
          strong_this->_cached_samples_mutex);
      long cached_samples_start_frame_index =
          strong_this->_cached_samples_start_frame_index;
      long cached_samples_end_frame_index =
          cached_samples_start_frame_index +
          (strong_this->_cached_samples.size() / strong_this->_channels);
      long end_frame_index = frame_index + frames;
      long frame_begin = 0;
      long frame_end = 0;
      if (!findBeginAndEndOfRange(
              frame_index, end_frame_index, cached_samples_start_frame_index,
              cached_samples_end_frame_index, frame_begin, frame_end)) {
        if (cached_samples_end_frame_index == frame_index) {
          // If we are at the current end of the sample cache then just add what
          // we have onto it
          strong_this->_cached_samples.insert(
              strong_this->_cached_samples.end(), samples,
              samples + (frames * strong_this->_channels));
        } else {
          // We are at a completely different part of the track, and we just
          // need to wipe the cached samples
          strong_this->_cached_samples.clear();
          strong_this->_cached_samples.insert(
              strong_this->_cached_samples.begin(), samples,
              samples + (frames * strong_this->_channels));
          strong_this->_cached_samples_start_frame_index = frame_index;
        }
      } else {
        // We are in a range, so we may have to overwrite some data
        long frame_copy_begin_index = frame_begin;
        long frames_to_copy = frame_end - frame_begin;
        if (frames_to_copy > 0) {
          auto n_channels = strong_this->_channels;
          std::copy_n(&samples[frame_copy_begin_index * n_channels],
                      frames_to_copy * n_channels,
                      &strong_this->_cached_samples[frame_begin * n_channels]);
        }
        if (frames_to_copy != frames) {
          long frames_to_insert = frames - frames_to_copy;
          if (frame_end == cached_samples_end_frame_index) {
            // Append the rest to the end
            long frames_insertion_index = frames - frames_to_insert;
            strong_this->_cached_samples.insert(
                strong_this->_cached_samples.end(),
                &samples[frames_insertion_index * strong_this->_channels],
                &samples[frames_insertion_index * strong_this->_channels] +
                    (frames_to_insert * strong_this->_channels));
          } else {
            // Append the rest to the beginning
            strong_this->_cached_samples.insert(
                strong_this->_cached_samples.begin(), samples,
                samples + (frames_to_insert * strong_this->_channels));
            strong_this->_cached_samples_start_frame_index = frame_index;
          }
        }
      }
      if (decoder->eof()) {
        long silence_frames =
            strong_this->_duration_frames -
            (decoder->frames() - strong_this->_track_start_frame_index);
        if (silence_frames > 0) {
          strong_this->_cached_samples.insert(
              strong_this->_cached_samples.end(),
              silence_frames * decoder->channels(), 0.0f);
        }
      }
    }
    if (callback) {
      callback(Load{true});
    }
    strong_this->_decoding = false;
  });
}

}  // namespace file
}  // namespace plugin
}  // namespace nativeformat
