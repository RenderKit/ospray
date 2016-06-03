#include "hayai/hayai.hpp"
#include "simple_outputter.hpp"

#include "OSPRayFixture.h"

#include "commandline/Utility.h"

using std::cout;
using std::endl;
using std::string;

BENCHMARK_F(OSPRayFixture, test1, 1, 100)
{
  renderer->renderFrame(*fb, OSP_FB_COLOR | OSP_FB_ACCUM);
}

// NOTE(jda) - Implement make_unique() as it didn't show up until C++14...
template<typename T, typename ...Args>
std::unique_ptr<T> make_unique(Args&& ...args)
{
  return std::unique_ptr<T>(new T( std::forward<Args>(args)... ));
}

void printUsageAndExit()
{
  cout << "Usage: ospBenchmark [options] model_file" << endl;

  cout << endl << "Args:" << endl;

  cout << endl;
  cout << "    model_file --> Scene used for benchmarking, supported types"
            << " are:" << endl;
  cout << "                   stl, msg, tri, xml, obj, hbp, x3d" << endl;

  cout << endl;
  cout << "Options:" << endl;

  cout << endl;
  cout << "**generic rendering options**" << endl;

  cout << endl;
  cout << "    -i | --image --> Specify the base filename to write the"
       << " framebuffer to a file." << endl;
  cout << "                     If ommitted, no file will be written." << endl;
  cout << "                     NOTE: this option adds '.ppm' to the end of the"
       << " filename" << endl;

  cout << endl;
  cout << "    -w | --width --> Specify the width of the benchmark frame"
       << endl;
  cout << "                     default: 1024" << endl;

  cout << endl;
  cout << "    -h | --height --> Specify the height of the benchmark frame"
       << endl;
  cout << "                      default: 1024" << endl;

  cout << endl;
  cout << "    -r | --renderer --> Specify the renderer to be benchmarked."
       << endl;
  cout << "                        Ex: -r pathtracer" << endl;
  cout << "                        default: ao1" << endl;

  cout << endl;
  cout << "    -bg | --background --> Specify the background color: R G B"
       << endl;

  cout << endl;
  cout << "    -wf | --warmup --> Specify the number of warmup frames: N"
       << endl;
  cout << "                       default: 10" << endl;

  cout << endl;
  cout << "**camera rendering options**" << endl;

  cout << endl;
  cout << "    -vp | --eye --> Specify the camera eye as: ex ey ez " << endl;

  cout << endl;
  cout << "    -vi | --gaze --> Specify the camera gaze point as: ix iy iz "
       << endl;

  cout << endl;
  cout << "    -vu | --up --> Specify the camera up as: ux uy uz " << endl;


  cout << endl;
  cout << "**volume rendering options**" << endl;

  cout << endl;
  cout << "    -s | --sampling-rate --> Specify the sampling rate for volumes."
       << endl;
  cout << "                             default: 0.125" << endl;

  cout << endl;
  cout << "    -dr | --data-range --> Specify the data range for volumes."
       << " If not specified, then the min and max data" << endl
       << " values will be used when reading the data into memory." << endl;
  cout << "                           Format: low high" << endl;

  cout << endl;
  cout << "    -tfc | --tf-color --> Specify the next color to in the transfer"
       << " function for volumes. Each entry will add to the total list of"
       << " colors in the order they are specified." << endl;
  cout << "                              Format: R G B A" << endl;
  cout << "                         Value Range: [0,1]" << endl;

  cout << "    -tfs | --tf-scale --> Specify the opacity the transfer function"
       << " will scale to: [0,x] where x is the input value." << endl;
  cout << "                          default: 1.0" << endl;

  cout << endl;
  cout << "    -is | --surface --> Specify an isosurface at value: val "
       << endl;

  exit(0);
}

void parseCommandLine(int argc, const char *argv[])
{
  if (argc <= 1) {
    printUsageAndExit();
  }

  for (int i = 1; i < argc; ++i) {
    string arg = argv[i];
    if (arg == "-i" || arg == "--image") {
      OSPRayFixture::imageOutputFile = argv[++i];
    } else if (arg == "-w" || arg == "--width") {
      OSPRayFixture::width = atoi(argv[++i]);
    } else if (arg == "-h" || arg == "--height") {
      OSPRayFixture::height = atoi(argv[++i]);
    } else if (arg == "-wf" || arg == "--warmup") {
      OSPRayFixture::numWarmupFrames = atoi(argv[++i]);
    } else if (arg == "-bg" || arg == "--background") {
      ospcommon::vec3f &color = OSPRayFixture::bg_color;
      color.x = atof(argv[++i]);
      color.y = atof(argv[++i]);
      color.z = atof(argv[++i]);
    }
  }

  auto ospObjs = parseWithDefaultParsers(argc, argv);

  ospcommon::box3f bbox;
  std::tie(bbox,
           *OSPRayFixture::model,
           *OSPRayFixture::renderer,
           *OSPRayFixture::camera) = ospObjs;

  float width  = OSPRayFixture::width;
  float height = OSPRayFixture::height;
  auto &camera = *OSPRayFixture::camera;
  camera.set("aspect", width/height);
  camera.commit();
}

void allocateFixtureObjects()
{
  // NOTE(jda) - Have to allocate objects here, because we can't allocate them
  //             statically (before ospInit) and can't in the fixture's
  //             constructor because they need to exist during parseCommandLine.
  OSPRayFixture::renderer = make_unique<ospray::cpp::Renderer>();
  OSPRayFixture::camera   = make_unique<ospray::cpp::Camera>();
  OSPRayFixture::model    = make_unique<ospray::cpp::Model>();
  OSPRayFixture::fb       = make_unique<ospray::cpp::FrameBuffer>();
}

int main(int argc, const char *argv[])
{
  ospInit(&argc, argv);
  allocateFixtureObjects();
  parseCommandLine(argc, argv);

# if 0
  hayai::ConsoleOutputter outputter;
#else
  hayai::SimpleOutputter outputter;
#endif

  hayai::Benchmarker::AddOutputter(outputter);

  hayai::Benchmarker::RunAllTests();
  return 0;
}
