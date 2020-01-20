// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// ospray_testing
#include "ospray_testing.h"
// ospcommon
#include "ospcommon/os/library.h"
// stl
#include <map>

using namespace ospcommon;
using namespace ospcommon::math;

namespace ospray {
namespace testing {

// Helper functions //

template <typename T>
inline T *objectFactory(
    const std::string &categoryName, const std::string &type)
{
  using namespace ospcommon;
  // Function pointer type for creating a concrete instance of a subtype of
  // this class.
  using creationFunctionPointer = T *(*)();

  // Function pointers corresponding to each subtype.
  static std::map<std::string, creationFunctionPointer> symbolRegistry;

  // Find the creation function for the subtype if not already known.
  if (symbolRegistry.count(type) == 0) {
    // Construct the name of the creation function to look for.
    std::string creationFunctionName =
        "ospray_create_" + categoryName + "__" + type;

    loadLibrary("ospray_testing", false);

    // Look for the named function.
    symbolRegistry[type] =
        (creationFunctionPointer)getSymbol(creationFunctionName);
  }

  // Create a concrete instance of the requested subtype.
  auto *object = symbolRegistry[type] ? (*symbolRegistry[type])() : nullptr;

  if (object == nullptr) {
    symbolRegistry.erase(type);
    throw std::runtime_error("Could not find " + categoryName + " of type: "
        + type + ".  Make sure you have the correct OSPRay libraries linked.");
  }

  return object;
}

// ospray_testing definitions //

SceneBuilderHandle newBuilder(const std::string &type)
{
  auto *b = objectFactory<detail::Builder>("testing_builder", type);
  return (SceneBuilderHandle)b;
}

cpp::Group buildGroup(SceneBuilderHandle _b)
{
  auto *b = (detail::Builder *)_b;
  return b->buildGroup();
}

cpp::World buildWorld(SceneBuilderHandle _b)
{
  auto *b = (detail::Builder *)_b;
  return b->buildWorld();
}

void commit(SceneBuilderHandle _b)
{
  auto *b = (detail::Builder *)_b;
  b->commit();
}

void release(SceneBuilderHandle _b)
{
  auto *b = (detail::Builder *)_b;
  b->refDec();
}

} // namespace testing
} // namespace ospray
