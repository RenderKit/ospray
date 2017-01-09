// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// #pragma once

#include "mpi/common/MPICommon.h"
#include "common/ObjectHandle.h"

// // macro for ignoring sends in collaborative mode
// // TODO: mpi command layer needs to be rethought out
// // to avoid this
// #define COLLAB_CHECK() { if (mpi::world.rank > 0) return; }

// namespace ospray {
//   namespace mpi {

//     inline void checkMpiError(int rc)
//     {
//       if (rc != MPI_SUCCESS)
//         throw std::runtime_error("MPI Error");
//     }

//         OSPRAY_INTERFACE void processWorkerCommand(const int command);

