// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ISPCDevice.h"
#include "common/Managed.h"
#include "common/StructShared.h"
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
  std::unique_ptr<BufferShared<unsigned int>> haltonPerm3Buf;
  std::unique_ptr<BufferShared<unsigned int>> haltonPerm5Buf;
  std::unique_ptr<BufferShared<unsigned int>> haltonPerm7Buf;

  std::unique_ptr<BufferShared<unsigned int>> sobolMatricesBuf;
};

} // namespace ospray
