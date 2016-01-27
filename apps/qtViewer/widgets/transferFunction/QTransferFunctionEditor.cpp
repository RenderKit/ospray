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

#include "QTransferFunctionEditor.h"
#include <algorithm>
// scene graph components
#include "sg/common/TransferFunction.h"

namespace ospray {
  namespace viewer {

    float QTransferFunctionAlphaEditor::pointPixelRadius = 8.;
    float QTransferFunctionAlphaEditor::linePixelWidth   = 2.;

    bool comparePointsByX(const ospray::vec2f &i, const ospray::vec2f &j)
    {
      return (i.x < j.x);
    }

    QTransferFunctionAlphaEditor::QTransferFunctionAlphaEditor()
      : selectedPointIndex(-1)
    {
      // set colorMap image to widget size
      colorMapImage = QImage(size(), QImage::Format_ARGB32_Premultiplied);

      // default colorMap color
      colorMapImage.fill(QColor::fromRgbF(1,1,1,1).rgb());

      // default transfer function points
      // points.push_back(ospray::vec2f(0.,1.));
      points.push_back(ospray::vec2f(0.,0.));
      points.push_back(ospray::vec2f(1.,1.));
    }

    void QTransferFunctionAlphaEditor::setColorMapImage(const QImage &image)
    {
      colorMapImage = image;

      repaint();
    }

    void QTransferFunctionAlphaEditor::paintEvent(QPaintEvent * event)
    {
      QWidget::paintEvent(event);
      
      QPainter painter(this);

      painter.setRenderHint(QPainter::Antialiasing, true);

      // colorMap image
      // clip to region below the transfer function lines
      QPainterPath clipPath;
      QPolygonF clipPolygon;

      for(unsigned int i=0; i<points.size(); i++)
        clipPolygon << pointToWidgetPoint(points[i]);

      clipPolygon << QPointF(float(width()), float(height()));
      clipPolygon << QPointF(0., float(height()));
      
      clipPath.addPolygon(clipPolygon);
      painter.setClipPath(clipPath);
      
      painter.setClipping(true);
      painter.drawImage(rect(), colorMapImage.scaledToWidth(width(), Qt::SmoothTransformation));
      painter.setClipping(false);
      
      // draw lines between points
      painter.setPen(QPen(Qt::black, linePixelWidth, Qt::SolidLine));

      for(unsigned int i=0; i<points.size() - 1; i++)
        painter.drawLine(pointToWidgetPoint(points[i]), pointToWidgetPoint(points[i+1]));
      
      // draw points
      painter.setPen(QPen(Qt::black, linePixelWidth, Qt::SolidLine));
      painter.setBrush(QBrush(Qt::white));
      
      for(unsigned int i=0; i<points.size(); i++)
        painter.drawEllipse(pointToWidgetPoint(points[i]), 
                            pointPixelRadius, pointPixelRadius);
    }
    
    void QTransferFunctionAlphaEditor::mousePressEvent(QMouseEvent * event)
    {
      QWidget::mousePressEvent(event);

      if(event->button() == Qt::LeftButton) {
        // -------------------------------------------------------
        // LEFT BUTTON - add new point or move existing one
        // -------------------------------------------------------

        // either select an existing point, or create a new one at this location
        QPointF widgetClickPoint = event->posF();
      
        selectedPointIndex = getSelectedPointIndex(widgetClickPoint);
      
        if(selectedPointIndex == -1) {
          // no point selected, create a new one
          ospray::vec2f newPoint = widgetPointToPoint(widgetClickPoint);
        
          // insert into points vector and sort ascending by x
          points.push_back(newPoint);
        
          std::stable_sort(points.begin(), points.end(), comparePointsByX);
        
          // set selected point index for the new point
          selectedPointIndex
            = std::find(points.begin(), points.end(), newPoint) 
            - points.begin();
        
          if(selectedPointIndex >= points.size())
            throw std::runtime_error("QTransferFunctionAlphaEditor::mousePressEvent(): "
                                     "selected point index out of range");

          // emit signal
          emit transferFunctionChanged();

          // trigger repaint
          repaint();
        }
      } else if(event->button() == Qt::RightButton) {
        // -------------------------------------------------------
        // RIGHT BUTTON - remove points
        // -------------------------------------------------------
        
        // delete a point if selected (except for first and last points!)
        QPointF widgetClickPoint = event->posF();
        
        selectedPointIndex = getSelectedPointIndex(widgetClickPoint);
        
        if(selectedPointIndex != -1 /* no mathcing point found */ && 
           selectedPointIndex != 0  /* found, but first point (can't delete this) */ && 
           selectedPointIndex != points.size()-1  /* last point, can't delete */)
          {
            // erase the point
            points.erase(points.begin() + selectedPointIndex);
            
            // trigger repaint
            repaint();
            
            PING;
            // emit signal
            emit transferFunctionChanged();
          }
     
        // no point selected now
        selectedPointIndex = -1;
      }
    }
    
    void QTransferFunctionAlphaEditor::mouseReleaseEvent(QMouseEvent * event)
    {
      QWidget::mouseReleaseEvent(event);

      selectedPointIndex = -1;
    }

    void QTransferFunctionAlphaEditor::mouseMoveEvent(QMouseEvent * event)
    {
      QWidget::mouseMoveEvent(event);

      if(selectedPointIndex == -1) 
        // no point selected
        return;

      QPointF widgetMousePoint = event->posF();
      ospray::vec2f mousePoint = widgetPointToPoint(widgetMousePoint);

      // clamp x value
      if(selectedPointIndex == 0) {
        // the first point must have x == 0
        mousePoint.x = 0.;
      } else if(selectedPointIndex == points.size() - 1) {
        // the last point must have x == 1
        mousePoint.x = 1.;
      } else {
        // intermediate points must have x between their neighbors
        mousePoint.x = std::max(mousePoint.x, points[selectedPointIndex - 1].x);
        mousePoint.x = std::min(mousePoint.x, points[selectedPointIndex + 1].x);
      }

      // clamp y value
      mousePoint.y = std::min(mousePoint.y, 1.f);
      mousePoint.y = std::max(mousePoint.y, 0.f);

      points[selectedPointIndex] = mousePoint;

      repaint();

      // emit signal
      emit transferFunctionChanged();
    }

    QPointF QTransferFunctionAlphaEditor::pointToWidgetPoint(const ospray::vec2f &point)
    {
      return QPointF(point.x * float(width()), 
                     (1.f - point.y) * float(height()));
    }
    ospray::vec2f QTransferFunctionAlphaEditor::widgetPointToPoint(const QPointF &widgetPoint)
    {
      return ospray::vec2f(float(widgetPoint.x()) / float(width()), 
                        1.f - float(widgetPoint.y()) / float(height()));
    }

    int QTransferFunctionAlphaEditor::getSelectedPointIndex(const QPointF &widgetClickPoint)
    {
      for(unsigned int i=0; i<points.size(); i++) {
        QPointF delta = pointToWidgetPoint(points[i]) - widgetClickPoint;

        if(sqrtf(delta.x()*delta.x() + delta.y()*delta.y()) <= pointPixelRadius)
          return int(i);
      }

      return -1;
    }

    float QTransferFunctionAlphaEditor::getInterpolatedValue(float x)
    {
      // boundary cases
      if(x <= 0.)
        return points[0].y;
      
      if(x >= 1.)
        return points[points.size()-1].y;

      // we could make this more efficient...
      for(unsigned int i=0; i<points.size()-1; i++) {
        if(x <= points[i+1].x) {
          const float f = (x - points[i].x) / (points[i+1].x - points[i].x);
          return (1.f-f)*points[i].y + f*points[i+1].y;
        }
      }

      // we shouldn't ever get to this point...
      assert(false);
      return 0.f;
    }




    QTransferFunctionEditor::QTransferFunctionEditor() 
      : activeColorMap(NULL)
    {
      // setup UI elments
      QVBoxLayout * layout = new QVBoxLayout();
      setLayout(layout);

      // color map choice
      colorMapComboBox = new QComboBox();
      layout->addWidget(colorMapComboBox);

      transferFunctionAlphaEditor = new QTransferFunctionAlphaEditor;

      // opacity transfer function widget
      layout->addWidget(transferFunctionAlphaEditor);

      setDefaultColorMaps();

      // //! The Qt 4.8 documentation says: "by default, for every connection you
      // //! make, a signal is emitted".  But this isn't happening here (Qt 4.8.5,
      // //! Mac OS 10.9.4) so we manually invoke these functions so the transfer
      // //! function is fully populated before the first render call.
      // //!
      // //! Unfortunately, each invocation causes all transfer function fields to
      // //! be rewritten on the ISPC side (due to repeated recommits of the OSPRay
      // //! transfer function object).
      // //!
      // setColorMapIndex(0);
      // transferFunctionAlphasChanged();


      selectColorMap(0);
      connect(colorMapComboBox, SIGNAL(currentIndexChanged(int)), 
              this, SLOT(selectColorMap(int)));
      connect(transferFunctionAlphaEditor, SIGNAL(transferFunctionChanged()), 
              this, SLOT(transferFunctionAlphasChanged()));

      //      emit selectColorMap(0);
    }
  
    void QTransferFunctionEditor::addColorMap(const ColorMap *colorMap) 
    {
      colorMaps.push_back(colorMap);
      colorMapComboBox->addItem(tr(colorMap->getName().c_str()));
    }
    
    void QTransferFunctionEditor::setDefaultColorMaps() 
    {
      // color maps based on ParaView default color maps
      std::vector<ospray::vec3f> colors;

      // jet
      colors.clear();
      colors.push_back(ospray::vec3f(0         , 0           , 0.562493   ));
      colors.push_back(ospray::vec3f(0         , 0           , 1          ));
      colors.push_back(ospray::vec3f(0         , 1           , 1          ));
      colors.push_back(ospray::vec3f(0.500008  , 1           , 0.500008   ));
      colors.push_back(ospray::vec3f(1         , 1           , 0          ));
      colors.push_back(ospray::vec3f(1         , 0           , 0          ));
      colors.push_back(ospray::vec3f(0.500008  , 0           , 0          ));
      addColorMap(new ColorMap("Jet", colors));

      // ice / fire
      colors.clear();
      colors.push_back(ospray::vec3f(0         , 0           , 0           ));
      colors.push_back(ospray::vec3f(0         , 0.120394    , 0.302678    ));
      colors.push_back(ospray::vec3f(0         , 0.216587    , 0.524575    ));
      colors.push_back(ospray::vec3f(0.0552529 , 0.345022    , 0.659495    ));
      colors.push_back(ospray::vec3f(0.128054  , 0.492592    , 0.720287    ));
      colors.push_back(ospray::vec3f(0.188952  , 0.641306    , 0.792096    ));
      colors.push_back(ospray::vec3f(0.327672  , 0.784939    , 0.873426    ));
      colors.push_back(ospray::vec3f(0.60824   , 0.892164    , 0.935546    ));
      colors.push_back(ospray::vec3f(0.881376  , 0.912184    , 0.818097    ));
      colors.push_back(ospray::vec3f(0.9514    , 0.835615    , 0.449271    ));
      colors.push_back(ospray::vec3f(0.904479  , 0.690486    , 0           ));
      colors.push_back(ospray::vec3f(0.854063  , 0.510857    , 0           ));
      colors.push_back(ospray::vec3f(0.777096  , 0.330175    , 0.000885023 ));
      colors.push_back(ospray::vec3f(0.672862  , 0.139086    , 0.00270085  ));
      colors.push_back(ospray::vec3f(0.508812  , 0           , 0           ));
      colors.push_back(ospray::vec3f(0.299413  , 0.000366217 , 0.000549325 ));
      colors.push_back(ospray::vec3f(0.0157473 , 0.00332647  , 0           ));
      addColorMap(new ColorMap("Ice / Fire", colors));

      // cool to warm
      colors.clear();
      colors.push_back(ospray::vec3f(0.231373  , 0.298039    , 0.752941    ));
      colors.push_back(ospray::vec3f(0.865003  , 0.865003    , 0.865003    ));
      colors.push_back(ospray::vec3f(0.705882  , 0.0156863   , 0.14902     ));
      addColorMap(new ColorMap("Cool to Warm", colors));

      // blue to red rainbow
      colors.clear();
      colors.push_back(ospray::vec3f(0         , 0           , 1           ));
      colors.push_back(ospray::vec3f(1         , 0           , 0           ));
      addColorMap(new ColorMap("Blue to Red Rainbow", colors));

      // grayscale
      colors.clear();
      colors.push_back(ospray::vec3f(0.));
      colors.push_back(ospray::vec3f(1.));
      addColorMap(new ColorMap("Grayscale", colors));
    }


    void QTransferFunctionEditor::transferFunctionAlphasChanged()
    {
      emit transferFunctionChanged();
      updateAlphaMap();
    }

    // void QTransferFunctionEditor::transferFunctionChanged()
    // {
    //   PING;
    // }
    // void QTransferFunctionAlphaEditor::transferFunctionChanged()
    // {
    //   PING;
    // }

    

    void QTransferFunctionEditor::selectColorMap(int index) 
    { 
      activeColorMap = colorMaps[index]; 

      transferFunctionAlphaEditor->setColorMapImage(activeColorMap->getRepresentativeImage());

      updateColorMap();
      emit transferFunctionChanged();
    }

    QTransferFunctionEditor::ColorMap::ColorMap(const std::string &name, 
                                                const std::vector<ospray::vec3f> &colors)
      : name(name), colors(colors)
    {}

    QImage QTransferFunctionEditor::ColorMap::getRepresentativeImage() const
    {
      int numRows = 1;

      QImage image = QImage(int(colors.size()), numRows,
                            QImage::Format_ARGB32_Premultiplied);

      for(unsigned int i=0; i<colors.size(); i++)
          for(unsigned int j=0; j<numRows; j++)
            image.setPixel(QPoint(i, j), 
                           QColor::fromRgbF(colors[i].x, colors[i].y, colors[i].z).rgb());

      return image;
    }



    void QOSPTransferFunctionEditor::updateColorMap()
    {
      // PING;
      sgNode->setColorMap(activeColorMap->getColors());
      sgNode->commit();
    }
    
    void QOSPTransferFunctionEditor::updateAlphaMap()
    {
      // PING;
      const int numAlphas = 256;
      std::vector<float> alphas;
      for (int i=0;i<numAlphas;i++)
        alphas.push_back(transferFunctionAlphaEditor->getInterpolatedValue(i/float(numAlphas-1)));

      // PING;
      sgNode->setAlphaMap(alphas);
      // PING;
      sgNode->commit();
    //   PING;
    }
 
  } // ::ospray::viewer
} // ::ospray
