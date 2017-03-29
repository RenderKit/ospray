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

#pragma once

#include <ospray/ospray.h>
#include <QtGui>
#include <stdexcept>

class LinearTransferFunctionWidget : public QWidget
{
  Q_OBJECT

  public:

  LinearTransferFunctionWidget();

  //! Minimum size of the widget.
  virtual QSize minimumSizeHint() const { return QSize(240, 180); }

  //! Get numValues of the transfer function regularly spaced over the entire interval [0, 1].
  std::vector<float> getInterpolatedValuesOverInterval(const unsigned int &numValues);

  //! Get transfer function control points.
  QVector<QPointF> getPoints() { return points; }

  //! Set transfer function control points.
  void setPoints(const QVector<QPointF> &points) { this->points = points; repaint(); emit(updated()); }

  //! Set background image for the widget (for example to show a color map).
  void setBackgroundImage(const QImage &image);

signals:

  //! Signal emitted when this widget is updated.
  void updated();

protected:

  virtual void paintEvent(QPaintEvent * event);
  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);
  virtual void mouseMoveEvent(QMouseEvent * event);

  //! Transform normalized coordinates to widget coordinates.
  QPointF pointToWidgetPoint(const QPointF &point);

  //! Transform widget coordinates to normalized coordinates.
  QPointF widgetPointToPoint(const QPointF &widgetPoint);

  //! Get the index of the selected point based on the clicked point in widget coordinates; returns -1 if no selected point.
  int getSelectedPointIndex(const QPointF &widgetClickPoint);

  //! Get y value based on linear interpolation of the points values for x in [0, 1].
  float getInterpolatedValue(float x);

  //! Background image for the widget (for example to show a color map).
  QImage backgroundImage;

  //! The points that define the transfer function, in normalize coordinates ([0,1], [0,1]).
  QVector<QPointF> points;

  //! Currently selected point.
  int selectedPointIndex;

  //! Drawing properties: point pixel radius.
  static float pointPixelRadius;

  //! Drawing properties: line pixel width.
  static float linePixelWidth;

  //! If true, will emit update signals during transfer function change; otherwise, will wait until change is complete (mouse release).
  static bool updateDuringChange;
};
