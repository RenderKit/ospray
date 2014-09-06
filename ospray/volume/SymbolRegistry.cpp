//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "ospray/volume/BrickedVolume.h"

namespace ospray {

    //! A volume type with 32-bit addressing and brick storage order.
    OSP_REGISTER_VOLUME(BrickedVolume, bricked);

    //! A volume type with 32-bit addressing and XYZ storage order.
//  OSP_REGISTER_VOLUME(MonolithicVolume, monolithic);

    //! A volume type with 64-bit addressing and multi-level bricked storage order.
//  OSP_REGISTER_VOLUME(PageBrickedVolume, page-bricked);

} // namespace ospray

