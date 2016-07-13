// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

#pragma once

#include "sg/SceneGraph.h"
// ospray public api
#include "ospray/ospray.h"
// qt
#include <QtGui>
//sg
#include "sg/Renderer.h"
// ospcomon
#include "ospcommon/common.h"

namespace ospray {
  namespace viewer {
    using namespace ospcommon;

    // ==================================================================
    //! \brief A QT Widget that allows mouse manipulation of a reference
    /*! The Widget works by keeping and manipulating a reference frame
      defined by two points (source and target), and a linear
      space. THe y axis if the linear space always points from
      source point to target point, thereby defining a natural
      "towards" direction (such as camera pointing towards lookat
      point, or spotlight pointing towards a point it's
      illuminating), while source and target define some natural
      pair of points associated with this direction (like said
      camera origin and camera lookat, or spotlight position and
      spotlight target position). By defining obvious directionality
      (forward=='from source to target') and orientation ('frame.vy
      points 'upwards'') this also allows for obvious manipulations
      such as "move X units forward", or "move Y units to the left",
      "snap frame to point upwards", etc, which this widget maps to mouse motion.

      How some given mouse motion affects the reference space is
      goverennd by the 'interactionMode' that can be set to
      different modes such as a 'fly' mode (representing the way a
      3D shooter game might allow a player to navigate in 3D space),
      or a 'inspect' mode (rotating the camera around some point of
      interest, etc.        
    */
    // ==================================================================
    struct LightInfo {
      OSPLight ospLight;
      vec3f color;
      vec3f direction;
      float intensity;
    };

    class QLightManipulator : public QWidget {
      Q_OBJECT;

      
    public slots:
        void apply();
    signals:
        void lightsChanged();
      
    public:
      //! constructor
      QLightManipulator(Ref<sg::Renderer> renderer, vec3f up);
      QLightManipulator(){}
      ~QLightManipulator();

    protected:
      LightInfo lightInfo;
      //The renderer we'll be using. Non-owning pointer
      Ref<sg::Renderer> sgRenderer;
      
      //All text boxes and labels
      QLabel *intensityLabel;
      QLineEdit *intensityValue;

      QLabel *colorLabel;
      QLineEdit *colorRedValue;
      QLineEdit *colorGreenValue;
      QLineEdit *colorBlueValue;

      QLabel *directionVectorLabel;
      QLineEdit *directionVectorXValue;
      QLineEdit *directionVectorYValue;
      QLineEdit *directionVectorZValue;

      QLabel *upVectorLabel;
      QLineEdit *upVectorXValue;
      QLineEdit *upVectorYValue;
      QLineEdit *upVectorZValue;

      //Apply any changes
      QPushButton *applyButton;

    private:
      vec3f upVector;
    };
  }
}

