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

#include <algorithm>
#include <chrono>
#include <thread>

#include "Collectives.h"
#include "MPIBcastFabric.h"
#include "maml/maml.h"

namespace mpicommon {

MPIFabric::MPIFabric(const Group &parentGroup, int bcastRoot)
    : group(parentGroup.dup()), bcastRoot(bcastRoot)
{
  if (!group.valid()) {
    throw std::runtime_error(
        "#osp:mpi: trying to set up an MPI fabric "
        "with an invalid MPI communicator");
  }

  int isInter = 0;
  MPI_CALL(Comm_test_inter(group.comm, &isInter));
  if (isInter && bcastRoot != MPI_ROOT) {
    throw std::runtime_error(
        "Invalid MPIFabric group config "
        "on an MPI intercomm group");
  }
}

void MPIFabric::sendBcast(std::shared_ptr<utility::AbstractArray<uint8_t>> buf)
{
  auto future = mpicommon::bcast(
      buf->data(), buf->size(), MPI_BYTE, bcastRoot, group.comm);

  pendingSends.emplace_back(
      std::make_shared<PendingSend>(std::move(future), buf));

  checkPendingSends();
}

void MPIFabric::recvBcast(utility::AbstractArray<uint8_t> &buf)
{
  mpicommon::bcast(buf.data(), buf.size(), MPI_BYTE, bcastRoot, group.comm)
      .wait();

  checkPendingSends();
}

void MPIFabric::send(
    std::shared_ptr<utility::AbstractArray<uint8_t>> buf, int rank)
{
  auto future =
      mpicommon::send(buf->data(), buf->size(), MPI_BYTE, rank, 0, group.comm);

  pendingSends.emplace_back(
      std::make_shared<PendingSend>(std::move(future), buf));

  checkPendingSends();
}

void MPIFabric::recv(utility::AbstractArray<uint8_t> &buf, int rank)
{
  mpicommon::recv(buf.data(), buf.size(), MPI_BYTE, rank, 0, group.comm).wait();

  checkPendingSends();
}

void MPIFabric::checkPendingSends()
{
  if (!pendingSends.empty()) {
    auto done = std::partition(pendingSends.begin(),
        pendingSends.end(),
        [](const std::shared_ptr<PendingSend> &ps) {
          return ps->future.wait_for(std::chrono::milliseconds(0))
              != std::future_status::ready;
        });
    pendingSends.erase(done, pendingSends.end());
  }
}

MPIFabric::PendingSend::PendingSend(std::future<void *> future,
    std::shared_ptr<utility::AbstractArray<uint8_t>> &buf)
    : future(std::move(future)), buf(buf)
{}
} // namespace mpicommon
