/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

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
      
      /*! quantize this color map into given 1D texture (which can
        then be used for ospray, gl, etc) */
      void makeTexture1D(float texel[], int numTexels);
      
      /*! set background image for a given color map */
      void setColorMapImage(QImage *image);
      
    signals:
      
      void transferFunctionChanged();

    public:
      // get y value based on linear interpolation of the points_ values for x in [0, 1]
      float getInterpolatedValue(float x);
      
      
    protected:
      //! @{ callbacks to intercept qt events
      virtual void resizeEvent(QResizeEvent * event);
      virtual void paintEvent(QPaintEvent * event);
      virtual void mousePressEvent(QMouseEvent * event);
      virtual void mouseReleaseEvent(QMouseEvent * event);
      virtual void mouseMoveEvent(QMouseEvent * event);
      //! @}
    
      // transform normalized coordinates to widget coordinates
      QPointF pointToWidgetPoint(const osp::vec2f &point);
      
      // transform widget coordinates to normalized coordinates
      osp::vec2f widgetPointToPoint(const QPointF &widgetPoint);

      // get the index of the selected point based on the clicked point in widget coordinates
      // returns -1 of no selected point
      int getSelectedPointIndex(const QPointF &widgetClickPoint);
      
      // this image shows the color map
      QImage *colorMapImage;
      
      //! the points that define the transfer function, in normalize
      //! coordinates ([0,0], [1,1])
      std::vector<osp::vec2f> points;
      
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
        ColorMap(const std::string &name, const std::vector<osp::vec3f> &colors);

        //! return the name of this color map
        inline std::string getName() const { return name; };

        /*! quantize this color map into given 1D texture (which can
          then be used for ospray, gl, etc) */
        void makeTexture1D(osp::vec3f texel[], int numTexels);

        /*! return a QImage that represents the color map (this image
          can be used in the transfer fct editor */
        QImage *getRepresentativeImage() const;

        //! query function that returns the current color map (to be
        //! used in a scene graph node, for example
        std::vector<osp::vec3f> getColors() const { return colors; };  
      protected:
        const std::string name;
        const std::vector<osp::vec3f> colors;
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
      // void setDataValueMin(double value);
      // void setDataValueMax(double value);

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
