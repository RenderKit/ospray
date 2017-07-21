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

// O_LARGEFILE is a GNU extension.
#ifdef __APPLE__
#define  O_LARGEFILE  0
#endif

// sg
#include "SceneGraph.h"
#include "sg/common/Texture2D.h"
#include "sg/geometry/TriangleMesh.h"
//
#include "../3rdParty/ply.h"

namespace ospray {
  namespace sg {
    namespace ply {
      using std::string;
      using std::cout;
      using std::endl;


#define FALSE           0
#define TRUE            1

#define X               0
#define Y               1
#define Z               2

      enum {
        FACE_INDICES = 0,
        FACE_RED,
        FACE_GREEN,
        FACE_BLUE
      };

      typedef float Point[3];
      typedef float Vector[3];

      typedef struct Vertex {
        int    id;
        Point  coord;            /* coordinates of vertex */
        Point  normal;            /* normal of vertex */
        unsigned char red;
        unsigned char green;
        unsigned char blue;
        unsigned char nfaces;    /* number of face indices in list */
        int *faces;              /* face index list */
        void *other_props;       /* other properties */
      } Vertex;


      typedef struct Face {
        int id;
        unsigned char nverts;    /* number of vertex indices in list */
        int *verts;              /* vertex index list */
        unsigned char red;
        unsigned char green;
        unsigned char blue;

        void *other_props;       /* other properties */
      } Face;

      PlyProperty vert_props[] = { /* list of property information for a vertex */
        {(char*)"x", PLY_FLOAT, PLY_FLOAT, offsetof(::ospray::sg::ply::Vertex,coord[X]), 0, 0, 0, 0},
        {(char*)"y", PLY_FLOAT, PLY_FLOAT, offsetof(::ospray::sg::ply::Vertex,coord[Y]), 0, 0, 0, 0},
        {(char*)"z", PLY_FLOAT, PLY_FLOAT, offsetof(::ospray::sg::ply::Vertex,coord[Z]), 0, 0, 0, 0},
        {(char*)"nx", PLY_FLOAT, PLY_FLOAT, offsetof(::ospray::sg::ply::Vertex,normal[X]), 0, 0, 0, 0},
        {(char*)"ny", PLY_FLOAT, PLY_FLOAT, offsetof(::ospray::sg::ply::Vertex,normal[Y]), 0, 0, 0, 0},
        {(char*)"nz", PLY_FLOAT, PLY_FLOAT, offsetof(::ospray::sg::ply::Vertex,normal[Z]), 0, 0, 0, 0},
        {(char*)"diffuse_red", PLY_UCHAR, PLY_UCHAR, offsetof(::ospray::sg::ply::Vertex,red), 0, 0, 0, 0},
        {(char*)"diffuse_green", PLY_UCHAR, PLY_UCHAR, offsetof(::ospray::sg::ply::Vertex,green), 0, 0, 0, 0},
        {(char*)"diffuse_blue", PLY_UCHAR, PLY_UCHAR, offsetof(::ospray::sg::ply::Vertex,blue), 0, 0, 0, 0},
      };
      enum {
        VTX_X = 0,
        VTX_Y,
        VTX_Z,
        VTX_NX,
        VTX_NY,
        VTX_NZ,
        VTX_RED,
        VTX_GREEN,
        VTX_BLUE
      };

      PlyProperty face_props[] = { /* list of property information for a face */
        {(char*)"vertex_indices", PLY_INT, PLY_INT, offsetof(Face,verts),
         1, PLY_UCHAR, PLY_UCHAR, offsetof(Face,nverts)},
        {(char*)"red", PLY_UCHAR, PLY_UCHAR, offsetof(Face,red), 0, 0, 0, 0},
        {(char*)"green", PLY_UCHAR, PLY_UCHAR, offsetof(Face,green), 0, 0, 0, 0},
        {(char*)"blue", PLY_UCHAR, PLY_UCHAR, offsetof(Face,blue), 0, 0, 0, 0},
      };


      void readFile(const std::string &fileName, std::shared_ptr<sg::TriangleMesh> mesh)
      {
        int nprops;
        PlyProperty **plist;
        float version;
        int vertices = 0;

        int triangles = 0;

        int has_x, has_y, has_z;
        int has_face_red=0; // face colors
        int has_face_green=0; // face colors
        int has_face_blue=0; // face colors
        int has_vertex_red=0; // vertex colors
        int has_vertex_green=0; // vertex colors
        int has_vertex_blue=0; // vertex colors
        int has_nx=0, has_ny=0, has_nz=0;
        int has_fverts=0;

        PlyOtherElems *other_elements = NULL;
        PlyOtherProp *vert_other,*face_other;//,*edge_other;

        char **element_list = NULL;
        int file_type = 0;


        auto pos = createNode("vertex", "DataVector3f")->nodeAs<DataVector3f>();
        auto nor = createNode("normal", "DataVector3f")->nodeAs<DataVector3f>();
        auto idx = createNode("index", "DataVector3i")->nodeAs<DataVector3i>();

        /*** Read in the original PLY object ***/
        const char *filename = fileName.c_str();
        FILE *file;
        bool isPipe = false;

        if (strlen(filename) > 7 && !strcmp(filename+strlen(filename)-7,".ply.gz")) {
#ifdef _WIN32
          THROW_SG_ERROR("#osp:sg:ply: gzipped file not supported yet on Windows");
#else
          isPipe = true;
          char cmd[10000];
          sprintf(cmd,"/usr/bin/gunzip -c %s",filename);
          file = popen(cmd,"r");
#endif
        } else
          file = fopen(filename,"rb");

        if (!file)
          throw std::runtime_error("#osp:sg:ply: could not open '"+fileName+"'");

        int nelems = -1;
        PlyFile *ply  = ply_read (file, &nelems, &element_list);
        ply_get_info (ply, &version, &file_type);

        for (int i=0; i<nelems; i++) {
          int num_elems;

          /* get the description of the first element */
          char *elem_name = element_list[i];
          plist = ply_get_element_description (ply, elem_name, &num_elems, &nprops);

          if (equal_strings ("vertex", elem_name)) {
            /* create a vertex list to hold all the vertices */

            /* set up for getting vertex elements */
            /* verify which properties these vertices have */
            has_x = has_y = has_z = FALSE;
            has_nx = has_ny = has_nz = FALSE;

            for (int j=0; j<nprops; j++) {
              if (equal_strings("x", plist[j]->name)) {
                ply_get_property (ply, elem_name, &vert_props[VTX_X]);  /* x */
                has_x = TRUE;
              } else if (equal_strings("y", plist[j]->name)) {
                ply_get_property (ply, elem_name, &vert_props[VTX_Y]);  /* y */
                has_y = TRUE;
              } else if (equal_strings("z", plist[j]->name)) {
                ply_get_property (ply, elem_name, &vert_props[VTX_Z]);  /* z */
                has_z = TRUE;
              }

              if (equal_strings("nx", plist[j]->name)) {
                ply_get_property (ply, elem_name, &vert_props[VTX_NX]);  /* x */
                has_nx = TRUE;
              } else if (equal_strings("ny", plist[j]->name)) {
                ply_get_property (ply, elem_name, &vert_props[VTX_NY]);  /* y */
                has_ny = TRUE;
              } else if (equal_strings("nz", plist[j]->name)) {
                ply_get_property (ply, elem_name, &vert_props[VTX_NZ]);  /* z */
                has_nz = TRUE;
              } else if (equal_strings("diffuse_red", plist[j]->name)) {
                ply_get_property (ply, elem_name, &vert_props[VTX_RED]);  /* z */
                has_vertex_red = TRUE;
              } else if (equal_strings("diffuse_green", plist[j]->name)) {
                ply_get_property (ply, elem_name, &vert_props[VTX_GREEN]);  /* z */
                has_vertex_green = TRUE;
              } else if (equal_strings("diffuse_blue", plist[j]->name)) {
                ply_get_property (ply, elem_name, &vert_props[VTX_BLUE]);  /* z */
                has_vertex_blue = TRUE;
              }
            }

            vert_other = ply_get_other_properties (ply, elem_name,
                                                   offsetof(Vertex,other_props));

            /* test for necessary properties */
            if ((!has_x) || (!has_y) || (!has_z))
              {
                fprintf(stderr, "Vertices don't have x, y, and z\n");
                exit(-1);
              }

            vertices = num_elems;
            if (has_nx && has_ny && has_nz)
              nor->v.resize(vertices);
            pos->v.resize(vertices);

            /* grab all the vertex elements */
            for (int j=0; j<vertices; j++) {
              Vertex tmp;
              ply_get_element (ply, (void *) &tmp);
              pos->v[j] = vec3f(tmp.coord[0],tmp.coord[1],tmp.coord[2]);
              if (has_nx && has_ny && has_nz)
                nor->v[j] = vec3f(tmp.normal[0],tmp.normal[1],tmp.normal[2]);
            }
          } else if (equal_strings ("face", elem_name)) {

            /* create a list to hold all the face elements */
            cout << "num faces : " << (num_elems) << endl;

            /* set up for getting face elements */
            /* verify which properties these vertices have */
            has_fverts = FALSE;

            for (int j=0; j<nprops; j++) {
              if (equal_strings("vertex_indices", plist[j]->name)) {
                ply_get_property(ply, elem_name, &face_props[FACE_INDICES]);/* vertex_indices */
                has_fverts = TRUE;
              } else if (equal_strings("red", plist[j]->name)) {
                ply_get_property(ply, elem_name, &face_props[FACE_RED]);/* vertex_indices */
                has_face_red = TRUE;
              } else if (equal_strings("green", plist[j]->name)) {
                ply_get_property(ply, elem_name, &face_props[FACE_GREEN]);/* vertex_indices */
                has_face_green = TRUE;
              } else if (equal_strings("blue", plist[j]->name)) {
                ply_get_property(ply, elem_name, &face_props[FACE_BLUE]);/* vertex_indices */
                has_face_blue = TRUE;
              }
            }
            face_other = ply_get_other_properties (ply, elem_name,
                                                   offsetof(Face,other_props));


            /* test for necessary properties */
            if (!has_fverts) {
              fprintf(stderr, "Faces must have vertex indices\n");
              exit(-1);
            }

            triangles = num_elems;
            // numTrisWritten += num_elems;

            /* grab all the face elements */
            Face tmp;
            tmp.verts = NULL;
            tmp.other_props = NULL;
            for (int j=0; j<num_elems; j++)  {
              ply_get_element (ply, (void *) &tmp);
              if (!has_face_blue) {
                tmp.red = tmp.green = tmp.blue = 255;
              }
              if (tmp.nverts == 3) {
                // if (has_face_blue)
                //   material = getMaterial(tmp.red,tmp.green,tmp.blue);;

                vec3i vtx;
                vtx.x = tmp.verts[0]; //builder.addVertex(pos[tmp.verts[0]]);
                vtx.y = tmp.verts[1]; //builder.addVertex(pos[tmp.verts[1]]);
                vtx.z = tmp.verts[2]; //builder.addVertex(pos[tmp.verts[2]]);
                idx->v.push_back(vtx);
              } else {
                PRINT((int)tmp.nverts);
                FATAL("can only read ply files made up of triangles...");
              }
              free(tmp.verts);
              if (tmp.other_props) free(tmp.other_props);
            }
          }
          else {
            other_elements = ply_get_other_element (ply, elem_name, num_elems);
            ply_free_other_elements(other_elements);
          }
        }


        int num_comments;
        char **comments;
        // PlyOtherProp *vert_other,*face_other;//,*edge_other;

        int num_obj_info;
        char **obj_info;


        comments = ply_get_comments (ply, &num_comments);
        obj_info = ply_get_obj_info (ply, &num_obj_info);

        ply_close (ply);

        for (int i=0;i<ply->nelems;i++) {
          if (ply->elems[i]) continue;
          PlyElement *e = ply->elems[i];
          FREE(e->name);
          FREE(e->store_prop);
          if (e->props) {
            for (int j=0;j<e->nprops;j++) {
              PlyProperty *p = e->props[j];
              if (!e) continue;
              FREE(p->name);
              FREE(p);
            }
            FREE(e->props);
          }
          FREE(e);
        }

        free(ply->elems);

        mesh->add(idx);
        mesh->add(pos);
        if (!nor->v.empty())
          mesh->add(nor);
      }

    } // ::ospray::sg::ply

    void importPLY(std::shared_ptr<Node> &world, const FileName &fileName)
    {
      auto mesh = sg::createNode(fileName.name(),
                                 "TriangleMesh")->nodeAs<TriangleMesh>();
      ply::readFile(fileName.str(), mesh);
      world->add(mesh);
    }

  } // ::ospray::sg
} // ::ospray
