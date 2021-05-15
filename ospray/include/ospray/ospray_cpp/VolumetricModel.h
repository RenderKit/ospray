// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "TransferFunction.h"
#include "Volume.h"

namespace ospray {
namespace cpp {

class VolumetricModel
    : public ManagedObject<OSPVolumetricModel, OSP_VOLUMETRIC_MODEL>
{
 public:
  VolumetricModel(const Volume &);
  VolumetricModel(OSPVolume);
  VolumetricModel(OSPVolumetricModel existing = nullptr);
};

static_assert(sizeof(VolumetricModel) == sizeof(OSPVolumetricModel),
    "cpp::VolumetricModel can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline VolumetricModel::VolumetricModel(const Volume &vol)
    : VolumetricModel(vol.handle())
{}

inline VolumetricModel::VolumetricModel(OSPVolume vol)
{
  ospObject = ospNewVolumetricModel(vol);
}

inline VolumetricModel::VolumetricModel(OSPVolumetricModel existing)
    : ManagedObject<OSPVolumetricModel, OSP_VOLUMETRIC_MODEL>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::VolumetricModel, OSP_VOLUMETRIC_MODEL);

} // namespace ospray
