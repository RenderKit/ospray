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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wundefined-func-template"

#include "ospcommon/array3D/Array3D.h"
#include "ospcommon/box.h"
#include "ospcommon/tasking/parallel_for.h"
// std
#include <mutex>

#define USE_THRESHOLD 1

namespace ospray {
  namespace vtu {
    using namespace ospcommon::array3D;
    using namespace ospcommon;
    using namespace std;

    enum CellType {
      Tetra = 0x0a,
      Hexa  = 0x0c,
      Wedge = 0x0d
    };

    struct PointsData {
      void resize(uint64_t count) {
        samples.resize(count);
        coords.resize(count);
      }

      std::vector<float> samples;
      range1f samplesRange;
      std::vector<vec3f> coords;
      box3f coordsRange;
    };

    struct CellsData {
      void resize(uint64_t count) {
        connectivity.resize(2 * count);
        offsets.resize(count);
        types.resize(count);
      }

      std::vector<vec4ul> connectivity;
      std::vector<uint64_t> offsets;
      std::vector<uint8_t> types;
    };

    static FILE* dataOut = nullptr;

    static size_t numWritten = 0;
    static size_t numRemoved = 0;

    struct Progress
    {
      Progress(const char *message, size_t numTotal, float pingInterval = 10.f)
          : numTotal(numTotal),
            pingInterval(pingInterval),
            message(message)
      {
      }

      void ping()
      {
        std::lock_guard<std::mutex> lock(mutex);
        double t = getSysTime();
        numDone++;
        if (t - lastPingTime > pingInterval || numDone == numTotal) {
          float pct = float(numDone * 100.f / numTotal);
          if (numRemoved + numWritten > 0)
            printf("%s: %li/%li (%.3f%%), removed %li/%li (%.2f%%)\n",
                   message.c_str(),
                   numDone,
                   numTotal,
                   pct,
                   numRemoved,
                   numRemoved + numWritten,
                   numRemoved * 100.f / (numRemoved + numWritten));
          else
            printf("%s: %li/%li (%.3f%%)\n",
                   message.c_str(),
                   numDone,
                   numTotal,
                   pct);
          lastPingTime = t;
        }
      }

      size_t numDone {0};
      size_t numTotal;
      float pingInterval;
      std::mutex mutex;
      std::string message;
      double lastPingTime {-1.0};
    };

    void makeVTU(const std::shared_ptr<Array3D<float>>& _in,
                 const range1f& threshold,
                 PointsData& points,
                 CellsData& cells)
    {
      std::shared_ptr<Array3D<float>> in = _in;

      const vec3i pointsTotal = in->size();
      const vec3i cellsTotal = in->size() - vec3i(1);
      points.resize(pointsTotal.product());
      cells.resize(cellsTotal.product());
      vector<int64_t> pointRefs;
      pointRefs.resize(pointsTotal.product());

      // compute the entire points array and cells; record the reference counts from cells to points
      Progress progress("progress", pointsTotal.product(), 1.f);
      ospcommon::tasking::serial_for(pointsTotal.product(), [&](int pointIdx) {
        vec3i coords(pointIdx % pointsTotal.x, (pointIdx / pointsTotal.x) % pointsTotal.y, (pointIdx / pointsTotal.x / pointsTotal.y));
        points.samples[pointIdx] = in->get(coords);
        points.coords[pointIdx] = coords;

        // create cells
        if (coords.x < cellsTotal.x && coords.y < cellsTotal.y && coords.z < cellsTotal.z) {
#if USE_THRESHOLD
          float sum = 0.f;
          for (unsigned int cubeVertex = 0; cubeVertex < 8; cubeVertex++)
            sum += in->get(coords + vec3i((cubeVertex&1) >> 0, (cubeVertex&2) >> 1, (cubeVertex&4) >> 2));
          sum /= 8;

          if (threshold.contains(sum)) {
#endif
            vec4ul baseLower{pointIdx,
                             pointIdx + 1,
                             pointIdx + 1 + pointsTotal.x,
                             pointIdx +     pointsTotal.x};
            vec4ul baseUpper{pointIdx +                     pointsTotal.y*pointsTotal.x,
                             pointIdx + 1 +                 pointsTotal.y*pointsTotal.x,
                             pointIdx + 1 + pointsTotal.x + pointsTotal.y*pointsTotal.x,
                             pointIdx +     pointsTotal.x + pointsTotal.y*pointsTotal.x};
            int64_t cellIdx = coords.x + coords.y*cellsTotal.x + coords.z*cellsTotal.x*cellsTotal.y;
            cells.connectivity[2*cellIdx]   = baseLower;
            cells.connectivity[2*cellIdx+1] = baseUpper;
            cells.offsets[cellIdx] = 2*(cellIdx+1) * sizeof(cells.connectivity[0])/sizeof(cells.connectivity[0][0]);
            cells.types[cellIdx] = CellType::Hexa;
            numWritten++;
#if USE_THRESHOLD
            for (unsigned int cubeVertex = 0; cubeVertex < 4; cubeVertex++) {
              pointRefs[baseLower[cubeVertex]]++;
              pointRefs[baseUpper[cubeVertex]]++;
            }
          }
          else
            numRemoved++;
#endif
        }
        progress.ping();
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
      for (int64_t i = 0; i < cellsTotal.product(); ) {
        for (; i < cellsTotal.product() && cells.types[i] == CellType::Hexa; i++, i_without_gaps++) {
          for (int j = 0; j < 4; j++) {
            cells.connectivity[2*i_without_gaps][j]   = pointRefs[cells.connectivity[2*i][j]];
            cells.connectivity[2*i_without_gaps+1][j] = pointRefs[cells.connectivity[2*i+1][j]];
          }
          cells.offsets[i_without_gaps] = 2*(i_without_gaps+1) * sizeof(cells.connectivity[0])/sizeof(cells.connectivity[0][0]);
          cells.types[i_without_gaps] = cells.types[i];
        }
        for (; i < cellsTotal.product() && cells.types[i] != CellType::Hexa; i++)
          ;
      }
      cells.resize(i_without_gaps);
#else
      for (int64_t i = 0; i < pointsTotal.product(); ) {
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

      ofstream osp(fileName);

      osp << "<VTKFile type=\"UnstructuredGrid\" version=\"1.0\" byte_order=\"LittleEndian\" header_type=\"UInt64\">" << endl;
      osp << "  <UnstructuredGrid>" << endl;
      osp << "    <Piece NumberOfPoints=\"" << points.samples.size() << "\" NumberOfCells=\"" << cells.offsets.size() << "\">" << endl;

      osp << "      <PointData Scalars=\"ImageFile\">" << endl;
      osp << "        <DataArray type=\"Float32\" Name=\"ImageFile\" format=\"appended\" RangeMin=\"" << points.samplesRange.lower << "\" RangeMax=\"" << points.samplesRange.upper << "\" offset=\"" << offset << "\">" << endl;
      osp << "        </DataArray>" << endl;
      offset += points.samples.size() * sizeof(points.samples[0]) + sizeof(len);
      osp << "      </PointData>" << endl;

      osp << "      <CellData>" << endl;
      offset += 0;
      osp << "      </CellData>" << endl;

      osp << "      <Points>" << endl;
      osp << "        <DataArray type=\"Float32\" Name=\"Points\" NumberOfComponents=\"3\" format=\"appended\" RangeMin=\"" << 0 << "\" RangeMax=\"" << ospcommon::length(points.coordsRange.size()) << "\" offset=\"" << offset << "\">" << endl;
      osp << "          <InformationKey name=\"L2_NORM_FINITE_RANGE\" location=\"vtkDataArray\" length=\"2\">" << endl;
      osp << "            <Value index=\"0\">" << endl;
      osp << "              " << 0 << endl;
      osp << "            </Value>" << endl;
      osp << "            <Value index=\"1\">" << endl;
      osp << "              " << /*points.coords.diameter()*/ospcommon::length(points.coordsRange.size()) << endl;
      osp << "            </Value>" << endl;
      osp << "          </InformationKey>" << endl;
      osp << "          <InformationKey name=\"L2_NORM_RANGE\" location=\"vtkDataArray\" length=\"2\">" << endl;
      osp << "            <Value index=\"0\">" << endl;
      osp << "              " << 0 << endl;
      osp << "            </Value>" << endl;
      osp << "            <Value index=\"1\">" << endl;
      osp << "              " << /*points.coords.diameter()*/ospcommon::length(points.coordsRange.size()) << endl;
      osp << "            </Value>" << endl;
      osp << "          </InformationKey>" << endl;
      osp << "        </DataArray>" << endl;
      offset += points.coords.size() * sizeof(points.coords[0]) + sizeof(len);
      osp << "      </Points>" << endl;

      osp << "      <Cells>" << endl;
      osp << "        <DataArray type=\"Int64\" Name=\"connectivity\" format=\"appended\" RangeMin=\"\" RangeMax=\"\" offset=\"" << offset << "\"/>" << endl;
      offset += cells.connectivity.size() * sizeof(cells.connectivity[0]) + sizeof(len);
      osp << "        <DataArray type=\"Int64\" Name=\"offsets\" format=\"appended\" RangeMin=\"\" RangeMax=\"\" offset=\"" << offset << "\"/>" << endl;
      offset += cells.offsets.size() * sizeof(cells.offsets[0]) + sizeof(len);
      osp << "        <DataArray type=\"UInt8\" Name=\"types\" format=\"appended\" RangeMin=\"\" RangeMax=\"\" offset=\"" << offset << "\"/>" << endl;
      offset += cells.types.size() * sizeof(cells.types[0]) + sizeof(len);
      osp << "      </Cells>" << endl;

      osp << "    </Piece>" << endl;
      osp << "  </UnstructuredGrid>" << endl;
      osp << "  <AppendedData encoding=\"raw\">" << endl;
      osp << "  _";

      osp.close();

      dataOut = fopen(fileName.c_str(), "ab");
      if (!dataOut)
        throw std::runtime_error("could not open data output file!");
      dumpVector(points.samples, dataOut);
      dumpVector(points.coords, dataOut);
      dumpVector(cells.connectivity, dataOut);
      dumpVector(cells.offsets, dataOut);
      dumpVector(cells.types, dataOut);
      fclose(dataOut);

      osp.open(fileName, std::ofstream::out | std::ofstream::app);
      osp << "  </AppendedData>" << endl;
      osp << "</VTKFile>" << endl;
    }

    extern "C" int main(int ac, char **av)
    {
#if USE_THRESHOLD
      if (ac != 10) {
        cout << "usage: ./raw2vtu in.raw <float|byte|double> Nx Ny Nz "
                "<tets|wedges|cubes> threshold_lo threshold_hi outfilebase"
             << endl;
        exit(0);
      }
#else
      if (ac != 8) {
        cout << "usage: ./raw2vtu in.raw <float|byte|double> Nx Ny Nz "
                "<tets|wedges|cubes> outfilebase"
             << endl;
        exit(0);
      }
#endif

      const char *inFileName     = av[1];
      const std::string format   = av[2];
      const vec3i inDims(atoi(av[3]), atoi(av[4]), atoi(av[5]));
      const std::string primType = av[6];  // ignored, so far only cubes are generated; TODO: tets & wedges
#if USE_THRESHOLD
      const range1f threshold(atof(av[7]), atof(av[8]));
      std::string outFileBase    = av[9];
#else
      const range1f threshold(-std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity());
      std::string outFileBase    = av[7];
#endif
      std::shared_ptr<Array3D<float>> in;
      if (format == "float") {
        in = mmapRAW<float>(inFileName, inDims);
      } else if (format == "byte" || format == "uchar" || format == "uint8") {
        in = std::make_shared<Array3DAccessor<unsigned char, float>>(
            mmapRAW<unsigned char>(inFileName, inDims));
      } else if (format == "double" || format == "float64") {
        in = std::make_shared<Array3DAccessor<double, float>>(
            mmapRAW<double>(inFileName, inDims));
      } else {
        throw std::runtime_error("unknown input voxel format");
      }

      PointsData points;
      CellsData cells;
      makeVTU(in, threshold, points, cells);
      outputVTU(outFileBase + ".vtu", points, cells);

      return 0;
    }
  }  // namespace vtu
}  // namespace ospray

#pragma clang diagnostic pop
