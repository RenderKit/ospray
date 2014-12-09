/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \defgroup ospray_render_tachyon OSPRay's Tachyon-style renderer
  
  \ingroup ospray_supported_renderers

  \brief Implements the data, materials, and lighting models of John
  Stone's "Tachyon" renderer as used by VMD.
 */

// ospray
#include "ospray/render/Renderer.h"

namespace ospray {

  struct Camera;
  struct Model;

  enum { 
    RC_EYELIGHT=0,
    RC_PRIMID,
    RC_GEOMID,
    RC_INSTID,
    RC_GNORMAL,
    RC_TESTSHADOW,
  } RC_SHADEMODE;

  /*! \brief Implements the family of simple ray cast renderers */
  struct TachyonRenderer : public Renderer {
    TachyonRenderer();
    virtual void commit();
    virtual std::string toString() const { return "ospray::TachyonRenderer"; }

      Model  *model;
      Camera *camera;
      Data   *textureData;
      Data   *pointLightData;
      void   *pointLightArray;
      uint32  numPointLights;
      Data   *dirLightData;
      void   *dirLightArray;
      uint32  numDirLights;
      bool    doShadows;
  };
};
