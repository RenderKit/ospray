// Copyright 2022 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/MathConstants.h"
#include "math/halton.h"
#include "math/sobol.h"

namespace ospray {

MathConstants::MathConstants(api::ISPCDevice &device)
    : AddStructShared(device.getIspcrtContext())
{
  haltonPerm3Buf =
      make_buffer_shared_unique<unsigned int>(device.getIspcrtContext(), 243);
  haltonPerm5Buf =
      make_buffer_shared_unique<unsigned int>(device.getIspcrtContext(), 125);
  haltonPerm7Buf =
      make_buffer_shared_unique<unsigned int>(device.getIspcrtContext(), 343);
  sobolMatricesBuf = make_buffer_shared_unique<unsigned int>(
      device.getIspcrtContext(), Sobol_numDimensions * Sobol_numBits);

  std::memcpy(haltonPerm3Buf->begin(),
      halton_perm3,
      haltonPerm3Buf->size() * sizeof(unsigned int));

  std::memcpy(haltonPerm5Buf->begin(),
      halton_perm5,
      haltonPerm5Buf->size() * sizeof(unsigned int));

  std::memcpy(haltonPerm7Buf->begin(),
      halton_perm7,
      haltonPerm7Buf->size() * sizeof(unsigned int));

  std::memcpy(sobolMatricesBuf->begin(),
      Sobol_matrices,
      sobolMatricesBuf->size() * sizeof(unsigned int));

  getSh()->haltonPerm3 = haltonPerm3Buf->data();
  getSh()->haltonPerm5 = haltonPerm5Buf->data();
  getSh()->haltonPerm7 = haltonPerm7Buf->data();
  getSh()->sobolMatrices = sobolMatricesBuf->data();
}

std::string MathConstants::toString() const
{
  return "ospray::MathConstants";
}

} // namespace ospray
