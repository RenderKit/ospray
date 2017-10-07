// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// ospcommon
#include "ospcommon/utility/SaveImage.h"
// mpiCommon
#include "mpiCommon/MPICommon.h"
// public-ospray
#include "ospray/ospray_cpp/Camera.h"
#include "ospray/ospray_cpp/Data.h"
#include "ospray/ospray_cpp/Device.h"
#include "ospray/ospray_cpp/Model.h"
#include "ospray/ospray_cpp/FrameBuffer.h"
#include "ospray/ospray_cpp/Renderer.h"
#include "ospray/ospray_cpp/TransferFunction.h"
#include "ospray/ospray_cpp/Volume.h"
// pico_bench
#include "apps/bench/pico_bench/pico_bench.h"
// stl
#include <random>
#include <algorithm>    // std::sort
#include "gensv/generateSciVis.h"
// visit module
#include "VisItModuleCommon.h"

#define RUN_LOCAL 0

/* This app demonstrates how to write a distributed scivis style
 * renderer using the distributed MPI device. Note that because
 * OSPRay uses sort-last compositing it is up to the user to ensure
 * that the data distribution across the nodes is suitable. Specifically,
 * each nodes' data must be convex and disjoint. This renderer only
 * supports multiple volumes and geometries per-node, to ensure they're
 * composited correctly you specify a list of bounding regions to the
 * model, within these regions can be arbitrary volumes/geometries
 * and each rank can have as many regions as needed. As long as the
 * regions are disjoint/convex the data will be rendered correctly.
 * In this example we set two regions on certain ranks just to produce
 * a gap in the ranks volume to demonstrate how they work.
 *
 * In the case that you have geometry crossing the boundary of nodes
 * and are replicating it on both nodes to render (ghost zones, etc.)
 * the region will be used by the renderer to clip rays against allowing
 * to split the object between the two nodes, with each rendering half.
 * This will keep the regions rendered by each rank disjoint and thus
 * avoid any artifacts. For example, if a sphere center is on the border
 * between two nodes, each would render half the sphere and the halves
 * would be composited to produce the final complete sphere in the image.
 *
 * See gensv::makeVolume for an example of how to properly load a volume
 * distributed across ranks with correct specification of brick positions
 * and ghost voxels.
 */

namespace ospVisItRandSciVisTest {

  using namespace ospcommon;

  int   numSpheresPerNode = 100;
  float sphereRadius      = 0.01f;
  vec2i fbSize            = vec2i(1024, 768);
  int   numFrames         = 32;
  bool  runDistributed    = true;
  int   logLevel          = 0;

  vec3f up;
  vec3f pos;
  vec3f gaze;
  float fovy = 60.f;
  bool customView = false;

  void setupCamera(ospray::cpp::Camera &camera, box3f worldBounds)
  {
    if (!customView) {
      vec3f diag = worldBounds.size();
      diag       = max(diag,vec3f(0.3f*length(diag)));

      gaze = ospcommon::center(worldBounds);

      pos = gaze - .75f*vec3f(-.6*diag.x,-1.2f*diag.y,.8f*diag.z);
      up  = vec3f(0.f, 1.f, 0.f);
    }

    camera.set("pos", pos);
    camera.set("dir", gaze - pos);
    camera.set("up",  up );
    camera.set("aspect", static_cast<float>(fbSize.x)/fbSize.y);

    camera.commit();
  }

  float SRGB(float c) {
#if 1
    return std::pow(c, 1.f/2.2f); // hacked version used in OSPRay
#else
    const float a = 0.055f;
    if (c <= 0.0031308) {
      return (12.92 * c);
    } else {
      return (1+a) * std::pow(c, 1.f/2.4f) - a; 
    }
#endif
  }

  void writePPM(const std::string &fileName,
		const ospray::visit::VisItTile &tile)
  {
    FILE *file = fopen(fileName.c_str(), "wb");
    const int sizeX = tile.region[2] - tile.region[0];
    const int sizeY = tile.region[3] - tile.region[1];
    fprintf(file, "P6\n%i %i\n255\n", sizeX, sizeY);
    unsigned char *out = (unsigned char *)alloca(3*sizeX);
    for (int y = 0; y < sizeY; y++) {
      const float *inr = &tile.r[(sizeY-1-y)*sizeX];
      const float *ing = &tile.g[(sizeY-1-y)*sizeX];
      const float *inb = &tile.b[(sizeY-1-y)*sizeX];
      for (int x = 0; x < sizeX; x++) {
	out[3*x + 0] = (unsigned char)(SRGB(inr[x]) * 255);
	out[3*x + 1] = (unsigned char)(SRGB(ing[x]) * 255);
	out[3*x + 2] = (unsigned char)(SRGB(inb[x]) * 255);
      }
      fwrite(out, 3*sizeX, sizeof(char), file);
    }
    fprintf(file, "\n");
    fclose(file);
  }

  void writePPM(const std::string &fileName, 
		const ospcommon::vec2i size,
		const float* image)
  {
    FILE *file = fopen(fileName.c_str(), "wb");
    fprintf(file, "P6\n%i %i\n255\n", size.x, size.y);
    unsigned char *out = (unsigned char *)alloca(3*size.x);
    for (int y = 0; y < size.y; y++) 
    {
      const float *in = &image[(size.y-1-y)*size.x*4];
      for (int x = 0; x < size.x; x++) {
	out[3*x + 0] = (unsigned char)(SRGB(in[4*x+0]) * 255);
	out[3*x + 1] = (unsigned char)(SRGB(in[4*x+1]) * 255);
	out[3*x + 2] = (unsigned char)(SRGB(in[4*x+2]) * 255);
      }
      fwrite(out, 3*size.x, sizeof(char), file);
    }
    fprintf(file, "\n");
    fclose(file);
  }

  void parseCommandLine(int ac, const char **av)
  {
    for (int i = 0; i < ac; ++i) {
      std::string arg = av[i];
      if (arg == "-w") {
        fbSize.x = std::atoi(av[++i]);
      } else if (arg == "-h") {
        fbSize.y = std::atoi(av[++i]);
      } else if (arg == "-spn" || arg == "--spheres-per-node") {
        numSpheresPerNode = std::atoi(av[++i]);
      } else if (arg == "-r" || arg == "--radius") {
        sphereRadius = std::atof(av[++i]);
      } else if (arg == "-nf" || arg == "--num-frames") {
        numFrames = std::atoi(av[++i]);
      } else if (arg == "-l" || arg == "--local") {
        runDistributed = false;
      } else if (arg == "--log") {
        logLevel = std::atoi(av[++i]);
      } else if (arg == "-vp" || arg == "--eye") {
        pos.x = atof(av[++i]);
        pos.y = atof(av[++i]);
        pos.z = atof(av[++i]);
        customView = true;
      } else if (arg == "-vu" || arg == "--up") {
        up.x = atof(av[++i]);
        up.y = atof(av[++i]);
        up.z = atof(av[++i]);
        customView = true;
      } else if (arg == "-vi" || arg == "--gaze") {
        gaze.x = atof(av[++i]);
        gaze.y = atof(av[++i]);
        gaze.z = atof(av[++i]);
        customView = true;
      } else if (arg == "-fv" || arg == "--fovy") {
        fovy = atof(av[++i]);
      }
    }
  }

  /* Manually set up the OSPRay device. In MPI distributed mode
   * we use the 'mpi_distributed' renderer, which allows each
   * rank to make separate independent OSPRay calls locally.
   * The model created by this device will handle coordinating
   * the regions of data and the renderer used in the distributed
   * case 'mpi_raycast' knows how to use this information to
   * perform sort-last compositing rendering of the data.
   */
  void initialize_ospray()
  {
    ospray::cpp::Device device;

    if (runDistributed) {
      ospLoadModule("mpi");
      device = ospray::cpp::Device("mpi_distributed");
      device.set("masterRank", 0);
      device.set("logLevel", logLevel);
      device.commit();
      device.setCurrent();
    } else {
      device = ospray::cpp::Device();
      device.set("logLevel", logLevel);
      device.commit();
      device.setCurrent();
    }
    ospLoadModule("visit");
    ospDeviceSetStatusFunc(device.handle(),
                           [](const char *msg) {
                             std::cerr << msg;
                           });
  }

  template<class T>
  constexpr const T& clamp( const T& v, const T& lo, const T& hi )
  {
    return (v < lo ? lo : (v > hi ? hi : v));
  }

  bool SortTileFcn (ospray::visit::VisItTile* i, ospray::visit::VisItTile* j) {
    return (i->minDepth < j->minDepth); 
  }

  void BlendFrontToBack
  (const ospray::visit::VisItTile* src, float* dstImage, int dstExtents[4])
  {  
    // image sizes
    const int srcX = src->region[2] - src->region[0];
    const int srcY = src->region[3] - src->region[1];
    const int dstX = dstExtents[1] - dstExtents[0];
    const int dstY = dstExtents[3] - dstExtents[2];
    // determin the region to blend
    const int startX = 
      std::max(src->region[0], dstExtents[0]);
    const int startY = 
      std::max(src->region[1], dstExtents[2]);
    const int endX = 
      std::min(src->region[2], dstExtents[1]);
    const int endY = 
      std::min(src->region[3], dstExtents[3]);

    for (int y = startY; y < endY; ++y) {
      for (int x = startX; x < endX; ++x) {	  	    
	if (true)
	{
	  bool printError = false;
	  if (x <  dstExtents[0]) { printError = true; }
	  if (x >= dstExtents[1]) { printError = true; }
	  if (y <  dstExtents[2]) { printError = true; }
	  if (y >= dstExtents[3]) { printError = true; }
	  if (x <  src->region[0]) { printError = true; }
	  if (x >= src->region[2]) { printError = true; }
	  if (y <  src->region[1]) { printError = true; }
	  if (y >= src->region[3]) { printError = true; }

	  if (printError) {
	    std::cout << "Err: "
		      << "index goes out of bound "
		      << "(" << x << "," << y << ") "
		      << "extents " 
		      << dstExtents[0] << " " << dstExtents[1] << " "
		      << dstExtents[2] << " " << dstExtents[3] << " "
		      << std::endl;
	    continue; 
	  }
	}

	// get indices
	int srcIndex = (srcX * (y-src->region[1]) + x-src->region[0]);
	int dstIndex = (dstX * (y-dstExtents[2]) + x-dstExtents[0]) * 4;

	// front to back compositing
	if (dstImage[dstIndex + 3] < 1.0f) {
	  float trans = 1.0f - dstImage[dstIndex + 3];
	  dstImage[dstIndex+0] = 
	    clamp(src->r[srcIndex] * trans + dstImage[dstIndex+0], 0.0f, 1.0f);
	  dstImage[dstIndex+1] = 
	    clamp(src->g[srcIndex] * trans + dstImage[dstIndex+1], 0.0f, 1.0f);
	  dstImage[dstIndex+2] = 
	    clamp(src->b[srcIndex] * trans + dstImage[dstIndex+2], 0.0f, 1.0f);
	  dstImage[dstIndex+3] = 
	    clamp(src->a[srcIndex] * trans + dstImage[dstIndex+3], 0.0f, 1.0f);
	}
      }
    }
  }

  struct SaveTiles: public ospray::visit::VisItTileRetriever 
  {
    int id = 0;
    float* image = nullptr;
    SaveTiles() {
      image = new float[fbSize.x * fbSize.y * 4]();
    }
    void reset() { id = 0; }
    void operator() (const ospray::visit::VisItTileArray& tiles) {
      ospray::visit::VisItTileArray sortedtiles = tiles;
      std::sort(sortedtiles.begin(), sortedtiles.end(), SortTileFcn);
      std::cout << "fb size: " << fbSize.x << " " << fbSize.y << std::endl;
      int imageExtents[4] = {0, fbSize.x, 0, fbSize.y};
      for (auto t : sortedtiles) 
      {
	writePPM("DistributedTile" + std::to_string(id++) + ".ppm", *t);
	std::cout << "new tile " 
		  << t->region[0] << " "
		  << t->region[1] << " "
		  << t->region[2] << " "
		  << t->region[3] << " "
		  << "depth " 
		  << t->minDepth << " "
		  << std::endl;
	BlendFrontToBack(t, image, imageExtents);
      }
      writePPM("randomComposed.ppm", fbSize, image);	
    }
  };

  extern "C" int main(int ac, const char **av)
  {
    using namespace std::chrono;

    parseCommandLine(ac, av);

    initialize_ospray();

    ospray::cpp::Model model;
    gensv::LoadedVolume volume = gensv::makeVolume();
    model.addVolume(volume.volume);

    // Generate spheres within the bounds of the volume
    auto spheres = gensv::makeSpheres(volume.bounds, numSpheresPerNode,
                                      sphereRadius);
    model.addGeometry(spheres);

    // We must use the global world bounds, not our local bounds
    // when computing the automatically picked camera position.
    box3f worldBounds(vec3f(0), vec3f(1));

    /* The regions listing specifies the data regions that this rank owns
     * and is responsible for rendering. All volumes and geometry on the rank
     * should be contained within these bounds and will be clipped against them.
     * In the case of ghost regions or splitting geometry across the region border
     * it's up to the user to ensure the other rank also has the geometry being
     * split and renders the correct region bounds. The region data is specified
     * as an OSPData of OSP_FLOAT3 to pass the lower and upper corners of each
     * regions bounding box.
     *
     * On some ranks we add some additional regions to clip the volume
     * and make some gaps, just to show usage and test multiple regions per-rank
     */
    std::vector<box3f> regions{volume.bounds};
    bool setGap = false;
    if (mpicommon::numGlobalRanks() % 2 == 0) {
      setGap = mpicommon::globalRank() % 3 == 0;
    } else  {
      setGap = mpicommon::globalRank() % 2 == 0;
    }
    if (setGap) {
      const float step = (regions[0].upper.x - regions[0].lower.x) / 4.0;
      const vec3f low = regions[0].lower;
      const vec3f hi = regions[0].upper;
      regions[0].upper.x = low.x + step;
      regions.push_back(box3f(vec3f(low.x + step * 3, low.y, low.z),
			      vec3f(low.x + step * 4, hi.y, hi.z)));
    }
    ospray::cpp::Data regionData(regions.size() * 2, OSP_FLOAT3,
        regions.data());
    model.set("regions", regionData);
    model.commit();

    auto camera = ospray::cpp::Camera("perspective");
    setupCamera(camera, worldBounds);

    // In the distributed mode we use the 'mpi_raycast' renderer which
    // knows how to read the region information from the model and render
    // the distributed data.
    ospray::cpp::Renderer renderer;
    if (runDistributed) {
      renderer = ospray::cpp::Renderer("visitrc");
    } else {
      renderer = ospray::cpp::Renderer("raycast");
    }
    renderer.set("world", model);
    renderer.set("model", model);
    renderer.set("camera", camera);
    renderer.set("bgColor", vec3f(0.02));
    renderer.commit();

    ospray::cpp::FrameBuffer fb(fbSize,OSP_FB_SRGBA,OSP_FB_COLOR|OSP_FB_ACCUM);
    fb.clear(OSP_FB_ACCUM);

    auto bencher = pico_bench::Benchmarker<milliseconds>{numFrames};

    if (runDistributed) {

      if (mpicommon::IamTheMaster()) {
        std::cout << "Benchmark results will be in (ms)" << '\n';
        std::cout << "...starting distributed tests" << '\n';
      }

      mpicommon::world.barrier();

      auto stats = bencher([&](){
        renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);
      });

      // Only the OSPRay master rank will have the final framebuffer which
      // can be saved out or displayed to the user, the others only store
      // the tiles which they composite.

      if (mpicommon::IamTheMaster()) {
        std::cout << stats << '\n';
        auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
        utility::writePPM("randomSciVisTestDistributed.ppm",
                          fbSize.x, fbSize.y, lfb);
        fb.unmap(lfb);
        std::cout << "\noutput: 'randomSciVisTestDistributed.ppm'" << std::endl;
      }

      // render tiles
      SaveTiles fcn;
      fcn.reset();
      renderer.set("VisItTileRetriever", &fcn);
      renderer.commit();
      renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);	

      mpicommon::world.barrier();

    } else {

      std::cout << "Benchmark results will be in (ms)" << '\n';
      std::cout << "...starting non-distributed tests" << '\n';

      auto stats = bencher([&](){
        renderer.renderFrame(fb, OSP_FB_COLOR | OSP_FB_ACCUM);
      });

      auto *lfb = (uint32_t*)fb.map(OSP_FB_COLOR);
      utility::writePPM("randomSciVisTestLocal.ppm", fbSize.x, fbSize.y, lfb);
      fb.unmap(lfb);
      std::cout << "\noutput: 'randomSciVisTestLocal.ppm'" << std::endl;
    }

    return 0;
  }

}

