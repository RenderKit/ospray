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

#include "sg/geometry/StreamLines.h"
#include "sg/common/World.h"

namespace ospray {
  namespace sg {

    StreamLines::StreamLines() : Geometry("streamlines")
    {
      createChild("material", "Material");
      createChild("radius", "float", 0.01f,
                  NodeFlags::required |
                  NodeFlags::valid_min_max).setMinMax(0.f, 1e20f);
    }

    std::string StreamLines::toString() const
    {
      return "ospray::sg::StreamLines";
    }

    box3f StreamLines::computeBounds() const
    {
      box3f bounds = empty;
      if (hasChild("vertex")) {
        auto v = vertex();
        for (uint32_t i = 0; i < v->size(); i++)
          bounds.extend(v->get<vec3fa>(i));
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
    void StreamLines::setFromXML(const xml::Node &node,
                                 const unsigned char *binBasePtr)
    {
      NOT_IMPLEMENTED;
    }

    void StreamLines::preCommit(RenderContext &ctx)
    {
      auto ospGeometry = (OSPGeometry)valueAs<OSPObject>();
      if (!ospGeometry) {
        ospGeometry = ospNewGeometry("streamlines");
        setValue((OSPObject)ospGeometry);

        if (!hasChild("vertex") || !hasChild("index"))
          return;

        ospGeometry = ospNewGeometry("streamlines");
        setValue((OSPObject)ospGeometry);

        // set vertex data
        if (!vertex()->empty())
          ospSetData(ospGeometry, "vertex", vertex()->getOSP());
        if (hasChild("color") && !color()->empty())
          ospSetData(ospGeometry, "color", color()->getOSP());
        // set index data
        if (!index()->empty())
          ospSetData(ospGeometry, "index", index()->getOSP());

        child("bounds").setValue(computeBounds());
      }
    }

    void StreamLines::postCommit(RenderContext &ctx)
    {
      auto ospGeometry = (OSPGeometry)valueAs<OSPObject>();
      ospSetMaterial(ospGeometry,
                     (OSPMaterial)child("material").valueAs<OSPObject>());
      ospCommit(ospGeometry);
    }

    void StreamLines::postRender(RenderContext& ctx)
    {
      auto ospGeometry = (OSPGeometry)valueAs<OSPObject>();
      if (ospGeometry)
        ospAddGeometry(ctx.currentOSPModel, ospGeometry);
    }

    std::shared_ptr<DataBuffer> StreamLines::vertex() const
    {
      return child("vertex").nodeAs<DataBuffer>();
    }

    std::shared_ptr<DataBuffer> StreamLines::color() const
    {
      return child("color").nodeAs<DataBuffer>();
    }

    std::shared_ptr<DataBuffer> StreamLines::index() const
    {
      return child("index").nodeAs<DataBuffer>();
    }

    OSP_REGISTER_SG_NODE(StreamLines);

  } // ::ospray::sg
} // ::ospray


