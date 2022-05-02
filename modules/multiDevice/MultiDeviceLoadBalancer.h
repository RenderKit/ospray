// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include "MultiDeviceFrameBuffer.h"
#include "MultiDeviceObject.h"
#include "render/LoadBalancer.h"
#include "rkcommon/utility/ArrayView.h"

namespace ospray {

struct MultiDeviceLoadBalancer
{
  MultiDeviceLoadBalancer(
      const std::vector<std::shared_ptr<TiledLoadBalancer>> &loadBalancers);

  void renderFrame(api::MultiDeviceFrameBuffer *framebuffer,
      api::MultiDeviceObject *renderer,
      api::MultiDeviceObject *camera,
      api::MultiDeviceObject *world);

  std::string toString() const;

 private:
  // The subdevice load balancers that this multidevice load balancer will
  // distribute the rendering workload over
  std::vector<std::shared_ptr<TiledLoadBalancer>> loadBalancers;
};
} // namespace ospray
