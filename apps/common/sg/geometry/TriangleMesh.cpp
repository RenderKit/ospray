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

    TriangleMesh::TriangleMesh() : Geometry("trianglemesh")
    {
      createChild("materialList", "MaterialList");
    }

    std::string TriangleMesh::toString() const
    {
      return "ospray::sg::TriangleMesh";
    }

    box3f TriangleMesh::bounds() const
    {
      box3f bounds = empty;
      if (hasChild("vertex")) {
        auto v = child("vertex").nodeAs<DataBuffer>();
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
    void TriangleMesh::setFromXML(const xml::Node &, const unsigned char *)
    {
      NOT_IMPLEMENTED;
    }

    void TriangleMesh::preCommit(RenderContext &ctx)
    {
      // NOTE(jda) - how many buffers to we minimally _have_ to have?
      if (!hasChild("vertex") || !hasChild("index"))
        throw std::runtime_error("#osp.sg - error, invalid TriangleMesh!");

      Geometry::preCommit(ctx);
    }

    void TriangleMesh::postCommit(RenderContext &ctx)
    {
      auto materialListNode = child("materialList").nodeAs<MaterialList>();
      const auto &materialList = materialListNode->nodes;
      if (!materialList.empty()) {
        std::vector<OSPObject> mats;
        for (auto mat : materialList) {
          auto m = mat->valueAs<OSPObject>();
          if (m)
            mats.push_back(m);
        }
        auto ospMaterialList = ospNewData(mats.size(), OSP_OBJECT, mats.data());
        ospCommit(ospMaterialList);
        ospSetData(valueAs<OSPObject>(), "materialList", ospMaterialList);
      }

      Geometry::postCommit(ctx);
    }

    OSP_REGISTER_SG_NODE(TriangleMesh);

  } // ::ospray::sg
} // ::ospray


