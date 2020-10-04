/*
 * Copyright (c) 2016 Spotify AB.
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
#include "StretchPlugin.h"

#include <NFDriver/NFDriver.h>

#include <cmath>

namespace nativeformat {
namespace plugin {
namespace time {

StretchPlugin::StretchPlugin(
    const nfgrapher::contract::StretchNodeInfo &stretch_node, int channels,
    double samplerate, const std::shared_ptr<plugin::Plugin> &child_plugin)
    : _channels(channels),
      _samplerate(samplerate),
      _child_plugin(child_plugin),
      _elastique(nullptr),
      _input_buffers(nullptr),
      _buffers_size(4096),
      _expected_feed_sample(0),
      _input_frame_offset(0),
      _stretch(param::createParam(stretch_node._stretch.getInitialVal(), 100.0f,
                                  0.0f, "stretch")),
      _pitch(param::createParam(stretch_node._pitch_ratio.getInitialVal(),
                                100.0f, 0.0f, "pitchRatio")),
      _formants(param::createParam(stretch_node._formant_ratio.getInitialVal(),
                                   100.0f, 0.0f, "formantRatio")),
      _previous_stretch_value(stretch_node._stretch.getInitialVal()),
      _previous_pitch_value(stretch_node._pitch_ratio.getInitialVal()),
      _previous_formant_value(stretch_node._formant_ratio.getInitialVal()),
      _prefilled(false),
      _frames_to_discard(0),
      _frames_to_preproc(0),
      _child_sample_offset(0),
      _child_sample_index(0) {
  nfgrapher::param::addCommands(_stretch, stretch_node._stretch);
  nfgrapher::param::addCommands(_pitch, stretch_node._pitch_ratio);
  nfgrapher::param::addCommands(_formants, stretch_node._formant_ratio);

  _input_buffers = (float **)malloc(sizeof(float *) * _channels);
  _output_buffers = (float **)malloc(sizeof(float *) * _channels);
  for (size_t i = 0; i < _channels; ++i) {
    _input_buffers[i] = (float *)malloc(sizeof(float) * _buffers_size);
    _output_buffers[i] = (float *)malloc(sizeof(float) * _buffers_size);
    memset(_input_buffers[i], 0, sizeof(float) * _buffers_size);
    memset(_output_buffers[i], 0, sizeof(float) * _buffers_size);
  }
  createElastique();
}

StretchPlugin::~StretchPlugin() {
  destroyElastique();
  for (size_t i = 0; i < _channels; ++i) {
    free(_input_buffers[i]);
    free(_output_buffers[i]);
  }
  free(_input_buffers);
  free(_output_buffers);
}

void StretchPlugin::feed(
    std::map<std::string, std::shared_ptr<Content>> &content, long sample_index,
    long graph_sample_index, nfgrapher::LoadingPolicy loading_policy) {
  std::lock_guard<std::mutex> residual_buffer_lock(_residual_buffer_mutex);
  auto &audio_content = content[AudioContentTypeKey];
  auto required_items = audio_content->requiredItems();
  size_t filled_items = 0;

  // Check if this feed sample_index overlaps with the previous call
  if (sample_index < _expected_feed_sample) {
    size_t previous_sample_count = _previous_output.size();
    if (sample_index < _expected_feed_sample - previous_sample_count) {
      // TODO reset stretch plugin state without actually calling
      // majorTimeChange
      majorTimeChange(sample_index, graph_sample_index);
      audio_content->setItems(0);
      return;
    }
    size_t old_samples_needed = _expected_feed_sample - sample_index;
    size_t old_sample_offset = previous_sample_count - old_samples_needed;
    float *previous_output_ptr = _previous_output.data() + old_sample_offset;
    std::copy_n(previous_output_ptr, old_samples_needed,
                audio_content->payload());
    audio_content->setItems(old_samples_needed);
    if (old_samples_needed == required_items) {
      return;
    }
  }

  // Copy any leftover output from previous feeds
  if (!_residual_buffer.empty()) {
    filled_items = copyResidualFrames(*audio_content);
  }

  // If we already have enough output samples, we're done
  auto still_required_items = required_items - filled_items;
  if (!still_required_items) {
    saveOutput(sample_index, *audio_content);
    return;
  }

  // Reset audio parameters if needed
  double time = (sample_index / _samplerate) / static_cast<double>(_channels);
  double end_time =
      time +
      ((static_cast<float>(audio_content->requiredItems()) / _samplerate) /
       static_cast<double>(_channels));
  float stretch = _stretch->smoothedValueForTimeRange(time, end_time);
  float pitch_ratio = _pitch->smoothedValueForTimeRange(time, end_time);
  float formant_ratio = _formants->smoothedValueForTimeRange(time, end_time);

  if (!almostEqual(stretch, _previous_stretch_value) ||
      !almostEqual(pitch_ratio, _previous_pitch_value) ||
      !almostEqual(formant_ratio, _previous_formant_value)) {
    _previous_stretch_value = stretch;
    _previous_pitch_value = pitch_ratio;
    _previous_formant_value = formant_ratio;
    _elastique->SetStretchPitchQFactor(stretch, pitch_ratio);
    _elastique->SetEnvelopeFactor(formant_ratio);
  }

  // Check our content is correct, resize payload if needed
  size_t maximum_samples = resizeContent(content);

  // Check when the child plugin should start. If it's
  // within this buffer, insert some zeros
  auto child_sample_start_index = _child_plugin->startSampleIndex();
  if (sample_index < child_sample_start_index) {
    if (sample_index + required_items < child_sample_start_index) {
      return;
    }
    size_t zero_samples = child_sample_start_index - sample_index;
    std::fill_n(audio_content->payload(), zero_samples, 0);
    audio_content->setItems(zero_samples);
  }

  // Fetch data from the child plugin
  auto dilation = child_sample_start_index + _child_sample_index;
  if (_child_plugin->shouldProcess(dilation, dilation + maximum_samples)) {
    _child_plugin->feed(_content, dilation, graph_sample_index, loading_policy);
    auto &child_audio_content = _content[AudioContentTypeKey];
    auto child_audio_content_items = child_audio_content->items();
    float *child_audio_samples = (float *)child_audio_content->payload();

    // Update _child_sample_index
    _child_sample_index = _child_sample_index + child_audio_content_items;
    // If we haven't finished prefilling, hand the audio content to prefill
    if (!_prefilled) {
      bool finished = prefill(*child_audio_content, *audio_content);
      if (finished) {
        _prefilled = true;
      } else {
        return;
      }
    } else {
      _child_sample_offset = 0;
    }

    // Loop over the child plugin's audio
    // This only requires multiple iterations if frames exceed _buffers_size
    int i = 0;
    float *content_samples = (float *)audio_content->payload();
    do {
      auto available_child_frames =
          (child_audio_content_items - _child_sample_offset) / _channels;
      auto input_frames =
          std::min(available_child_frames - (_buffers_size * i), _buffers_size);

      if (!input_frames) {
        break;
      }
      // Perform the time stretch using Elastique's requested frame count
      size_t frames_processed = 0, samples_processed = 0;
      while (frames_processed < input_frames) {
        int frames_to_process = _elastique->GetFramesNeeded();

        // Make sure we have enough frames leftover for Elastique
        // unless we're at the end of the file
        dilation = child_sample_start_index + _child_sample_index;
        long input_frame_offset = _input_frame_offset;
        long frames_available = input_frames - frames_processed;
        bool filled = fillInput(frames_to_process, frames_available,
                                child_audio_samples + samples_processed);

        // Only count child samples that are newly inserted into input buffer as
        // "processed"
        long new_frames_consumed = filled
                                       ? frames_to_process - input_frame_offset
                                       : (long)_input_frame_offset;
        frames_processed += new_frames_consumed;
        samples_processed = frames_processed * _channels;

        if (!filled && !_child_plugin->finished(dilation, dilation)) {
          ++i;
          break;
        }

        _elastique->ProcessData(_input_buffers, frames_to_process);
        int process_calls = _elastique->GetNumOfProcessCalls();
        for (int j = 0; j < process_calls; ++j) {
          _elastique->ProcessData();
        }

        // Get the output of the time stretch
        auto output_frames = _elastique->GetProcessedData(_output_buffers);
        filled_items = copyProcessedFrames(*audio_content, output_frames);
      }
      ++i;
    } while (i < child_audio_content_items / _channels / _buffers_size);
    audio_content->setItems(filled_items);

    // If child plugin is finished, flush elastique buffers
    auto current_child_plugin_sample_index =
        child_sample_start_index + _child_sample_index;
    if (_child_plugin->finished(current_child_plugin_sample_index,
                                current_child_plugin_sample_index)) {
      int output_frames = 0;
      while ((output_frames = _elastique->FlushBuffer(_output_buffers)) > 0) {
        auto residual_buffer_size = _residual_buffer.size();
        still_required_items = required_items - filled_items;
        auto still_required_frames = still_required_items / _channels;
        auto output_samples = output_frames * _channels;

        // Deal with however many output_samples we have at this stage,
        // regardless of still_required_items
        if (output_samples > still_required_items) {
          _residual_buffer.resize(
              residual_buffer_size + output_samples - still_required_items,
              0.0f);
        }
        auto residual_buffer_pointer = (float *)_residual_buffer.data();
        for (long frame = 0; frame < output_frames; ++frame) {
          for (int channel = 0; channel < _channels; ++channel) {
            if (frame < still_required_frames) {
              content_samples[filled_items + (frame * _channels) + channel] =
                  _output_buffers[channel][frame];
            } else {
              size_t residual_buffer_index =
                  residual_buffer_size +
                  ((frame - still_required_frames) * _channels) + channel;
              residual_buffer_pointer[residual_buffer_index] =
                  _output_buffers[channel][frame];
            }
          }
        }
        filled_items =
            std::min(required_items, filled_items + output_frames * _channels);
      }
    }
    audio_content->setItems(filled_items);
  }
  saveOutput(sample_index, *audio_content);
}

void StretchPlugin::majorTimeChange(long sample_index,
                                    long graph_sample_index) {
  std::lock_guard<std::mutex> residual_buffer_lock(_residual_buffer_mutex);
  auto start_sample_index = _child_plugin->startSampleIndex();
  _content.clear();
  _previous_output.clear();
  _expected_feed_sample = sample_index;

  destroyElastique();
  createElastique();

  if (sample_index < start_sample_index) {
    _residual_buffer.clear();

    // TODO make intitial values static in grapher cpp, then use here
    _previous_pitch_value = 1.0f;
    _previous_stretch_value = 1.0f;
    _previous_formant_value = 1.0f;
    _child_sample_index = 0;
  } else {
    auto child_start_sample_index = _child_plugin->startSampleIndex();
    auto start_time = ((child_start_sample_index / _channels) / _samplerate);
    auto end_time = ((sample_index / _channels) / _samplerate);
    auto render_time_diff = end_time - start_time;
    auto precision = (NF_DRIVER_SAMPLE_BLOCK_SIZE / _channels) / _samplerate;
    auto stretch_factor =
        render_time_diff /
        _stretch->cumulativeValueForTimeRange(start_time, end_time, precision);
    auto relative_time = stretch_factor * render_time_diff;
    auto dilated_samples = relative_time * _samplerate * _channels;
    _child_sample_index =
        clipIntervleavedSamplesToChannel(dilated_samples, _channels);
  }
}

std::string StretchPlugin::name() {
  return nfgrapher::contract::StretchNodeInfo::kind();
}

long StretchPlugin::timeDilation(long sample_index) {
  // Ignore the passed sample index and use _child_sample_index
  // due to the delay in elastique processing
  long child_start_sample_index = _child_plugin->startSampleIndex();
  if (child_start_sample_index >= sample_index) {
    return sample_index;
  }
  return child_start_sample_index + _child_sample_index;
}

std::vector<std::string> StretchPlugin::paramNames() {
  return {_stretch->name(), _pitch->name()};
}

std::shared_ptr<param::Param> StretchPlugin::paramForName(
    const std::string &name) {
  if (name == _stretch->name()) {
    return _stretch;
  } else if (name == _pitch->name()) {
    return _pitch;
  }
  return nullptr;
}

bool StretchPlugin::finished(long sample_index, long sample_index_end) {
  std::lock_guard<std::mutex> residual_buffer_lock(_residual_buffer_mutex);
  auto dilated_start_sample = timeDilation(sample_index);
  auto dilated_end_sample = timeDilation(sample_index_end);
  auto child_finished =
      _child_plugin->finished(dilated_start_sample, dilated_end_sample);
  auto residual_buffer_empty = _residual_buffer.empty();
  return child_finished && residual_buffer_empty;
}

void StretchPlugin::notifyFinished(long sample_index, long graph_sample_index) {
  return _child_plugin->notifyFinished(timeDilation(sample_index),
                                       graph_sample_index);
}

Plugin::PluginType StretchPlugin::type() const {
  return Plugin::PluginTypeProducerConsumer;
}

void StretchPlugin::run(long sample_index, const NodeTimes &node_times,
                        long node_sample_index) {
  auto dilated_sample_index = timeDilation(sample_index);
  _child_plugin->run(dilated_sample_index, node_times, node_sample_index);
}

void StretchPlugin::load(LOAD_CALLBACK callback) {
  _child_plugin->load(callback);
}

bool StretchPlugin::loaded() const { return _child_plugin->loaded(); }

bool StretchPlugin::shouldProcess(long sample_index_start,
                                  long sample_index_end) {
  std::lock_guard<std::mutex> residual_buffer_lock(_residual_buffer_mutex);
  auto start_sample_index = _child_plugin->startSampleIndex();
  if (sample_index_end < start_sample_index) {
    return false;
  }
  auto sample_difference = sample_index_end - sample_index_start;
  auto child_sample_index = start_sample_index + _child_sample_index;
  return _child_plugin->shouldProcess(child_sample_index,
                                      child_sample_index + sample_difference) ||
         !_residual_buffer.empty();
}

void StretchPlugin::createElastique() {
  if (_elastique != nullptr) {
    return;
  }
#if TARGET_IPHONE_SIMULATOR || TARGET_OS_IPHONE
  CElastiqueProV3DirectIf::ElastiqueVersion_t version =
      CElastiqueProV3DirectIf::kV3mobile;
#else
  CElastiqueProV3DirectIf::ElastiqueVersion_t version =
      CElastiqueProV3DirectIf::kV3Pro;
#endif
  CElastiqueProV3DirectIf::CreateInstance(_elastique, _channels, _samplerate,
                                          version);
}

void StretchPlugin::destroyElastique() {
  if (_elastique == nullptr) {
    return;
  }
  CElastiqueProV3DirectIf::DestroyInstance(_elastique);
  _elastique = nullptr;
  _prefilled = false;
  _frames_to_discard = 0;
  _frames_to_preproc = 0;
}

size_t StretchPlugin::copyResidualFrames(Content &output) {
  size_t current_items = output.items();
  if (current_items >= output.requiredItems()) return 0;
  size_t required_items = output.requiredItems() - current_items;
  auto copy_samples = std::min(required_items, _residual_buffer.size());
  float *content_samples = ((float *)output.payload()) + current_items;
  std::copy_n(_residual_buffer.data(), copy_samples, content_samples);
  size_t total_items = current_items + copy_samples;
  output.setItems(total_items);
  _residual_buffer.erase(_residual_buffer.begin(),
                         _residual_buffer.begin() + copy_samples);
  return total_items;
}

size_t StretchPlugin::copyProcessedFrames(Content &output, int output_frames) {
  auto filled_items = output.items();
  auto still_required_frames =
      (output.requiredItems() - filled_items) / _channels;
  long usable_frames = output_frames - _frames_to_discard;
  if (usable_frames <= 0) {
    _frames_to_discard = _frames_to_discard - output_frames;
    return 0;
  }
  auto new_residual_items = (usable_frames - still_required_frames) * _channels;
  auto residual_buffer_size = _residual_buffer.size();
  if (usable_frames > still_required_frames) {
    auto new_size = residual_buffer_size + new_residual_items;
    _residual_buffer.resize(new_size, 0.0f);
    output.setItems(output.requiredItems());
  } else if (usable_frames == still_required_frames) {
    output.setItems(output.requiredItems());
  } else {
    output.setItems(filled_items + usable_frames * _channels);
  }
  float *content_samples = (float *)output.payload();
  auto residual_buffer_ptr = _residual_buffer.data();
  for (int frame = 0; frame < usable_frames; ++frame) {
    for (int channel = 0; channel < _channels; ++channel) {
      if (frame < still_required_frames) {
        content_samples[filled_items + (frame * _channels) + channel] =
            _output_buffers[channel][frame + _frames_to_discard];
      } else {
        size_t residual_buffer_index =
            residual_buffer_size +
            ((frame - still_required_frames) * _channels) + channel;
        residual_buffer_ptr[residual_buffer_index] =
            _output_buffers[channel][frame + _frames_to_discard];
      }
    }
  }
  _frames_to_discard = 0;
  return output.items();
}

size_t StretchPlugin::resizeContent(
    std::map<std::string, std::shared_ptr<Content>> &content) {
  size_t maximum_samples = 0;
  for (auto &content_pair : content) {
    auto &content_instance = content_pair.second;
    if (content_instance->type() != ContentPayloadTypeBuffer) {
      continue;
    }
    long items = clipIntervleavedSamplesToChannel(
        content_instance->requiredItems() * _previous_stretch_value, _channels);

    // Round items to the nearest multiple of 2048 samples
    // to avoid stalling realtime playback
    // TODO maybe move some processing to run()?
    static const size_t MIN_ITEMS = 2048;
    items = (items + MIN_ITEMS - 1) / MIN_ITEMS * MIN_ITEMS;

    // TODO: WHY ARE WE ERASING HERE?!
    if (!_content.count(content_pair.first)) {
      _content[content_pair.first] = std::make_shared<plugin::Content>(
          items, 0, content_instance->sampleRate(),
          content_instance->channels(), items, content_instance->type());

    } else {
      _content[content_pair.first]->erase();
      _content[content_pair.first]->resize(items);
    }
    maximum_samples = std::max(maximum_samples, (size_t)items);
  }
  return maximum_samples;
}

bool StretchPlugin::prefill(Content &child_content, Content &output_content) {
  bool ret = false;
  if (!_frames_to_discard) {
    _frames_to_discard = _elastique->GetNumOfInitialUnusedFrames();
    _frames_to_preproc = _elastique->GetPreFramesNeeded();
  }
  size_t prefilled_samples = _prefill_buffer.size();
  size_t child_samples = child_content.items();
  size_t total_available_samples = child_samples + prefilled_samples;
  size_t total_available_frames = total_available_samples / _channels;
  size_t samples_needed = _frames_to_preproc * _channels - prefilled_samples;
  long samples_to_copy = std::min(child_samples, samples_needed);
  _prefill_buffer.resize(prefilled_samples + samples_to_copy);
  float *prefill_ptr = _prefill_buffer.data() + prefilled_samples;
  if (samples_to_copy < child_samples) {
    _child_sample_offset = child_samples - samples_to_copy;
  } else {
    _child_sample_offset = 0;
  }
  auto child_audio_ptr = child_content.payload();
  std::copy_n(child_audio_ptr, samples_to_copy, prefill_ptr);

  // If we still don't have enough samples, we're done for now
  if (total_available_frames < _frames_to_preproc) {
    child_content.setItems(0);
    return ret;
  }

  // Otherwise, do the actual preprocessing
  // Fill input buffers
  for (int channel = 0; channel < _channels; ++channel) {
    for (long frame = 0; frame < _frames_to_preproc; ++frame) {
      auto buffer_offset = ((frame * _channels) + channel);
      _input_buffers[channel][frame] = _prefill_buffer[buffer_offset];
    }
  }
  ret = true;
  int output_frames = _elastique->PreFillData(
      _input_buffers, _frames_to_preproc, _output_buffers);
  int frames_to_write = std::max(output_frames - _frames_to_discard, 0);
  if (!frames_to_write) {
    _frames_to_discard = _frames_to_discard - output_frames;
    return ret;
  }
  copyProcessedFrames(output_content, output_frames);
  return ret;
}

void StretchPlugin::saveOutput(long sample_index, Content &output_content) {
  auto items = output_content.items();
  _expected_feed_sample = sample_index + items;
  _previous_output.resize(items);
  std::copy_n(output_content.payload(), items, _previous_output.data());
}

bool StretchPlugin::fillInput(long frames_to_process, long frames_available,
                              float *samples) {
  long frames_needed = frames_to_process - _input_frame_offset;
  long frames_to_copy = std::min(frames_needed, frames_available);
  for (int channel = 0; channel < _channels; ++channel) {
    for (long frame = 0; frame < frames_to_copy; ++frame) {
      auto input_audio_offset =
          (((frame)*_channels) + channel) + _child_sample_offset;
      _input_buffers[channel][frame + _input_frame_offset] =
          samples[input_audio_offset];
    }
  }
  if (frames_to_copy < frames_needed) {
    _input_frame_offset = _input_frame_offset + frames_to_copy;
    return false;
  }
  _input_frame_offset = 0;
  return true;
}

bool StretchPlugin::almostEqual(float a, float b, float tolerance_factor) {
  float delta = fabs(a - b);
  a = fabs(a);
  b = fabs(b);
  float bigger = a > b ? a : b;
  if (delta <= bigger * tolerance_factor) {
    return true;
  }
  return false;
}

}  // namespace time
}  // namespace plugin
}  // namespace nativeformat
