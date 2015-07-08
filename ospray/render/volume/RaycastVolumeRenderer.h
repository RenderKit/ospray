// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "ospray/camera/Camera.h"
#include "ospray/common/Model.h"
#include "ospray/lights/Light.h"
#include "ospray/render/Renderer.h"

namespace ospray {

  //! \brief A concrete implemetation of the Renderer class for rendering
  //!  volumes optionally containing embedded surfaces.
  //!
  class RaycastVolumeRenderer : public Renderer {
  public:

    //! Constructor.
    RaycastVolumeRenderer() {};

    //! Destructor.
    ~RaycastVolumeRenderer() {};

    //! Initialize the renderer state, and create the equivalent ISPC volume renderer object.
    virtual void commit();

    //! A string description of this class.
    virtual std::string toString() const { return("ospray::RaycastVolumeRenderer"); }

  protected:

    //! Required renderer state.
    Camera *camera;
    Model *model;

    //! Print an error message.
    void emitMessage(const std::string &kind, const std::string &message) const
    { std::cerr << "  " + toString() + "  " + kind + ": " + message + "." << std::endl; }

    //! Error checking.
    void exitOnCondition(bool condition, const std::string &message) const
    { if (!condition) return;  emitMessage("ERROR", message);  exit(1); }

    //! Gather pointers to the ISPC equivalents from an array of Light objects.
    void **getLightsFromData(const Data *buffer);

  };

} // ::ospray

