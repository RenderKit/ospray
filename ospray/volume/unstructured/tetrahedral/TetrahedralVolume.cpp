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

#include "TetrahedralVolume.h"
#include "../../../common/Data.h"

// auto-generated .h file.
#include "TetrahedralVolume_ispc.h"

namespace ospray {

  std::string TetrahedralVolume::toString() const
  {
    return std::string("ospray::TetrahedralVolume");
  }

  void TetrahedralVolume::commit()
  {
    if (ispcEquivalent == nullptr)
      createEquivalentISPC();

    updateEditableParameters();
    finish();
    Volume::commit();
  }

  void TetrahedralVolume::finish()
  {
    if (!finished) {
      Volume::finish();
      finished = true;
    }
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
    NOT_IMPLEMENTED;
  }

  box4f TetrahedralVolume::getTetBBox(size_t id)
  {
    auto t = tetrahedra[id];

    box4f bbox;

    for (int i = 0; i < 4; i++) {
      const auto &v = vertices[t[i]];
      const float f = field[t[i]];
      const auto p  = vec4f(v.x, v.y, v.z, f);

      if (i == 0)
        bbox.upper = bbox.lower = p;
      else
        bbox.extend(p);
    }

    return bbox;
  }

  void TetrahedralVolume::verifyAllTetrahedraRightHanded()
  {
    // Make sure each tetrahedron is right-handed, and fix if necessary.
    for (int i = 0; i < nTetrahedra; i++) {
      auto &t        = tetrahedra[i];
      const auto &p0 = vertices[t[0]];
      const auto &p1 = vertices[t[1]];
      const auto &p2 = vertices[t[2]];
      const auto &p3 = vertices[t[3]];

      auto q0 = p1 - p0;
      auto q1 = p2 - p0;
      auto q2 = p3 - p0;

      auto norm = cross(q0, q1);

      if (dot(norm, q2) < 0)
        std::swap(t[1], t[2]);
    }
  }

  //! Create the equivalent ISPC volume container.
  void TetrahedralVolume::createEquivalentISPC()
  {
    // Create an ISPC SharedStructuredVolume object and assign
    // type-specific function pointers.
    float samplingRate = getParam1f("samplingRate", 1);

    Data *verticesData   = getParamData("vertices", nullptr);
    Data *tetrahedraData = getParamData("tetrahedra", nullptr);
    Data *fieldData      = getParamData("field", nullptr);

    if (!verticesData || !tetrahedraData || !fieldData) {
      throw std::runtime_error(
          "#osp: missing correct data arrays in "
          " TetrahedralVolume!");
    }

    nVertices   = verticesData->size();
    nTetrahedra = tetrahedraData->size();

    vec3f *verts_data = nullptr;
    vec4i *tets_data  = nullptr;
    float *vals_data  = nullptr;

    if (verticesData != nullptr) {
      vertices.resize(nVertices);
      verts_data = (vec3f *)verticesData->data;
      for (int i = 0; i < nVertices; i++)
        vertices[i] = verts_data[i];
    } else {
      nVertices = 0;
    }

    if (tetrahedraData != nullptr) {
      tets_data = (vec4i *)tetrahedraData->data;
      tetrahedra.resize(nTetrahedra);
      for (int i = 0; i < nTetrahedra; i++)
        tetrahedra[i] = tets_data[i];
    } else {
      nTetrahedra = 0;
    }

    float min_val, max_val;
    if (fieldData != nullptr) {
      field.resize(nVertices);
      vals_data = (float *)fieldData->data;
      for (int i = 0; i < nVertices; i++) {
        field[i] = vals_data[i];

        float val = vals_data[i];

        if (i == 0) {
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

    float min_x = 0, min_y = 0, min_z = 0, max_x = 0, max_y = 0, max_z = 0;
    for (int i = 0; i < nVertices; i++) {
      const auto &v = vertices[i];

      float x = v.x;
      float y = v.y;
      float z = v.z;

      if (i == 0) {
        min_x = max_x = x;
        min_y = max_y = y;
        min_z = max_z = z;
      } else {
        if (x < min_x) {
          min_x = x;
        }
        if (x > max_x) {
          max_x = x;
        }

        if (y < min_y) {
          min_y = y;
        }
        if (y > max_y) {
          max_y = y;
        }

        if (z < min_z) {
          min_z = z;
        }
        if (z > max_z) {
          max_z = z;
        }
      }
    }

    bbox.lower = vec3f(min_x, min_y, min_z);
    bbox.upper = vec3f(max_x, max_y, max_z);

    float samplingStep = 1;
    float dx           = max_x - min_x;
    float dy           = max_y - min_y;
    float dz           = max_z - min_z;
    if (dx < dy && dx < dz && dx != 0) {
      samplingStep = dx * 0.01f;
    } else if (dy < dx && dy < dz && dy != 0) {
      samplingStep = dy * 0.01f;
    } else {
      samplingStep = dz * 0.01f;
    }

    samplingStep = getParam1f("samplingStep", samplingStep);

    verifyAllTetrahedraRightHanded();

    int64 *primID     = new int64[nTetrahedra];
    box4f *primBounds = new box4f[nTetrahedra];

    for (int i = 0; i < nTetrahedra; i++) {
      primID[i] = i;
      primBounds[i] = getTetBBox(i);
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
                          (int64_t *)bvh.getItemListPtr(),
                          samplingRate,
                          samplingStep);
  }

  OSP_REGISTER_VOLUME(TetrahedralVolume, tetrahedral_volume);

}  // ::ospray