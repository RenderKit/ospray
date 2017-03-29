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

#include "LinearTransferFunctionWidget.h"
#include <algorithm>

float LinearTransferFunctionWidget::pointPixelRadius = 8.;
float LinearTransferFunctionWidget::linePixelWidth = 2.;

bool LinearTransferFunctionWidget::updateDuringChange = true;

bool comparePointsByX(const QPointF &i, const QPointF &j) {
  return (i.x() < j.x());
}

LinearTransferFunctionWidget::LinearTransferFunctionWidget() : selectedPointIndex(-1)
{
  // Set background image to widget size
  backgroundImage = QImage(size(), QImage::Format_ARGB32_Premultiplied);

  // Default background color
  backgroundImage.fill(QColor::fromRgbF(1,1,1,1).rgb());

  // Default transfer function points
  points.push_back(QPointF(0.,0.));
  points.push_back(QPointF(1.,1.));
}

std::vector<float> LinearTransferFunctionWidget::getInterpolatedValuesOverInterval(const unsigned int &numValues)
{
  std::vector<float> yValues;

  for(unsigned int i=0; i<numValues; i++)
    yValues.push_back(getInterpolatedValue(float(i) / float(numValues - 1)));

  return yValues;
}

void LinearTransferFunctionWidget::setBackgroundImage(const QImage &image)
{
  backgroundImage = image;

  // Trigger repaint
  repaint();
}

void LinearTransferFunctionWidget::paintEvent(QPaintEvent * event)
{
  QWidget::paintEvent(event);

  QPainter painter(this);

  painter.setRenderHint(QPainter::Antialiasing, true);

  // Draw a border around the widget
  QPen pen(Qt::gray, 4);
  painter.setPen(pen);
  painter.drawRect(0, 0, width(), height());

  // Background image; clip to the region below the transfer function lines.
  QPainterPath clipPath;
  QPolygonF clipPolygon;

  for(unsigned int i=0; i<points.size(); i++)
    clipPolygon << pointToWidgetPoint(points[i]);

  clipPolygon << QPointF(float(width()), float(height()));
  clipPolygon << QPointF(0., float(height()));

  clipPath.addPolygon(clipPolygon);
  painter.setClipPath(clipPath);

  painter.setClipping(true);
  painter.drawImage(rect(), backgroundImage.scaledToWidth(width(), Qt::SmoothTransformation));
  painter.setClipping(false);

  // Draw lines between points
  painter.setPen(QPen(Qt::black, linePixelWidth, Qt::SolidLine));

  for(unsigned int i=0; i<points.size() - 1; i++)
    painter.drawLine(pointToWidgetPoint(points[i]), pointToWidgetPoint(points[i+1]));

  // Draw points
  painter.setPen(QPen(Qt::black, linePixelWidth, Qt::SolidLine));
  painter.setBrush(QBrush(Qt::white));

  for(unsigned int i=0; i<points.size(); i++)
    painter.drawEllipse(pointToWidgetPoint(points[i]), pointPixelRadius, pointPixelRadius);
}

void LinearTransferFunctionWidget::mousePressEvent(QMouseEvent * event)
{
  QWidget::mousePressEvent(event);

  if(event->button() == Qt::LeftButton) {

    // Either select an existing point, or create a new one at this location.
    QPointF widgetClickPoint = event->posF();

    selectedPointIndex = getSelectedPointIndex(widgetClickPoint);

    if(selectedPointIndex == -1) {

      // No point selected, create a new one.
      QPointF newPoint = widgetPointToPoint(widgetClickPoint);

      // Insert into points vector and sort ascending by x.
      points.push_back(newPoint);

      std::stable_sort(points.begin(), points.end(), comparePointsByX);

      // Set selected point index for the new point.
      selectedPointIndex = std::find(points.begin(), points.end(), newPoint) - points.begin();

      if(selectedPointIndex >= points.size())
        throw std::runtime_error("LinearTransferFunctionWidget::mousePressEvent(): selected point index out of range");

      // Trigger repaint.
      repaint();
    }
  }
  else if(event->button() == Qt::RightButton) {

    // Delete a point if selected (except for first and last points!).
    QPointF widgetClickPoint = event->posF();

    selectedPointIndex = getSelectedPointIndex(widgetClickPoint);

    if(selectedPointIndex != -1 && selectedPointIndex != 0 && selectedPointIndex != points.size() - 1) {

      points.erase(points.begin() + selectedPointIndex);

      // Trigger repaint.
      repaint();

      // Emit signal.
      emit updated();
    }

    selectedPointIndex = -1;
  }
}

void LinearTransferFunctionWidget::mouseReleaseEvent(QMouseEvent * event)
{
  QWidget::mouseReleaseEvent(event);

  // Emit signal if we were manipulating a point.
  if(selectedPointIndex != -1) {

    selectedPointIndex = -1;

    emit updated();
  }
}

void LinearTransferFunctionWidget::mouseMoveEvent(QMouseEvent * event)
{
  QWidget::mouseMoveEvent(event);

  if(selectedPointIndex != -1) {
    QPointF widgetMousePoint = event->posF();
    QPointF mousePoint = widgetPointToPoint(widgetMousePoint);

    // Clamp x value.
    if(selectedPointIndex == 0) {

      // the first point must have x == 0.
      mousePoint.rx() = 0.;
    }
    else if(selectedPointIndex == points.size() - 1) {

      // The last point must have x == 1.
      mousePoint.rx() = 1.;
    }
    else {

      // Intermediate points must have x between their neighbors.
      mousePoint.rx() = std::max(mousePoint.x(), points[selectedPointIndex - 1].x());
      mousePoint.rx() = std::min(mousePoint.x(), points[selectedPointIndex + 1].x());
    }

    // Clamp y value.
    mousePoint.ry() = std::min(mousePoint.y(), 1.);
    mousePoint.ry() = std::max(mousePoint.y(), 0.);

    points[selectedPointIndex] = mousePoint;

    repaint();

    if(updateDuringChange == true)
      emit updated();
  }
}

QPointF LinearTransferFunctionWidget::pointToWidgetPoint(const QPointF &point)
{
  return QPointF(point.x() * float(width()), (1. - point.y()) * float(height()));
}

QPointF LinearTransferFunctionWidget::widgetPointToPoint(const QPointF &widgetPoint)
{
  return QPointF(float(widgetPoint.x()) / float(width()), 1. - float(widgetPoint.y()) / float(height()));
}

int LinearTransferFunctionWidget::getSelectedPointIndex(const QPointF &widgetClickPoint)
{
  for(unsigned int i=0; i<points.size(); i++) {

    QPointF delta = pointToWidgetPoint(points[i]) - widgetClickPoint;

    if(sqrtf(delta.x()*delta.x() + delta.y()*delta.y()) <= pointPixelRadius)
      return int(i);
  }

  return -1;
}

float LinearTransferFunctionWidget::getInterpolatedValue(float x)
{
  // Boundary cases.
  if(x <= 0.)
    return points[0].y();
  else if(x >= 1.)
    return points[points.size() - 1].y();

  // We could make this more efficient...
  for(unsigned int i=0; i<points.size() - 1; i++) {
    if(x <= points[i + 1].x()) {
      float delta = x - points[i].x();
      float interval = points[i+1].x() - points[i].x();

      if(delta == 0. || interval == 0.)
        return points[i].y();

      return points[i].y() + delta / interval * (points[i+1].y() - points[i].y());
    }
  }

  // We shouldn't get to this point...
  throw std::runtime_error("LinearTransferFunctionWidget::getInterpolatedValue(): error!");
}
