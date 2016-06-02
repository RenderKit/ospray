#pragma once

#include "ospray/mpi/MPICommon.h"

#define WILL_LOG_RANK 0

#if 0
// Log out some workers information as specified by WILL_LOG_RANK (-1 = all workers), otherwise
// just the worker with rank == WILL_LOG_RANK
#define WILL_DBG(a) \
  if (mpi::world.rank == 0 || WILL_LOG_RANK == -1 || mpi::worker.rank == WILL_LOG_RANK) { \
    a \
  }
#else
#define WILL_DBG(a) /* ignore */
#endif

