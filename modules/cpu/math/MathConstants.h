// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDevice.h"
#include "common/BufferShared.h"
#include "common/Managed.h"
// ispc shared
#include "MathConstantsShared.h"

namespace ospray {

/* MathConstants manages storing the precompute RNG sequence data in USM
 */
struct OSPRAY_SDK_INTERFACE MathConstants
    : public AddStructShared<ManagedObject, ispc::MathConstants>
{
  MathConstants(api::ISPCDevice &device);

  virtual std::string toString() const override;

 private:
  // need buffershared members here
  std::unique_ptr<BufferShared<unsigned int>> halton_perm3_buf;
  std::unique_ptr<BufferShared<unsigned int>> halton_perm5_buf;
  std::unique_ptr<BufferShared<unsigned int>> halton_perm7_buf;

  // std::unique_ptr<BufferShared<unsigned int>> sobol_matrices_buf;
};

} // namespace ospray
