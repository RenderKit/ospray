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

#ifdef _WIN32
#  ifdef ospray_sg_EXPORTS
#    define OSPSG_INTERFACE __declspec(dllexport)
#  else
#    define OSPSG_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPSG_INTERFACE
#endif

// sg components
#include "sg/common/Node.h"
#include "sg/common/Data.h"
#include "sg/common/Transform.h"

#include "sg/camera/PerspectiveCamera.h"

#include "sg/geometry/Geometry.h"
#include "sg/geometry/Spheres.h"

#include "sg/importer/Importer.h"

#include "sg/volume/Volume.h"

// ospcommon
#include "ospcommon/FileName.h"

namespace ospray {
  namespace sg {

    using ospcommon::FileName;

    /*! \brief allows for adding semantical info to a model/scene
     graph.  \note will not do anything by itself. */
    struct Info : public sg::Node
    {
      /*! \brief returns a std::string with the c++ name of this class */
      std::string toString() const override;

      std::string permissions;
      std::string acks;
      std::string description;
    };

    struct Group : public sg::Renderable
    {
      /*! \brief returns a std::string with the c++ name of this class */
      std::string toString() const override;

      std::vector<std::shared_ptr<sg::Node>> children;
    };

    /*! a geometry node - the generic geometry node */
    struct GenericGeometry : public sg::Geometry
    {
      GenericGeometry(const std::string &type);

      /*! \brief returns a std::string with the c++ name of this class */
      std::string toString() const override;
      box3f bounds() const override;

      /*! geometry type, i.e., 'spheres', 'cylinders', 'trianglemesh', ... */
      const std::string type;
      box3f _bounds;
    };

    /*! @} */

  } // ::ospray::sg
} // ::ospray

