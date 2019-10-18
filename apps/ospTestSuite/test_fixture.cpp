// ======================================================================== //
// Copyright 2017-2019 Intel Corporation                                    //
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

#include "test_fixture.h"

extern OSPRayEnvironment *ospEnv;

namespace OSPRayTestScenes {

  // Helper functions /////////////////////////////////////////////////////////

  // creates a torus
  // volumetric data: stores data of torus
  // returns created ospvolume of torus
  static cpp::Volume CreateTorus(const int size)
  {
    std::vector<float> volumetricData(size * size * size, 0);

    const float r    = 30;
    const float R    = 80;
    const int size_2 = size / 2;

    for (int x = 0; x < size; ++x) {
      for (int y = 0; y < size; ++y) {
        for (int z = 0; z < size; ++z) {
          const float X = x - size_2;
          const float Y = y - size_2;
          const float Z = z - size_2;

          const float d = (R - std::sqrt(X * X + Y * Y));
          volumetricData[size * size * x + size * y + z] =
              r * r - d * d - Z * Z;
        }
      }
    }

    cpp::Volume torus("structured_volume");
    torus.setParam("voxelData", cpp::Data(volumetricData));
    torus.setParam("dimensions", vec3i(size, size, size));
    torus.setParam<int>("voxelType", OSP_FLOAT);
    torus.setParam("gridOrigin", vec3f(-0.5f, -0.5f, -0.5f));
    torus.setParam("gridSpacing", vec3f(1.f / size, 1.f / size, 1.f / size));
    return torus;
  }

  /////////////////////////////////////////////////////////////////////////////

  Base::Base()
  {
    const ::testing::TestCase *const testCase =
        ::testing::UnitTest::GetInstance()->current_test_case();
    const ::testing::TestInfo *const testInfo =
        ::testing::UnitTest::GetInstance()->current_test_info();
    imgSize = ospEnv->GetImgSize();

    framebuffer = cpp::FrameBuffer(
        imgSize, frameBufferFormat, OSP_FB_COLOR | OSP_FB_ACCUM | OSP_FB_DEPTH);

    {
      std::string testCaseName = testCase->name();
      std::string testInfoName = testInfo->name();
      size_t pos               = testCaseName.find('/');
      if (pos == std::string::npos) {
        testName = testCaseName + "_" + testInfoName;
      } else {
        std::string instantiationName = testCaseName.substr(0, pos);
        std::string className         = testCaseName.substr(pos + 1);
        testName = className + "_" + instantiationName + "_" + testInfoName;
      }
      for (char &byte : testName)
        if (byte == '/')
          byte = '_';
    }

    rendererType    = "scivis";
    frames          = 1;
    samplesPerPixel = 16;

    imageTool.reset(
        new OSPImageTools(imgSize, GetTestName(), frameBufferFormat));
  }

  void Base::SetUp()
  {
    ASSERT_NO_FATAL_FAILURE(CreateEmptyScene());
  }

  void Base::AddLight(cpp::Light light)
  {
    light.commit();
    lightsList.push_back(light);
  }

  void Base::AddModel(cpp::GeometricModel model, affine3f xfm)
  {
    model.commit();

    cpp::Group group;
    group.setParam("geometry", cpp::Data(model));
    group.commit();

    cpp::Instance instance(group);
    instance.setParam("xfm", xfm);

    AddInstance(instance);
  }

  void Base::AddModel(cpp::VolumetricModel model, affine3f xfm)
  {
    model.commit();

    cpp::Group group;
    group.setParam("volume", cpp::Data(model));
    group.commit();

    cpp::Instance instance(group);
    instance.setParam("xfm", xfm);

    AddInstance(instance);
  }

  void Base::AddInstance(cpp::Instance instance)
  {
    instance.commit();
    instances.push_back(instance);
  }

  void Base::PerformRenderTest()
  {
    SetLights();

    if (!instances.empty())
      world.setParam("instance", cpp::Data(instances));

    camera.commit();
    world.commit();
    renderer.commit();

    framebuffer.resetAccumulation();

    RenderFrame();

    auto *framebuffer_data = (uint32_t *)framebuffer.map(OSP_FB_COLOR);

    if (ospEnv->GetDumpImg()) {
      EXPECT_EQ(imageTool->saveTestImage(framebuffer_data), OsprayStatus::Ok);
    } else {
      EXPECT_EQ(imageTool->compareImgWithBaseline(framebuffer_data),
                OsprayStatus::Ok);
    }

    framebuffer.unmap(framebuffer_data);
  }

  void Base::CreateEmptyScene()
  {
    camera = cpp::Camera("perspective");
    camera.setParam("aspect", imgSize.x / (float)imgSize.y);
    camera.setParam("position", vec3f(0.f));
    camera.setParam("direction", vec3f(0.f, 0.f, 1.f));
    camera.setParam("up", vec3f(0.f, 1.f, 0.f));

    renderer = cpp::Renderer(rendererType);
    renderer.setParam("aoSamples", 0);
    renderer.setParam("bgColor", vec3f(1.0f));
    renderer.setParam("spp", samplesPerPixel);

    world = cpp::World();
  }

  void Base::SetLights()
  {
    if (!lightsList.empty())
      world.setParam("light", cpp::Data(lightsList));
  }

  void Base::RenderFrame()
  {
    for (int frame = 0; frame < frames; ++frame)
      framebuffer.renderFrame(renderer, camera, world);
  }

  SpherePrecision::SpherePrecision()
  {
    auto params  = GetParam();
    radius       = std::get<0>(params);
    dist         = std::get<1>(params) * radius;
    move_cam     = std::get<2>(params);
    rendererType = std::get<3>(params);
  }

  void SpherePrecision::SetUp()
  {
    Base::SetUp();

    float fov  = 160.0f * std::min(std::tan(radius / std::abs(dist)), 1.0f);
    float cent = move_cam ? 0.0f : dist + radius;

    camera.setParam("position",
                    vec3f(0.f, 0.f, move_cam ? -dist - radius : 0.0f));
    camera.setParam("direction", vec3f(0.f, 0.f, 1.f));
    camera.setParam("up", vec3f(0.f, 1.f, 0.f));
    camera.setParam("fovy", fov);

    renderer.setParam("spp", 16);
    renderer.setParam("bgColor", vec4f(0.2f, 0.2f, 0.4f, 1.0f));
    // scivis params
    renderer.setParam("aoSamples", 16);
    renderer.setParam("aoIntensity", 1.f);
    // pathtracer params
    renderer.setParam("maxDepth", 2);

    cpp::Geometry sphere("spheres");
    cpp::Geometry inst_sphere("spheres");

    std::vector<vec3f> sph_center = {
        vec3f(-0.5f * radius, -0.3f * radius, cent),
        vec3f(0.8f * radius, -0.3f * radius, cent),
        vec3f(0.8f * radius, 1.5f * radius, cent),
        vec3f(0.0f, -10001.3f * radius, cent)};

    std::vector<float> sph_radius = {
        radius, 0.9f * radius, 0.9f * radius, 10000.f * radius};

    sphere.setParam("sphere.position", cpp::Data(sph_center));
    sphere.setParam("sphere.radius", cpp::Data(sph_radius));
    sphere.commit();

    inst_sphere.setParam("sphere.position", cpp::Data(vec3f(0.f)));
    inst_sphere.setParam("sphere.radius", cpp::Data(90.f * radius));
    inst_sphere.commit();

    cpp::GeometricModel model1(sphere);
    cpp::GeometricModel model2(inst_sphere);

    cpp::Material sphereMaterial(rendererType, "default");
    sphereMaterial.setParam("d", 1.0f);
    sphereMaterial.commit();

    model1.setParam("material", cpp::Data(sphereMaterial));
    model2.setParam("material", cpp::Data(sphereMaterial));

    affine3f xfm(vec3f(0.01, 0, 0),
                 vec3f(0, 0.01, 0),
                 vec3f(0, 0, 0.01),
                 vec3f(-0.5f * radius, 1.6f * radius, cent));

    AddModel(model1);
    AddModel(model2, xfm);

    cpp::Light distant("distant");
    distant.setParam("intensity", 3.0f);
    distant.setParam("direction", vec3f(0.3f, -4.0f, 0.8f));
    distant.setParam("color", vec3f(1.0f, 0.5f, 0.5f));
    distant.setParam("angularDiameter", 1.0f);
    AddLight(distant);

    cpp::Light ambient = ospNewLight("ambient");
    ambient.setParam("intensity", 0.1f);
    AddLight(ambient);
  }

  Torus::Torus()
  {
    rendererType = GetParam();
  }

  void Torus::SetUp()
  {
    Base::SetUp();

    camera.setParam("position", vec3f(-0.7f, -1.4f, 0.f));
    camera.setParam("direction", vec3f(0.5f, 1.f, 0.f));
    camera.setParam("up", vec3f(0.f, 0.f, -1.f));

    cpp::Volume torus = CreateTorus(256);
    torus.commit();

    cpp::VolumetricModel volumetricModel(torus);

    cpp::TransferFunction transferFun("piecewise_linear");
    transferFun.setParam("valueRange", vec2f(-10000.f, 10000.f));

    std::vector<vec3f> colors = {vec3f(1.0f, 0.0f, 0.0f),
                                 vec3f(0.0f, 1.0f, 0.0f)};

    std::vector<float> opacities = {1.0f, 1.0f};

    transferFun.setParam("color", cpp::Data(colors));
    transferFun.setParam("opacity", cpp::Data(opacities));
    transferFun.commit();

    volumetricModel.setParam("transferFunction", transferFun);
    volumetricModel.commit();

    cpp::Geometry isosurface("isosurfaces");
    isosurface.setParam("volume", volumetricModel);

    std::vector<float> isovalues = {-7000.f, 0.f};
    isosurface.setParam("isovalue", cpp::Data(isovalues));
    isosurface.commit();

    cpp::GeometricModel model(isosurface);
    AddModel(model);

    cpp::Light ambient = ospNewLight("ambient");
    ambient.setParam("intensity", 0.5f);
    AddLight(ambient);
  }

  TextureVolume::TextureVolume()
  {
    rendererType = GetParam();
  }

  void TextureVolume::SetUp()
  {
    Base::SetUp();

    camera.setParam("position", vec3f(-0.7f, -1.4f, 0.f));
    camera.setParam("direction", vec3f(0.5f, 1.f, 0.f));
    camera.setParam("up", vec3f(0.f, 0.f, -1.f));

    cpp::Volume torus = CreateTorus(256);
    torus.commit();

    cpp::VolumetricModel volumetricModel(torus);

    cpp::TransferFunction transferFun("piecewise_linear");
    transferFun.setParam("valueRange", vec2f(-10000.f, 10000.f));

    std::vector<vec3f> colors = {vec3f(1.0f, 0.0f, 0.0f),
                                 vec3f(0.0f, 1.0f, 0.0f)};

    std::vector<float> opacities = {1.0f, 1.0f};

    transferFun.setParam("color", cpp::Data(colors));
    transferFun.setParam("opacity", cpp::Data(opacities));
    transferFun.commit();

    volumetricModel.setParam("transferFunction", transferFun);
    volumetricModel.commit();

    cpp::Texture tex("volume");
    tex.setParam("volume", volumetricModel);
    tex.commit();

    cpp::Material sphereMaterial(rendererType, "default");
    sphereMaterial.setParam("map_Kd", tex);
    sphereMaterial.commit();

    cpp::Geometry sphere("spheres");
    sphere.setParam("sphere.position", cpp::Data(vec3f(0.f)));
    sphere.setParam("radius", 0.51f);
    sphere.commit();

    cpp::GeometricModel model(sphere);
    model.setParam("material", cpp::Data(sphereMaterial));
    AddModel(model);

    cpp::Light ambient("ambient");
    ambient.setParam("intensity", 0.5f);
    AddLight(ambient);
  }

  DepthCompositeVolume::DepthCompositeVolume()
  {
    auto params  = GetParam();
    rendererType = std::get<0>(params);
    bgColor      = std::get<1>(params);
  }

  void DepthCompositeVolume::SetUp()
  {
    Base::SetUp();

    camera.setParam("position", vec3f(0.f, 0.f, 1.f));
    camera.setParam("direction", vec3f(0.f, 0.f, -1.f));
    camera.setParam("up", vec3f(0.f, 1.f, 0.f));

    cpp::Volume torus = CreateTorus(256);
    torus.commit();

    cpp::VolumetricModel volumetricModel(torus);

    cpp::TransferFunction transferFun("piecewise_linear");
    transferFun.setParam("valueRange", vec2f(-10000.f, 10000.f));

    std::vector<vec3f> colors = {vec3f(1.0f, 0.0f, 0.0f),
                                 vec3f(0.0f, 1.0f, 0.0f)};

    std::vector<float> opacities = {0.05f, 1.0f};

    transferFun.setParam("color", cpp::Data(colors));
    transferFun.setParam("opacity", cpp::Data(opacities));
    transferFun.commit();

    volumetricModel.setParam("transferFunction", transferFun);
    volumetricModel.commit();

    AddModel(volumetricModel);

    cpp::Texture depthTex("texture2d");

    std::vector<float> texData(imgSize.product());
    for (int y = 0; y < imgSize.y; y++) {
      for (int x = 0; x < imgSize.x; x++) {
        const size_t index = imgSize.x * y + x;
        if (x < imgSize.x / 3) {
          texData[index] = 999.f;
        } else if (x < (imgSize.x * 2) / 3) {
          texData[index] = 0.75f;
        } else {
          texData[index] = 0.00001f;
        }
      }
    }

    depthTex.setParam<int>("format", OSP_TEXTURE_R32F);
    depthTex.setParam<int>("filter", OSP_TEXTURE_FILTER_NEAREST);
    depthTex.setParam("data", cpp::Data(imgSize, texData.data()));
    depthTex.commit();

    renderer.setParam("maxDepthTexture", depthTex);
    renderer.setParam("bgColor", bgColor);

    cpp::Light ambient("ambient");
    ambient.setParam("intensity", 0.5f);
    AddLight(ambient);
  }

  /* heavily based on Perlin's Java reference implementation of
   * the improved perlin noise paper from Siggraph 2002 from here
   * https://mrl.nyu.edu/~perlin/noise/
   **/
  class PerlinNoise
  {
    struct PerlinNoiseData
    {
      PerlinNoiseData()
      {
        const int permutation[256] = {
            151, 160, 137, 91,  90,  15,  131, 13,  201, 95,  96,  53,  194,
            233, 7,   225, 140, 36,  103, 30,  69,  142, 8,   99,  37,  240,
            21,  10,  23,  190, 6,   148, 247, 120, 234, 75,  0,   26,  197,
            62,  94,  252, 219, 203, 117, 35,  11,  32,  57,  177, 33,  88,
            237, 149, 56,  87,  174, 20,  125, 136, 171, 168, 68,  175, 74,
            165, 71,  134, 139, 48,  27,  166, 77,  146, 158, 231, 83,  111,
            229, 122, 60,  211, 133, 230, 220, 105, 92,  41,  55,  46,  245,
            40,  244, 102, 143, 54,  65,  25,  63,  161, 1,   216, 80,  73,
            209, 76,  132, 187, 208, 89,  18,  169, 200, 196, 135, 130, 116,
            188, 159, 86,  164, 100, 109, 198, 173, 186, 3,   64,  52,  217,
            226, 250, 124, 123, 5,   202, 38,  147, 118, 126, 255, 82,  85,
            212, 207, 206, 59,  227, 47,  16,  58,  17,  182, 189, 28,  42,
            223, 183, 170, 213, 119, 248, 152, 2,   44,  154, 163, 70,  221,
            153, 101, 155, 167, 43,  172, 9,   129, 22,  39,  253, 19,  98,
            108, 110, 79,  113, 224, 232, 178, 185, 112, 104, 218, 246, 97,
            228, 251, 34,  242, 193, 238, 210, 144, 12,  191, 179, 162, 241,
            81,  51,  145, 235, 249, 14,  239, 107, 49,  192, 214, 31,  181,
            199, 106, 157, 184, 84,  204, 176, 115, 121, 50,  45,  127, 4,
            150, 254, 138, 236, 205, 93,  222, 114, 67,  29,  24,  72,  243,
            141, 128, 195, 78,  66,  215, 61,  156, 180};
        for (int i = 0; i < 256; i++)
          p[256 + i] = p[i] = permutation[i];
      }
      inline int operator[](size_t idx) const
      {
        return p[idx];
      }
      int p[512];
    };

    static PerlinNoiseData p;
    static inline float smooth(float t)
    {
      return t * t * t * (t * (t * 6.f - 15.f) + 10.f);
    }
    static inline float lerp(float t, float a, float b)
    {
      return a + t * (b - a);
    }
    static inline float grad(int hash, float x, float y, float z)
    {
      const int h   = hash & 15;
      const float u = h < 8 ? x : y;
      const float v = h < 4 ? y : h == 12 || h == 14 ? x : z;
      return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
    }

   public:
    static float noise(vec3f q, float frequency = 8.f)
    {
      float x     = q.x * frequency;
      float y     = q.y * frequency;
      float z     = q.z * frequency;
      const int X = (int)floor(x) & 255;
      const int Y = (int)floor(y) & 255;
      const int Z = (int)floor(z) & 255;
      x -= floor(x);
      y -= floor(y);
      z -= floor(z);
      const float u = smooth(x);
      const float v = smooth(y);
      const float w = smooth(z);
      const int A   = p[X] + Y;
      const int B   = p[X + 1] + Y;
      const int AA  = p[A] + Z;
      const int BA  = p[B] + Z;
      const int BB  = p[B + 1] + Z;
      const int AB  = p[A + 1] + Z;

      return lerp(
          w,
          lerp(v,
               lerp(u, grad(p[AA], x, y, z), grad(p[BA], x - 1, y, z)),
               lerp(u, grad(p[AB], x, y - 1, z), grad(p[BB], x - 1, y - 1, z))),
          lerp(v,
               lerp(u,
                    grad(p[AA + 1], x, y, z - 1),
                    grad(p[BA + 1], x - 1, y, z - 1)),
               lerp(u,
                    grad(p[AB + 1], x, y - 1, z - 1),
                    grad(p[BB + 1], x - 1, y - 1, z - 1))));
    }
  };
  PerlinNoise::PerlinNoiseData PerlinNoise::p;

  HeterogeneousVolume::HeterogeneousVolume()
  {
    rendererType = "pathtracer";

    auto params = GetParam();

    albedo             = std::get<0>(params);
    anisotropy         = std::get<1>(params);
    densityScale       = std::get<2>(params);
    ambientColor       = std::get<3>(params);
    enableDistantLight = std::get<4>(params);
    enableGeometry     = std::get<5>(params);
    constantVolume     = std::get<6>(params);
    samplesPerPixel    = std::get<7>(params);

    imgSize = imgSize / 2.f;
  }

  void HeterogeneousVolume::SetUp()
  {
    Base::SetUp();

    if (!constantVolume)
      densityScale *= 10.f;

    const float theta = 1.f / 2.f * M_PI - M_PI / 6;
    const float phi   = 3.f / 2.f * M_PI - M_PI / 6;
    const float r     = 5.f;
    vec3f p =
        r * vec3f(sin(theta) * cos(phi), cos(theta), sin(theta) * sin(phi));
    vec3f v = vec3f(0.f, -0.2f, 0.f) - p;

    camera.setParam("position", p);
    camera.setParam("direction", v);
    camera.setParam("up", vec3f(0.f, 1.f, 0.f));

    // create volume
    vec3l dims;
    if (constantVolume)
      dims = vec3l(8, 8, 8);
    else
      dims = vec3l(64, 64, 64);

    const float spacing = 3.f / (reduce_max(dims) - 1);
    cpp::Volume volume("structured_volume");

    auto turbulence = [](const vec3f &p, float base_freqency, int octaves) {
      float value = 0.f;
      float scale = 1.f;
      for (int o = 0; o < octaves; ++o) {
        value += PerlinNoise::noise(scale * p, base_freqency) / scale;
        scale *= 2.f;
      }
      return value;
    };

    // generate volume data
    auto numVoxels = dims.product();
    std::vector<float> voxels(numVoxels, 0);
    for (int z = 0; z < dims.z; ++z)
      for (int y = 0; y < dims.y; ++y)
        for (int x = 0; x < dims.x; ++x) {
          if (constantVolume)
            voxels[dims.x * dims.y * z + dims.x * y + x] = 1.0f;
          else {
            vec3f p = vec3f(x + 0.5f, y + 0.5f, z + 0.5f) / dims;
            vec3f X = 2.f * p - vec3f(1.f);
            if (length((1.4f + 0.4 * turbulence(p, 12.f, 12)) * X) < 1.f)
              voxels[dims.x * dims.y * z + dims.x * y + x] =
                  0.5f + 0.5f * PerlinNoise::noise(p, 12);
          }
        }
    voxels[0] = 0.0f;

    volume.setParam("voxelData", cpp::Data(voxels));
    volume.setParam<int>("voxelType", OSP_FLOAT);
    volume.setParam<vec3i>("dimensions", dims);
    volume.setParam("gridOrigin", vec3f(-1.f));
    volume.setParam("gridSpacing", vec3f(spacing));
    volume.commit();

    cpp::TransferFunction transferFun("piecewise_linear");
    transferFun.setParam("valueRange", vec2f(0.f, 1.f));

    std::vector<float> opacities = {1.0f, 1.0f};

    transferFun.setParam("color", cpp::Data(albedo));
    transferFun.setParam("opacity", cpp::Data(opacities));
    transferFun.commit();

    cpp::VolumetricModel volumetricModel(volume);
    volumetricModel.setParam("densityScale", densityScale);
    volumetricModel.setParam("anisotropy", anisotropy);
    volumetricModel.setParam("transferFunction", transferFun);
    volumetricModel.commit();

    cpp::Group volumeGroup;
    volumeGroup.setParam("volume", cpp::Data(volumetricModel));
    volumeGroup.commit();

    cpp::Instance volumeInstance(volumeGroup);
    if (!constantVolume)
      volumeInstance.setParam("xfm", AffineSpace3f::scale(vec3f(1.25f)));
    AddInstance(volumeInstance);

    if (enableGeometry) {
      std::vector<vec3f> planeVertices = {vec3f(-8.f, -2.5f, -8.f),
                                          vec3f(+8.f, -2.5f, -8.f),
                                          vec3f(+8.f, -2.5f, +8.f),
                                          vec3f(-8.f, -2.5f, +8.f)};

      cpp::Geometry mesh("quads");
      mesh.setParam("vertex.position", cpp::Data(planeVertices));
      mesh.setParam("index", cpp::Data(vec4ui(0, 1, 2, 3)));
      mesh.commit();

      cpp::GeometricModel model(mesh);
      cpp::Material material("pathtracer", "OBJMaterial");
      material.commit();

      model.setParam("material", cpp::Data(material));
      model.commit();

      cpp::Group modelsGroup;
      modelsGroup.setParam("geometry", cpp::Data(model));
      modelsGroup.commit();

      AddInstance(cpp::Instance(modelsGroup));
    }

    vec4f backgroundColor(ambientColor, 1.f);

    cpp::Light ambientLight("ambient");
    ambientLight.setParam("intensity", 1.f);
    ambientLight.setParam("color", ambientColor);
    AddLight(ambientLight);

    if (enableDistantLight) {
      cpp::Light distantLight("distant");
      distantLight.setParam("intensity", 2.6f);
      distantLight.setParam("color", vec3f(1.0f, 0.96f, 0.88f));
      distantLight.setParam("angularDiameter", 1.f);
      distantLight.setParam("direction", vec3f(-0.5826f, -0.7660f, -0.2717f));
      AddLight(distantLight);
    }

    cpp::Texture backplateTexture("texture2d");
    backplateTexture.setParam("size", vec2i(1));
    backplateTexture.setParam<int>("format", OSP_TEXTURE_RGB32F);
    backplateTexture.setParam<int>("filter", OSP_TEXTURE_FILTER_NEAREST);
    backplateTexture.setParam("data", cpp::Data(ambientColor));
    backplateTexture.commit();

    renderer.setParam("backplate", backplateTexture);

    renderer.setParam("maxDepth", std::max(20, samplesPerPixel));
    renderer.setParam("spp", samplesPerPixel);
  }
}  // namespace OSPRayTestScenes
