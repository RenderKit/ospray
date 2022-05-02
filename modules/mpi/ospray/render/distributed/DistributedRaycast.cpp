// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include <chrono>
#include <fstream>
#include <limits>
#include <map>
#include <utility>
#include "common/Data.h"
#include "lights/AmbientLight.h"
#include "lights/Light.h"
#include "rkcommon/tasking/parallel_for.h"
#include "rkcommon/utility/getEnvVar.h"

#include "../../common/DistributedWorld.h"
#include "../../common/Profiling.h"
#include "../../fb/DistributedFrameBuffer.h"
#include "AlphaCompositeTileOperation.h"
#include "DistributedRaycast.h"
// ispc exports
#include "render/distributed/DistributedRaycast_ispc.h"

namespace ospray {
namespace mpi {
using namespace std::chrono;
using namespace mpicommon;

static bool DETAILED_LOGGING = false;

// DistributedRaycastRenderer definitions /////////////////////////////////

DistributedRaycastRenderer::DistributedRaycastRenderer()
    : mpiGroup(mpicommon::worker.dup())
{
  getSh()->super.renderRegionSample = ispc::DRR_renderRegionSample_addr();

  DETAILED_LOGGING =
      utility::getEnvVar<int>("OSPRAY_DP_API_TRACING").value_or(0);

  if (DETAILED_LOGGING) {
    auto job_name =
        utility::getEnvVar<std::string>("OSPRAY_JOB_NAME").value_or("log");
    std::string statsLogFile = job_name + std::string("-rank")
        + std::to_string(mpiGroup.rank) + ".txt";
    statsLog = rkcommon::make_unique<std::ofstream>(statsLogFile.c_str());
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

  getSh()->aoSamples = getParam<int>("aoSamples", 0);
  getSh()->aoRadius =
      getParam<float>("aoDistance", getParam<float>("aoRadius", 1e20f));
  getSh()->shadowsEnabled =
      getParam<bool>("shadows", getParam<int>("shadowsEnabled", 0));
  getSh()->volumeSamplingRate = getParam<float>("volumeSamplingRate", 1.f);
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
