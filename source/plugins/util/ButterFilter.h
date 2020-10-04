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
#pragma once

#include <complex>
#include <vector>

namespace nativeformat {
namespace plugin {
namespace util {

enum class FilterType { LowPassFilter, HighPassFilter, BandPassFilter };

class ButterFilter {
 public:
  static constexpr unsigned MAX_ORDER = 10;
  static constexpr unsigned MAX_PZ =
      2 * MAX_ORDER;  // allow for doubling of poles in BP filter

  ButterFilter(unsigned order, FilterType type, unsigned channels);
  ~ButterFilter();

  // Compute coefficients for the desired frequency cutoff(s)
  void compute(double alpha1, double alpha2 = 0);

  // Filter the provided input sample in place
  void filter(float *samples, size_t frames);

  // Return a human-readable description of current configuration
  std::string description();

 private:
  struct pzrep {
    std::complex<double> poles[MAX_PZ], zeros[MAX_PZ];
    int numpoles, numzeros;
  };

  void choosepole(std::complex<double> z);
  static std::complex<double> blt(std::complex<double> pz);
  static void multin(std::complex<double> w, int npz,
                     std::complex<double> coeffs[]);
  static void expand(std::complex<double> pz[], int npz,
                     std::complex<double> coeffs[]);
  static inline double hypot(std::complex<double> z) {
    return ::hypot(z.imag(), z.real());
  }
  static std::complex<double> eval(std::complex<double> coeffs[], int npz,
                                   std::complex<double> z);
  static std::complex<double> evaluate(std::complex<double> topco[], int nz,
                                       std::complex<double> botco[], int np,
                                       std::complex<double> z);

  /* because of deficiencies in hypot on Sparc, it's possible for arg of Xsqrt
   to be small and -ve, which logically it can't be (since r >= |x.re|).
   Take it as 0. */
  static inline double Xsqrt(double x) { return (x >= 0.0) ? sqrt(x) : 0.0; }
  static std::complex<double> csqrt(std::complex<double> x);

  const FilterType _filter_type;
  const unsigned _order;
  const unsigned _channels;
  pzrep _splane;
  pzrep _zplane;
  double _alpha1;
  double _alpha2;
  double _gain;
  double _x_coeffs[MAX_PZ + 1];
  double _y_coeffs[MAX_PZ + 1];
  std::vector<std::vector<float>> _x_mem;
  std::vector<std::vector<float>> _y_mem;
};

}  // namespace util
}  // namespace plugin
}  // namespace nativeformat
