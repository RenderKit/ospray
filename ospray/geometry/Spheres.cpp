// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

// ospray
#include "Spheres.h"
#include "common/Data.h"
#include "common/Model.h"
#include "common/OSPCommon.h"
// ispc-generated files
#include "Spheres_ispc.h"

namespace ospray {

  Spheres::Spheres()
  {
    this->ispcEquivalent = ispc::Spheres_create(this);
  }

  std::string Spheres::toString() const
  {
    return "ospray::Spheres";
  }

  void Spheres::finalize(Model *model)
  {
    Geometry::finalize(model);

    radius            = getParam1f("radius",0.01f);
    materialID        = getParam1i("materialID",0);
    bytesPerSphere    = getParam1i("bytes_per_sphere",4*sizeof(float));
    texcoordData      = getParamData("texcoord");
    offset_center     = getParam1i("offset_center",0);
    offset_radius     = getParam1i("offset_radius",-1);
    offset_materialID = getParam1i("offset_materialID",-1);
    offset_colorID    = getParam1i("offset_colorID",-1);
    sphereData        = getParamData("spheres");
    colorData         = getParamData("color");
    colorOffset       = getParam1i("color_offset",0);

    if (colorData) {
      if (hasParam("color_format")) {
        colorFormat = static_cast<OSPDataType>(getParam1i("color_format",
                                                          OSP_UNKNOWN));
      } else {
        colorFormat = colorData->type;
      }
      if (colorFormat != OSP_FLOAT4 && colorFormat != OSP_FLOAT3
          && colorFormat != OSP_FLOAT3A && colorFormat != OSP_UCHAR4)
      {
        throw std::runtime_error("#ospray:geometry/spheres: invalid "
                                 "colorFormat specified! Must be one of: "
                                 "OSP_FLOAT4, OSP_FLOAT3, OSP_FLOAT3A or "
                                 "OSP_UCHAR4.");
      }
    } else {
      colorFormat = OSP_UNKNOWN;
    }
    colorStride = getParam1i("color_stride",
                             colorFormat == OSP_UNKNOWN ?
                             0 : sizeOf(colorFormat));


    if (sphereData.ptr == nullptr) {
      throw std::runtime_error("#ospray:geometry/spheres: no 'spheres' data "
                               "specified");
    }

    numSpheres = sphereData->numBytes / bytesPerSphere;
    postStatusMsg(2) << "#osp: creating 'spheres' geometry, #spheres = "
                     << numSpheres;

    if (numSpheres >= (1ULL << 30)) {
      throw std::runtime_error("#ospray::Spheres: too many spheres in this "
                               "sphere geometry. Consider splitting this "
                               "geometry in multiple geometries with fewer "
                               "spheres (you can still put all those "
                               "geometries into a single model, but you can't "
                               "put that many spheres into a single geometry "
                               "without causing address overflows)");
    }

    const char* spherePtr = (const char*)sphereData->data;
    bounds = empty;
    for (uint32_t i = 0; i < numSpheres; i++, spherePtr += bytesPerSphere) {
      const float r = offset_radius < 0 ?
          radius : *(const float*)(spherePtr + offset_radius);
      const vec3f center = *(const vec3f*)(spherePtr + offset_center);
      bounds.extend(box3f(center - r, center + r));
    }

    // check whether we need 64-bit addressing
    bool huge_mesh = false;
    if (colorData && colorData->numBytes > INT32_MAX)
      huge_mesh = true;
    if (texcoordData && texcoordData->numBytes > INT32_MAX)
      huge_mesh = true;

    ispc::SpheresGeometry_set(getIE(),
                              model->getIE(),
                              sphereData->data,
                              materialList ? ispcMaterialPtrs.data() : nullptr,
                              texcoordData ?
                                  (ispc::vec2f *)texcoordData->data : nullptr,
                              colorData ? colorData->data : nullptr,
                              colorOffset,
                              colorStride,
                              colorFormat,
                              numSpheres,
                              bytesPerSphere,
                              radius,
                              materialID,
                              offset_center,
                              offset_radius,
                              offset_materialID,
                              offset_colorID,
                              huge_mesh);
  }

  OSP_REGISTER_GEOMETRY(Spheres,spheres);

} // ::ospray
