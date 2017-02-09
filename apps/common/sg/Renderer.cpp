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

namespace ospray {
  namespace sg {
    using std::cout;
    using std::endl;

    Renderer::Renderer()
      // : ospFrameBuffer(NULL)
    {}

    int Renderer::renderFrame()
    { 
      if (!integrator) return 1;
      if (!frameBuffer) return 2;
      if (!camera) return 3;
      if (!world) return 4;

      assert(integrator->ospRenderer);

      if (!world->ospModel) {
        RenderContext rootContext;
        // geometries need the integrator to create materials
        rootContext.integrator = integrator;
        world->render(rootContext);
        assert(world->ospModel);
      }

      integrator->setWorld(world);
      integrator->setCamera(camera);
      integrator->commit();
      camera->commit();

      ospSet1f(camera->ospCamera,"aspect",frameBuffer->getSize().x/float(frameBuffer->getSize().y));
      ospCommit(camera->ospCamera);
      ospRenderFrame(frameBuffer->getOSPHandle(),
                     integrator->getOSPHandle(),
                     OSP_FB_COLOR|OSP_FB_ACCUM);
      accumID++;
      
      return 0;
    }

    /*! re-start accumulation (for progressive rendering). make sure
      that this function gets called at lesat once every time that
      anything changes that might change the appearance of the
      converged image (e.g., camera position, scene, frame size,
      etc) */
    void Renderer::resetAccumulation()
    {
      if (accumID == 0) {
        // cout << "accumID already 0..." << endl;
      } else {
        accumID = 0;
        // cout << "resetting accum" << endl;
        if (frameBuffer)
        frameBuffer->clear(); 
      }
    }

    //! create a default camera
    std::shared_ptr<sg::Camera> Renderer::createDefaultCamera(vec3f up)
    {
      // create a default camera
      std::shared_ptr<sg::PerspectiveCamera> camera = std::make_shared<sg::PerspectiveCamera>();
      if (world) {
      
        // now, determine world bounds to automatically focus the camera
        box3f worldBounds = world->getBounds();
        if (worldBounds == box3f(empty)) {
          cout << "#osp:qtv: world bounding box is empty, using default camera pose" << endl;
        } else {
          cout << "#osp:qtv: found world bounds " << worldBounds << endl;
          cout << "#osp:qtv: focussing default camera on world bounds" << endl;

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
      if (camera) 
        this->camera->commit();
      // if (this->camera) {
      //   this->camera->commit();
      // }
      if (integrator)
        integrator->setCamera(camera); 
// camera && integrator && integrator->ospRenderer) {
//         ospSetObject(integrator->ospRenderer,"camera",camera->ospCamera);
//         ospCommit(integrator->ospRenderer);
//       }
      resetAccumulation();
    }

    void Renderer::setIntegrator(const std::shared_ptr<sg::Integrator> &integrator) 
    {      
      this->integrator = integrator;
      if (integrator) {
        integrator->commit();
      }
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
      } else 
        std::cout << "#osp:sg:renderer: no world defined, yet\n#ospQTV: (did you forget to pass a scene file name on the command line?)" << std::endl;

      resetAccumulation();
      std::cout << "#osp:sg:renderer: new world with " << world->nodes.size() << " nodes" << endl;
    }

    //! find the last camera in the scene graph
    std::shared_ptr<sg::Camera> Renderer::getLastDefinedCamera() const
    {
      std::shared_ptr<sg::Camera> lastCamera;
      for (std::shared_ptr<Serialization::Object> obj : uniqueNodes.object) {
        std::shared_ptr<sg::Camera> asCamera = std::dynamic_pointer_cast<sg::Camera>(obj->node);
        if (asCamera) lastCamera = asCamera;
      }
      return lastCamera;
    }
    
    //! find the last integrator in the scene graph
    std::shared_ptr<sg::Integrator> Renderer::getLastDefinedIntegrator() const
    {
      std::shared_ptr<sg::Integrator> lastIntegrator;
      for (std::shared_ptr<Serialization::Object> obj : uniqueNodes.object) {
        std::shared_ptr<sg::Integrator> asIntegrator = std::dynamic_pointer_cast<sg::Integrator>(obj->node);
        if (asIntegrator) lastIntegrator = asIntegrator;
      }
      return lastIntegrator;
    }
    
  }
}
