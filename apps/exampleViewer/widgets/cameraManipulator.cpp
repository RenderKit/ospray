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

#include <GLFW/glfw3.h>
#include "imgui3D.h"
#include "cameraManipulator.h"

namespace ospray {
  //! dedicated namespace for 3D glut viewer widget
  namespace imgui3D {

    // ------------------------------------------------------------------
    // base manipulator
    // ------------------------------------------------------------------
    void Manipulator::motion(ImGui3DWidget *widget)
    {
      auto &state = widget->currButton;
      if (state[GLFW_MOUSE_BUTTON_RIGHT] == GLFW_PRESS) {
        dragRight(widget,widget->currMousePos,widget->lastMousePos);
      } else if (state[GLFW_MOUSE_BUTTON_MIDDLE] == GLFW_PRESS) {
        dragMiddle(widget,widget->currMousePos,widget->lastMousePos);
      } else if (state[GLFW_MOUSE_BUTTON_LEFT] == GLFW_PRESS) {
        dragLeft(widget,widget->currMousePos,widget->lastMousePos);
      }
    }

    // ------------------------------------------------------------------
    // INSPECT_CENTER manipulator
    // ------------------------------------------------------------------
    InspectCenter::InspectCenter(ImGui3DWidget *widget)
      : Manipulator(widget)
      , pivot(ospcommon::center(widget->worldBounds))
    {}

    void InspectCenter::rotate(float du, float dv)
    {
      ImGui3DWidget::ViewPort &cam = widget->viewPort;
      const vec3f pivot = widget->viewPort.at;//center(widget->worldBounds);
      AffineSpace3fa xfm
        = AffineSpace3fa::translate(pivot)
        * AffineSpace3fa::rotate(cam.frame.l.vx,-dv)
        * AffineSpace3fa::rotate(cam.frame.l.vz,-du)
        * AffineSpace3fa::translate(-pivot);
      cam.frame = xfm * cam.frame;
      cam.from  = xfmPoint(xfm,cam.from);
      cam.at    = xfmPoint(xfm,cam.at);
      cam.snapFrameUp();
      cam.modified = true;
    }

    /*! INSPECT_CENTER::RightButton: move lookfrom/viewPort position
      forward/backward on right mouse button */
    void InspectCenter::dragRight(ImGui3DWidget *widget,
                                  const vec2i &to, const vec2i &from)
    {
      ImGui3DWidget::ViewPort &cam = widget->viewPort;
      float fwd =
#ifdef INVERT_RMB
#else
        -
#endif
        (to.y - from.y) * 4 * widget->motionSpeed;
      // * length(widget->worldBounds.size());
      float oldDist = length(cam.at - cam.from);
      float newDist = oldDist - fwd;
      if (newDist < 1e-3f)
        return;
      cam.from = cam.at - newDist * cam.frame.l.vy;
      cam.frame.p = cam.from;
      cam.modified = true;
    }

    /*! INSPECT_CENTER::MiddleButton: move lookat/center of interest
      forward/backward on middle mouse button */
    void InspectCenter::dragMiddle(ImGui3DWidget *widget,
                                   const vec2i &to, const vec2i &from)
    {
      ImGui3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x);
      float dv = (to.y - from.y);

      AffineSpace3fa xfm =
          AffineSpace3fa::translate(widget->motionSpeed * dv * cam.frame.l.vz )
        * AffineSpace3fa::translate(-1.0f * widget->motionSpeed
                                    * du * cam.frame.l.vx);

      cam.frame = xfm * cam.frame;
      cam.from = xfmPoint(xfm, cam.from);
      cam.at = xfmPoint(xfm, cam.at);
      cam.modified = true;
    }

    void InspectCenter::dragLeft(ImGui3DWidget *widget,
                                 const vec2i &to, const vec2i &from)
    {
      ImGui3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x) * widget->rotateSpeed;
      float dv = (to.y - from.y) * widget->rotateSpeed;

      if (widget->upAnchored) {
        const float theta = std::acos(dot(cam.up, normalize(cam.from-cam.at)));
        // prevent instabilities at the poles by enforcing a minimum angle to up
        dv = clamp(dv, theta-float(pi)+0.05f, theta-0.05f);
      }

      const vec3f pivot = cam.at;
      AffineSpace3fa xfm
        = AffineSpace3fa::translate(pivot)
        * AffineSpace3fa::rotate(cam.frame.l.vx,-dv)
        * AffineSpace3fa::rotate(cam.frame.l.vz,-du)
        * AffineSpace3fa::translate(-pivot);
      cam.frame = xfm * cam.frame;
      cam.from  = xfmPoint(xfm,cam.from);
      cam.at    = xfmPoint(xfm,cam.at);

      if (!widget->upAnchored)
        cam.snapViewUp();

      cam.snapFrameUp();
      cam.modified = true;
    }

    // ------------------------------------------------------------------
    // MOVE_MOVE manipulator - TODO.
    // ------------------------------------------------------------------

    void MoveMode::dragRight(ImGui3DWidget *widget,
                             const vec2i &to, const vec2i &from)
    {
      ImGui3DWidget::ViewPort &cam = widget->viewPort;
      float fwd =
#ifdef INVERT_RMB
#else
        -
#endif
        (to.y - from.y) * 4 * widget->motionSpeed;
      cam.from = cam.from + fwd * cam.frame.l.vy;
      cam.at   = cam.at   + fwd * cam.frame.l.vy;
      cam.frame.p = cam.from;
      cam.modified = true;
    }

    /*! todo */
    void MoveMode::dragMiddle(ImGui3DWidget *widget,
                              const vec2i &to, const vec2i &from)
    {
      ImGui3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x);
      float dv = (to.y - from.y);

      auto xfm =
        AffineSpace3fa::translate(widget->motionSpeed * dv * cam.frame.l.vz ) *
        AffineSpace3fa::translate(-1.0f * widget->motionSpeed * du * cam.frame.l.vx);

      cam.frame = xfm * cam.frame;
      cam.from = xfmPoint(xfm, cam.from);
      cam.at = xfmPoint(xfm, cam.at);
      cam.modified = true;
    }

    void MoveMode::dragLeft(ImGui3DWidget *widget,
                            const vec2i &to, const vec2i &from)
    {
      ImGui3DWidget::ViewPort &cam = widget->viewPort;
      float du = (to.x - from.x) * widget->rotateSpeed;
      float dv = (to.y - from.y) * widget->rotateSpeed;

      const vec3f pivot = cam.from; //center(widget->worldBounds);
      AffineSpace3fa xfm
        = AffineSpace3fa::translate(pivot)
        * AffineSpace3fa::rotate(cam.frame.l.vx,-dv)
        * AffineSpace3fa::rotate(cam.frame.l.vz,-du)
        * AffineSpace3fa::translate(-pivot);
      cam.frame = xfm * cam.frame;
      cam.from  = xfmPoint(xfm,cam.from);
      cam.at    = xfmPoint(xfm,cam.at);
      cam.snapFrameUp();
      cam.modified = true;
    }
  }
}


