// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

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

void MPIFabric::flushBcastSends()
{
  while (!pendingSends.empty()) {
    checkPendingSends();
  }
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
