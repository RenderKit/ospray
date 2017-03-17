// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include "Renderer.h"

#include "sg/common/FrameBuffer.h"
#include <climits>
#include <cfloat>

namespace ospray {
  namespace sg {

    void Renderer::init()
    {
      add(createNode("bounds", "box3f"));
      add(createNode("rendererType", "string", std::string("scivis"),
                     NodeFlags::required | NodeFlags::valid_whitelist | NodeFlags::gui_combo,
                     "scivis: standard whitted style ray tracer. "
                     "pathtracer/pt: photo-realistic path tracer"));
      child("rendererType")->setWhiteList({std::string("scivis"),
                                           std::string("sv"),
                                           std::string("raytracer"),
                                           std::string("rt"),
                                           std::string("ao"),
                                           std::string("ao1"),
                                           std::string("ao2"),
                                           std::string("ao4"),
                                           std::string("ao8"),
                                           std::string("ao16"),
                                           std::string("dvr"),
                                           std::string("pathtracer"),
                                           std::string("pt")});
      add(createNode("world", "World"));
      child("world")->setDocumentation("model containing scene objects");
      add(createNode("camera", "PerspectiveCamera"));
      add(createNode("frameBuffer", "FrameBuffer"));
      add(createNode("lights"));

      add(createNode("bgColor", "vec3f", vec3f(0.9f, 0.9f, 0.9f),
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_color));

      //TODO: move these to seperate SciVisRenderer
      add(createNode("shadowsEnabled", "bool", true));
      add(createNode("maxDepth", "int", 5,
                     NodeFlags::required | NodeFlags::valid_min_max,
                     "maximum number of ray bounces"));
      child("maxDepth")->setMinMax(0,999);
      add(createNode("aoSamples", "int", 1,
                     NodeFlags::required | NodeFlags::valid_min_max | NodeFlags::gui_slider,
                     "number of ambient occlusion samples.  0 means off"));
      child("aoSamples")->setMinMax(0,128);
      add(createNode("spp", "int", 1, NodeFlags::required | NodeFlags::gui_slider,
                     "the number of samples rendered per pixel. The higher "
                     "the number, the smoother the resulting image."));
      child("spp")->setMinMax(-8,128);
      add(createNode("aoDistance", "float", 10000.f,
                     NodeFlags::required | NodeFlags::valid_min_max,
                     "maximum distance ao rays will trace to."
                     " Useful if you do not want a large interior of a"
                     " building to be completely black from occlusion."));
      child("aoDistance")->setMinMax(float(1e-31),FLT_MAX);
      add(createNode("oneSidedLighting", "bool",true, NodeFlags::required));
      add(createNode("aoTransparency", "bool",true, NodeFlags::required));
    }

    /*! re-start accumulation (for progressive rendering). make sure
      that this function gets called at lesat once every time that
      anything changes that might change the appearance of the
      converged image (e.g., camera position, scene, frame size,
      etc) */
    void Renderer::resetAccumulation()
    {
      accumID = 0;
      if (frameBuffer) frameBuffer->clear();
    }

    //! create a default camera
    std::shared_ptr<sg::Camera> Renderer::createDefaultCamera(vec3f up)
    {
      auto camera = std::make_shared<sg::PerspectiveCamera>();

      if (world) {
        // now, determine world bounds to automatically focus the camera
        box3f worldBounds = world->bounds();
        if (worldBounds == box3f(empty)) {
          std:: cout << "#osp:qtv: world bounding box is empty, using default camera pose"
                     << std::endl;
        } else {
          std::cout << "#osp:qtv: found world bounds " << worldBounds
                    << std::endl;
          std::cout << "#osp:qtv: focussing default camera on world bounds"
                    << std::endl;

          camera->setAt(center(worldBounds));
          if (up == vec3f(0,0,0))
            up = vec3f(0,1,0);
          camera->setUp(up);
          camera->setFrom(center(worldBounds) + .3f*vec3f(-1,+3,+1.5)*worldBounds.size());
        }
      }
      camera->commit();
      return std::dynamic_pointer_cast<sg::Camera>(camera);
    }

    void Renderer::setCamera(const std::shared_ptr<sg::Camera> &camera)
    {
      this->camera = camera;
      if (camera)     this->camera->commit();
      if (integrator) integrator->setCamera(camera);
      resetAccumulation();
    }

    void Renderer::setIntegrator(const std::shared_ptr<sg::Integrator> &integrator)
    {
      this->integrator = integrator;
      if (integrator) integrator->commit();
      resetAccumulation();
    }

    void Renderer::setWorld(const std::shared_ptr<World> &world)
    {
      this->world = world;
      allNodes.clear();
      uniqueNodes.clear();
      if (world) {
        allNodes.serialize(world,sg::Serialization::DONT_FOLLOW_INSTANCES);
        uniqueNodes.serialize(world,sg::Serialization::DO_FOLLOW_INSTANCES);
      } else {
        std::cout << "#osp:sg:renderer: no world defined, yet" << std::endl;
        std::cout << "#ospQTV: (did you forget to pass a scene file name on the command line?)"
                  << std::endl;
      }

      resetAccumulation();
      std::cout << "#osp:sg:renderer: new world with " << world->nodes.size()
                << " nodes" << std::endl;
    }

    //! find the last camera in the scene graph
    std::shared_ptr<sg::Camera> Renderer::lastDefinedCamera() const
    {
      std::shared_ptr<sg::Camera> lastCamera;
      for (std::shared_ptr<Serialization::Object> obj : uniqueNodes.object) {
        std::shared_ptr<sg::Camera> asCamera =
            std::dynamic_pointer_cast<sg::Camera>(obj->node);
        if (asCamera) lastCamera = asCamera;
      }
      return lastCamera;
    }

    //! find the last integrator in the scene graph
    std::shared_ptr<sg::Integrator> Renderer::lastDefinedIntegrator() const
    {
      std::shared_ptr<sg::Integrator> lastIntegrator;
      for (std::shared_ptr<Serialization::Object> obj : uniqueNodes.object) {
        std::shared_ptr<sg::Integrator> asIntegrator =
            std::dynamic_pointer_cast<sg::Integrator>(obj->node);
        if (asIntegrator) lastIntegrator = asIntegrator;
      }
      return lastIntegrator;
    }

    void Renderer::postRender(RenderContext &ctx)
    {
      PING;
      ospSetObject(ospRenderer, "model",
                   child("world")->valueAs<OSPObject>());
      PING;
      ospCommit(ospRenderer);

      PING;
      // PING;
      // PRINT(this);
      // std::cout << "Rendering ...." << std::endl;
      OSPFrameBuffer fb = (OSPFrameBuffer)child("frameBuffer")->valueAs<OSPObject>();
      // PRINT(fb);
      // PRINT(ospRenderer);
      PING;
      ospRenderFrame(fb,
                     ospRenderer,
                     OSP_FB_COLOR | OSP_FB_ACCUM);
      accumID++;
    }

    void Renderer::preRender(RenderContext& ctx)
    {
      ctx.ospRenderer = ospRenderer;
    }

    void Renderer::preCommit(RenderContext &ctx)
    {
      if (child("frameBuffer")["size"]->lastModified() >
          child("camera")["aspect"]->lastCommitted()) {
        
        child("camera")["aspect"]->setValue(
                                            child("frameBuffer")["size"]->valueAs<vec2i>().x /
                                            float(child("frameBuffer")["size"]->valueAs<vec2i>().y)
                                            );
      }
      auto rendererType = child("rendererType")->valueAs<std::string>();
      if (!ospRenderer || rendererType != createdType) {
        traverse(ctx, "modified");
        ospRenderer = ospNewRenderer(rendererType.c_str());
        assert(ospRenderer);
        createdType = rendererType;
        ospCommit(ospRenderer);
        setValue((OSPObject)ospRenderer);
      }
      ctx.ospRenderer = ospRenderer;
    }

    void Renderer::postCommit(RenderContext &ctx)
    {
      ospSetObject(ospRenderer,"model", child("world")->valueAs<OSPObject>());
      ospSetObject(ospRenderer,"camera", child("camera")->valueAs<OSPObject>());

      if (lightsData == nullptr || lightsBuildTime < child("lights")->childrenLastModified())
      {
        // create and setup light for Ambient Occlusion
        std::vector<OSPLight> lights;
        for(auto &lightNode : child("lights")->children())
          lights.push_back((OSPLight)lightNode->valueAs<OSPObject>());

        if (lightsData)
          ospRelease(lightsData);
        lightsData = ospNewData(lights.size(), OSP_LIGHT, &lights[0]);
        ospCommit(lightsData);
        lightsBuildTime = TimeStamp();
      }

      // complete setup of renderer
      ospSetObject(ospRenderer, "model",  child("world")->valueAs<OSPObject>());
      ospSetObject(ospRenderer, "lights", lightsData);

      //TODO: some child is kicking off modified every frame...Should figure
      //      out which and ignore it

      if (child("camera")->childrenLastModified() > frameMTime
        || child("lights")->childrenLastModified() > frameMTime
        || child("world")->childrenLastModified() > frameMTime
        || lastModified() > frameMTime
        || child("shadowsEnabled")->lastModified() > frameMTime
        || child("aoSamples")->lastModified() > frameMTime
        || child("spp")->lastModified() > frameMTime
        )
      {
        ospFrameBufferClear(
          (OSPFrameBuffer)child("frameBuffer")->valueAs<OSPObject>(),
          OSP_FB_COLOR | OSP_FB_ACCUM
        );

        frameMTime = TimeStamp();
      }

      ospCommit(ospRenderer);
    }

    OSP_REGISTER_SG_NODE(Renderer);

  } // ::ospray::sg
} // ::ospray
