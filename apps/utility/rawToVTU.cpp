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

#include "rawToVTU.h"

namespace ospray {
    namespace vtu {
        void makeVTU(const std::shared_ptr<Array3D<float>>& _in,
                const std::string primType,
                const range1f& threshold,
                PointsData& points,
                CellsData& cells)
        {
            std::shared_ptr<Array3D<float>> in = _in;

            int verticesCubes[][8]  = {{0, 1, 3, 2, 4, 5, 7, 6}};
            int verticesTets[][4]   = {{0, 1, 3, 5}, {3, 2, 0, 6}, {5, 4, 6, 0}, {6, 7, 5, 3}, {0, 3, 6, 5}};  // can also be 6 [mirror] identical tets per cube
            int verticesWedges[][6] = {{0, 1, 3, 4, 5, 7}, {3, 2, 0, 7, 6, 4}};

            CellType cellType = (primType == "cubes"  ? CellType::Hexa :
                    primType == "tets"   ? CellType::Tetra :
                    primType == "wedges" ? CellType::Wedge :
                    CellType::unknown);

            unsigned int cellsPerMacroCell, vertsPerCell;
            int* vertIndices;
            switch (cellType) {
                case CellType::Hexa:  cellsPerMacroCell = 1; vertsPerCell = 8; vertIndices = &verticesCubes[0][0];  break;
                case CellType::Tetra: cellsPerMacroCell = 5; vertsPerCell = 4; vertIndices = &verticesTets[0][0];   break;
                case CellType::Wedge: cellsPerMacroCell = 2; vertsPerCell = 6; vertIndices = &verticesWedges[0][0]; break;
                default:              cellsPerMacroCell = 0; vertsPerCell = 0; vertIndices = nullptr; break;
            }

            const vec3i pointsTotal = in->size();
            const vec3i macroCellsTotal = in->size() - vec3i(1);
            points.resize(pointsTotal.product());
            cells.resize(cellsPerMacroCell * macroCellsTotal.product(), vertsPerCell);
            std::vector<int64_t> pointRefs;
            pointRefs.resize(pointsTotal.product());

            // compute the entire points array and cells; record the reference counts from cells to points
            //Progress progress("progress", pointsTotal.product(), 1.f);
            ospcommon::tasking::serial_for(pointsTotal.product(), [&](int pointIdx) {
                    vec3i coords(pointIdx % pointsTotal.x, (pointIdx / pointsTotal.x) % pointsTotal.y, (pointIdx / pointsTotal.x / pointsTotal.y));
                    points.samples[pointIdx] = in->get(coords);
                    points.coords[pointIdx] = coords;

                    // create cells
                    if (coords.x < macroCellsTotal.x && coords.y < macroCellsTotal.y && coords.z < macroCellsTotal.z) {
                    for (unsigned int cellInMacroCellIdx = 0; cellInMacroCellIdx < cellsPerMacroCell; cellInMacroCellIdx++) {
#if USE_THRESHOLD
                    float sum = 0.f;
                    for (unsigned int cellVertexIdx = 0; cellVertexIdx < vertsPerCell; cellVertexIdx++) {
                    int vertIdx = vertIndices[cellInMacroCellIdx*vertsPerCell + cellVertexIdx];
                    sum += in->get(coords + vec3i((vertIdx&1) >> 0, (vertIdx&2) >> 1, (vertIdx&4) >> 2));
                    }
                    sum /= vertsPerCell;

                    if (threshold.contains(sum)) {
#endif
                    int64_t cellIdx = cellsPerMacroCell * (coords.x + coords.y*macroCellsTotal.x + coords.z*macroCellsTotal.x*macroCellsTotal.y) + cellInMacroCellIdx;
                    for (unsigned int cellVertexIdx = 0; cellVertexIdx < vertsPerCell; cellVertexIdx++) {
                    int vertIdx = vertIndices[cellInMacroCellIdx*vertsPerCell + cellVertexIdx];
                    cells.connectivity[vertsPerCell*cellIdx + cellVertexIdx] = pointIdx + ((vertIdx&1) >> 0)*1 + ((vertIdx&2) >> 1)*pointsTotal.x + ((vertIdx&4) >> 2)*pointsTotal.y*pointsTotal.x;
                    }
                    cells.offsets[cellIdx] = vertsPerCell * (cellIdx+1);
                    cells.types[cellIdx] = cellType;
                    //numWritten++;
#if USE_THRESHOLD
                    for (unsigned int cellVertexIdx = 0; cellVertexIdx < vertsPerCell; cellVertexIdx++)
                        pointRefs[cells.connectivity[vertsPerCell*cellIdx + cellVertexIdx]]++;
                    }
                    //else
                    //numRemoved++;
#endif
                    }
                    }

                    //progress.ping();
            });

#if USE_THRESHOLD
            int64_t i_without_gaps;
            // compact the points array
            i_without_gaps = 0;
            for (int64_t i = 0; i < pointsTotal.product(); ) {
                for (; i < pointsTotal.product() && pointRefs[i] != 0; i++, i_without_gaps++) {
                    points.samples[i_without_gaps] = points.samples[i];
                    points.samplesRange.extend(points.samples[i_without_gaps]);
                    points.coords[i_without_gaps] = points.coords[i];
                    points.coordsRange.extend(points.coords[i_without_gaps]);
                    pointRefs[i] = i_without_gaps;
                }
                for (; i < pointsTotal.product() && pointRefs[i] == 0; i++)
                    ;
            }
            points.resize(i_without_gaps);

            // update the cell references to the just compacted points and compact it as well
            i_without_gaps = 0;
            for (int64_t i = 0; i < cellsPerMacroCell * macroCellsTotal.product(); ) {
                for (; i < cellsPerMacroCell * macroCellsTotal.product() && cells.types[i] == cellType; i++, i_without_gaps++) {
                    for (unsigned int cellVertexIdx = 0; cellVertexIdx < vertsPerCell; cellVertexIdx++)
                        cells.connectivity[vertsPerCell*i_without_gaps + cellVertexIdx] = pointRefs[cells.connectivity[vertsPerCell*i + cellVertexIdx]];
                    cells.offsets[i_without_gaps] = vertsPerCell * (i_without_gaps+1);
                    cells.types[i_without_gaps] = cells.types[i];
                }
                for (; i < cellsPerMacroCell * macroCellsTotal.product() && cells.types[i] != cellType; i++)
                    ;
            }
            cells.resize(i_without_gaps, vertsPerCell);
#else
            for (int64_t i = 0; i < pointsTotal.product(); i++) {
                points.samplesRange.extend(points.samples[i]);
                points.coordsRange.extend(points.coords[i]);
            }
#endif
        }

        template <class T> void dumpVector(const std::vector<T> v, FILE* out) {
            uint64_t len = v.size() * sizeof(v[0]);
            fwrite(&len, sizeof(len), 1, out);
            fwrite(v.data(), sizeof(v[0]), v.size(), out);
        }

        void outputVTU(const std::string& fileName,
                const PointsData& points,
                const CellsData& cells)
        {
            uint64_t offset = 0;
            uint64_t len;

            std::ofstream vtuStructure(fileName);

            using namespace std;

            vtuStructure << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt64\">" << endl;
            vtuStructure << "  <UnstructuredGrid>" << endl;
            vtuStructure << "    <Piece NumberOfPoints=\"" << points.samples.size() << "\" NumberOfCells=\"" << cells.offsets.size() << "\">" << endl;

            vtuStructure << "      <PointData Scalars=\"ImageFile\">" << endl;
            vtuStructure << "        <DataArray type=\"Float32\" Name=\"ImageFile\" format=\"appended\" RangeMin=\"" << points.samplesRange.lower << "\" RangeMax=\"" << points.samplesRange.upper << "\" offset=\"" << offset << "\">" << endl;
            vtuStructure << "        </DataArray>" << endl;
            offset += points.samples.size() * sizeof(points.samples[0]) + sizeof(len);
            vtuStructure << "      </PointData>" << endl;

            vtuStructure << "      <CellData>" << endl;
            offset += 0;
            vtuStructure << "      </CellData>" << endl;

            vtuStructure << "      <Points>" << endl;
            vtuStructure << "        <DataArray type=\"Float32\" Name=\"Points\" NumberOfComponents=\"3\" format=\"appended\" RangeMin=\"" << 0 << "\" RangeMax=\"" << ospcommon::length(points.coordsRange.size()) << "\" offset=\"" << offset << "\">" << endl;
            vtuStructure << "          <InformationKey name=\"L2_NORM_FINITE_RANGE\" location=\"vtkDataArray\" length=\"2\">" << endl;
            vtuStructure << "            <Value index=\"0\">" << endl;
            vtuStructure << "              " << 0 << endl;
            vtuStructure << "            </Value>" << endl;
            vtuStructure << "            <Value index=\"1\">" << endl;
            vtuStructure << "              " << ospcommon::length(points.coordsRange.size()) << endl;
            vtuStructure << "            </Value>" << endl;
            vtuStructure << "          </InformationKey>" << endl;
            vtuStructure << "          <InformationKey name=\"L2_NORM_RANGE\" location=\"vtkDataArray\" length=\"2\">" << endl;
            vtuStructure << "            <Value index=\"0\">" << endl;
            vtuStructure << "              " << 0 << endl;
            vtuStructure << "            </Value>" << endl;
            vtuStructure << "            <Value index=\"1\">" << endl;
            vtuStructure << "              " << ospcommon::length(points.coordsRange.size()) << endl;
            vtuStructure << "            </Value>" << endl;
            vtuStructure << "          </InformationKey>" << endl;
            vtuStructure << "        </DataArray>" << endl;
            offset += points.coords.size() * sizeof(points.coords[0]) + sizeof(len);
            vtuStructure << "      </Points>" << endl;

            vtuStructure << "      <Cells>" << endl;
            vtuStructure << "        <DataArray type=\"Int64\" Name=\"connectivity\" format=\"appended\" RangeMin=\"\" RangeMax=\"\" offset=\"" << offset << "\"/>" << endl;
            offset += cells.connectivity.size() * sizeof(cells.connectivity[0]) + sizeof(len);
            vtuStructure << "        <DataArray type=\"Int64\" Name=\"offsets\" format=\"appended\" RangeMin=\"\" RangeMax=\"\" offset=\"" << offset << "\"/>" << endl;
            offset += cells.offsets.size() * sizeof(cells.offsets[0]) + sizeof(len);
            vtuStructure << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"appended\" RangeMin=\"\" RangeMax=\"\" offset=\"" << offset << "\"/>" << endl;
            offset += cells.types.size() * sizeof(cells.types[0]) + sizeof(len);
            vtuStructure << "      </Cells>" << endl;

            vtuStructure << "    </Piece>" << endl;
            vtuStructure << "  </UnstructuredGrid>" << endl;
            vtuStructure << "  <AppendedData encoding=\"raw\">" << endl;
            vtuStructure << "  _";

            vtuStructure.close();

            FILE* vtuData = fopen(fileName.c_str(), "ab");
            if (!vtuData)
                throw std::runtime_error("could not open data output file!");
            dumpVector(points.samples, vtuData);
            dumpVector(points.coords, vtuData);
            dumpVector(cells.connectivity, vtuData);
            dumpVector(cells.offsets, vtuData);
            dumpVector(cells.types, vtuData);
            fclose(vtuData);

            vtuStructure.open(fileName, std::ofstream::out | std::ofstream::app);
            vtuStructure << "  </AppendedData>" << endl;
            vtuStructure << "</VTKFile>" << endl;
        }

    }  // namespace vtu
}  // namespace ospray

#pragma clang diagnostic pop
