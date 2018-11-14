// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

// sg
#include "../common/Renderable.h"
#include "../transferFunction/TransferFunction.h"

namespace ospray {
  namespace sg {

    struct OSPSG_INTERFACE Volume : public Renderable
    {
      Volume();
      virtual ~Volume() override;

      virtual std::string toString() const override;

      virtual void serialize(sg::Serialization::State &state) override;

      virtual void postCommit(RenderContext &ctx) override;

      virtual void postRender(RenderContext &ctx) override;

      OSPGeometry isosurfacesGeometry{nullptr};
    };

    /*! a plain old structured volume */
    struct OSPSG_INTERFACE StructuredVolume : public Volume
    {
      StructuredVolume();

      std::string toString() const override;

      //! return bounding box of all primitives
      box3f computeBounds() const override;

      void preCommit(RenderContext &ctx) override;
    };

    /*! a plain old structured volume */
    struct OSPSG_INTERFACE StructuredVolumeFromFile : public StructuredVolume
    {
      StructuredVolumeFromFile();

      std::string toString() const override;

      void preCommit(RenderContext &ctx) override;
      void postCommit(RenderContext &ctx) override;

      //! \brief file name of the xml doc when the node was loaded from xml
      /*! \detailed we need this to properly resolve relative file names */
      FileName fileNameOfCorrespondingXmlDoc;

      std::string fileName;

      bool fileLoaded{false};
    };

  } // ::ospray::sg
} // ::ospray
