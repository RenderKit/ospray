// Copyright 2009-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <fstream>
#include <limits>
#include <map>
#include <utility>
#include "common/Data.h"
#include "lights/AmbientLight.h"
#include "lights/Light.h"
#include "ospcommon/tasking/parallel_for.h"
#include "ospcommon/utility/getEnvVar.h"

#include "../../common/DistributedWorld.h"
#include "../../common/Profiling.h"
#include "../../fb/DistributedFrameBuffer.h"
#include "AlphaCompositeTileOperation.h"
#include "DistributedRaycast.h"

#include "DistributedRaycast_ispc.h"

namespace ospray {
namespace mpi {
using namespace std::chrono;
using namespace mpicommon;

static bool DETAILED_LOGGING = false;

// DistributedRaycastRenderer definitions /////////////////////////////////

DistributedRaycastRenderer::DistributedRaycastRenderer()
    : mpiGroup(mpicommon::worker.dup())
{
  ispcEquivalent = ispc::DistributedRaycastRenderer_create(this);

  DETAILED_LOGGING =
      utility::getEnvVar<int>("OSPRAY_DP_API_TRACING").value_or(0);

  if (DETAILED_LOGGING) {
    auto job_name =
        utility::getEnvVar<std::string>("OSPRAY_JOB_NAME").value_or("log");
    std::string statsLogFile = job_name + std::string("-rank")
        + std::to_string(mpiGroup.rank) + ".txt";
    statsLog = ospcommon::make_unique<std::ofstream>(statsLogFile.c_str());
  }
}

DistributedRaycastRenderer::~DistributedRaycastRenderer()
{
  if (DETAILED_LOGGING) {
    *statsLog << "\n" << std::flush;
  }
}

void DistributedRaycastRenderer::commit()
{
  Renderer::commit();

  ispc::DistributedRaycastRenderer_set(getIE(),
      getParam<int>("aoSamples", 0),
      getParam<float>("aoRadius", 1e20f),
      getParam<int>("shadowsEnabled", 0),
      getParam<float>("volumeSamplingRate", 0.125f));
}

std::shared_ptr<TileOperation> DistributedRaycastRenderer::tileOperation()
{
  return std::make_shared<AlphaCompositeTileOperation>();
}

std::string DistributedRaycastRenderer::toString() const
{
  return "ospray::mpi::DistributedRaycastRenderer";
}

} // namespace mpi
} // namespace ospray
