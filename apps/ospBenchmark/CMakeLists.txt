## Copyright 2018 Intel Corporation
## SPDX-License-Identifier: Apache-2.0

if (NOT OSPRAY_ENABLE_APPS_BENCHMARK)
  return()
endif()

find_package(benchmark REQUIRED)

add_executable(ospBenchmark
  ${OSPRAY_RESOURCE}

  ospBenchmark.cpp
  BaseFixture.cpp

  benchmarks/Simple.cpp
  benchmarks/BoxesAo.cpp
  benchmarks/CornellBoxSpp.cpp
  benchmarks/GravitySpheresVolume.cpp
  benchmarks/PerlinNoiseVolumes.cpp
  benchmarks/ClippingGeometries.cpp
)

target_link_libraries(ospBenchmark
PRIVATE
  arcball_camera
  benchmark::benchmark
  ospray_testing
)

ospray_sign_target(ospBenchmark)

install(TARGETS ospBenchmark
  DESTINATION ${CMAKE_INSTALL_BINDIR}
  COMPONENT apps
)
