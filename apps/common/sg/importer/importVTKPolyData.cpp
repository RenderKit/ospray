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
#include "../common/NodeList.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextra-semi"
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
#pragma clang diagnostic ignored "-Winconsistent-missing-destructor-override"

// vtk
#include <vtkSmartPointer.h>
#include <vtkCell.h>
#include <vtkCellData.h>
#include <vtkCellTypes.h>
#include <vtkDataSet.h>
#include <vtkDoubleArray.h>
#include <vtkFloatArray.h>
#include <vtkGenericDataObjectReader.h>
#include <vtkPolyData.h>
#include <vtkXMLPolyDataReader.h>
#include <vtksys/SystemTools.hxx>

#pragma clang diagnostic pop

// NOTE(jda) - This should be put in a more general place, only used here
//             right now.

template <typename T>
inline vtkSmartPointer<T> make_vtkSP(T *p)
{
  return vtkSmartPointer<T>(p);
}

namespace ospray {
  namespace sg {

    class PolyMesh
    {
     public:

      PolyMesh()
      {
        vertices =
            createNode("vertex", "DataVector3f")->nodeAs<DataVector3f>();
        indices =
            createNode("index", "DataVector3i")->nodeAs<DataVector3i>();
      }

      std::shared_ptr<DataVector3f> vertices;
      std::shared_ptr<DataVector3i> indices;

      template <class TReader>
      vtkDataSet *readVTKFile(const FileName &fileName)
      {
        vtkSmartPointer<TReader> reader = vtkSmartPointer<TReader>::New();

        reader->SetFileName(fileName.c_str());
        reader->Update();

        reader->GetOutput()->Register(reader);
        return vtkDataSet::SafeDownCast(reader->GetOutput());
      }

      template <class TReader>
      void loadVTKFile(const FileName &fileName)
      {
        vtkDataSet *dataSet = readVTKFile<TReader>(fileName);

        int numberOfCells  = dataSet->GetNumberOfCells();
        int numberOfPoints = dataSet->GetNumberOfPoints();

        double point[3];
        for (int i = 0; i < numberOfPoints; i++) {
          dataSet->GetPoint(i, point);
          vertices->push_back(vec3f(point[0], point[1], point[2]));
        }

        for (int i = 0; i < numberOfCells; i++) {
          vtkCell *cell = dataSet->GetCell(i);

          if (cell->GetCellType() == VTK_TRIANGLE) {
            indices->push_back(vec3i(cell->GetPointId(0),
                                     cell->GetPointId(1),
                                     cell->GetPointId(2)));
          }
        }
      }

      void loadFile(const FileName &fileName)
      {
        std::string extension = fileName.ext();

        if (extension == "vtp")
          loadVTKFile<vtkXMLPolyDataReader>(fileName.c_str());
      }
    };

    void importVTKPolyData(const std::shared_ptr<Node> &world,
                           const FileName &fileName)
    {
      PolyMesh mesh;
      mesh.loadFile(fileName);

      auto &v = world->createChild("vtk_triangles", "TriangleMesh");

      v.add(mesh.vertices);
      v.add(mesh.indices);
      //v.add(mesh.cellFields, "cellFields");
    }

  }  // ::ospray::sg
}  // ::ospray
