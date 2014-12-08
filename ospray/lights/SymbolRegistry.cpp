//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include "ospray/lights/DirectionalLight.h"
#include "ospray/lights/PointLight.h"
#include "ospray/lights/SpotLight.h"

namespace ospray {

    //! A directional light.
    OSP_REGISTER_LIGHT(DirectionalLight, DirectionalLight);

    //! A point light with bounded range.
    OSP_REGISTER_LIGHT(PointLight, PointLight);

    //! A spot light with bounded angle and range.
    OSP_REGISTER_LIGHT(SpotLight, SpotLight);

} // namespace ospray

