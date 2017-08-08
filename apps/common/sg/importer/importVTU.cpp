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

// sg
#include "SceneGraph.h"

// vtk
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

namespace ospray {
  namespace sg {

    class TetMesh
    {
     public:
      TetMesh()
      {
        vertices = createNode("vertex", "DataVector3f")->nodeAs<DataVector3f>();
        field    = createNode("field", "DataVector1f")->nodeAs<DataVector1f>();
        tetrahedra = createNode("tets", "DataVector4i")->nodeAs<DataVector4i>();
      }

      std::shared_ptr<DataVector3f> vertices;
      std::shared_ptr<DataVector1f> field;
      std::shared_ptr<DataVector4i> tetrahedra;

      template <class TReader>
      vtkDataSet *ReadVTKFile(const char *fileName)
      {
        vtkSmartPointer<TReader> reader = vtkSmartPointer<TReader>::New();

        reader->SetFileName(fileName);
        reader->Update();

        reader->GetOutput()->Register(reader);
        return vtkDataSet::SafeDownCast(reader->GetOutput());
      }

      template <class TReader>
      bool loadVTKFile(std::string fileName)
      {
        vtkDataSet *dataSet = ReadVTKFile<TReader>(fileName.c_str());

        int numberOfCells  = dataSet->GetNumberOfCells();
        int numberOfPoints = dataSet->GetNumberOfPoints();

        double point[3];
        for (int i = 0; i < numberOfPoints; i++) {
          dataSet->GetPoint(i, point);
          vertices->push_back(vec3f(point[0], point[1], point[2]));
        }

        // Generate a report
        std::cout << "------------------------" << std::endl;
        std::cout << fileName << std::endl
                  << " contains a " << std::endl
                  << dataSet->GetClassName() << " that has " << numberOfCells
                  << " cells"
                  << " and " << numberOfPoints << " points." << std::endl;

        using CellContainer = std::map<int, int>;
        CellContainer cellMap;

        for (int i = 0; i < numberOfCells; i++) {
          // cellMap[dataSet->GetCellType(i)]++;

          vtkCell *cell = dataSet->GetCell(i);

          if (cell->GetCellType() == 10) {  // vtkTetra
            tetrahedra->push_back(vec4i(cell->GetPointId(0),
                                        cell->GetPointId(1),
                                        cell->GetPointId(2),
                                        cell->GetPointId(3)));
          }
        }

        auto it = cellMap.begin();
        while (it != cellMap.end()) {
          std::cout << "\tCell type "
                    << vtkCellTypes::GetClassNameFromTypeId(it->first)
                    << " occurs " << it->second << " times." << std::endl;
          ++it;
        }

        // Now check for point data
        vtkPointData *pd = dataSet->GetPointData();
        if (pd) {
          std::cout << " contains point data with " << pd->GetNumberOfArrays()
                    << " arrays." << std::endl;

          for (int i = 0; i < pd->GetNumberOfArrays(); i++) {
            std::cout << "\tArray " << i << " is named "
                      << (pd->GetArrayName(i) ? pd->GetArrayName(i) : "NULL")
                      << std::endl;

            vtkAbstractArray *ad = pd->GetAbstractArray(i);
            int nDataPoints      = ad->GetNumberOfValues();

            int nComponents = ad->GetNumberOfComponents();
            int nTuples     = ad->GetNumberOfTuples();

            int arrayType = ad->GetArrayType();
            std::cout << "array type = " << arrayType << "\n";
            // VTK_FLOAT

            vtkSmartPointer<vtkFloatArray> array =
                vtkFloatArray::SafeDownCast(ad);
            vtkSmartPointer<vtkDoubleArray> array_d =
                vtkDoubleArray::SafeDownCast(ad);

            std::cout << "\tArray " << i << " is named "
                      << (pd->GetArrayName(i) ? pd->GetArrayName(i) : "NULL")
                      << " and has " << nDataPoints << " points" << std::endl;

            std::cout << "field values ";
            for (int j = 0; j < nDataPoints; j++) {
              float val = array->GetValue(j);
              std::cout << val << " ";
              field->push_back(val);
            }
            std::cout << "\n";
          }
        }

        return true;
      }

      bool loadOFFFile(std::string fileName)
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
          tetrahedra->push_back(vec4i(c0, c1, c2, c3));
        }
      }

      bool loadFile(std::string fileName)
      {
        std::string extension =
            vtksys::SystemTools::GetFilenameLastExtension(fileName);

        if (extension == ".vtk") {
          loadVTKFile<vtkGenericDataObjectReader>(fileName.c_str());
        } else if (extension == ".vtu") {
          loadVTKFile<vtkXMLUnstructuredGridReader>(fileName.c_str());
        } else if (extension == ".off") {
          loadOFFFile(fileName);
        } else {
          return false;
        }

        return true;
      }

      ospcommon::box3f calcualteBounds() const
      {
        ospcommon::box3f bounds;

        for (const auto &v : vertices->v)
          bounds.extend(v);

        return bounds;
      }
    };

    void importVTU(const std::shared_ptr<Node> &world, const FileName &fileName)
    {
      std::cout << "TODO: unpack .vtu file into OSPRay volume..." << std::endl;

      TetMesh mesh;
      mesh.loadFile(fileName.str());
    }

  }  // ::ospray::sg
}  // ::ospray
