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

#include "sg/geometry/TriangleMesh.h"
#include "sg/common/World.h"

namespace ospray {
  namespace sg {

    TriangleMesh::TriangleMesh()
      : Geometry("trianglemesh")
    {
      createChild("material", "Material");
    }

    std::string TriangleMesh::toString() const
    {
      return "ospray::sg::Geometry";
    }

    box3f TriangleMesh::bounds() const
    {
      box3f bounds = empty;
      for (uint32_t i = 0; i < vertex->getSize(); i++)
        bounds.extend(vertex->get3f(i));
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
    void TriangleMesh::setFromXML(const xml::Node &node,
                                  const unsigned char *binBasePtr)
    {
      std::string fileName = node.getProp("fileName");
      if (fileName.empty()) {
        xml::for_each_child_of(node, [&](const xml::Node &child){
            if (child.name == "vertex") {
              size_t num = std::stoll(child.getProp("num"));
              size_t ofs = std::stoll(child.getProp("ofs"));
              vertex = std::shared_ptr<DataBuffer>(new DataArray3f((vec3f*)((char*)binBasePtr+ofs),num,false));
            }
            else if (child.name == "index") {
              size_t num = std::stoll(child.getProp("num"));
              size_t ofs = std::stoll(child.getProp("ofs"));
              index = std::shared_ptr<DataBuffer>(new DataArray3i((vec3i*)((char*)binBasePtr+ofs),num,false));
            }
          });
      }
    }

    PTMTriangleMesh::PTMTriangleMesh() : TriangleMesh()
    {
    }

    box3f PTMTriangleMesh::bounds() const
    {
      box3f bounds = empty;
      for (uint32_t i = 0; i < vertex->getSize(); i++)
        bounds.extend(vertex->get3f(i));
      return bounds;
    }

    void TriangleMesh::postCommit(RenderContext &ctx)
    {
      if (ospGeometry)
      {
        ospSetMaterial(ospGeometry,
                       (OSPMaterial)child("material").valueAs<OSPObject>());
        ospCommit(ospGeometry);
        return;
      }

      if (ospGeometry)
        ospRelease(ospGeometry);
      // if (ospModel)
        // ospRelease(ospModel);
      ospGeometry = ospNewGeometry("trianglemesh");
      // ospModel    = ospNewModel();

      // set vertex data
      if (vertex && vertex->notEmpty())
        ospSetData(ospGeometry,"vertex",vertex->getOSP());
      if (normal && normal->notEmpty())
        ospSetData(ospGeometry,"vertex.normal",normal->getOSP());
      if (texcoord && texcoord->notEmpty())
        ospSetData(ospGeometry,"vertex.texcoord",texcoord->getOSP());
      // set index data
      if (index && index->notEmpty())
        ospSetData(ospGeometry,"index",index->getOSP());

      ospSetMaterial(ospGeometry,
                     (OSPMaterial)child("material").valueAs<OSPObject>());
      ospCommit(ospGeometry);
      // ospAddGeometry(ctx.world->ospModel,ospGeometry);
    }

    void TriangleMesh::postRender(RenderContext& ctx)
    {
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
    }

    OSP_REGISTER_SG_NODE(TriangleMesh);

  } // ::ospray::sg
} // ::ospray


