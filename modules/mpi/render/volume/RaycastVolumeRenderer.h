// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#include "OSPMPIConfig.h"
#include "render/Renderer.h"

namespace ospray {

  //! \brief A concrete implemetation of the Renderer class for rendering
  //!  volumes optionally containing embedded surfaces.
  //!
  class RaycastVolumeRenderer : public Renderer {

  public:

    RaycastVolumeRenderer()  = default;
    ~RaycastVolumeRenderer() = default;

    //! Create a material of the given type.
    Material* createMaterial(const char *type) override;

    //! Initialize the renderer state, and create the equivalent ISPC volume
    //! renderer object.
    void commit() override;

    //! A string description of this class.
    std::string toString() const override;

    /*! per-frame data to describe the data-parallel components */
    float renderFrame(FrameBuffer *fb, const uint32 channelFlags) override;

  private:

    //! Print an error message.
    void emitMessage(const std::string &kind, const std::string &message) const;

    //! Error checking.
    void exitOnCondition(bool condition, const std::string &message) const;

    //! ISPC equivalents for lights.
    std::vector<void *> lights;

  };

  // Inlined function definitions /////////////////////////////////////////////

  inline std::string RaycastVolumeRenderer::toString() const
  {
    return("ospray::RaycastVolumeRenderer");
  }

  inline void RaycastVolumeRenderer::emitMessage(const std::string &kind,
                                                 const std::string &msg) const
  {
    std::cerr << "  " + toString() + "  " << kind + ": " + msg + "."
              << std::endl;
  }

  inline void
  RaycastVolumeRenderer::exitOnCondition(bool condition,
                                         const std::string &message) const
  {
    if (!condition)
      return;

    emitMessage("ERROR", message);
    exit(1);
  }

} // ::ospray
