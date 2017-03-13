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

#undef NDEBUG

#include "sg/geometry/Spheres.h"
#include "sg/common/Integrator.h"
#include "sg/common/Data.h"
// xml parser
#include "common/xml/XML.h"

namespace ospray {
  namespace sg {

    Spheres::Sphere::Sphere(const vec3f &position, 
                            float radius, 
                            uint32_t typeID)
      : position(position), 
        radius(radius), 
        typeID(typeID) 
    {
    }

    Spheres::Spheres()
      : Geometry("spheres"), 
        ospGeometry(nullptr) 
    {
    }

    void Spheres::init()
    {
      add(createNode("data"));
      add(createNode("material", "Material"));
    }

    box3f Spheres::bounds() const
    {
      Node::Handle data = child("data");
      std::shared_ptr<DataVectorT<Sphere,OSP_RAW>> spheres 
        = std::dynamic_pointer_cast<DataVectorT<Sphere,OSP_RAW>>(data.node);
      
      box3f bounds = empty;
      for (auto &s : spheres->v)
        bounds.extend(s.bounds());
      return bounds;
    }

    //! \brief Initialize this node's value from given XML node 
    /*!
      \detailed This allows a plug-and-play concept where a XML
      file can specify all kind of nodes wihout needing to know
      their actual types: The XML parser only needs to be able to
      create a proper C++ instance of the given node type (the
      OSP_REGISTER_SG_NODE() macro will allow it to do so), and can
      tell the node to parse itself from the given XML content and
      XML children 
        
      \param node The XML node specifying this node's fields

      \param binBasePtr A pointer to an accompanying binary file (if
      existant) that contains additional binary data that the xml
      node fields may point into
    */
    void Spheres::setFromXML(const xml::Node &node,
                             const unsigned char *binBasePtr)
    {
      throw std::runtime_error("setFromXML no longer makes sense with the new scene graph design");
    }

    void Spheres::postCommit(RenderContext &ctx)
    {
      ospGeometry = ospNewGeometry("spheres");

      Node::Handle dataNode = child("data");
      std::shared_ptr<DataVectorT<Sphere,OSP_RAW>> spheres 
        = std::dynamic_pointer_cast<DataVectorT<Sphere,OSP_RAW>>(dataNode.node);
      
      OSPData data = ospNewData(spheres->v.size()*5,OSP_FLOAT,
                                spheres->v.data(),OSP_DATA_SHARED_BUFFER);
      ospSetData(ospGeometry,"spheres",data);

      ospSet1i(ospGeometry,"bytes_per_sphere",sizeof(Spheres::Sphere));
      ospSet1i(ospGeometry,"center_offset",     0*sizeof(float));
      ospSet1i(ospGeometry,"offset_radius",     3*sizeof(float));
      ospSet1i(ospGeometry,"offset_materialID", 4*sizeof(float));

      ospSetMaterial(ospGeometry,
                     (OSPMaterial)child("material")->valueAs<OSPObject>());
      ospCommit(ospGeometry);
      
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
      ospCommit(data);
    }

    OSP_REGISTER_SG_NODE(Spheres);
  }
}
