// Copyright 2017-2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_fixture.h"
#include "ArcballCamera.h"
// ospray_testing
#include "ospray_testing.h"
// ospcommon
#include "ospcommon/utility/multidim_index_sequence.h"

extern OSPRayEnvironment *ospEnv;

namespace OSPRayTestScenes {

// Helper functions /////////////////////////////////////////////////////////

// creates a torus
// volumetric data: stores data of torus
// returns created ospvolume of torus
static cpp::Volume CreateTorus(const int size)
{
  std::vector<float> volumetricData(size * size * size, 0);

  const float r = 30;
  const float R = 80;
  const int size_2 = size / 2;

  for (int x = 0; x < size; ++x) {
    for (int y = 0; y < size; ++y) {
      for (int z = 0; z < size; ++z) {
        const float X = x - size_2;
        const float Y = y - size_2;
        const float Z = z - size_2;

        const float d = (R - std::sqrt(X * X + Y * Y));
        volumetricData[size * size * x + size * y + z] = r * r - d * d - Z * Z;
      }
    }
  }

  cpp::Volume torus("structured_regular");
  torus.setParam("data", cpp::Data(vec3ul(size), volumetricData.data()));
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
    size_t pos = testCaseName.find('/');
    if (pos == std::string::npos) {
      testName = testCaseName + "_" + testInfoName;
    } else {
      std::string instantiationName = testCaseName.substr(0, pos);
      std::string className = testCaseName.substr(pos + 1);
      testName = className + "_" + instantiationName + "_" + testInfoName;
    }
    for (char &byte : testName)
      if (byte == '/')
        byte = '_';
  }

  rendererType = "scivis";
  frames = 1;
  samplesPerPixel = 16;

  imageTool.reset(new OSPImageTools(imgSize, GetTestName(), frameBufferFormat));
}

void Base::SetUp()
{
  CreateEmptyScene();
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
    EXPECT_EQ(
        imageTool->compareImgWithBaseline(framebuffer_data), OsprayStatus::Ok);
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
  if (rendererType == "scivis")
    renderer.setParam("aoSamples", 0);
  renderer.setParam("backgroundColor", vec3f(1.0f));
  renderer.setParam("pixelSamples", samplesPerPixel);

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
  auto params = GetParam();
  radius = std::get<0>(params);
  dist = std::get<1>(params) * radius;
  move_cam = std::get<2>(params);
  rendererType = std::get<3>(params);
}

void SpherePrecision::SetUp()
{
  Base::SetUp();

  float fov = 160.0f * std::min(std::tan(radius / std::abs(dist)), 1.0f);
  float cent = move_cam ? 0.0f : dist + radius;

  camera.setParam(
      "position", vec3f(0.f, 0.f, move_cam ? -dist - radius : 0.0f));
  camera.setParam("direction", vec3f(0.f, 0.f, 1.f));
  camera.setParam("up", vec3f(0.f, 1.f, 0.f));
  camera.setParam("fovy", fov);

  renderer.setParam("pixelSamples", 16);
  renderer.setParam("backgroundColor", vec4f(0.2f, 0.2f, 0.4f, 1.0f));
  if (rendererType == "scivis") {
    renderer.setParam("aoSamples", 16);
    renderer.setParam("aoIntensity", 1.f);
  } else if (rendererType == "pathtracer") {
    renderer.setParam("maxPathLength", 2);
  }

  cpp::Geometry sphere("sphere");
  cpp::Geometry inst_sphere("sphere");

  std::vector<vec3f> sph_center = {vec3f(-0.5f * radius, -0.3f * radius, cent),
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

  cpp::Material sphereMaterial(rendererType, "obj");
  sphereMaterial.setParam("d", 1.0f);
  sphereMaterial.commit();

  model1.setParam("material", sphereMaterial);
  model2.setParam("material", sphereMaterial);

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

Texture2D::Texture2D()
{
  rendererType = "pathtracer";
  samplesPerPixel = 64;
}

void Texture2D::SetUp()
{
  Base::SetUp();
  camera.setParam("position", vec3f(4.f, 3.f, 0.f));
  // flip image to have origin in upper left corner, plus mirror
  camera.setParam("imageStart", vec2f(1.f, 1.f));
  camera.setParam("imageEnd", vec2f(0.f, 0.f));
  auto params = GetParam();
  int filter = std::get<0>(params);

  // create (4*2) x 4 grid
  constexpr int cols = 8;
  constexpr int rows = 4;
  std::array<vec3f, (cols + 1) * (rows + 1)> vertex;
  ospcommon::index_sequence_2D vidx(vec2i(cols + 1, rows + 1));
  for (auto i : vidx)
    vertex[vidx.flatten(i)] = vec3f(i.x, i.y * 1.5f, 5.3f);
  std::array<vec4ui, cols * rows> index;
  ospcommon::index_sequence_2D iidx(vec2i(cols, rows));
  for (auto i : iidx) {
    index[iidx.flatten(i)] = vec4i(vidx.flatten({i.x, i.y}),
        vidx.flatten({i.x + 1, i.y}),
        vidx.flatten({i.x + 1, i.y + 1}),
        vidx.flatten({i.x, i.y + 1}));
  }
  cpp::Geometry mesh("mesh");
  mesh.setParam("vertex.position", cpp::Data(vertex));
  mesh.setParam("index", cpp::Data(index));
  mesh.commit();

  // create textures: columns=#channels, rows=type=[byte, byte, float, short]
  std::array<OSPTextureFormat, 4 * 4> format = {OSP_TEXTURE_R8,
      OSP_TEXTURE_RA8,
      OSP_TEXTURE_RGB8,
      OSP_TEXTURE_RGBA8,

      OSP_TEXTURE_L8,
      OSP_TEXTURE_LA8,
      OSP_TEXTURE_SRGB,
      OSP_TEXTURE_SRGBA,

      OSP_TEXTURE_R32F,
      OSP_TEXTURE_R32F,
      OSP_TEXTURE_RGB32F,
      OSP_TEXTURE_RGBA32F,

      OSP_TEXTURE_R16,
      OSP_TEXTURE_RA16,
      OSP_TEXTURE_RGB16,
      OSP_TEXTURE_RGBA16};

  std::array<OSPDataType, 4 * 4> eltype = {OSP_UCHAR,
      OSP_VEC2UC,
      OSP_VEC3UC,
      OSP_VEC4UC,

      OSP_UCHAR,
      OSP_VEC2UC,
      OSP_VEC3UC,
      OSP_VEC4UC,

      OSP_FLOAT,
      OSP_FLOAT,
      OSP_VEC3F,
      OSP_VEC4F,

      OSP_USHORT,
      OSP_VEC2US,
      OSP_VEC3US,
      OSP_VEC4US};

  std::array<vec4uc, 15> dbyte;
  std::array<vec4us, 15> dshort;
  std::array<vec4f, 15> dfloat;
  ospcommon::index_sequence_2D didx(vec2i(3, 5));
  for (auto idx : didx) {
    auto i = didx.flatten(idx);
    // the center texel should be 127 / 32767 / 0.5, to test normal maps
    dbyte[i] = vec4uc(idx.x * 85 + 42, idx.y * 50 + 27, 155, i * 17 + 8);
    dshort[i] = vec4us(
        idx.x * 22768 + 9999, idx.y * 13000 + 6767, 33000, i * 4400 + 2200);
    dfloat[i] = vec4f(idx.x * 0.3125f + 0.1875f,
        idx.y * 0.21875f + 0.0625f,
        0.8f,
        i * 0.06f + 0.15f);
  }

  std::array<void *, 4> addr = {
      dbyte.data(), dbyte.data(), dfloat.data(), dshort.data()};
  std::array<int, 4> stride = {4, 4, 16, 8};

  cpp::GeometricModel model(mesh);
  std::array<cpp::Material, cols * rows> material;
  for (auto idx : iidx) {
    auto i = iidx.flatten(idx);
    auto fmt = format[i / 2];
    auto eltp = eltype[i / 2];
    cpp::Texture tex("texture2d");
    tex.setParam("format", fmt);
    tex.setParam("filter", filter);
    auto tmp = ospNewSharedData(addr[idx.y], eltp, 3, stride[idx.y], 5);
    auto data = ospNewData(eltp, 3, 5);
    ospCopyData(tmp, data);
    ospRelease(tmp);
    tex.setParam("data", data);
    tex.commit();
    cpp::Material mat(rendererType, "obj");
    mat.setParam(i & 1 ? "map_bump" : "map_kd", tex);
    mat.commit();
    material[i] = mat;
  }
  model.setParam("material", cpp::Data(material));
  AddModel(model);

  { // set up backplate texture
    std::array<vec3uc, 125 * 94> bpdata;
    bool toggle = false;
    for (auto &el : bpdata) {
      el = toggle ? vec3uc(5, 10, 5) : vec3uc(10, 5, 10);
      toggle = !toggle;
    }
    cpp::Data texData(vec2ul(125, 94), bpdata.data());

    cpp::Texture backplateTex("texture2d");
    backplateTex.setParam("format", OSP_TEXTURE_RGB8);
    backplateTex.setParam("filter", OSP_TEXTURE_FILTER_NEAREST);
    backplateTex.setParam("data", texData);
    backplateTex.commit();

    renderer.setParam("map_backplate", backplateTex);
  }

  cpp::Light light1("distant");
  cpp::Light light2("distant");

  // two light sets
  if (std::get<1>(params)) {
    // default light
    light1.setParam("intensity", 3.f);
    // highlighting normal direction
    light2.setParam("direction", vec3f(0.7f, 0.2f, 0.f));
    light2.setParam("color", vec3f(0.f, 0.f, 0.5f));
  } else {
    // perpendicular bright lights, testing center normal
    light1.setParam("direction", vec3f(0.5f, -1.f, 0.f));
    light1.setParam("color", vec3f(999.f, 0.f, 0.0f));
    light2.setParam("direction", vec3f(-0.5f, 1.f, 0.f));
    light2.setParam("color", vec3f(0.f, 999.f, 0.0f));
  }

  AddLight(light1);
  AddLight(light2);
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

  cpp::TransferFunction transferFun("piecewiseLinear");
  transferFun.setParam("valueRange", vec2f(-10000.f, 10000.f));

  std::vector<vec3f> colors = {
      vec3f(1.0f, 0.0f, 0.0f), vec3f(0.0f, 1.0f, 0.0f)};

  std::vector<float> opacities = {1.0f, 1.0f};

  transferFun.setParam("color", cpp::Data(colors));
  transferFun.setParam("opacity", cpp::Data(opacities));
  transferFun.commit();

  volumetricModel.setParam("transferFunction", transferFun);
  volumetricModel.commit();

  cpp::Texture tex("volume");
  tex.setParam("volume", volumetricModel);
  tex.commit();

  cpp::Material sphereMaterial(rendererType, "obj");
  sphereMaterial.setParam("map_kd", tex);
  sphereMaterial.commit();

  cpp::Geometry sphere("sphere");
  sphere.setParam("sphere.position", cpp::Data(vec3f(0.f)));
  sphere.setParam("radius", 0.51f);
  sphere.commit();

  cpp::GeometricModel model(sphere);
  model.setParam("material", sphereMaterial);
  AddModel(model);

  cpp::Light ambient("ambient");
  ambient.setParam("intensity", 0.5f);
  AddLight(ambient);
}

DepthCompositeVolume::DepthCompositeVolume()
{
  auto params = GetParam();
  rendererType = std::get<0>(params);
  bgColor = std::get<1>(params);
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

  cpp::TransferFunction transferFun("piecewiseLinear");
  transferFun.setParam("valueRange", vec2f(-10000.f, 10000.f));

  std::vector<vec3f> colors = {
      vec3f(1.0f, 0.0f, 0.0f), vec3f(0.0f, 1.0f, 0.0f)};

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

  depthTex.setParam("format", OSP_TEXTURE_R32F);
  depthTex.setParam("filter", OSP_TEXTURE_FILTER_NEAREST);
  depthTex.setParam("data", cpp::Data(imgSize, texData.data()));
  depthTex.commit();

  renderer.setParam("map_maxDepth", depthTex);
  renderer.setParam("backgroundColor", bgColor);

  cpp::Light ambient("ambient");
  ambient.setParam("intensity", 0.5f);
  AddLight(ambient);
}

RendererMaterialList::RendererMaterialList()
{
  rendererType = GetParam();
}

void RendererMaterialList::SetUp()
{
  Base::SetUp();

  // Setup geometry //

  cpp::Geometry sphereGeometry("sphere");

  constexpr int dimSize = 3;

  ospcommon::index_sequence_2D numSpheres(dimSize);

  std::vector<vec3f> spheres;
  std::vector<uint32_t> index;
  std::vector<cpp::Material> materials;

  auto makeObjMaterial =
      [](const std::string &rendererType, vec3f Kd, vec3f Ks) -> cpp::Material {
    cpp::Material mat(rendererType, "obj");
    mat.setParam("kd", Kd);
    if (rendererType == "pathtracer")
      mat.setParam("ks", Ks);
    mat.commit();

    return mat;
  };

  for (auto i : numSpheres) {
    auto i_f = static_cast<vec2f>(i);
    spheres.emplace_back(i_f.x, i_f.y, 0.f);

    auto l = i_f / (dimSize - 1);
    materials.push_back(makeObjMaterial(rendererType,
        lerp(l.x, vec3f(0.1f), vec3f(0.f, 0.f, 1.f)),
        lerp(l.y, vec3f(0.f), vec3f(1.f))));

    index.push_back(static_cast<uint32_t>(numSpheres.flatten(i)));
  }

  sphereGeometry.setParam("sphere.position", cpp::Data(spheres));
  sphereGeometry.setParam("radius", 0.4f);
  sphereGeometry.commit();

  cpp::GeometricModel model(sphereGeometry);
  model.setParam("material", cpp::Data(index));

  AddModel(model);

  // Setup renderer material list //

  renderer.setParam("material", cpp::Data(materials));

  // Create the world to get bounds for camera setup //

  if (!instances.empty())
    world.setParam("instance", cpp::Data(instances));

  instances.clear();

  world.commit();

  auto worldBounds = world.getBounds();

  ArcballCamera arcballCamera(worldBounds, imgSize);

  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());

  // Setup lights //

  cpp::Light ambient("ambient");
  ambient.setParam("intensity", 0.5f);
  AddLight(ambient);
}

FromOsprayTesting::FromOsprayTesting()
{
  auto params = GetParam();

  sceneName = std::get<0>(params);
  rendererType = std::get<1>(params);
}

void FromOsprayTesting::SetUp()
{
  Base::SetUp();

  instances.clear();

  auto builder = ospray::testing::newBuilder(sceneName);
  ospray::testing::setParam(builder, "rendererType", rendererType);
  ospray::testing::commit(builder);

  world = ospray::testing::buildWorld(builder);
  ospray::testing::release(builder);

  world.commit();

  auto worldBounds = world.getBounds();

  ArcballCamera arcballCamera(worldBounds, imgSize);

  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());
}

} // namespace OSPRayTestScenes
