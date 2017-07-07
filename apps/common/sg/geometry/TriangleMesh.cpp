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
      #if 0
      createChild("vertex");
      createChild("normal");
      createChild("color");
      createChild("texcoord");
      createChild("index");
      #endif
    }

    std::string TriangleMesh::toString() const
    {
      return "ospray::sg::TriangleMesh";
    }

    box3f TriangleMesh::computeBounds() const
    {
      box3f bounds = empty;
      if (hasChild("vertex")) {
        auto v = vertex();
        for (uint32_t i = 0; i < v->size(); i++)
          bounds.extend(v->get<vec3f>(i));
      }
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

              auto vertex_buf =
                std::make_shared<DataArray3f>((vec3f*)((char*)binBasePtr+ofs),
                                              num,
                                              false);
              add(vertex_buf);
            }
            else if (child.name == "index") {
              size_t num = std::stoll(child.getProp("num"));
              size_t ofs = std::stoll(child.getProp("ofs"));
              auto index_buf =
                std::make_shared<DataArray3i>((vec3i*)((char*)binBasePtr+ofs),
                                              num,
                                              false);
              add(index_buf);
            }
          });
      }
    }

    void TriangleMesh::postCommit(RenderContext &ctx)
    {
      auto ospGeometry = (OSPGeometry)valueAs<OSPObject>();

      if (ospGeometry) {
        assert((OSPMaterial)child("material").valueAs<OSPObject>());
        ospSetMaterial(ospGeometry,
                       (OSPMaterial)child("material").valueAs<OSPObject>());
        ospCommit(ospGeometry);
        return;
      }

      // NOTE(jda) - how many buffers to we minimally _have_ to have?
      if (!hasChild("vertex") || !hasChild("index"))
        return;

      ospGeometry = ospNewGeometry("trianglemesh");
      setValue((OSPObject)ospGeometry);

      // set vertex data
      if (!vertex()->empty())
        ospSetData(ospGeometry, "vertex", vertex()->getOSP());
      if (hasChild("normal") && !normal()->empty())
        ospSetData(ospGeometry, "vertex.normal", normal()->getOSP());
      if (hasChild("texcoord") && !texcoord()->empty())
        ospSetData(ospGeometry, "vertex.texcoord", texcoord()->getOSP());
      // set index data
      if (!index()->empty())
        ospSetData(ospGeometry, "index", index()->getOSP());

      assert((OSPMaterial)child("material").valueAs<OSPObject>());
      ospSetMaterial(ospGeometry,
                     (OSPMaterial)child("material").valueAs<OSPObject>());
      ospCommit(ospGeometry);
      child("bounds").setValue(computeBounds());
    }

    void TriangleMesh::postRender(RenderContext& ctx)
    {
      auto ospGeometry = (OSPGeometry)valueAs<OSPObject>();
      if (ospGeometry)
        ospAddGeometry(ctx.currentOSPModel, ospGeometry);
    }

    std::shared_ptr<DataBuffer> TriangleMesh::vertex() const
    {
      return child("vertex").nodeAs<DataBuffer>();
    }

    std::shared_ptr<DataBuffer> TriangleMesh::normal() const
    {
      return child("normal").nodeAs<DataBuffer>();
    }

    std::shared_ptr<DataBuffer> TriangleMesh::color() const
    {
      return child("color").nodeAs<DataBuffer>();
    }

    std::shared_ptr<DataBuffer> TriangleMesh::texcoord() const
    {
      return child("texcoord").nodeAs<DataBuffer>();
    }

    std::shared_ptr<DataBuffer> TriangleMesh::index() const
    {
      return child("index").nodeAs<DataBuffer>();
    }

    OSP_REGISTER_SG_NODE(TriangleMesh);

  } // ::ospray::sg
} // ::ospray


