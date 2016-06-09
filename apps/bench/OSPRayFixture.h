
#include "hayai/hayai.hpp"

#include <ospray_cpp/Camera.h>
#include <ospray_cpp/Model.h>
#include <ospray_cpp/Renderer.h>

struct OSPRayFixture : public hayai::Fixture
{
  // Fixture hayai interface //

  void SetUp() override;
  void TearDown() override;

  // Fixture data //

  static std::unique_ptr<ospray::cpp::Renderer>    renderer;
  static std::unique_ptr<ospray::cpp::Camera>      camera;
  static std::unique_ptr<ospray::cpp::Model>       model;
  static std::unique_ptr<ospray::cpp::FrameBuffer> fb;

  // Command-line configuration data //

  static std::string imageOutputFile;

  static std::vector<std::string> benchmarkModelFiles;

  static int width;
  static int height;

  static int numWarmupFrames;

  static ospcommon::vec3f bg_color;
};
