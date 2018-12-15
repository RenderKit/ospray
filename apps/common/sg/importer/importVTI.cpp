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
#include <vtkPointData.h>
#include <vtkXMLImageDataReader.h>

namespace ospray {
  namespace sg {

    std::shared_ptr<Node> importVTI_createVolumeNode(const FileName &fileName)
    {
      auto volumeNode = createNode(fileName.base()+"_volume", "StructuredVolume");

      vtkSmartPointer<vtkXMLImageDataReader> reader =
          vtkSmartPointer<vtkXMLImageDataReader>::New();
      reader->SetFileName(fileName.str().c_str());
      reader->Update();
      reader->GetOutput()->Register(reader);

      auto output = reader->GetOutput();
      std::string voxelType = output->GetPointData()->GetArray(0)->GetDataTypeAsString();
      void* voxelData = output->GetPointData()->GetArray(0)->GetVoidPointer(0);
      if (output->GetDataDimension() != 3)
        throw std::runtime_error("vti importer only supports 3d datasets");
      vec3i dims(output->GetDimensions()[0], output->GetDimensions()[1],
          output->GetDimensions()[2]);
      auto numVoxels = dims.product();

      std::cout << "vtiimporter voxelType: " << voxelType << " dimensions: " << dims.x
                << " " << dims.y << " " << dims.z
                << " number of scalars: " << output->GetNumberOfScalarComponents()
                << std::endl;

      std::shared_ptr<Node> voxelDataNode;
      if (voxelType == "float") {
        voxelDataNode = std::make_shared<DataArray1f>(
              (float*)voxelData, numVoxels);
      } else if (voxelType == "unsigned char") {
        voxelDataNode = std::make_shared<DataArray1uc>(
              (unsigned char*)voxelData, numVoxels);
        voxelType = "uchar";
      } else if (voxelType == "int") {
        voxelDataNode = std::make_shared<DataArray1i>(
              (int*)voxelData, numVoxels);
      }  //TODO: support short, unsigned short
      else
        throw std::runtime_error("unsupported voxel type in vti loader: " + voxelType);
      voxelDataNode->setName("voxelData");
      volumeNode->add(voxelDataNode);
      volumeNode->child("voxelType") = voxelType;
      volumeNode->child("dimensions") = dims;

      return volumeNode;
    }

    //! import VTK .vti files made up of 3d regular structured grids.
    void importVTI(const std::shared_ptr<Node> &world,
                           const FileName &fileName)
    {
      world->add(importVTI_createVolumeNode(fileName));
    }

    //! import multiple VTK .vti files with the same tf
    void importVTIs(const std::shared_ptr<Node> &world,
                           const std::vector<FileName> &fileNames)
    {
      auto tf = createNode("transferFunction", "TransferFunction");
      for(auto file : fileNames)
      {
        auto volume = importVTI_createVolumeNode(file);
        volume->add(tf);
        world->add(volume);
      }
    }

  }  // ::ospray::sg
}  // ::ospray
