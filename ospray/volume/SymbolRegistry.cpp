//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "ospray/volume/BlockBrickedVolume.h"

namespace ospray {

    //! A volume type with 64-bit addressing and multi-level bricked storage order.
    OSP_REGISTER_VOLUME(BlockBrickedVolume, block_bricked_volume);

    //! A volume type with 32-bit addressing and brick storage order.
//  OSP_REGISTER_VOLUME(BrickedVolume, bricked_volume);

    //! A volume type with 32-bit addressing and XYZ storage order.
//  OSP_REGISTER_VOLUME(MonolithicVolume, monolithic_volume);

} // namespace ospray

