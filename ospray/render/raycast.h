#pragma once

// ospray
#include "renderer.h"
// embree
#include "common/sys/taskscheduler.h"

namespace ospray {
  using embree::TaskScheduler;

  /*! test renderer that renders a simple test image using ispc */
  struct RayCastRenderer : public TileRenderer {
    virtual std::string toString() const { return "ospray::RayCastRenderer"; }
    virtual void renderTile(Tile &tile);
  private:
    /*! used internally to produce a slightly different image every frame */
    int frameID;
  };
};
