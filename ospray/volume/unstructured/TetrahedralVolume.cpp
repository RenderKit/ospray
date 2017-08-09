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

#include "../../common/Data.h"
#include "TetrahedralVolume.h"


// auto-generated .h file.
#include "TetrahedralVolume_ispc.h"

namespace ospray
{

  std::string TetrahedralVolume::toString() const
  {
    return std::string("ospray::TetrahedralVolume");
  }

  void TetrahedralVolume::commit()
  {
    // Create the equivalent ISPC volume container.

    if (ispcEquivalent == nullptr) {
      createEquivalentISPC();
    }

    updateEditableParameters();

    if (!finished) {
      finish();
      finished = true;
    }

    Volume::commit();

    rtcCommit( embree_scene );
  }

  void TetrahedralVolume::finish()
  {

    // Volume finish actions.
    Volume::finish();
  }

  int TetrahedralVolume::setRegion(const void *source_pointer,
                                   const vec3i &target_index,
                                   const vec3i &source_count)
  {
    return 0;
  }

  void TetrahedralVolume::computeSamples(float **results,
                                         const vec3f *worldCoordinates,
                                         const size_t &count)
  {

    std::cout << "cpp TetrahedralVolume::computeSamples\n";
  }

  void TetrahedralVolume::getTetBBox(size_t id, box4f & bbox)
  {
    Int4 t = tetrahedra[id];// The 4 corner indices of the tetrahedron.

    float
    & x0 = bbox.lower.x,
    & y0 = bbox.lower.y,
    & z0 = bbox.lower.z,
    & x1 = bbox.upper.x,
    & y1 = bbox.upper.y,
    & z1 = bbox.upper.z,
    & val0 = bbox.lower.w,
    & val1 = bbox.upper.w;

    for (int i = 0; i < 4; i++) {
      const Point &p = vertices[t[i]];
      float val = field[t[i]];
      if (i == 0) {
        x0 = x1 = p.x;
        y0 = y1 = p.y;
        z0 = z1 = p.z;
        val0 = val1 = val;
      } else {
        if (p.x < x0) x0 = p.x;
        if (p.x > x1) x1 = p.x;

        if (p.y < y0) y0 = p.y;
        if (p.y > y1) y1 = p.y;

        if (p.z < z0) z0 = p.z;
        if (p.z > z1) z1 = p.z;

        if (val < val0) val0 = val;
        if (val > val1) val1 = val;
      }
    }
  }

  //! Create the equivalent ISPC volume container.
  void TetrahedralVolume::createEquivalentISPC()
  {
    // Create an ISPC SharedStructuredVolume object and assign
    // type-specific function pointers.
    float samplingRate = getParam1f("samplingRate",1);

    Data *verticesData = getParamData("vertices", nullptr);
    Data *tetrahedraData = getParamData("tetrahedra", nullptr);
    Data *fieldData = getParamData("field", nullptr);

    if (!verticesData || !tetrahedraData || !fieldData) {
      throw std::runtime_error("#osp: missing correct data arrays in "
                               " TetrahedralVolume!");
    }

    nVertices = verticesData->size();
    nTetrahedra = tetrahedraData->size();

    vec3f *verts_data = nullptr;
    vec4i *tets_data = nullptr;
    float *vals_data = nullptr;

    if (verticesData != nullptr) {
      vertices.resize(nVertices);
      verts_data = (vec3f *)verticesData->data;
      for (int i=0; i<nVertices; i++) {
        vec3f & v = verts_data[i];
        Point & p = vertices[i];

        p.x = v.x;
        p.y = v.y;
        p.z = v.z;
      }
    } else {
      nVertices = 0;
    }

    if (tetrahedraData != nullptr ) {
      tets_data = (vec4i *)tetrahedraData->data;
      tetrahedra.resize(nTetrahedra);
      for (int i=0; i<nTetrahedra; i++) {
        vec4i & v = tets_data[i];
        Int4 & t = tetrahedra[i];

        t.i = v[0];
        t.j = v[1];
        t.k = v[2];
        t.w = v[3];
      }
    } else {
      nTetrahedra = 0;
    }

    float min_val, max_val;
    if (fieldData != nullptr) {
      field.resize(nVertices);
      vals_data = (float *)fieldData->data;
      for (int i=0; i<nVertices; i++) {
        field[i] = vals_data[i];

        float val = vals_data[i];

        if (i==0) {
          min_val = max_val = val;
        } else {
          if (val < min_val) {
            min_val = val;
          }
          if (val > max_val) {
            max_val = val;
          }
        }
      }
    }

    float min_x=0, min_y=0, min_z=0, max_x=0, max_y=0, max_z=0;
    for (int i=0; i<nVertices; i++) {
      const Point &v = vertices[i];
      float x = v.x;
      float y = v.y;
      float z = v.z;
      if (i == 0) {
        min_x = max_x = x;
        min_y = max_y = y;
        min_z = max_z = z;
      } else {
        if(x < min_x) {
          min_x = x;
        }
        if(x > max_x) {
          max_x = x;
        }

        if(y < min_y) {
          min_y = y;
        }
        if(y > max_y) {
          max_y = y;
        }

        if(z < min_z) {
          min_z = z;
        }
        if(z > max_z) {
          max_z = z;
        }
      }
      //std::cout << "cpp vertex " << i << ": " << x << " " << y << " " << z << "\n";
    }

    bbox.lower = vec3f(min_x, min_y, min_z);
    bbox.upper = vec3f(max_x, max_y, max_z);

    float samplingStep = 1;
    float dx = max_x - min_x;
    float dy = max_y - min_y;
    float dz = max_z - min_z;
    if (dx < dy && dx < dz && dx != 0) {
      samplingStep = dx * 0.01f;
    } else if (dy < dx && dy < dz && dy != 0) {
      samplingStep = dy * 0.01f;
    } else {
      samplingStep = dz * 0.01f;
    }

    samplingStep = getParam1f("samplingStep", samplingStep);

    // Make sure each tetrahedron is right-handed, and fix if necessary.
    for (int i = 0; i < nTetrahedra; i++) {
      Int4 & t = tetrahedra[i];
      const Point & p0 = vertices[t[0]];
      const Point & p1 = vertices[t[1]];
      const Point & p2 = vertices[t[2]];
      const Point & p3 = vertices[t[3]];

      Point q0, q1, q2;
      Sub(p1, p0, q0);
      Sub(p2, p0, q1);
      Sub(p3, p0, q2);

      Point norm;
      Cross(q0, q1, norm);

      if (Dot(norm,q2) < 0) {
        int tmp1 = t[1];
        int tmp2 = t[2];
        t[1] = tmp2;
        t[2] = tmp1;
      }
    }

    faceNormals.resize(nTetrahedra);

    // Calculate outward normal of all faces.
    for (int i=0; i<nTetrahedra; i++) {
      Int4 &t = tetrahedra[i];
      TetFaceNormal & faceNormal = faceNormals[i];

      int faces[4][3] = {{1,2,3}, {2,0,3}, {3,0,1}, {0,2,1}};
      for (int j=0; j<4; j++) {
        int t0 = t[faces[j][0]];
        int t1 = t[faces[j][1]];
        int t2 = t[faces[j][2]];

        const Point & p0 = vertices[t0];
        const Point & p1 = vertices[t1];
        const Point & p2 = vertices[t2];

        Point q0, q1;
        Sub(p1, p0, q0);
        Sub(p2, p0, q1);

        //vec3f q0 = p1 - p0;
        //vec3f q1 = p2 - p0;

        Point norm;
        Cross(q0, q1, norm);
        Normalize(norm);

        //vec3f norm = normalize(cross(q0, q1));

        faceNormal.normals[j] = norm;
      }
    }

    addTetrahedralMeshToScene(embree_scene);

    int64 *primID = new int64[nTetrahedra];
    box4f *primBounds = new box4f[nTetrahedra];
    for (int i = 0; i < nTetrahedra; i++) {
      primID[i] = i;
      getTetBBox(i, primBounds[i]);
    }
    bvh.build(primBounds, primID, nTetrahedra);
    delete[] primBounds;
    delete[] primID;

    ispcEquivalent = ispc::TetrahedralVolume_createInstance(this);

    TetrahedralVolume_set(ispcEquivalent,
                          nVertices,
                          nTetrahedra,
                          (const ispc::box3f &)bbox,
                          (const ispc::vec3f *)verts_data,
                          (const ispc::vec4i *)tets_data,
                          (const float *)vals_data,
                          bvh.rootRef,
                          bvh.getNodePtr(),
                          (int64_t*)bvh.getItemListPtr(),
                          samplingRate,
                          samplingStep);

    std::cout << "nTetrahedra = " << nTetrahedra << std::endl;
    std::cout << "nVertices = " << nVertices << std::endl;
    std::cout << "samplingRate = " << samplingRate << std::endl;
    std::cout << "samplingStep = " << samplingStep << std::endl;
  }

  float TetrahedralVolume::sample(float world_x, float world_y, float world_z) {

    Point world(world_x, world_y, world_z);

    //if(!bbox.contains(world)) {
    //    return 0;
    //}

    bool found = false;
    Point p0,p1,p2,p3,norm0,norm1,norm2,norm3;
    float d0,d1,d2,d3;
    Int4 t;
    TetFaceNormal faceNormal;



    if(1) {
      // Accelerator - get a list of possible tetrahedra.
      RTCRay ray;
      ray.dir[0] = 1;
      ray.dir[1] = 0;
      ray.dir[2] = 0;
      ray.org[0] = world_x;
      ray.org[1] = world_y;
      ray.org[2] = world_z;
      ray.tnear = 0;
      ray.tfar = 1.0e8;
      ray.primID = -1;
      ray.geomID = -1;
      rtcIntersect(embree_scene,ray);

      if(ray.primID < nTetrahedra ) {
        int i = ray.primID;

        ospray::TetrahedralVolume::BBox tet_bbox = tetBBoxes[i];
        if (!tet_bbox.Contains(world)) {
          std::cout << "Error, embree did not actually find an intersection!\n";
        }

        t = tetrahedra[i];
        p0 = vertices[t[0]];
        p1 = vertices[t[1]];
        p2 = vertices[t[2]];
        p3 = vertices[t[3]];

        faceNormal = faceNormals[i];
        norm0 = faceNormal.normals[0];
        norm1 = faceNormal.normals[1];
        norm2 = faceNormal.normals[2];
        norm3 = faceNormal.normals[3];

        // Distance from the world point to the faces.
        Point q0,q1,q2,q3;
        Sub(p1, world, q0);
        Sub(p2, world, q1);
        Sub(p3, world, q2);
        Sub(p0, world, q3);

        d0 = Dot(norm0, q0);
        d1 = Dot(norm1, q1);
        d2 = Dot(norm2, q2);
        d3 = Dot(norm3, q3);

        float smallEps = 1.0e-8;
        if(d0 > -smallEps && d1 > -smallEps && d2 > -smallEps && d3 > -smallEps) {
          found = true;
        } else {
          std::cout << "Error, embree did not actually find an intersection!\n";
        }
      }
    } else {
      for(int i=0; i<nTetrahedra && !found; i++) {
        t = tetrahedra[i];

        p0 = vertices[t[0]];
        p1 = vertices[t[1]];
        p2 = vertices[t[2]];
        p3 = vertices[t[3]];

        faceNormal = faceNormals[i];
        norm0 = faceNormal.normals[0];
        norm1 = faceNormal.normals[1];
        norm2 = faceNormal.normals[2];
        norm3 = faceNormal.normals[3];

        // Distance from the world point to the faces.
        Point q0,q1,q2,q3;
        Sub(p1, world, q0);
        Sub(p2, world, q1);
        Sub(p3, world, q2);
        Sub(p0, world, q3);

        d0 = Dot(norm0, q0);
        d1 = Dot(norm1, q1);
        d2 = Dot(norm2, q2);
        d3 = Dot(norm3, q3);

        if(d0 > 0 && d1 > 0 && d2 > 0 && d3 > 0) {
          found = true;
        }
      }
    }

    if(!found) {
      return 0;
    }

    // Interpolate the field value at the world position using the values at the corners.

    // Distance of tetrahedron corners to their opposite faces.
    Point q0,q1,q2,q3;
    Sub(p1, p0, q0);
    Sub(p2, p1, q1);
    Sub(p3, p2, q2);
    Sub(p0, p3, q3);
    float h0 = Dot(norm0, q0);
    float h1 = Dot(norm1, q1);
    float h2 = Dot(norm2, q2);
    float h3 = Dot(norm3, q3);

    // Local coordinates = ratio of distances.
    float z0 = d0/h0;
    float z1 = d1/h1;
    float z2 = d2/h2;
    float z3 = d3/h3;

    // The sum of the location coordinates should add up to approximately 1.
    float z_total = z0 + z1 + z2 + z3;
    assert((0.9) < z_total && z_total < (1.1));

    // Field/attribute values at the tetrahedron corners.
    float v0 = field[t[0]];
    float v1 = field[t[1]];
    float v2 = field[t[2]];
    float v3 = field[t[3]];

    // Interpolated field/attribute value at the world position.
    return (z0*v0 + z1*v1 + z2*v2 + z3*v3);
  }

  void TetrahedralVolume::addTetrahedralMeshToScene (RTCScene scene)
  {
    tetBBoxes.resize(nTetrahedra);
    for(int i=0; i<nTetrahedra; i++) {
      addTetBBox(i);
    }

    //int geomID = rtcNewUserGeometry3(scene, RTC_GEOMETRY_STATIC, 1, 1);
    geomID = rtcNewUserGeometry(scene, nTetrahedra);

    rtcSetUserData(scene, geomID, this);
    rtcSetBoundsFunction(scene, geomID, (RTCBoundsFunc)&tetBoundsFunc);
    //rtcSetBoundsFunction3(scene, geomID, tetBoundsFunc3, this);
    rtcSetIntersectFunction(scene, geomID, tetIntersectFunc);
    rtcSetOccludedFunction(scene, geomID, tetOccludedFunc);
  }

  OSP_REGISTER_VOLUME(TetrahedralVolume, tetrahedral_volume);

} // ::ospray

void tetBoundsFunc (void *userData, size_t id, RTCBounds & bounds_o) {
  ospray::TetrahedralVolume * volume = (ospray::TetrahedralVolume *)userData;
  const ospray::TetrahedralVolume::BBox & bbox = volume->tetBBoxes[id];

  bounds_o.lower_x = bbox.min_x;
  bounds_o.lower_y = bbox.min_y;
  bounds_o.lower_z = bbox.min_z;
  bounds_o.upper_x = bbox.max_x;
  bounds_o.upper_y = bbox.max_y;
  bounds_o.upper_z = bbox.max_z;
}

void tetBoundsFunc3(void *userData, size_t id, size_t timeStep, RTCBounds & bounds_o) {
  ospray::TetrahedralVolume * volume = (ospray::TetrahedralVolume *)userData;
  /*
   ospray::vec4i &t = volume->tetrahedra[id];
   ospray::vec3f p0 = volume->vertices[t[0]];
   ospray::vec3f p1 = volume->vertices[t[1]];
   ospray::vec3f p2 = volume->vertices[t[2]];
   ospray::vec3f p3 = volume->vertices[t[3]];

   ospray::box3f bbox;
   bbox.extend(p0);
   bbox.extend(p1);
   bbox.extend(p2);
   bbox.extend(p3);

   bounds_o.lower_x = bbox.lower.x;
   bounds_o.lower_y = bbox.lower.y;
   bounds_o.lower_z = bbox.lower.z;
   bounds_o.upper_x = bbox.upper.x;
   bounds_o.upper_y = bbox.upper.y;
   bounds_o.upper_z = bbox.upper.z;
   */
}

void tetIntersectFunc(void *userData, RTCRay & ray, size_t id) {
  ospray::TetrahedralVolume * volume = (ospray::TetrahedralVolume *)userData;
  ospray::TetrahedralVolume::Point p0,p1,p2,p3,norm0,norm1,norm2,norm3;
  float d0,d1,d2,d3;

  ospray::TetrahedralVolume::Point world(ray.org[0],ray.org[1],ray.org[2]);
  ospray::TetrahedralVolume::Int4 & t = volume->tetrahedra[id];
  ospray::TetrahedralVolume::TetFaceNormal & faceNormal = volume->faceNormals[id];

  ospray::TetrahedralVolume::BBox bbox = volume->tetBBoxes[id];
  if (!bbox.Contains(world)) {
    return;
  }

  p0 = volume->vertices[t[0]];
  p1 = volume->vertices[t[1]];
  p2 = volume->vertices[t[2]];
  p3 = volume->vertices[t[3]];

  norm0 = faceNormal.normals[0];
  norm1 = faceNormal.normals[1];
  norm2 = faceNormal.normals[2];
  norm3 = faceNormal.normals[3];

  // Distance from the world point to the faces.
  ospray::TetrahedralVolume::Point q0,q1,q2,q3;
  ospray::TetrahedralVolume::Sub(p1, world, q0);
  ospray::TetrahedralVolume::Sub(p2, world, q1);
  ospray::TetrahedralVolume::Sub(p3, world, q2);
  ospray::TetrahedralVolume::Sub(p0, world, q3);
  d0 = ospray::TetrahedralVolume::Dot(norm0, q0);
  d1 = ospray::TetrahedralVolume::Dot(norm1, q1);
  d2 = ospray::TetrahedralVolume::Dot(norm2, q2);
  d3 = ospray::TetrahedralVolume::Dot(norm3, q3);

  if (d0 > 0 && d1 > 0 && d2 > 0 && d3 > 0) {
    ray.primID = id;
    ray.geomID = volume->geomID;
  }

  // std::cout << "tetIntersectFunc\n";
}

void tetOccludedFunc(void* userData, RTCRay& ray, size_t item) {

  std::cout << "tetOccludedFunc\n";
}
