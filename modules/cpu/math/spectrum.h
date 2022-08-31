// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

// ==> 430..675
#define SPECTRUM_SAMPLES 8
#define SPECTRUM_FIRSTWL 430.f
#define SPECTRUM_SPACING 35.f

#ifdef __cplusplus
#include <array>
#include <cassert>
struct alignas(SPECTRUM_SAMPLES * sizeof(float)) spectrum
    : public std::array<float, SPECTRUM_SAMPLES>
{
  spectrum() = default;
  spectrum(const spectrum &) = default;
  ~spectrum() = default;
  spectrum &operator=(const spectrum &) = default;

  // The initializer_list constructor lets us construct the spectrum object
  // in C++ as if it's directly a std::array using the array init syntax
  spectrum(std::initializer_list<float> l)
  {
    assert(l.size() == SPECTRUM_SAMPLES);
    std::copy(l.begin(), l.end(), begin());
  }
};
#endif // __cplusplus

#define SPECTRUM_AL_ETA                                                        \
  {                                                                            \
    0.570, 0.686, 0.813, 0.952, 1.11, 1.29, 1.49, 1.73                         \
  }
#define SPECTRUM_AL_K                                                          \
  {                                                                            \
    5.21, 5.64, 6.05, 6.45, 6.85, 7.24, 7.61, 7.94                             \
  }

#define RGB_AL_ETA                                                             \
  {                                                                            \
    1.47f, 0.984f, 0.602f                                                      \
  }
#define RGB_AL_K                                                               \
  {                                                                            \
    7.64f, 6.55f, 5.36f                                                        \
  }
// = ((eta-1)^2+k^2)/((eta+1)^2+k^2)
#define RGB_AL_COLOR                                                           \
  {                                                                            \
    0.909f, 0.916f, 0.923f                                                     \
  }
