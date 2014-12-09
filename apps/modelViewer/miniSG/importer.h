#pragma once

/*! \file importer.h File format importer helper class */

// minisg stuff
#include "miniSG.h"
// stl stuff
#include <map>

namespace ospray {
  namespace miniSG {

    /*! helper class to help with properly importing triangle meshes */
    struct ImportHelper
    {
      Model *model; /*!< current model we're importing a new mesh into */
      Mesh  *mesh;  /*!< current mesh we're importing */

      /*! to tell the ID of any known position in the mesh's list of vertex positions */
      std::map<vec3f,uint32> known_positions;
      /*! to tell the ID of any known vtx normal in the mesh's list of vertex normals */
      std::map<vec3f,uint32> known_normals;
      /*! to tell the ID of any known texcoord in the mesh's list of texcoords */
      std::map<vec2f,uint32> known_texcoords;
      
      ImportHelper(Model &model, const std::string &name = "");

      /*! find given vertex and return its ID, or add if it doesn't yet exist */
      uint32 addVertex(const vec3f &position);
      /*! find given vertex and return its ID, or add if it doesn't yet exist */
      uint32 addVertex(const vec3f &position, const vec2f &texcoord);
      /*! find given vertex and return its ID, or add if it doesn't yet exist */
      uint32 addVertex(const vec3f &position, const vec3f &normal, const vec2f &texcoord);
      /*! find given vertex and return its ID, or add if it doesn't yet exist */
      uint32 addVertex(const vec3f &position, const vec3f &normal);
      /*! add new triangle to the mesh. may discard the triangle if it is degenerated. */
      void addTriangle(const miniSG::Triangle &triangle);

      /*! done with this import, add this mesh to the model */
      void finalize();
    };

  } // ::ospray::minisg
} // ::ospray
