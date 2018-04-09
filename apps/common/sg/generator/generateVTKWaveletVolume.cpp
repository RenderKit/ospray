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
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/StringManip.h"
// sg
#include "../common/Data.h"
#include "Generator.h"
// vtk
#include <vtkImageData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkSmartPointer.h>

namespace ospray {
  namespace sg {

    void generateVTKWaveletVolume(const std::shared_ptr<Node> &world,
                                  const std::vector<string_pair> &params)
    {
      auto volume_node = createNode("wavelet", "StructuredVolume");

      // get generator parameters

      vec3i dims(256, 256, 256);

      for (auto &p : params) {
        if (p.first == "dimensions") {
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
        } else {
          std::cout << "WARNING: unknown spheres generator parameter '"
                    << p.first << "' with value '" << p.second << "'"
                    << std::endl;
        }
      }

      // generate volume data

      auto halfDims = dims / 2;

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

      // validate expected outputs

      std::string voxelType = imageData->GetScalarTypeAsString();

      if (voxelType != "float")
        throw std::runtime_error("wavelet not floats? got '" + voxelType + "'");

      auto dimentionality = imageData->GetDataDimension();

      if (dimentionality != 3)
        throw std::runtime_error("wavelet not 3 dimentional?");

      // import data into sg nodes

      dims = vec3i(imageData->GetDimensions()[0],
                   imageData->GetDimensions()[1],
                   imageData->GetDimensions()[2]);

      auto numVoxels = dims.product();

      auto *voxels_ptr = (float*)imageData->GetScalarPointer();

      auto voxel_data = std::make_shared<DataArray1f>(voxels_ptr, numVoxels);

      voxel_data->setName("voxelData");

      volume_node->add(voxel_data);

      // volume attributes

      volume_node->child("voxelType")  = voxelType;
      volume_node->child("dimensions") = dims;

      volume_node->createChild("stashed_vtk_source", "Node", wavelet);

      // add volume to world

      world->add(volume_node);
    }

    OSPSG_REGISTER_GENERATE_FUNCTION(generateVTKWaveletVolume, vtkWavelet);

  }  // ::ospray::sg
}  // ::ospray
