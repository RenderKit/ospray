#pragma once

#include "ospray/render/Renderer.h"
#include "ospray/common/Model.h"
#include "ospray/camera/Camera.h"

namespace ospray {
  struct PathTracer : public Renderer {
    virtual std::string toString() const
    { return "ospray::PathTracer"; }

    PathTracer();
    virtual void commit();
    /*! \brief create a material of given type */
    virtual Material *createMaterial(const char *type);

    /*! the model we're rendering */
    Ref<Model>  model;
    Ref<Camera> camera;
  };
}

