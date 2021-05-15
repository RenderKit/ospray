// Copyright 2020 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "../BaseFixture.h"

class Simple : public BaseFixture
{
 public:
  Simple(const std::string &n, const std::string &s, const std::string &r)
      : BaseFixture(n + s, s, r)
  {}
};

OSPRAY_DEFINE_BENCHMARK(Simple, "boxes", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(Simple, "random_spheres", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "random_spheres", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "random_spheres", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(Simple, "random_spheres", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(Simple, "streamlines", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "streamlines", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "streamlines", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(Simple, "streamlines", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(Simple, "planes", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "planes", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "planes", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(Simple, "planes", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(Simple, "clip_gravity_spheres_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "clip_gravity_spheres_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "clip_gravity_spheres_volume", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(
    Simple, "clip_gravity_spheres_volume", "scivis");

OSPRAY_DEFINE_BENCHMARK(Simple, "clip_perlin_noise_volumes", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "clip_perlin_noise_volumes", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "clip_perlin_noise_volumes", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(
    Simple, "clip_perlin_noise_volumes", "scivis");

OSPRAY_DEFINE_BENCHMARK(Simple, "clip_particle_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "clip_particle_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "clip_particle_volume", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(Simple, "clip_particle_volume", "scivis");

OSPRAY_DEFINE_BENCHMARK(Simple, "particle_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "particle_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "particle_volume", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(Simple, "particle_volume", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(Simple, "particle_volume_isosurface", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "particle_volume_isosurface", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "particle_volume_isosurface", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(
    Simple, "particle_volume_isosurface", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(Simple, "vdb_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "vdb_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "vdb_volume", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(Simple, "vdb_volume", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(Simple, "unstructured_volume", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "unstructured_volume", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "unstructured_volume", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(Simple, "unstructured_volume", "pathtracer");

OSPRAY_DEFINE_BENCHMARK(Simple, "unstructured_volume_isosurface", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "unstructured_volume_isosurface", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "unstructured_volume_isosurface", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(
    Simple, "unstructured_volume_isosurface", "scivis");

OSPRAY_DEFINE_BENCHMARK(Simple, "gravity_spheres_amr", "ao");
OSPRAY_DEFINE_BENCHMARK(Simple, "gravity_spheres_amr", "scivis");
OSPRAY_DEFINE_BENCHMARK(Simple, "gravity_spheres_amr", "pathtracer");
OSPRAY_DEFINE_SETUP_BENCHMARK(Simple, "gravity_spheres_amr", "pathtracer");
