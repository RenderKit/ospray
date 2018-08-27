//
// Created by qadwu on 8/6/18.
//

#include "ospcommon/vec.h"
#include "ospcommon/box.h"
#include "ospcommon/utility/SaveImage.h"
#include "ospray/ospray.h"
#include <vector>

#define USE_MPI 1

#if USE_MPI
#include <mpi.h>
#endif

#define debug(x) if (world_rank == x) std::cout

using namespace ospcommon;

// struct DistributedRegion {
//   box3f bd;
//   int id = 0;
//   DistributedRegion(const box3f &b, const int &i) : bd(b), id(i) {}
// };

static int world_size = 1; // Get the number of processes
static int world_rank = 0; // Get the rank of the process

static void OSPContext_ErrorFunc(OSPError, const char *msg) {
  std::cerr << "#osp: (rank " << world_rank << ")" << msg;
}
static void OSPContext_StatusFunc(const char *msg) {
  std::cerr << "#osp: (rank " << world_rank << ")" << msg;
}
void InitOSPRay() {
  OSPError err;
  err = ospLoadModule("ispc");
  if (err != OSP_NO_ERROR) {
    std::cerr << "[Error] can't load ispc module" << std::endl;
  }
  err = ospLoadModule("mpi");
  if (err != OSP_NO_ERROR) {
    std::cerr << "[Error] can't load mpi module" << std::endl;
  }
#if USE_MPI
  OSPDevice device = ospNewDevice("mpi_distributed");
  ospDeviceSet1i(device, "masterRank", 0);
#else
  OSPDevice device = ospNewDevice("default");
#endif
  ospDeviceSetErrorFunc(device, OSPContext_ErrorFunc);
  ospDeviceSetStatusFunc(device, OSPContext_StatusFunc);
  ospDeviceCommit(device);
  ospSetCurrentDevice(device);
}

int main(int argc, char **argv) {

#if USE_MPI
  //MPI_Init(&argc, &argv);
  int provided = 0;
  MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);
  assert(provided == MPI_THREAD_MULTIPLE);
  MPI_Comm_size(MPI_COMM_WORLD, &world_size);
  MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
#endif

  // Initialize OSPRay
  InitOSPRay();

  // Wait for debug ...
#if USE_MPI && 0
  MPI_Barrier(MPI_COMM_WORLD);
  if (world_rank == 0) {
    std::cout << "please attach a debugger to a process, "
	      << "and then type continue ..."
	      << std::endl;
    std::string tmp; std::cin >> tmp;
  }
  MPI_Barrier(MPI_COMM_WORLD);
#endif

  // Parse arguments
  size_t num_of_bricks_per_rank = 1;
  vec3f vp(-128, -128, 512), vu(0, 1, 0), vi(128, 128, 128);
  for (int i = 1; i < argc; ++i) {
    std::string arg(argv[i]);
    if (arg == "-num-bricks") {
      num_of_bricks_per_rank = size_t(std::stoi(argv[++i]));
    } else if (arg == "-vp") {
      vp.x = std::stof(argv[++i]);
      vp.y = std::stof(argv[++i]);
      vp.z = std::stof(argv[++i]);
    } else if (arg == "-vu") {
      vu.x = std::stof(argv[++i]);
      vu.y = std::stof(argv[++i]);
      vu.z = std::stof(argv[++i]);
    } else if (arg == "-vi") {
      vi.x = std::stof(argv[++i]);
      vi.y = std::stof(argv[++i]);
      vi.z = std::stof(argv[++i]);
    } else {
      std::cerr << "unknown argument: " << arg << std::endl;
    }
  }

  // create volume
  const vec_t<size_t, 3> g_dim(256, 256, 256);
  const vec_t<size_t, 3> l_dim(3 + g_dim.x / num_of_bricks_per_rank,
                               3 + g_dim.y / world_size, g_dim.z);
  const size_t l_num_voxels(l_dim.x * l_dim.y * l_dim.z);
  std::vector<std::vector<float>> brickData(num_of_bricks_per_rank,
                                            std::vector<float>(l_num_voxels));
  for (size_t i = 0; i < num_of_bricks_per_rank; ++i) {
    size_t id = num_of_bricks_per_rank * world_rank + i;
    std::fill(brickData[i].begin(), brickData[i].end(), float(id));
  }

  // create ospray scene
#if USE_MPI
  auto ospRen = ospNewRenderer("mpi_raycast");
#else
  auto ospRen = ospNewRenderer("scivis");
#endif

  // create transfer function
  const std::vector<vec3f> colors = {
      vec3f(0, 0, 0.5),
      vec3f(0, 0, 1),
      vec3f(0, 1, 1),
      vec3f(0.5, 1, 0.5),
      vec3f(1, 1, 0),
      vec3f(1, 0, 0),
      vec3f(0.5, 0, 0),
  };
  const std::vector<float> opacities = {
      0.005f,
      0.01f
  };
  auto c_data = ospNewData(colors.size(), OSP_FLOAT3, colors.data());
  ospCommit(c_data);
  auto o_data = ospNewData(opacities.size(), OSP_FLOAT, opacities.data());
  ospCommit(o_data);
  auto ospTfn = ospNewTransferFunction("piecewise_linear");
  ospSetData(ospTfn, "colors", c_data);
  ospSetData(ospTfn, "opacities", o_data);
  ospSet2f(ospTfn, "valueRange", 0, world_size * num_of_bricks_per_rank - 1);
  ospCommit(ospTfn);
  ospRelease(c_data);
  ospRelease(o_data);

  // create volume & model one by one
#if USE_MPI
  std::vector<OSPModel> models;
#else
  auto ospMod = ospNewModel();
#endif

  for (size_t i = 0; i < num_of_bricks_per_rank; ++i) {

    const vec3f l_dim_f(l_dim.x, l_dim.y, l_dim.z);
    const vec3f actual_bd((l_dim.x - 3.f) * i - 1.f,
                          (l_dim.y - 3.f) * world_rank - 1.f,
                          0.f);
    const box3f logical_bd(actual_bd + 1.f, actual_bd + l_dim_f - 2.f);
    const int id = static_cast<int>(world_rank * num_of_bricks_per_rank + i);

    for (int r = 0; r < world_size; ++r) {
#if USE_MPI
      MPI_Barrier(MPI_COMM_WORLD);
#endif
      if (r == world_rank) {
        std::cerr << "rank " << world_rank << " id " << id << " dims "
                  << l_dim << std::endl
                  << " actual  "
                  << actual_bd << " "
                  << " logical "
                  << logical_bd.lower << " "
                  << logical_bd.upper << " "
                  << std::endl;
      }
#if USE_MPI
      MPI_Barrier(MPI_COMM_WORLD);
#endif
    }

    auto ospVol = ospNewVolume("shared_structured_volume");
    auto ospVox = ospNewData(brickData[i].size(), OSP_FLOAT,
                             brickData[i].data(), OSP_DATA_SHARED_BUFFER);
    ospSetString(ospVol, "voxelType", "float");
    ospSet3i(ospVol, "dimensions",
             static_cast<int>(l_dim.x),
             static_cast<int>(l_dim.y),
             static_cast<int>(l_dim.z));
    ospSet3f(ospVol, "gridOrigin", actual_bd.x, actual_bd.y, actual_bd.z);
    ospSet3f(ospVol, "gridSpacing", 1.0f, 1.0f, 1.0f);
    ospSet1f(ospVol, "samplingRate", 1.0f);
    ospSet1i(ospVol, "gradientShadingEnabled", 0);
    ospSet1i(ospVol, "useGridAccelerator", 0);
    ospSet1i(ospVol, "adaptiveSampling", 0);
    ospSet1i(ospVol, "preIntegration", 0);
    ospSet1i(ospVol, "singleShade", 0);
    ospSetVec3f(ospVol, "volumeClippingBoxLower", (const osp::vec3f&)(logical_bd.lower));
    ospSetVec3f(ospVol, "volumeClippingBoxUpper", (const osp::vec3f&)(logical_bd.upper));
    ospSetData(ospVol, "voxelData", ospVox);
    ospSetObject(ospVol, "transferFunction", ospTfn);
    ospCommit(ospVol);
    ospRelease(ospVox);

#if USE_MPI
    auto ospMod = ospNewModel();
#endif

    ospAddVolume(ospMod, ospVol);

#if USE_MPI
    ospSet1i(ospMod, "id", id);
    ospSetVec3f(ospMod, "region.lower", (const osp::vec3f&)(logical_bd.lower));
    ospSetVec3f(ospMod, "region.upper", (const osp::vec3f&)(logical_bd.upper));
    ospCommit(ospMod);
    models.push_back(ospMod);
#endif

  }

  // setup model
#if USE_MPI
  auto mData = ospNewData(models.size() * sizeof(OSPModel),
                          OSP_CHAR, models.data(), 
			  OSP_DATA_SHARED_BUFFER);
  ospSetData(ospRen, "models", mData);
#else
  ospCommit(ospMod);
  ospSetObject(ospRen, "model", ospMod);
  ospSetObject(ospRen, "world", ospMod);
#endif

  // create camera
  osp::vec2i fb_size{300, 300};
  auto ospCam = ospNewCamera("perspective");
  ospSet3f(ospCam, "dir", vi.x - vp.x, vi.y - vp.y, vi.z - vp.z);
  ospSet3f(ospCam, "pos", vp.x, vp.y, vp.z);
  ospSet3f(ospCam, "up", vu.x, vu.y, vu.z);
  ospSet1f(ospCam, "aspect", static_cast<float>(fb_size.x) / fb_size.y);
  ospCommit(ospCam);
  ospSetObject(ospRen, "camera", ospCam);

  // setup renderer
  ospSet3f(ospRen, "bgColor", 0.01f, 0.01f, 0.01f);
  ospSet1i(ospRen, "shadowEnabled", 0);
  ospSet1i(ospRen, "oneSidedLighting", 0);
  ospCommit(ospRen);

  // create frame buffer
  debug(0) << "start rendering" << std::endl;
  auto ospFb = ospNewFrameBuffer(fb_size, OSP_FB_RGBA32F, OSP_FB_COLOR);
  ospFrameBufferClear(ospFb, OSP_FB_COLOR);
  ospRenderFrame(ospFb, ospRen, OSP_FB_COLOR);

  debug(0) << "save image" << std::endl;
  if (world_rank == 0) {
    const auto* mapfb_float = static_cast<const float*>(ospMapFrameBuffer(ospFb, OSP_FB_COLOR));
    std::vector<uint32_t> mapfb_uint32(static_cast<size_t>(fb_size.x * fb_size.y * 4));
    for (int i = 0; i < fb_size.x * fb_size.y; ++i) {
      auto* color = reinterpret_cast<uint8_t*>(&mapfb_uint32[i]);
      color[0] = static_cast<uint8_t>(mapfb_float[4 * i    ] * 255);
      color[1] = static_cast<uint8_t>(mapfb_float[4 * i + 1] * 255);
      color[2] = static_cast<uint8_t>(mapfb_float[4 * i + 2] * 255);
      color[3] = static_cast<uint8_t>(mapfb_float[4 * i + 3] * 255);
    }
    utility::writePPM("framebuffer.ppm", fb_size.x, fb_size.y, mapfb_uint32.data());
  }

  ospShutdown();
#if USE_MPI
  MPI_Finalize();
#endif

  return 0;
}
