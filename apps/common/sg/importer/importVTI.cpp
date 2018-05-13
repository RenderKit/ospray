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

// sg
#include "SceneGraph.h"

// vtk
#include <vtkSmartPointer.h>
#include <vtkImageData.h>
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkCellTypes.h>
#include <vtkDataSet.h>
#include <vtkXMLImageDataReader.h>

namespace ospray {
  namespace sg {

    //! import VTK .vti files made up of 3d regular structured grids.
    void importVTI(const std::shared_ptr<Node> &world,
                           const FileName &fileName)
    {
      auto volumeNode = createNode("vtiVolume", "StructuredVolume");

      vtkSmartPointer<vtkXMLImageDataReader> reader =
          vtkSmartPointer<vtkXMLImageDataReader>::New();
      reader->SetFileName(fileName.str().c_str());
      reader->Update();
      reader->GetOutput()->Register(reader);

      auto output = reader->GetOutput();
      std::string voxelType = output->GetScalarTypeAsString();
      if (output->GetDataDimension() != 3)
        throw std::runtime_error("vti importer only supports 3d datasets");
      vec3i dims(output->GetDimensions()[0], output->GetDimensions()[1],
          output->GetDimensions()[2]);
      auto numVoxels = dims.product();

      std::shared_ptr<Node> voxelDataNode;
      if (voxelType == "float") {
        voxelDataNode = std::make_shared<DataArray1f>(
              (float*)output->GetScalarPointer(), numVoxels);
      } else if (voxelType == "unsigned char") {
        voxelDataNode = std::make_shared<DataArray1uc>(
              (unsigned char*)output->GetScalarPointer(), numVoxels);
      } else if (voxelType == "int") {
        voxelDataNode = std::make_shared<DataArray1i>(
              (int*)output->GetScalarPointer(), numVoxels);
      }  //TODO: support short, unsigned short
      else
        throw std::runtime_error("unsupported voxel type in vti loader: " + voxelType);
      voxelDataNode->setName("voxelData");
      volumeNode->add(voxelDataNode);
      volumeNode->child("voxelType") = voxelType;
      volumeNode->child("dimensions") = dims;

      world->add(volumeNode);
    }
  }  // ::ospray::sg
}  // ::ospray
