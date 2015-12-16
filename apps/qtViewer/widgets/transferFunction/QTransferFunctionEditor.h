// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

// ospray
#include <ospray/ospray.h>
// stl
#include <string>
#include <vector>
// qt
#include <QtGui>
// scene graph
#include "sg/SceneGraph.h"

namespace ospray {
  namespace viewer {

    /*! the editor widget to edit a transfer function's alpha
        values. This will show the current color map as a background
        image, and allow to edit a set of points defining the alpha
        values */
    class QTransferFunctionAlphaEditor : public QWidget
    {
      Q_OBJECT;

    public:

      QTransferFunctionAlphaEditor();

      /*! set background image for a given color map */
      void setColorMapImage(const QImage &image);

    signals:

      void transferFunctionChanged();

    public:

      // get y value based on linear interpolation of the points values for x in [0, 1]
      float getInterpolatedValue(float x);

    protected:

      //! @{ callbacks to intercept qt events
      virtual void paintEvent(QPaintEvent * event);
      virtual void mousePressEvent(QMouseEvent * event);
      virtual void mouseReleaseEvent(QMouseEvent * event);
      virtual void mouseMoveEvent(QMouseEvent * event);
      //! @}

      // transform normalized coordinates to widget coordinates
      QPointF pointToWidgetPoint(const ospray::vec2f &point);

      // transform widget coordinates to normalized coordinates
      ospray::vec2f widgetPointToPoint(const QPointF &widgetPoint);

      // get the index of the selected point based on the clicked point in widget coordinates
      // returns -1 if no selected point
      int getSelectedPointIndex(const QPointF &widgetClickPoint);

      // color map image used as the widget background
      QImage colorMapImage;

      //! the points that define the transfer function, in normalize
      //! coordinates ([0,0], [1,1])
      std::vector<ospray::vec2f> points;

      //! currently selected point
      int selectedPointIndex;

      //! \{ drawing properties
      static float pointPixelRadius;
      static float linePixelWidth;
      //! \}

    };


    /*! \brief simple editor widget that allows for editing a tranfer fcuntion's color map and alpha mapping */
    class QTransferFunctionEditor : public QWidget
    {
      Q_OBJECT

      /*! color map component of the transfer function.  */
      class ColorMap
      {
      public:

        //! construct a new color map from given colors
        ColorMap(const std::string &name, const std::vector<ospray::vec3f> &colors);

        //! return the name of this color map
        inline std::string getName() const { return name; };

        /*! return a QImage that represents the color map (this image
          can be used in the transfer function editor */
        QImage getRepresentativeImage() const;

        //! query function that returns the current color map (to be
        //! used in a scene graph node, for example
        std::vector<ospray::vec3f> getColors() const { return colors; };

      protected:

        const std::string name;
        const std::vector<ospray::vec3f> colors;
      };

    public:
      // constructor
      QTransferFunctionEditor();

      // add a new color map to the list of selectable color maps
      void addColorMap(const ColorMap *colorMap);

    signals:
      void transferFunctionChanged();
    public slots:
      void transferFunctionAlphasChanged();
    protected slots:

      void selectColorMap(int index);

    protected:

      const ColorMap *activeColorMap;
      void setDefaultColorMaps();

      // color maps
      std::vector<const ColorMap *> colorMaps;

      //! the combo box we use for selecting the color map
      QComboBox *colorMapComboBox;

      // transfer function widget for opacity
      QTransferFunctionAlphaEditor *transferFunctionAlphaEditor;

      virtual void updateColorMap() {};
      virtual void updateAlphaMap() {};
    };

    /*! a transfer fucntion editor that automatically updates an
        ospray::sg::TransferFunction node */
    struct QOSPTransferFunctionEditor : public QTransferFunctionEditor
    {
      QOSPTransferFunctionEditor(Ref<sg::TransferFunction> sgNode)
        : sgNode(sgNode)
      {
        // sgNode->render();
      }

      virtual void updateColorMap();
      virtual void updateAlphaMap();

      //! the node we are editing
      Ref<sg::TransferFunction> sgNode;
    };

  }
}
