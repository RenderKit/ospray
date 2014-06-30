#pragma once


// ospray
#include "ospray/render/renderer.h"
#include "ospray/common/material.h"

// obj renderer
#include "objpointlight.h"

// system
#include <vector>

namespace ospray {
  struct Camera;
  struct Model;

  namespace obj {
    using embree::TaskScheduler;

    /*! \brief Renderer for the OBJ Wavefront Material/Lighting format 

      See \ref ospray_render_obj
    */
    struct OBJRenderer : public Renderer {
      OBJRenderer();
      virtual std::string toString() const { return "ospray::OBJRenderer"; }

      std::vector<void*> pointLightArray; // the 'IE's of the 'pointLightData'
      std::vector<void*> dirLightArray;   // the 'IE's of the 'dirLightData'

      Model    *world;
      Camera   *camera;
      Data     *pointLightData;
      Data     *dirLightData;

      uint32    numDirLights;
      
      virtual void commit();

      /*! \brief create a material of given type */
      virtual Material *createMaterial(const char *type);
      /*! \brief create a light of given type */
      virtual Light    *createLight(const char *type);
    };

  }
}

