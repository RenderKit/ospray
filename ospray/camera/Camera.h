/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include "../common/Managed.h"
#include "../common/Ray.h"

namespace ospray {
  
  //! base camera class abstraction 
  /*! the base class itself does not do anything useful; look into
      perspectivecamera etc for that */
  struct Camera : public ManagedObject {
    //! \brief common function to help printf-debugging 
    /*! Every derived class should overrride this! */
    virtual std::string toString() const { return "ospray::Camera (base class)"; }
    static Camera *createCamera(const char *identifier);
    virtual void initRay(Ray &ray, const vec2f &sample) = 0;
  };

  /*! \brief registers a internal ospray::'ClassName' camera under
      the externally accessible name "external_name" 
      
      \internal This currently works by defining a extern "C" function
      with a given predefined name that creates a new instance of this
      camera. By having this symbol in the shared lib ospray can
      lateron always get a handle to this fct and create an instance
      of this camera.
  */
#define OSP_REGISTER_CAMERA(InternalClassName,external_name)        \
  extern "C" Camera *ospray_create_camera__##external_name()        \
  {                                                                 \
    return new InternalClassName;                                   \
  }                                                                 \

}
