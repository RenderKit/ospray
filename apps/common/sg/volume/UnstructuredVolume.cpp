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

// sg
#include "UnstructuredVolume.h"
#include "../common/Data.h"
#include "../common/NodeList.h"

namespace ospray {
  namespace sg {

    UnstructuredVolume::UnstructuredVolume()
    {
      createChild("hexMethod", "string", std::string("planar"))
        .setWhiteList({std::string("planar"),
              std::string("nonplanar")});
      createChild("precomputedNormals", "bool", true);
    }

    std::string UnstructuredVolume::toString() const
    {
      return "ospray::sg::UnstructuredVolume";
    }

    void UnstructuredVolume::preCommit(RenderContext &)
    {
      auto ospVolume = valueAs<OSPVolume>();

      if (ospVolume) {
        ospCommit(ospVolume);
      } else {
        if (!hasChild("vertices"))
          throw std::runtime_error("#osp:sg UnstructuredVolume -> no 'vertices' array!");
        else if (!hasChild("indices"))
          throw std::runtime_error("#osp:sg UnstructuredVolume -> no 'indices' array!");
        else if (!hasChild("vertexFields") && !hasChild("cellFields"))
          throw std::runtime_error("#osp:sg UnstructuredVolume -> no vertex or cell field data!");

        ospVolume = ospNewVolume("unstructured_volume");

        if (!ospVolume)
          THROW_SG_ERROR("could not allocate volume");

        // unclear how to define isosurfaces for cell-valued volumes
        if (!hasChild("cellFieldName")) {
          isosurfacesGeometry = ospNewGeometry("isosurfaces");
          ospSetObject(isosurfacesGeometry, "volume", ospVolume);
        }

        setValue(ospVolume);

        auto vertices   = child("vertices").nodeAs<DataBuffer>();

        ospcommon::box3f bounds;
        for (size_t i = 0; i < vertices->size(); ++i)
          bounds.extend(vertices->get<vec3f>(i));
        child("bounds") = bounds;
      }

      if ((hasChild("cellFieldName") && child("cellFieldName").lastModified() > cellFieldTime) ||
          (hasChild("vertexFieldName") && child("vertexFieldName").lastModified() > vertexFieldTime)) {
        if (hasChild("cellFieldName"))
          cellFieldTime = child("cellFieldName").lastModified();
        if (hasChild("vertexFieldName"))
          vertexFieldTime = child("vertexFieldName").lastModified();

        std::shared_ptr<DataVector1f> field;

        std::string targetName;
        std::string fieldList;
        std::string nameList;

        if (hasChild("cellFieldName")) {
          targetName = "cellField";
          fieldList = "cellFields";
          nameList = "cellFieldName";
        } else if (hasChild("vertexFieldName")) {
          targetName = "field";
          fieldList = "vertexFields";
          nameList = "vertexFieldName";
        }

        auto fields = child(fieldList).nodeAs<NodeList<DataVector1f>>();
        auto name = child(nameList).nodeAs<Node>();
        auto whitelist = name->whitelist();
        auto idx = std::distance(whitelist.begin(),
                                 std::find(whitelist.begin(),
                                           whitelist.end(),
                                           name->valueAs<std::string>()));

        field = ((*fields)[idx]).nodeAs<DataVector1f>();
        ospSetData(ospVolume, targetName.c_str(), field->getOSP());

        auto *field_array = field->baseAs<float>();
        auto minMax = std::minmax_element(field_array,
                                          field_array + field->size());
        vec2f voxelRange(*minMax.first, *minMax.second);

        if (voxelRange.x == voxelRange.y) {
          voxelRange.x -= 1.f;
          voxelRange.y += 1.f;
        }

        child("voxelRange") = voxelRange;
        child("transferFunction")["valueRange"] = voxelRange;

        child("isosurface").setMinMax(voxelRange.x, voxelRange.y);
        float iso = child("isosurface").valueAs<float>();
        if (iso < voxelRange.x || iso > voxelRange.y)
          child("isosurface") = (voxelRange.y + voxelRange.x) / 2.f;
      }

      if (child("isosurfaceEnabled").valueAs<bool>() == true
          && isosurfacesGeometry) {
        OSPData isovaluesData = ospNewData(1, OSP_FLOAT,
                                           &child("isosurface").valueAs<float>());
        ospSetData(isosurfacesGeometry, "isovalues", isovaluesData);
        ospCommit(isosurfacesGeometry);
      }
    }

    OSP_REGISTER_SG_NODE(UnstructuredVolume);

  } // ::ospray::sg
} // ::ospray
