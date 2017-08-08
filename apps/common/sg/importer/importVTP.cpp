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

// sg
#include "SceneGraph.h"
#include "sg/common/Texture2D.h"
#include "sg/geometry/TriangleMesh.h"

// vtk
#include <vtkPolyDataMapper.h>
#include <vtkSmartPointer.h>
#include <vtkXMLPolyDataReader.h>

namespace ospray {
  namespace sg {

    void importVTP(const std::shared_ptr<Node> &world, const FileName &fileName)
    {
      // Read all the data from the file
      auto reader = vtkSmartPointer<vtkXMLPolyDataReader>::New();
      reader->SetFileName(fileName.c_str());
      reader->Update();

      std::cout << "TODO: unpack .vtp file into OSPRay mesh..." << std::endl;
    }

  } // ::ospray::sg
} // ::ospray
