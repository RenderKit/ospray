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
#include <vtkPointData.h>
#include <vtkUnstructuredGrid.h>
#include <vtkXMLUnstructuredGridReader.h>
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

    class TetMesh
    {
     public:

      TetMesh()
      {
        vertices =
            createNode("vertices", "DataVector3f")->nodeAs<DataVector3f>();
        field = createNode("field", "DataVector1f")->nodeAs<DataVector1f>();
        indices =
            createNode("indices", "DataVector4i")->nodeAs<DataVector4i>();
      }

      std::shared_ptr<DataVector3f> vertices;
      std::shared_ptr<DataVector1f> field;
      std::shared_ptr<DataVector4i> indices;

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
      bool loadVTKFile(const FileName &fileName)
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

          if (cell->GetCellType() == VTK_TETRA) {
            indices->push_back(vec4i(-1, -1, -1, -1));
            indices->push_back(vec4i(cell->GetPointId(0),
                                     cell->GetPointId(1),
                                     cell->GetPointId(2),
                                     cell->GetPointId(3)));
          }

          if (cell->GetCellType() == VTK_HEXAHEDRON) {
            indices->push_back(vec4i(cell->GetPointId(0),
                                     cell->GetPointId(1),
                                     cell->GetPointId(2),
                                     cell->GetPointId(3)));
            indices->push_back(vec4i(cell->GetPointId(4),
                                     cell->GetPointId(5),
                                     cell->GetPointId(6),
                                     cell->GetPointId(7)));
          }
        }

        // Now check for point data
        vtkPointData *pd = dataSet->GetPointData();
        if (pd) {
          for (int i = 0; i < pd->GetNumberOfArrays(); i++) {
            vtkAbstractArray *ad = pd->GetAbstractArray(i);
            int nDataPoints      = ad->GetNumberOfTuples()
                                   * ad->GetNumberOfComponents();

            auto array = make_vtkSP(vtkDataArray::SafeDownCast(ad));

            for (int j = 0; j < nDataPoints; j++) {
              float val = static_cast<float>(array->GetTuple1(j));
              field->push_back(val);
            }
          }
        }

        return true;
      }

      void loadOFFFile(const FileName &fileName)
      {
        ifstream in(fileName);
        int nPoints = 0, nTetrahedra = 0;

        in >> nPoints >> nTetrahedra;

        float x, y, z, val;
        for (int i = 0; i < nPoints; i++) {
          in >> x >> y >> z >> val;
          vertices->push_back(vec3f(x, y, z));
          field->push_back(val);
        }

        int c0, c1, c2, c3;
        for (int i = 0; i < nTetrahedra; i++) {
          in >> c0 >> c1 >> c2 >> c3;
          indices->push_back(vec4i(-1, -1, -1, -1));
          indices->push_back(vec4i(c0, c1, c2, c3));
        }
      }

      void loadFile(const FileName &fileName)
      {
        std::string extension = fileName.ext();

        if (extension == "vtk")
          loadVTKFile<vtkGenericDataObjectReader>(fileName.c_str());
        else if (extension == "vtu")
          loadVTKFile<vtkXMLUnstructuredGridReader>(fileName.c_str());
        else if (extension == "off")
          loadOFFFile(fileName);
      }
    };

    void importUnstructuredVolume(const std::shared_ptr<Node> &world,
                                  const FileName &fileName)
    {
      TetMesh mesh;
      mesh.loadFile(fileName);

      auto &v = world->createChild("unstructured_volume", "UnstructuredVolume");

      v.add(mesh.vertices);
      v.add(mesh.indices);
      v.add(mesh.field);
    }

  }  // ::ospray::sg
}  // ::ospray
