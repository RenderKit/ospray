// ======================================================================== //
// Copyright 2009-2014 Intel Corporation                                    //
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

#include <QtGui>
#include <ospray/ospray.h>

class LinearTransferFunctionWidget : public QWidget
{
  Q_OBJECT

  public:

  LinearTransferFunctionWidget();

  // minimum size of the widget
  virtual QSize minimumSizeHint() const { return QSize(240, 180); }

  // get numValues of the transfer function regularly spaced over the entire interval [0, 1]
  std::vector<float> getInterpolatedValuesOverInterval(unsigned int numValues);

  // get transfer function control points
  QVector<QPointF> getPoints() { return points; }

  // set transfer function control points
  void setPoints(const QVector<QPointF> &points) { this->points = points; repaint(); emit(transferFunctionChanged()); }

  void setBackgroundImage(QImage image);

signals:

  void transferFunctionChanged();

protected:

  virtual void resizeEvent(QResizeEvent * event);
  virtual void paintEvent(QPaintEvent * event);
  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);
  virtual void mouseMoveEvent(QMouseEvent * event);

  // transform normalized coordinates to widget coordinates
  QPointF pointToWidgetPoint(const QPointF &point);

  // transform widget coordinates to normalized coordinates
  QPointF widgetPointToPoint(const QPointF &widgetPoint);

  // get the index of the selected point based on the clicked point in widget coordinates
  // returns -1 of no selected point
  int getSelectedPointIndex(const QPointF &widgetClickPoint);

  // get y value based on linear interpolation of the points values for x in [0, 1]
  float getInterpolatedValue(float x);

  // this image shows the color map
  QImage backgroundImage;

  // the points that define the transfer function, in normalize coordinates ([0,1], [0,1])
  QVector<QPointF> points;

  // currently selected point
  int selectedPointIndex;

  // drawing properties
  static float pointPixelRadius;
  static float linePixelWidth;

  // if true, will emit update signals during transfer function change
  // otherwise, will wait until change is complete (mouse release)
  static bool updateDuringChange;
};
