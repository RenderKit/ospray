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

int main(int ac, char **av)
{
#if USE_THRESHOLD
    if (ac != 10) {
        std::cout << "usage: ./raw2vtu in.raw <float|byte|double> Nx Ny Nz "
            "<tets|wedges|cubes> threshold_lo threshold_hi outfilebase"
            << std::endl;
        exit(0);
    }
#else
    if (ac != 8) {
        std::cout << "usage: ./raw2vtu in.raw <float|byte|double> Nx Ny Nz "
            "<tets|wedges|cubes> outfilebase"
            << std::endl;
        exit(0);
    }
#endif

    const char *inFileName     = av[1];
    const std::string format   = av[2];
    const vec3i inDims(atoi(av[3]), atoi(av[4]), atoi(av[5]));
    const std::string primType = av[6];
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

    ospray::vtu::PointsData points;
    ospray::vtu::CellsData cells;
    ospray::vtu::makeVTU(in, primType, threshold, points, cells);
    ospray::vtu::outputVTU(outFileBase + ".vtu", points, cells);

    return 0;
}

