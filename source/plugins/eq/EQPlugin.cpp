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
#include "EQPlugin.h"

namespace nativeformat {
namespace plugin {
namespace eq {

EQPlugin::EQPlugin(const nfgrapher::contract::Eq3bandNodeInfo &eq_node,
                   int channels, double samplerate,
                   const std::shared_ptr<plugin::Plugin> &child_plugin)
    : _child_plugin(child_plugin),
      _low_cutoff(param::createParam(eq_node._low_cutoff._initial_val, 22000.0f,
                                     0.0f, "lowCutoff")),
      _mid_freq(param::createParam(eq_node._mid_frequency._initial_val,
                                   22000.0f, 0.0f, "midFrequency")),
      _high_cutoff(param::createParam(eq_node._high_cutoff._initial_val,
                                      22000.0f, 0.0f, "highCutoff")),
      _low_gain(param::createParam(eq_node._low_gain._initial_val, 24.0f,
                                   -24.0f, "lowGain")),
      _mid_gain(param::createParam(eq_node._mid_gain._initial_val, 24.0f,
                                   -24.0f, "midGain")),
      _high_gain(param::createParam(eq_node._high_gain._initial_val, 24.0f,
                                    -24.0f, "highGain")) {
  nfgrapher::param::addCommands(_low_cutoff, eq_node._low_cutoff);
  nfgrapher::param::addCommands(_mid_freq, eq_node._mid_frequency);
  nfgrapher::param::addCommands(_high_cutoff, eq_node._high_cutoff);
  nfgrapher::param::addCommands(_low_gain, eq_node._low_gain);
  nfgrapher::param::addCommands(_mid_gain, eq_node._mid_gain);
  nfgrapher::param::addCommands(_high_gain, eq_node._high_gain);

  _old_freqs = {_low_cutoff->valueForTime(0), _mid_freq->valueForTime(0),
                _high_cutoff->valueForTime(0)};
  _old_gains = {_low_gain->valueForTime(0), _mid_gain->valueForTime(0),
                _low_gain->valueForTime(0)};

  x_ = peqbank_new(samplerate, channels, 512);

  // make sure peqbank_perform_smooth gets used
  x_->b_mode = SMOOTH;

  t_filter **filters = new_filters(2);

  filters[0] =
      new_shelf(_low_gain->valueForTime(0), 0, _high_gain->valueForTime(0),
                _low_cutoff->valueForTime(0), _high_cutoff->valueForTime(0));
  // TODO should BW be a param?
  filters[1] = new_peq(_mid_freq->valueForTime(0), 0.5, 0,
                       _mid_gain->valueForTime(0), 0);

  peqbank_setup(x_, filters);
  // peqbank_print_info(x_);
}

EQPlugin::~EQPlugin() { peqbank_freemem(x_); }

void EQPlugin::feed(std::map<std::string, std::shared_ptr<Content>> &content,
                    long sample_index, long graph_sample_index,
                    nfgrapher::LoadingPolicy loading_policy) {
  _child_plugin->feed(content, sample_index, graph_sample_index,
                      loading_policy);
  Content &audio_content = *content[AudioContentTypeKey];
  size_t sample_count = audio_content.items();
  size_t channels = audio_content.channels();
  size_t frame_count = sample_count / channels;
  double sample_rate = audio_content.sampleRate();
  float *samples = (float *)audio_content.payload();
  double time = (sample_index / sample_rate) / static_cast<double>(channels);
  double end_time = time + ((static_cast<float>(sample_count) / sample_rate) /
                            static_cast<double>(channels));

  // get the new values for the EQ based on the current time
  float low_gain = _low_gain->smoothedValueForTimeRange(time, end_time);
  float mid_gain = _mid_gain->smoothedValueForTimeRange(time, end_time);
  float high_gain = _high_gain->smoothedValueForTimeRange(time, end_time);

  float low_cutoff = _low_cutoff->smoothedValueForTimeRange(time, end_time);
  float mid_freq = _mid_freq->smoothedValueForTimeRange(time, end_time);
  float high_cutoff = _high_cutoff->smoothedValueForTimeRange(time, end_time);

  // if the frequencies or gains have changed, recompute filter coefficients
  std::vector<float> new_freqs{low_cutoff, mid_freq, high_cutoff};
  std::vector<float> new_gains{low_gain, mid_gain, high_gain};

  // for now, assume filters[0] is a shelf and filters[1] is a peq
  if (_old_freqs != new_freqs || _old_gains != new_gains) {
    t_shelf *shelf = (t_shelf *)(x_->filters[0]->filter);
    t_peq *peq = (t_peq *)(x_->filters[1]->filter);

    // recreate some safety checks from new_peq without redoing the memory
    // allocation
    float G0 = peq->gain_dc;
    float G = mid_gain;
    float GB = peq->gain_bandwidth;

    if (G0 == G) {
      GB = G0;
      G = G0 + SMALL;
      G0 = G0 - SMALL;
    } else if (!((G0 < GB) && (GB < G)) && !((G0 > GB) && (GB > G))) {
      GB = (G0 + G) * 0.5f;
    }

    peq->gain_dc = G0;
    peq->gain_peak = G;
    peq->gain_bandwidth = GB;

    // I think it's safe to modify these shelf params without checks, but maybe
    // double check
    shelf->freq_low = low_cutoff;
    shelf->freq_high = high_cutoff;
    shelf->gain_low = low_gain;
    shelf->gain_high = high_gain;

    peqbank_compute(x_);

    _old_freqs = new_freqs;
    _old_gains = new_gains;
  }

  // resize buffer inside peqbank if necessary (should happen infrequently)
  // TODO the peqbank filtering won't work perfectly unless the buffer size
  // divisible by 4
  if (frame_count > x_->s_n) {
    peqbank_resize_buffer(x_, frame_count);
  } else if (frame_count < x_->s_n) {
    // don't need to reallocate buffer if we're shrinking it
    x_->s_n = frame_count;
  }

  peqbank_callback_float(x_, samples, samples);
}

std::string EQPlugin::name() {
  return nfgrapher::contract::Eq3bandNodeInfo ::kind();
}

std::vector<std::string> EQPlugin::paramNames() {
  return {_low_cutoff->name(), _mid_freq->name(), _high_cutoff->name(),
          _low_gain->name(),   _mid_gain->name(), _high_gain->name()};
}

std::shared_ptr<param::Param> EQPlugin::paramForName(const std::string &name) {
  if (name == _low_cutoff->name()) {
    return _low_cutoff;
  }
  if (name == _mid_freq->name()) {
    return _mid_freq;
  }
  if (name == _high_cutoff->name()) {
    return _high_cutoff;
  }
  if (name == _low_gain->name()) {
    return _low_gain;
  }
  if (name == _mid_gain->name()) {
    return _mid_gain;
  }
  if (name == _high_gain->name()) {
    return _high_gain;
  }
  return nullptr;
}

bool EQPlugin::finished(long sample_index, long sample_index_end) {
  return _child_plugin->finished(sample_index, sample_index_end);
}

void EQPlugin::notifyFinished(long sample_index, long graph_sample_index) {
  return _child_plugin->notifyFinished(sample_index, graph_sample_index);
}

Plugin::PluginType EQPlugin::type() const { return PluginTypeConsumer; }

std::vector<float> EQPlugin::get_freqs(double time) {
  return std::vector<float>{_low_cutoff->valueForTime(time),
                            _mid_freq->valueForTime(time),
                            _high_cutoff->valueForTime(time)};
}

std::vector<float> EQPlugin::get_gains(double time) {
  return std::vector<float>{_low_gain->valueForTime(time),
                            _mid_gain->valueForTime(time),
                            _high_gain->valueForTime(time)};
}

std::vector<float> EQPlugin::get_coefs(size_t filter_idx) {
  int c = 0;
  for (int i = 0; i <= filter_idx; ++i) {
    switch (x_->filters[i]->type) {
      case SHELF: {
        if (i == filter_idx) {
          return std::vector<float>{x_->coeff[c], x_->coeff[c + 1],
                                    x_->coeff[c + 2], x_->coeff[c + 3],
                                    x_->coeff[c + 4]};
        }
        c += NBCOEFF;
        break;
      }
      case PEQ: {
        if (i == filter_idx) {
          return std::vector<float>{x_->coeff[c], x_->coeff[c + 1],
                                    x_->coeff[c + 2], x_->coeff[c + 3],
                                    x_->coeff[c + 4]};
        }
        c += NBCOEFF;
        break;
      }
      case LPHP: {
        t_filter *f = x_->filters[i];
        t_lphp *lphp = (t_lphp *)f->filter;
        if (i == filter_idx) {
          std::vector<float> v;
          for (int j = c; j < c + ((lphp->order / 2) * NBCOEFF); ++c) {
            v.push_back(x_->coeff[j]);
          }
        }
        c += (lphp->order / 2) * NBCOEFF;
        break;
      }
    }
  }
  return std::vector<float>();
}

size_t EQPlugin::get_filter_count() {
  size_t i = 0;
  while (x_->filters[i]->type != NONE) {
    i++;
  }
  return i;
}

void EQPlugin::load(LOAD_CALLBACK callback) { _child_plugin->load(callback); }

bool EQPlugin::loaded() const { return _child_plugin->loaded(); }

void EQPlugin::run(long sample_index, const NodeTimes &node_times,
                   long node_sample_index) {
  _child_plugin->run(sample_index, node_times, node_sample_index);
}

bool EQPlugin::shouldProcess(long sample_index_start, long sample_index_end) {
  return _child_plugin->shouldProcess(sample_index_start, sample_index_end);
}

}  // namespace eq
}  // namespace plugin
}  // namespace nativeformat
