// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "ManagedObject.h"

namespace ospray {
namespace cpp {

class Renderer : public ManagedObject<OSPRenderer, OSP_RENDERER>
{
 public:
  Renderer(const std::string &type);
  Renderer(OSPRenderer existing = nullptr);
};

static_assert(sizeof(Renderer) == sizeof(OSPRenderer),
    "cpp::Renderer can't have data members!");

// Inlined function definitions ///////////////////////////////////////////

inline Renderer::Renderer(const std::string &type)
{
  ospObject = ospNewRenderer(type.c_str());
}

inline Renderer::Renderer(OSPRenderer existing)
    : ManagedObject<OSPRenderer, OSP_RENDERER>(existing)
{}

} // namespace cpp

OSPTYPEFOR_SPECIALIZATION(cpp::Renderer, OSP_RENDERER);

} // namespace ospray
