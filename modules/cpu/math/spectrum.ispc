// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "math/spectrum.ih"

// convolutions of: normalization * D65 * CIE_XYZ * xyz2sRGB

OSPRAY_BEGIN_ISPC_NAMESPACE

#ifdef ISPC
spectrum spectrum_sRGB_r = {0.0598548f,
    -0.0234574f,
    -0.220138f,
    -0.238902f,
    0.316327f,
    0.738315f,
    0.323302f,
    0.0446981f};

spectrum spectrum_sRGB_g = {-0.0567346f,
    -0.0160361f,
    0.223861f,
    0.531185f,
    0.337221f,
    0.0149718f,
    -0.0296053f,
    -0.00486239f};

spectrum spectrum_sRGB_b = {0.420693f,
    0.616597f,
    0.0796766f,
    -0.0496266f,
    -0.0473149f,
    -0.0167536f,
    -0.00295686f,
    -0.000314818f};
#endif

OSPRAY_END_ISPC_NAMESPACE
