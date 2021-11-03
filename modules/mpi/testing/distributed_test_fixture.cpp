// Copyright 2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "distributed_test_fixture.h"
#include <mpi.h>
#include "ArcballCamera.h"
#include "ospray_testing.h"

extern OSPRayEnvironment *ospEnv;

namespace MPIDistribOSPRayTestScenes {

bool computeDivisor(int x, int &divisor)
{
  int upperBound = std::sqrt(x);
  for (int i = 2; i <= upperBound; ++i) {
    if (x % i == 0) {
      divisor = i;
      return true;
    }
  }
  return false;
}

// Compute an X x Y x Z grid to have 'num' grid cells,
// only gives a nice grid for numbers with even factors since
// we don't search for factors of the number, we just try dividing by two
vec3i computeGrid(int num)
{
  vec3i grid(1);
  int axis = 0;
  int divisor = 0;
  while (computeDivisor(num, divisor)) {
    grid[axis] *= divisor;
    num /= divisor;
    axis = (axis + 1) % 3;
  }
  if (num != 1) {
    grid[axis] *= num;
  }
  return grid;
}

MPIDistribBase::MPIDistribBase()
{
  MPI_Comm_size(MPI_COMM_WORLD, &mpiSize);
  MPI_Comm_rank(MPI_COMM_WORLD, &mpiRank);

  rankGridDims = computeGrid(mpiSize);
  rankBrickId = vec3i(mpiRank % rankGridDims.x,
      (mpiRank / rankGridDims.x) % rankGridDims.y,
      mpiRank / (rankGridDims.x * rankGridDims.y));
}

void MPIDistribBase::AddInstance(cpp::Instance instance)
{
  worldBounds.extend(instance.getBounds<box3f>());
  Base::AddInstance(instance);
}

void MPIDistribBase::PerformRenderTest()
{
  SetLights();

  if (!instances.empty()) {
    world.setParam("instance", cpp::CopiedData(instances));

    // Determine the bounds of this rank's region in world space
    const vec3f regionDims = worldBounds.size() / vec3f(rankGridDims);
    box3f localRegion(rankBrickId * regionDims + worldBounds.lower,
        rankBrickId * regionDims + regionDims + worldBounds.lower);

    // Special case for the ospray test data: we might have geometry right at
    // the region bounding box which will z-fight with the clipping region. If
    // we have a region at the edge of the domain, apply some padding
    for (int i = 0; i < 3; ++i) {
      if (localRegion.lower[i] == worldBounds.lower[i]) {
        localRegion.lower[i] -= 0.001;
      }
      if (localRegion.upper[i] == worldBounds.upper[i]) {
        localRegion.upper[i] += 0.001;
      }
    }

    world.setParam("region", cpp::CopiedData(localRegion));
  }

  camera.commit();
  world.commit();
  renderer.commit();

  framebuffer.resetAccumulation();

  RenderFrame();

  if (mpiRank == 0) {
    auto *framebuffer_data = (uint32_t *)framebuffer.map(OSP_FB_COLOR);

    if (ospEnv->GetDumpImg()) {
      EXPECT_EQ(imageTool->saveTestImage(framebuffer_data), OsprayStatus::Ok);
    } else {
      EXPECT_EQ(imageTool->compareImgWithBaseline(framebuffer_data),
          OsprayStatus::Ok);
    }

    framebuffer.unmap(framebuffer_data);
  }
}

MPIFromOsprayTesting::MPIFromOsprayTesting()
{
  auto params = GetParam();

  sceneName = std::get<0>(params);
  rendererType = "mpiRaycast";
  samplesPerPixel = std::get<1>(params);
}

void MPIFromOsprayTesting::SetUp()
{
  MPIDistribBase::SetUp();

  instances.clear();

  auto builder = ospray::testing::newBuilder(sceneName);
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  // Build the group to get the bounds
  auto group = ospray::testing::buildGroup(builder);
  worldBounds = group.getBounds<box3f>();

  cpp::Instance instance(group);
  instance.commit();
  instances.push_back(instance);

  ospray::testing::release(builder);

  ArcballCamera arcballCamera(worldBounds, imgSize);

  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());
}

} // namespace MPIDistribOSPRayTestScenes
