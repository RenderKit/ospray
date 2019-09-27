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

#pragma once

#include <future>
#include <memory>
#include "ospcommon/networking/DataStreaming.h"
#include "ospcommon/networking/Fabric.h"
#include "ospcommon/utility/AbstractArray.h"

#include "MPICommon.h"

namespace mpicommon {

/* Plans for reworking of MPI fabric:
 *
 * - Want to be able to chunk up large buffers to send them and
 *   re-assemble them on the host side, kind of like in the socket
 *   recv stuff.
 *
 * - Want to keep large buffers (newData, setRegion) separate from
 *   regular command stuff so we don't need to pack/unpack them all
 *   the time to cut out these copies.
 */

/*! a specific fabric based on MPI. Note that in the case of an
 *  MPIFabric using an intercommunicator the send rank must
 *  be MPI_ROOT and the recv rank must be 0. The group passed will
 *  be duplicated to avoid MPI collective matching issues with other
 *  fabrics and collectives.
 */
class OSPRAY_MPI_INTERFACE MPIFabric : public networking::Fabric
{
 public:
  MPIFabric(const Group &parentGroup, int bcastRoot);

  virtual ~MPIFabric() override = default;

  void sendBcast(std::shared_ptr<utility::AbstractArray<uint8_t>> buf) override;

  void recvBcast(utility::AbstractArray<uint8_t> &buf) override;

  void send(
      std::shared_ptr<utility::AbstractArray<uint8_t>> buf, int rank) override;

  void recv(utility::AbstractArray<uint8_t> &buf, int rank) override;

 private:
  void checkPendingSends();

  Group group;
  int bcastRoot;

  struct PendingSend
  {
    std::future<void *> future;
    std::shared_ptr<utility::AbstractArray<uint8_t>> buf = nullptr;

    PendingSend() = default;
    PendingSend(std::future<void *> future,
        std::shared_ptr<utility::AbstractArray<uint8_t>> &buf);

    PendingSend(const PendingSend &) = delete;
    PendingSend &operator=(const PendingSend &) = delete;
  };

  std::vector<std::shared_ptr<PendingSend>> pendingSends;
};
} // namespace mpicommon
