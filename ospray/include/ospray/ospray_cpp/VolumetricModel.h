// Copyright 2009-2019 Intel Corporation
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
  VolumetricModel(const Volume &geom);
  VolumetricModel(OSPVolume geom);
  VolumetricModel(const VolumetricModel &copy);
  VolumetricModel(OSPVolumetricModel existing = nullptr);
};

static_assert(sizeof(VolumetricModel) == sizeof(OSPVolumetricModel),
    "cpp::VolumetricModel can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline VolumetricModel::VolumetricModel(const Volume &geom)
    : VolumetricModel(geom.handle())
{}

inline VolumetricModel::VolumetricModel(OSPVolume existing)
{
  ospObject = ospNewVolumetricModel(existing);
}

inline VolumetricModel::VolumetricModel(const VolumetricModel &copy)
    : ManagedObject<OSPVolumetricModel, OSP_VOLUMETRIC_MODEL>(copy.handle())
{
  ospRetain(copy.handle());
}

inline VolumetricModel::VolumetricModel(OSPVolumetricModel existing)
    : ManagedObject<OSPVolumetricModel, OSP_VOLUMETRIC_MODEL>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::VolumetricModel, OSP_VOLUMETRIC_MODEL);

} // namespace ospray
