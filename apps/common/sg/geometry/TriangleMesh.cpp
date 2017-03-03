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
#include "sg/common/Integrator.h"

namespace ospray {
  namespace sg {

    OSP_REGISTER_SG_NODE(Importer);

    void TriangleMesh::init()
    {
      add(createNode("material", "Material"));
    }

    box3f TriangleMesh::getBounds() const
    {
      box3f bounds = empty;
      for (uint32_t i = 0; i < vertex->getSize(); i++)
        bounds.extend(vertex->get3f(i));
      return bounds;
    }

    //! return the bounding box of all primitives
    box3f PTMTriangleMesh::getBounds() const
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
    void TriangleMesh::setFromXML(const xml::Node *const node, const unsigned char *binBasePtr)
    {
      xml::for_each_child_of(*node,[&](const xml::Node &child){
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

    /*! 'render' the nodes */
    void TriangleMesh::render(RenderContext &ctx)
    {
      std::cout << __PRETTY_FUNCTION__ << std::endl;
      if (ospGeometry) return;

      assert(ctx.world);
      assert(ctx.world->ospModel);

      ospGeometry = ospNewGeometry("trianglemesh");
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

      ospCommit(ospGeometry);
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
    }

    /*! 'render' the nodes */
    void PTMTriangleMesh::render(RenderContext &ctx)
    {
      if (ospGeometry) return;

      assert(ctx.world);
      assert(ctx.world->ospModel);

      ospGeometry = ospNewGeometry("trianglemesh");
      // set vertex arrays
      if (vertex && vertex->notEmpty()) {
        OSPData data = vertex->getOSP();
        ospSetData(ospGeometry,"vertex",data);
      }
      if (normal && normal->notEmpty())
        ospSetData(ospGeometry,"vertex.normal",normal->getOSP());
      if (texcoord && texcoord->notEmpty())
        ospSetData(ospGeometry,"vertex.texcoord",texcoord->getOSP());
      // set index array
      if (index && index->notEmpty())
        ospSetData(ospGeometry,"index",index->getOSP());

      Triangle *triangles = (Triangle*)index->getBase();
      for(size_t i = 0; i < index->getSize(); i++) {
        materialIDs.push_back(triangles[i].materialID >> 16);
      }
      primMatIDs = ospNewData(materialIDs.size(), OSP_INT, &materialIDs[0], 0);
      ospSetData(ospGeometry,"prim.materialID",primMatIDs);

      ospCommit(ospGeometry);
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
    }

    void TriangleMesh::postCommit(RenderContext &ctx)
    {
      if (ospGeometry)
      {
        ospSetMaterial(ospGeometry,
                       (OSPMaterial)child("material")->getValue<OSPObject>());
        ospCommit(ospGeometry);
        return;
      }

      ospGeometry = ospNewGeometry("trianglemesh");
      ospModel    = ospNewModel();

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
                     (OSPMaterial)child("material")->getValue<OSPObject>());
      ospCommit(ospGeometry);
      ospAddGeometry(ctx.world->ospModel,ospGeometry);
    }

    void Importer::setChildrenModified(TimeStamp t)
    {
      Node::setChildrenModified(t);
      ospcommon::FileName file(child("fileName")->getValue<std::string>());

      if (file.str() == loadedFileName)
        return;

      std::cout << "attempting importing file: " << file.str() << std::endl;

      if (loadedFileName != "" || file.str() == "")
        return; //TODO: support dynamic re-loading, need to clear children first

      loadedFileName = "";

      if (file.ext() == "obj")
      {
        std::cout << "importing file: " << file.str() << std::endl;
        sg::importOBJ(std::static_pointer_cast<sg::World>(shared_from_this()), file);
      }
      else if (file.ext() == "ply")
      {
        std::shared_ptr<sg::World> wsg(std::dynamic_pointer_cast<sg::World>(shared_from_this()));
        sg::importPLY(wsg, file);
      }
      else if (file.ext() == "osg" || file.ext() == "osp")
      {
        std::shared_ptr<sg::World> wsg(std::dynamic_pointer_cast<sg::World>(shared_from_this()));
        sg::loadOSP(file, wsg);
        instanced = false;
      }
      else
      {
        std::cout << "unsupported file format\n";
        return;
      }

      loadedFileName = file.str();
    }

    OSP_REGISTER_SG_NODE(TriangleMesh);

  } // ::ospray::sg
} // ::ospray


