// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#ifdef __cplusplus
namespace ispc {
#endif // __cplusplus

struct Medium
{
  vec3f attenuation; //!< negative Napierian attenuation coefficient,
                     // i.e. wrt. the natural base e
  float ior; //!< Refraction index of medium.

#ifdef __cplusplus
  Medium() : attenuation(0.f), ior(1.f) {}
};
} // namespace ispc
#else
};
#endif // __cplusplus
