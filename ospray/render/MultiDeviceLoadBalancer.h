// Copyright 2009-2021 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include "render/LoadBalancer.h"
#include "rkcommon/utility/ArrayView.h"

namespace ospray {

namespace api {
// TODO: Needs own header
struct MultiDeviceObject;
} // namespace api

struct OSPRAY_SDK_INTERFACE MultiDeviceLoadBalancer
{
  MultiDeviceLoadBalancer(
      const std::vector<std::shared_ptr<TiledLoadBalancer>> &loadBalancers);

  void renderFrame(FrameBuffer *fb,
      api::MultiDeviceObject *renderer,
      api::MultiDeviceObject *camera,
      api::MultiDeviceObject *world);

  void renderTiles(FrameBuffer *fb,
      api::MultiDeviceObject *renderer,
      api::MultiDeviceObject *camera,
      api::MultiDeviceObject *world,
      const utility::ArrayView<int> &tileIDs,
      std::vector<void *> &perFrameDatas);

  std::string toString() const;

 private:
  void initAllTileList(const FrameBuffer *fb);

  std::vector<int> allTileIDs;

  // The subdevice load balancers that this multidevice load balancer will
  // distribute the rendering workload over
  std::vector<std::shared_ptr<TiledLoadBalancer>> loadBalancers;
};
} // namespace ospray
