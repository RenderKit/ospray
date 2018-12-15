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

#pragma once

#include "ospcommon/networking/DataStreaming.h"
#include "ospcommon/networking/Fabric.h"

#include "MPICommon.h"

namespace mpicommon {

  /*! a specific fabric based on MPI. Note that in the case of an
   *  MPIBcastFabric using an intercommunicator the send rank must
   *  be MPI_ROOT and the recv rank must be 0.
   */
  class OSPRAY_MPI_INTERFACE MPIBcastFabric : public networking::Fabric
  {
  public:
    MPIBcastFabric(const Group &group, int sendRank, int recvRank);

    virtual ~MPIBcastFabric() override = default;

    /*! send exact number of bytes - the fabric can do that through
      multiple smaller messages, but all bytes have to be
      delivered */
    virtual void send(const void *mem, size_t size) override;

    /*! receive some block of data - whatever the sender has sent -
      and give us size and pointer to this data */
    virtual size_t read(void *&mem) override;

  private:
    // wait for Bcast with non-blocking test, and barrier
    void waitForBcast(MPI_Request &);

    std::vector<byte_t> buffer;
    Group   group;
    int     sendRank, recvRank;
  };

} // ::mpicommon
