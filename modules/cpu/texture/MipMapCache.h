// Copyright 2024 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <unordered_map>

namespace ospray {

struct MipMapCache
{
  using BufferPtr = std::shared_ptr<devicert::BufferShared<char>>;

  // Find appropriate MIP map pyramid for given texture data
  inline BufferPtr find(char *rootLevel);

  // Manage cache entries
  inline void add(char *rootLevel, BufferPtr mipMaps);
  inline void remove(char *rootLevel);

 private:

  // Hash map containing MIP map buffers
  std::unordered_map<char *, BufferPtr> map;
};

MipMapCache::BufferPtr MipMapCache::find(char *rootLevel)
{
    auto it = map.find(rootLevel);
    return it != map.end() ? it->second : BufferPtr();
}

void MipMapCache::add(char *rootLevel, MipMapCache::BufferPtr mipMaps)
{
    map.insert(std::make_pair(rootLevel, mipMaps));
}

void MipMapCache::remove(char *rootLevel)
{
    map.erase(rootLevel);
}

} // namespace ospray