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

#include "Volume.h"

namespace ospray {
  namespace sg {

    // =======================================================
    // base volume class
    // =======================================================

    /*! \brief returns a std::string with the c++ name of this class */
    std::string Volume::toString() const
    { return "ospray::sg::Volume"; }
    
    // =======================================================
    // structured volume class
    // =======================================================

    /*! \brief returns a std::string with the c++ name of this class */
    std::string StructuredVolume::toString() const
    { return "ospray::sg::StructuredVolume"; }
    
    //! return bounding box of all primitives
    box3f StructuredVolume::getBounds()
    { return box3f(vec3f(0.f),vec3f(getDimensions())); }

      //! \brief Initialize this node's value from given XML node 
    void StructuredVolume::setFromXML(const xml::Node *const node, const unsigned char *binBasePtr)
    {
      for (int paramID=0;paramID < node->child.size();paramID++) {
        const std::string param = node->child[paramID]->name;
        const std::string content = node->child[paramID]->content;
        if (param == "dimensions") {
          PING;
          PRINT(content);
          setDimensions(parseVec3i(content));
        } else
          throw std::runtime_error("unknown parameter '"+param+"' in ProceduralTestVolume");
      }
      std::cout << "#osp:sg: created StructuredVolume from XML file, dimensions = " << getDimensions() << std::endl;
    }
    
    /*! \brief 'render' the object to ospray */
    void StructuredVolume::render(RenderContext &ctx)
    {
      NOTIMPLEMENTED;
    }

    OSP_REGISTER_SG_NODE(StructuredVolume);

    // =======================================================
    // procedural test volume class
    // =======================================================

    //! \brief constructor
    ProceduralTestVolume::ProceduralTestVolume()
      : dimensions(256), volume(NULL)
    {
      coeff[0] = 0.;
      coeff[1] = 13;
      coeff[2] = 17;
      coeff[3] = 23;
      coeff[4] = 5*7;
      coeff[5] = 11;
      coeff[6] = 19;
      coeff[7] = 0.;
    }

    /*! \brief returns a std::string with the c++ name of this class */
    std::string ProceduralTestVolume::toString() const
    { return "ospray::sg::ProceduralTestVolume"; }
    
    //! return bounding box of all primitives
    box3f ProceduralTestVolume::getBounds() 
    { return box3f(vec3f(0.f),vec3f(1.f)); };
    
    /*! \brief 'render' the object to ospray */
    void ProceduralTestVolume::render(RenderContext &ctx) 
    {
      if (volume) return;

      if (!transferFunction) 
        setTransferFunction(new TransferFunction);
      transferFunction->render(ctx);

      volume = ospNewVolume("block_bricked_volume");
      ospSetString(volume,"voxelType","float");
      ospSetObject(volume,"transferFunction",transferFunction->getOSPHandle());
      ospCommit(volume);
      PRINT(volume);
    };

    //! \brief Initialize this node's value from given XML node 
    void ProceduralTestVolume::setFromXML(const xml::Node *const node, const unsigned char *binBasePtr)
    {
      for (int paramID=0;paramID < node->child.size();paramID++) {
        const std::string param = node->child[paramID]->name;
        if (param == "dimensions") {
          PING;
        } else
          throw std::runtime_error("unknown parameter '"+param+"' in ProceduralTestVolume");
      }
      std::cout << "#osp:sg: created ProceduralTestVolume from XML file, dimensions = " << getDimensions() << std::endl;
    }

    OSP_REGISTER_SG_NODE(ProceduralTestVolume);

  } // ::ospray::sg
} // ::ospray

