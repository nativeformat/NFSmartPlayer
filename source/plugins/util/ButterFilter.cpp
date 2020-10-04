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
#include "ButterFilter.h"

#include <cmath>
#include <iostream>

namespace nativeformat {
namespace plugin {
namespace util {

static constexpr double TWOPI = (2.0 * M_PI);
static constexpr double EPS = 1e-10;

ButterFilter::ButterFilter(unsigned order, FilterType type, unsigned channels)
    : _filter_type(type),
      _order(order),
      _channels(channels),
      _alpha1(0.0),
      _alpha2(0.0),
      _gain(1),
      _x_coeffs{0},
      _y_coeffs{0} {
  _x_mem.resize(channels);
  _y_mem.resize(channels);
}

ButterFilter::~ButterFilter() {}

void ButterFilter::filter(float *samples, size_t frames) {
  float x_n = 0, y_n = 0;
  size_t yc_max = _zplane.numpoles;
  size_t xc_max = _zplane.numzeros;

  // TODO consider using direct II transpose structure for efficiency
  for (size_t i = 0; i < frames; ++i) {
    for (size_t c = 0; c < _channels; ++c) {
      size_t n = i * _channels + c;
      y_n = 0;
      x_n = samples[n] / _gain;
      y_n += _x_coeffs[xc_max] * x_n;
      for (size_t j = 1; j <= xc_max; ++j) {
        y_n += _x_coeffs[xc_max - j] * _x_mem[c][j - 1];
      }
      for (size_t j = 1; j <= yc_max; ++j) {
        y_n -= _y_coeffs[yc_max - j] * _y_mem[c][j - 1];
      }
      y_n /= _y_coeffs[yc_max];

      // shift oldest x and y out, replace with x_n and y_n
      for (int j = _zplane.numzeros - 1; j > 0; --j) {
        _x_mem[c][j] = _x_mem[c][j - 1];
      }
      for (int j = _zplane.numpoles - 1; j > 0; --j) {
        _y_mem[c][j] = _y_mem[c][j - 1];
      }
      _x_mem[c][0] = x_n;
      _y_mem[c][0] = y_n;
      samples[n] = y_n;
    }
  }
}

void ButterFilter::compute(double alpha1, double alpha2) {
  // reset pole-zero representations
  _splane.numpoles = 0;
  _zplane.numpoles = 0;
  _alpha1 = alpha1;
  _alpha2 = alpha2;

  // compute_s
  _splane.numpoles = 0;
  for (int i = 0; i < 2 * _order; i++) {
    double theta =
        (_order & 1) ? (i * M_PI) / _order : ((i + 0.5) * M_PI) / _order;
    choosepole(std::complex<double>(std::cos(theta), std::sin(theta)));
  }

  // prewarp
  double warped_alpha1 = tan(M_PI * alpha1) / M_PI;
  double warped_alpha2 = tan(M_PI * alpha2) / M_PI;

  // normalise
  double w1 = TWOPI * warped_alpha1;
  double w2 = TWOPI * warped_alpha2;
  switch (_filter_type) {
    case FilterType::LowPassFilter: {
      for (int i = 0; i < _splane.numpoles; i++) {
        _splane.poles[i] = _splane.poles[i] * w1;
      }
      _splane.numzeros = 0;
      break;
    }
    case FilterType::HighPassFilter: {
      for (int i = 0; i < _splane.numpoles; i++) {
        _splane.poles[i] = w1 / _splane.poles[i];
      }
      for (int i = 0; i < _splane.numpoles; i++) {
        _splane.zeros[i] = 0.0; /* also N zeros at (0,0) */
      }
      _splane.numzeros = _splane.numpoles;
      break;
    }
    case FilterType::BandPassFilter: {
      double w0 = sqrt(w1 * w2), bw = w2 - w1;
      for (int i = 0; i < _splane.numpoles; i++) {
        std::complex<double> hba = 0.5 * (_splane.poles[i] * bw);
        std::complex<double> temp = csqrt(1.0 - ((w0 / hba) * (w0 / hba)));
        _splane.poles[i] = hba * (1.0 + temp);
        _splane.poles[_splane.numpoles + i] = hba * (1.0 - temp);
      }
      for (int i = 0; i < _splane.numpoles; i++) {
        _splane.zeros[i] = 0.0; /* also N zeros at (0,0) */
      }
      _splane.numzeros = _splane.numpoles;
      _splane.numpoles *= 2;
      break;
    }
  }

  // compute z domain filter using bilinear transformation
  _zplane.numpoles = _splane.numpoles;
  _zplane.numzeros = _splane.numzeros;
  for (int i = 0; i < _zplane.numpoles; i++) {
    _zplane.poles[i] = blt(_splane.poles[i]);
  }
  for (int i = 0; i < _zplane.numzeros; i++) {
    _zplane.zeros[i] = blt(_splane.zeros[i]);
  }
  while (_zplane.numzeros < _zplane.numpoles) {
    _zplane.zeros[_zplane.numzeros++] = -1.0;
  }

  // expandpoly
  std::complex<double> topcoeffs[MAX_PZ + 1], botcoeffs[MAX_PZ + 1];
  expand(_zplane.zeros, _zplane.numzeros, topcoeffs);
  expand(_zplane.poles, _zplane.numpoles, botcoeffs);

  std::complex<double> dc_gain, fc_gain, hf_gain;
  dc_gain =
      evaluate(topcoeffs, _zplane.numzeros, botcoeffs, _zplane.numpoles, 1.0);
  double theta = TWOPI * 0.5 * (alpha1 + alpha2); /* "jwT" for centre freq. */
  fc_gain = evaluate(topcoeffs, _zplane.numzeros, botcoeffs, _zplane.numpoles,
                     std::complex<double>(std::cos(theta), std::sin(theta)));
  hf_gain =
      evaluate(topcoeffs, _zplane.numzeros, botcoeffs, _zplane.numpoles, -1.0);

  switch (_filter_type) {
    case FilterType::HighPassFilter:
      _gain = std::abs(hf_gain);
      break;
    case FilterType::LowPassFilter:
      _gain = std::abs(dc_gain);
      break;
    case FilterType::BandPassFilter:
      _gain = std::abs(fc_gain);
      break;
  }

  for (int i = 0; i <= _zplane.numzeros; i++) {
    _x_coeffs[i] = +(topcoeffs[i].real() / botcoeffs[_zplane.numpoles].real());
  }
  for (int i = 0; i <= _zplane.numpoles; i++) {
    _y_coeffs[i] = -(botcoeffs[i].real() / botcoeffs[_zplane.numpoles].real());
  }

  // create vectors to hold history if not yet initialized
  if (!_x_mem[0].size()) {
    for (size_t i = 0; i < _channels; ++i) {
      _x_mem[i].resize(_zplane.numzeros);
      _y_mem[i].resize(_zplane.numpoles);
    }
  }

  /*
  printf("Computed coefficients for alpha1 = %f, alpha2 = %f\n", alpha1,
  alpha2); printf("X coeffs (b): "); for (size_t i = 0; i <= _zplane.numzeros;
  ++i) printf("%f, ", _x_coeffs[i]); printf("\nY coeffs (a): "); for (size_t i =
  0; i <= _zplane.numpoles; ++i) printf("%f, ", _y_coeffs[i]); printf("\n");
  printf("Gain: %f\n", _gain);
  */
}

void ButterFilter::choosepole(std::complex<double> z) {
  if (z.real() < 0.0) {
    _splane.poles[_splane.numpoles++] = z;
    // printf("added pole: real = %e, imag = %e\n", z.real(), z.imag());
  } else {
    // printf("not adding pole, z.real() >= 0 (z = %e, %e)\n", z.real(),
    // z.imag());
  }
}

std::complex<double> ButterFilter::blt(std::complex<double> pz) {
  return (2.0 + pz) / (2.0 - pz);
}

void ButterFilter::multin(std::complex<double> w, int npz,
                          std::complex<double> coeffs[]) {
  /* multiply factor (z-w) into coeffs */
  std::complex<double> nw = -w;
  for (int i = npz; i >= 1; i--) {
    coeffs[i] = (nw * coeffs[i]) + coeffs[i - 1];
  }
  coeffs[0] = nw * coeffs[0];
}

void ButterFilter::expand(std::complex<double> pz[], int npz,
                          std::complex<double> coeffs[]) {
  /* compute product of poles or zeros as a polynomial of z */
  coeffs[0] = 1.0;
  for (int i = 0; i < npz; i++) {
    coeffs[i + 1] = 0.0;
  }
  for (int i = 0; i < npz; i++) {
    multin(pz[i], npz, coeffs);
  }
  /* check computed coeffs of z^k are all real */
  for (int i = 0; i < npz + 1; i++) {
    if (fabs(coeffs[i].imag()) > EPS) {
      fprintf(stderr,
              "mkfilter: coeff of z^%d is not real; poles/zeros are not "
              "complex conjugates\n",
              i);
      exit(1);
    }
  }
}

std::complex<double> ButterFilter::csqrt(std::complex<double> x) {
  double r = hypot(x);
  std::complex<double> z = std::complex<double>(Xsqrt(0.5 * (r + x.real())),
                                                Xsqrt(0.5 * (r - x.real())));
  if (x.imag() < 0.0) {
    z.imag(-z.imag());
  }
  return z;
}

std::complex<double> ButterFilter::eval(std::complex<double> coeffs[], int npz,
                                        std::complex<double> z) {
  /* evaluate polynomial in z, substituting for z */
  std::complex<double> sum = std::complex<double>(0.0);
  for (int i = npz; i >= 0; i--) sum = (sum * z) + coeffs[i];
  return sum;
}

std::complex<double> ButterFilter::evaluate(std::complex<double> topco[],
                                            int nz,
                                            std::complex<double> botco[],
                                            int np, std::complex<double> z) {
  /* evaluate response, substituting for z */
  return eval(topco, nz, z) / eval(botco, np, z);
}

std::string ButterFilter::description() {
  std::stringstream ss;
  switch (_filter_type) {
    case FilterType::LowPassFilter:
      ss << "Low-pass filter  (order = " << _order << ", cutoff = " << _alpha1
         << ")";
      break;
    case FilterType::HighPassFilter:
      ss << "High-pass filter (order = " << _order << ", cutoff = " << _alpha1
         << ")";
      break;
    case FilterType::BandPassFilter:
      ss << "Band-pass filter (order = " << _order << ", passband = " << _alpha1
         << " - " << _alpha2 << ")";
      break;
  }
  return ss.str();
}

}  // namespace util
}  // namespace plugin
}  // namespace nativeformat
