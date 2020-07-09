// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <future>
#include <memory>
#include "rkcommon/networking/DataStreaming.h"
#include "rkcommon/networking/Fabric.h"
#include "rkcommon/utility/AbstractArray.h"

#include "MPICommon.h"

namespace mpicommon {

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

  void flushBcastSends() override;

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
