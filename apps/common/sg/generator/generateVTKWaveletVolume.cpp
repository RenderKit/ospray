// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

// ospcommon
#include "ospcommon/containers/AlignedVector.h"
#include "ospcommon/utility/StringManip.h"
// sg
#include "../common/Data.h"
#include "Generator.h"
#include "../geometry/TriangleMesh.h"
#include "../volume/Volume.h"
// vtk
#include <vtkImageData.h>
#include <vtkMarchingCubes.h>
#include <vtkRTAnalyticSource.h>
#include <vtkSmartPointer.h>
#include <vtkCell.h>

namespace ospray {
  namespace sg {

    void generateVTKWaveletVolume(const std::shared_ptr<Node> &world,
                                  const std::vector<string_pair> &params)
    {
      auto volume_node =
        createNode("wavelet", "StructuredVolume")->nodeAs<StructuredVolume>();

      auto iso_node =
        createNode("wavelet_isosurface", "TriangleMesh")->nodeAs<TriangleMesh>();

      // get generator parameters

      vec3i dims(256, 256, 256);
      bool addSphereProbe = false;
      containers::AlignedVector<float> isoValues;
      containers::AlignedVector<vec4f> slices;

      for (auto &p : params) {
        if (p.first == "dimensions" || p.first == "dims") {
          auto string_dims = ospcommon::utility::split(p.second, 'x');
          if (string_dims.size() != 3) {
            std::cout << "WARNING: ignoring incorrect 'dimensions' parameter,"
                      << " it must be of the form 'dimensions=XxYxZ'"
                      << std::endl;
            continue;
          }

          dims = vec3i(std::atoi(string_dims[0].c_str()),
                       std::atoi(string_dims[1].c_str()),
                       std::atoi(string_dims[2].c_str()));
        } else if (p.first == "viewSlice") {
          slices.emplace_back(1.f, 0.f, 0.f, dims.x / 2.f);
        } else if (p.first == "sphereProbe" || p.first == "addProbe") {
          addSphereProbe = true;
        } else if (p.first == "isosurfaces" || p.first == "isovalues") {
          auto string_isos = ospcommon::utility::split(p.second, '/');
          for (const auto &s : string_isos)
            isoValues.push_back(std::atof(s.c_str()));
        } else {
          std::cout << "WARNING: unknown wavelet generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      const bool computeIsosurface = !isoValues.empty();
      const bool addSlices = !slices.empty();

      // generate volume data

      std::cout << "...generating wavelet volume with dims=" << dims << "..."
                << std::endl;

      const auto halfDims = dims / 2;

      vtkSmartPointer<vtkRTAnalyticSource> wavelet = vtkRTAnalyticSource::New();
      wavelet->SetWholeExtent(-halfDims.x, halfDims.x-1,
                              -halfDims.y, halfDims.y-1,
                              -halfDims.z, halfDims.z-1);

      wavelet->SetCenter(0, 0, 0);
      wavelet->SetMaximum(255);
      wavelet->SetStandardDeviation(.5);
      wavelet->SetXFreq(60);
      wavelet->SetYFreq(30);
      wavelet->SetZFreq(40);
      wavelet->SetXMag(10);
      wavelet->SetYMag(18);
      wavelet->SetZMag(5);
      wavelet->SetSubsampleRate(1);

      wavelet->Update();

      auto imageData = wavelet->GetOutput();

      // generate isosurface

      vtkSmartPointer<vtkMarchingCubes> mc =
        vtkSmartPointer<vtkMarchingCubes>::New();

      if (computeIsosurface) {
        std::cout << "...creating isosurfaces with wavelet volume data..."
                  << std::endl;

        mc->SetInputConnection(wavelet->GetOutputPort());
        mc->ComputeNormalsOn();
        mc->ComputeGradientsOn();
        mc->SetNumberOfContours(int(isoValues.size()));

        for (int i = 0; i < int(isoValues.size()); ++i)
          mc->SetValue(i, isoValues[i]);

        mc->Update();
      }

      std::cout << "...data generated, now creating scene graph objects..."
                << std::endl;

      // validate expected outputs

      std::string voxelType = imageData->GetScalarTypeAsString();

      if (voxelType != "float")
        throw std::runtime_error("wavelet not floats? got '" + voxelType + "'");

      const auto dimentionality = imageData->GetDataDimension();

      if (dimentionality != 3)
        throw std::runtime_error("wavelet not 3 dimentional?");

      if (computeIsosurface) {
        // import isosurface data into sg nodes
        auto polyData = mc->GetOutput();

        auto vertices =
              createNode("vertex", "DataVector3f")->nodeAs<DataVector3f>();
        auto indices =
              createNode("index", "DataVector3i")->nodeAs<DataVector3i>();

        int numberOfCells  = polyData->GetNumberOfCells();
        int numberOfPoints = polyData->GetNumberOfPoints();

        double point[3];
        for (int i = 0; i < numberOfPoints; i++) {
          polyData->GetPoint(i, point);
          vertices->push_back(vec3f(point[0], point[1], point[2]));
        }

        for (int i = 0; i < numberOfCells; i++) {
          vtkCell *cell = polyData->GetCell(i);

          if (cell->GetCellType() == VTK_TRIANGLE) {
            indices->push_back(vec3i(cell->GetPointId(0),
                                     cell->GetPointId(1),
                                     cell->GetPointId(2)));
          }
        }

        iso_node->add(vertices);
        iso_node->add(indices);

        // add isosurface to world
        world->add(iso_node);
      } else {
        // import volume data into sg nodes

        dims = vec3i(imageData->GetDimensions()[0],
                     imageData->GetDimensions()[1],
                     imageData->GetDimensions()[2]);

        const auto numVoxels = dims.product();

        auto *voxels_ptr = (float*)imageData->GetScalarPointer();

        auto voxel_data = std::make_shared<DataArray1f>(voxels_ptr, numVoxels);

        voxel_data->setName("voxelData");

        volume_node->add(voxel_data);

        // volume attributes

        volume_node->child("voxelType")  = voxelType;
        volume_node->child("dimensions") = dims;

        volume_node->createChild("stashed_vtk_source", "Node", wavelet);

        if (addSlices) {
          auto slices_node = createNode("slices", "Slices");
          auto slices_data = std::make_shared<DataVector4f>();
          slices_data->v = slices;
          slices_data->setName("planes");

          slices_node->add(slices_data);
          slices_node->setChild("volume", volume_node);

          // add slices to world
          world->add(slices_node);
        } else if (addSphereProbe) {
          auto probe_node = createNode("sphere_probe", "Spheres");

          auto probe_data = std::make_shared<DataVector3f>();
          probe_data->v.emplace_back(halfDims);
          probe_data->setName("spheres");

          probe_node->add(probe_data);

          probe_node->createChild("radius", "float", 1.f);
          probe_node->createChild("bytes_per_sphere", "int", int(sizeof(vec3f)));

          // add volume texture
          auto texture_node = createNode("volume_texture", "TextureVolume");
          texture_node->setChild("volume", volume_node);

          auto &default_material = probe_node->child("materialList")["default"];
          default_material.setChild("map_Kd", texture_node);

          // add probe to world
          world->add(probe_node);
        } else {
          // add volume to world
          world->add(volume_node);
        }
      }

      std::cout << "...finished!" << std::endl;
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateVTKWaveletVolume, vtkWavelet);

  }  // ::ospray::sg
}  // ::ospray
