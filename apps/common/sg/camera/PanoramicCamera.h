#pragma once

#include "sg/camera/Camera.h"

namespace ospray {
namespace sg {

struct OSPSG_INTERFACE PanoramicCamera : public sg::Camera
{
public:
  PanoramicCamera();
  void postCommit(RenderContext &ctx) override;
};

}
}
