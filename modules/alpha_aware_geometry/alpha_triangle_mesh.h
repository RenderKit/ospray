/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "ospray/geometry/trianglemesh.h"

namespace ospray {
  struct Model;
  struct Texture2D;

  //Used to specify where to look in the alpha map for the actual alpha value
  enum AlphaComponent {
    UNKOWN,
    R,
    G,
    B,
    A,
    X = R,
    Y = G,
    Z = B,
    W = A,
  };

  //Used to specify if we should use 1-alpha or direct alpha.
  enum AlphaType {
    ALPHA,
    OPACITY,
  };

  struct AlphaAwareTriangleMesh : public TriangleMesh
  {
    AlphaAwareTriangleMesh();

    //overloads
    virtual std::string toString() const { return "ospray::AlphaAwareTriangleMesh"; }
    virtual void finalize(Model *model);

    //!Returns extracted alpha value (handles switch between opacity and alpha if necessary)
    float getAlpha(const vec4f &sample) const;

    //data
    Ref<Data> alphaMapData;
    Ref<Data> alphaData;
    int num_materials;
    float *globalAlphas; //Global alpha value, overridden if map is non-null
    Texture2D **mapPointers;
    void **ispcMapPointers;
    AlphaComponent alphaComponent;
    AlphaType      alphaType;
  };

  extern "C" AlphaAwareTriangleMesh *createAlphaAwareTriangleMesh();
}
