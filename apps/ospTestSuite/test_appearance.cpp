// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "test_appearance.h"
#include "ArcballCamera.h"
#include "rkcommon/utility/multidim_index_sequence.h"

namespace OSPRayTestScenes {

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
  rkcommon::index_sequence_2D vidx(vec2i(cols + 1, rows + 1));
  for (auto i : vidx)
    vertex[vidx.flatten(i)] = vec3f(i.x, i.y * 1.5f, 5.3f);
  std::array<vec4ui, cols * rows> index;
  rkcommon::index_sequence_2D iidx(vec2i(cols, rows));
  for (auto i : iidx) {
    index[iidx.flatten(i)] = vec4i(vidx.flatten({i.x, i.y}),
        vidx.flatten({i.x + 1, i.y}),
        vidx.flatten({i.x + 1, i.y + 1}),
        vidx.flatten({i.x, i.y + 1}));
  }
  cpp::Geometry mesh("mesh");
  mesh.setParam("vertex.position", cpp::CopiedData(vertex));
  mesh.setParam("index", cpp::CopiedData(index));
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
  rkcommon::index_sequence_2D didx(vec2i(3, 5));
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
    ospRelease(data);
    cpp::Material mat(rendererType, "obj");
    mat.setParam("kd", vec3f(0.8));
    mat.setParam(i & 1 ? "map_bump" : "map_kd", tex);
    mat.commit();
    material[i] = mat;
  }
  model.setParam("material", cpp::CopiedData(material));
  AddModel(model);

  { // set up backplate texture
    std::array<vec3uc, 125 * 94> bpdata;
    bool toggle = false;
    for (auto &el : bpdata) {
      el = toggle ? vec3uc(5, 10, 5) : vec3uc(10, 5, 10);
      toggle = !toggle;
    }
    cpp::CopiedData texData(bpdata.data(), vec2ul(125, 94));

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

static cpp::Texture createTexture2D()
{
  // Prepare pixel data
  constexpr uint32_t width = 32;
  constexpr uint32_t height = 32;
  std::array<vec3uc, width * height> dbyte;
  rkcommon::index_sequence_2D idx(vec2i(width, height));
  for (auto i : idx)
    dbyte[idx.flatten(i)] =
        vec3uc(i.x * (256 / width), i.y * (256 / height), i.x * (256 / width));

  // Create texture object
  cpp::Texture tex("texture2d");
  tex.setParam("format", OSP_TEXTURE_RGB8);
  tex.setParam("data", cpp::CopiedData(dbyte.data(), vec2ul(width, height)));
  tex.commit();
  return tex;
}

Texture2DTransform::Texture2DTransform()
{
  rendererType = GetParam();
}

void Texture2DTransform::SetUp()
{
  Base::SetUp();

  camera.setParam("position", vec3f(4.f, 4.f, -10.f));
  camera.setParam("direction", vec3f(0.f, 0.f, 1.f));
  camera.setParam("up", vec3f(0.f, 1.f, 0.f));

  // Create quad geometry
  constexpr int cols = 2;
  constexpr int rows = 2;
  cpp::Geometry quads("mesh");
  {
    // Create quads data
    std::array<vec3f, 4 * cols * rows> position;
    std::array<vec4ui, cols * rows> index;
    rkcommon::index_sequence_2D idx(vec2i(cols, rows));
    for (auto i : idx) {
      auto l = static_cast<vec2f>(i) * 5.f;
      auto u = l + (.75f * 5.f);
      position[4 * idx.flatten(i) + 0] = vec3f(l.x, l.y, 0.f);
      position[4 * idx.flatten(i) + 1] = vec3f(l.x, u.y, 0.f);
      position[4 * idx.flatten(i) + 2] = vec3f(u.x, u.y, 0.f);
      position[4 * idx.flatten(i) + 3] = vec3f(u.x, l.y, 0.f);
      index[idx.flatten(i)] = vec4ui(0, 1, 2, 3) + 4 * idx.flatten(i);
    }

    // Set quads parameters
    quads.setParam("vertex.position", cpp::CopiedData(position));
    quads.setParam("index", cpp::CopiedData(index));
    quads.commit();
  }

  // Create materials
  std::array<cpp::Material, cols * rows> materials;
  cpp::Texture tex = createTexture2D();
  for (int i = 0; i < cols * rows; i++) {
    cpp::Material mat(rendererType, "obj");
    mat.setParam("map_kd", tex);
    mat.commit();
    materials[i] = mat;
  }

  // Set scale
  materials[1].setParam("map_kd.scale", vec2f(.5f));
  materials[1].commit();

  // Set rotation
  materials[2].setParam("map_kd.rotation", 45.f);
  materials[2].commit();

  // Set translation
  materials[3].setParam("map_kd.translation", vec2f(.5f));
  materials[3].commit();

  // Create geometric model
  cpp::GeometricModel model(quads);
  model.setParam("material", cpp::CopiedData(materials));
  AddModel(model);

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

  rkcommon::index_sequence_2D numSpheres(dimSize);

  std::vector<vec3f> spheres;
  std::vector<uint32_t> index;
  std::vector<cpp::Material> materials;

  auto makeObjMaterial =
      [](const std::string &rendererType, vec3f Kd, vec3f Ks) -> cpp::Material {
    cpp::Material mat(rendererType, "obj");
    mat.setParam("kd", Kd);
    if (rendererType == "pathtracer" || rendererType == "scivis")
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

  sphereGeometry.setParam("sphere.position", cpp::CopiedData(spheres));
  sphereGeometry.setParam("radius", 0.4f);
  sphereGeometry.commit();

  cpp::GeometricModel model(sphereGeometry);
  model.setParam("material", cpp::CopiedData(index));

  AddModel(model);

  // Setup renderer material list //

  renderer.setParam("material", cpp::CopiedData(materials));

  // Create the world to get bounds for camera setup //

  if (!instances.empty())
    world.setParam("instance", cpp::CopiedData(instances));

  instances.clear();

  world.commit();

  auto worldBounds = world.getBounds<box3f>();

  ArcballCamera arcballCamera(worldBounds, imgSize);

  camera.setParam("position", arcballCamera.eyePos());
  camera.setParam("direction", arcballCamera.lookDir());
  camera.setParam("up", arcballCamera.upDir());

  // Setup lights //

  cpp::Light ambient("ambient");
  ambient.setParam("intensity", 0.5f);
  AddLight(ambient);
}

PTBackgroundRefraction::PTBackgroundRefraction()
{
  rendererType = "pathtracer";
  samplesPerPixel = 64;
}

void PTBackgroundRefraction::SetUp()
{
  bool backgroundRefraction = GetParam();

  Base::SetUp();

  // Setup geometry //
  cpp::Geometry boxGeometry("box");

  rkcommon::index_sequence_3D numBoxes(vec3i(2, 2, 1));

  std::vector<box3f> boxes;

  for (auto i : numBoxes) {
    auto f3 = static_cast<vec3f>(i);
    auto lower = f3 * vec3f(1.5f, 1.5f, 0.0f) + vec3f(-1.25f, -1.25f, 2.5f);
    auto upper = lower + vec3f(1.f, 1.f, 0.27f);
    boxes.emplace_back(lower, upper);
  }

  boxGeometry.setParam("box", cpp::CopiedData(boxes));
  boxGeometry.commit();

  cpp::GeometricModel model(boxGeometry);

  std::vector<cpp::Material> materials;
  materials.emplace_back(cpp::Material(rendererType, "thinGlass"));
  materials.emplace_back(cpp::Material(rendererType, "glass"));
  materials.emplace_back(cpp::Material(rendererType, "glass"));
  materials.back().setParam("eta", 1.2f);
  materials.emplace_back(cpp::Material(rendererType, "obj"));
  materials.back().setParam("d", 0.2f);
  materials.back().setParam("kd", vec3f(0.7f, 0.5f, 0.1f));
  for (auto &m : materials)
    m.commit();
  model.setParam("material", cpp::CopiedData(materials));
  AddModel(model);

  renderer.setParam("backgroundRefraction", backgroundRefraction);

  { // set up backplate texture
    std::vector<vec4uc> bpdata;
    bpdata.push_back(vec4uc(199, 60, 10, 255));
    bpdata.push_back(vec4uc(60, 199, 40, 255));
    bpdata.push_back(vec4uc(80, 40, 199, 255));
    bpdata.push_back(vec4uc(99, 10, 99, 255));

    cpp::CopiedData texData(bpdata.data(), vec2ul(2, 2));

    cpp::Texture backplateTex("texture2d");
    backplateTex.setParam("format", OSP_TEXTURE_RGBA8);
    backplateTex.setParam("data", texData);
    backplateTex.commit();

    renderer.setParam("map_backplate", backplateTex);
  }

  cpp::Light light("sunSky");
  light.setParam("turbidity", 8.0f);
  light.setParam("intensity", 0.2f);
  AddLight(light);
  cpp::Light mirrorlight("sunSky");
  mirrorlight.setParam("up", vec3f(0.0f, -1.0f, 0.0f));
  AddLight(mirrorlight);
}

// Test Instantiations //////////////////////////////////////////////////////

TEST_P(RendererMaterialList, material_list)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(MaterialLists,
    RendererMaterialList,
    ::testing::Values("scivis", "pathtracer"));

INSTANTIATE_TEST_SUITE_P(TestScenesPtMaterials,
    FromOsprayTesting,
    ::testing::Combine(::testing::Values("test_pt_glass",
                           "test_pt_luminous",
                           "test_pt_metal_roughness",
                           "test_pt_metallic_flakes",
                           "test_pt_obj"),
        ::testing::Values("pathtracer")));

TEST_P(Texture2D, filter)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Appearance,
    Texture2D,
    ::testing::Values(std::make_tuple(OSP_TEXTURE_FILTER_BILINEAR, true),
        std::make_tuple(OSP_TEXTURE_FILTER_NEAREST, true),
        std::make_tuple(OSP_TEXTURE_FILTER_NEAREST, false)));

TEST_P(Texture2DTransform, simple)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(
    Appearance, Texture2DTransform, ::testing::Values("scivis"));

TEST_P(PTBackgroundRefraction, backgroundRefraction)
{
  PerformRenderTest();
}

INSTANTIATE_TEST_SUITE_P(Appearance, PTBackgroundRefraction, ::testing::Bool());

} // namespace OSPRayTestScenes
