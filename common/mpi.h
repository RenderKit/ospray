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

#include <mpi.h>
#include <stdexcept>

#define MPI_CALL(cmd) { int rc = MPI_##cmd; if (rc != MPI_SUCCESS) throw std::runtime_error("mpi error"); }

#define error(msg) throw std::runtime_error(msg)
    

namespace ospcommon {

  struct MPIComm {
    MPI_Comm comm;
    int size, rank, isInterComm;

    void setTo(MPI_Comm comm);
    void dupFrom(MPI_Comm comm);
    void setSizeAndRank();
  };

  inline void MPIComm::setSizeAndRank() 
  {
    MPI_CALL(Comm_test_inter(comm,&isInterComm));
    if (isInterComm) {
      MPI_CALL(Comm_remote_size(comm,&size));
      rank = -1;
    } else {
      MPI_CALL(Comm_size(comm,&size));
      MPI_CALL(Comm_rank(comm,&rank));
    }
  }

  inline void MPIComm::setTo(MPI_Comm comm)
  { 
    this->comm = comm;
    setSizeAndRank();
  }

  inline void MPIComm::dupFrom(MPI_Comm comm)
  { 
    MPI_CALL(Comm_dup(comm,&this->comm));
    setSizeAndRank();
  }

} // ::ospcommon


