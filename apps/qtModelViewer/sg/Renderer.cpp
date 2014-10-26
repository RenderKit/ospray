/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "Renderer.h"

namespace ospray {
  namespace sg {

    Renderer::Renderer()
      // : ospFrameBuffer(NULL)
    {}

    int Renderer::renderFrame()
    { 
      if (!integrator) return 1;
      if (!frameBuffer) return 2;
      if (!camera) return 3;
      if (!world) return 4;

      ospRenderFrame(frameBuffer->ospFrameBuffer,
                     integrator->ospRenderer,
                     OSP_FB_COLOR|OSP_FB_ACCUM);
      return 0;
    }

    /*! re-start accumulation (for progressive rendering). make sure
      that this function gets called at lesat once every time that
      anything changes that might change the appearance of the
      converged image (e.g., camera position, scene, frame size,
      etc) */
    void Renderer::restartAccumulation()
    {
      accumID = 0;
      if (frameBuffer) 
        frameBuffer->clearAccum(); 
// {
//         ospFrameBufferClear(ospFrameBuffer,OSP_FB_ACCUM);
//       }
    }

    //! create a default camera
    Ref<sg::Camera> Renderer::createDefaultCamera()
    {
      Ref<sg::PerspectiveCamera> camera = new sg::PerspectiveCamera;
      return camera.cast<sg::Camera>();
    }

    void Renderer::setCamera(const Ref<sg::Camera> &camera) 
    {
      this->camera = camera;
      restartAccumulation();
    }

    void Renderer::setWorld(const Ref<World> &world)
    {
      this->world = world;
      allNodes.clear();
      uniqueNodes.clear();
      if (world) {
        allNodes.serialize(world,sg::Serialization::DONT_FOLLOW_INSTANCES);
        uniqueNodes.serialize(world,sg::Serialization::DO_FOLLOW_INSTANCES);
      } else 
        std::cout << "#ospQTV: no world defined, yet\n#ospQTV: (did you forget to pass a scene file name on the command line?)" << std::endl;

      restartAccumulation();
    }

    sg::Camera *Renderer::getLastDefinedCamera() const
    {
      for (size_t i=0;i<uniqueNodes.size();i++) {
        sg::Camera *asCamera
          = dynamic_cast<sg::Camera*>(uniqueNodes.object[i]->node.ptr);
        if (asCamera != NULL)
          return asCamera;
      }
      return NULL;
    }
    
  }
}
