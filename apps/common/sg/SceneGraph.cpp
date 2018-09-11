// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "SceneGraph.h"

#include "visitor/VerifyNodes.h"

namespace ospray {
  namespace sg {

    Frame::Frame()
    {
      createChild("frameBuffer", "FrameBuffer");
      createChild("camera", "PerspectiveCamera");
      createChild("renderer", "Renderer");

      createChild("frameAccumulationLimit", "int", -1);
    }

    std::string Frame::toString() const
    {
      return "ospray::sg::Frame";
    }

    void Frame::preCommit(RenderContext &)
    {
      if (child("camera").hasChild("aspect") &&
          child("frameBuffer")["size"].lastModified() >
          child("camera")["aspect"].lastCommitted()) {
        auto fbSize = child("frameBuffer")["size"].valueAs<vec2i>();
        child("camera")["aspect"] = fbSize.x / float(fbSize.y);
      }

      auto rendererNode = child("renderer").nodeAs<Renderer>();
      rendererNode->updateRenderer();

      auto rHandle = rendererNode->valueAs<OSPRenderer>();
      auto cHandle = child("camera").valueAs<OSPCamera>();

      // XXX this is not sufficient (at least for FB):
      // the FB can be released and re-created (i.e. on size change) and per
      // (high) chance get the same handle (which is just the heap address when
      // using the local device)
      bool newRenderer = currentRenderer != rHandle;
      bool newCamera = currentCamera != cHandle;

      if (newRenderer)
        currentRenderer = rHandle;

      if (newCamera)
        currentCamera = cHandle;

      if (newRenderer || newCamera)
        ospSetObject(rHandle, "camera", cHandle);

      bool rChanged = rendererNode->subtreeModifiedButNotCommitted();
      bool cChanged = child("camera").subtreeModifiedButNotCommitted();

      clearFB = newCamera || newRenderer || rChanged || cChanged;

      frameAccumulationLimit = child("frameAccumulationLimit").valueAs<int>();
    }

    void Frame::postCommit(RenderContext &)
    {
      // after commit of child FB, which can lead to a new handle
      auto fHandle = child("frameBuffer").valueAs<OSPFrameBuffer>();
      bool newFB = currentFB != fHandle;
      if (newFB)
        currentFB = fHandle;

      if (newFB || clearFB) {
        child("frameBuffer").nodeAs<FrameBuffer>()->clear();

        clearFB = false;
        numAccumulatedFrames = 0;
        etaSeconds = inf;
        etaVariance = inf;
        etaAccumulation = inf;
      }
    }

    std::shared_ptr<FrameBuffer> Frame::renderFrame(bool verifyCommit)
    {
      if (verifyCommit) {
        Node::traverse(VerifyNodes{});
        commit();
      }

      traverse("render");

      auto fb1Node = child("frameBuffer").nodeAs<FrameBuffer>();
      auto fb2Node = child("navFrameBuffer").nodeAs<FrameBuffer>();
      // use nav FB?
      auto fbNode = (numAccumulatedFrames == 0 &&
          fb1Node->child("size").valueAs<vec2i>() !=
          fb2Node->child("size").valueAs<vec2i>()) ?
        fb2Node : fb1Node;

      auto rendererNode = child("renderer").nodeAs<Renderer>();

      const bool accumBudgetReached = frameAccumulationLimit >= 0 &&
        numAccumulatedFrames >= frameAccumulationLimit;

      const float varianceThreshold =
        rendererNode->child("varianceThreshold").valueAs<float>();
      const float lastVariance = numAccumulatedFrames < 2 ? inf
        : rendererNode->getLastVariance();
      const bool varianceReached = varianceThreshold > 0.f &&
          lastVariance <= varianceThreshold;

      if (accumBudgetReached || varianceReached) {
        etaSeconds = elapsedSeconds();
        return nullptr;
      }

      if (numAccumulatedFrames == 0)
        accumulationTimer.start();

      rendererNode->renderFrame(fbNode);

      numAccumulatedFrames++;
      accumulationTimer.stop();

      if (frameAccumulationLimit >= 0)
        etaAccumulation = frameAccumulationLimit * elapsedSeconds()
          / numAccumulatedFrames;

      if (varianceThreshold > 0.f) {
        const float currentVariance = rendererNode->getLastVariance();
        if (numAccumulatedFrames == 4) { // need stable variance estimate
          firstVariance = currentVariance;
          firstSeconds = elapsedSeconds();
        }
        // update estimate only on even frames (when variance was updated)
        if (((numAccumulatedFrames&1)==0) && numAccumulatedFrames >= 6)
          etaVariance = firstSeconds + (elapsedSeconds() - firstSeconds)
            / (1.f/sqr(currentVariance) - 1.f/sqr(firstVariance))
            / sqr(varianceThreshold);
      }

      // whichever is earlier
      etaSeconds = std::min(etaVariance, etaAccumulation);
      return fbNode;
    }

    OSPPickResult Frame::pick(const vec2f &pickPos)
    {
      auto rendererNode = child("renderer").nodeAs<Renderer>();
      return rendererNode->pick(pickPos);
    }

    int Frame::frameId() const
    {
      return numAccumulatedFrames;
    }

    float Frame::elapsedSeconds() const
    {
      return accumulationTimer.seconds();
    }

    float Frame::estimatedSeconds() const
    {
      return etaSeconds;
    }

    OSP_REGISTER_SG_NODE(Frame);

  } // ::ospray::sg
} // ::ospray
