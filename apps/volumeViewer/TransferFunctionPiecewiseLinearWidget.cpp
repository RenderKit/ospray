#include "TransferFunctionPiecewiseLinearWidget.h"
#include <algorithm>

float TransferFunctionPiecewiseLinearWidget::pointPixelRadius_ = 8.;
float TransferFunctionPiecewiseLinearWidget::linePixelWidth_ = 2.;

bool TransferFunctionPiecewiseLinearWidget::updateDuringChange_ = true;

bool comparePointsByX(const osp::vec2f &i, const osp::vec2f &j)
{
    return (i.x < j.x);
}

TransferFunctionPiecewiseLinearWidget::TransferFunctionPiecewiseLinearWidget() : selectedPointIndex_(-1)
{
    // set background image to widget size
    backgroundImage_ = QImage(size(), QImage::Format_ARGB32_Premultiplied);

    // default background color
    // backgroundImage_.fill(QColor::fromRgbF(1,1,1,1));

    // default transfer function points
    points_.push_back(osp::vec2f(0.,0.));
    points_.push_back(osp::vec2f(1.,1.));
}

std::vector<float> TransferFunctionPiecewiseLinearWidget::getInterpolatedValuesOverInterval(unsigned int numValues)
{
    std::vector<float> yValues;

    for(unsigned int i=0; i<numValues; i++)
    {
        yValues.push_back(getInterpolatedValue(float(i) / float(numValues - 1)));
    }

    return yValues;
}

void TransferFunctionPiecewiseLinearWidget::setBackgroundImage(QImage image)
{
    backgroundImage_ = image;

    // trigger repaint
    repaint();
}

void TransferFunctionPiecewiseLinearWidget::resizeEvent(QResizeEvent * event)
{
    QWidget::resizeEvent(event);
}

void TransferFunctionPiecewiseLinearWidget::paintEvent(QPaintEvent * event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);

    painter.setRenderHint(QPainter::Antialiasing, true);

    // background image
    // clip to region below the transfer function lines
    QPainterPath clipPath;
    QPolygonF clipPolygon;

    for(unsigned int i=0; i<points_.size(); i++)
    {
        clipPolygon << pointToWidgetPoint(points_[i]);
    }

    clipPolygon << QPointF(float(width()), float(height()));
    clipPolygon << QPointF(0., float(height()));

    clipPath.addPolygon(clipPolygon);
    painter.setClipPath(clipPath);

    painter.setClipping(true);
    painter.drawImage(rect(), backgroundImage_.scaledToWidth(width(), Qt::SmoothTransformation));
    painter.setClipping(false);

    // draw lines between points
    painter.setPen(QPen(Qt::black, linePixelWidth_, Qt::SolidLine));

    for(unsigned int i=0; i<points_.size() - 1; i++)
    {
        painter.drawLine(pointToWidgetPoint(points_[i]), pointToWidgetPoint(points_[i+1]));
    }

    // draw points
    painter.setPen(QPen(Qt::black, linePixelWidth_, Qt::SolidLine));
    painter.setBrush(QBrush(Qt::white));

    for(unsigned int i=0; i<points_.size(); i++)
    {
        painter.drawEllipse(pointToWidgetPoint(points_[i]), pointPixelRadius_, pointPixelRadius_);
    }
}

void TransferFunctionPiecewiseLinearWidget::mousePressEvent(QMouseEvent * event)
{
    QWidget::mousePressEvent(event);

    if(event->button() == Qt::LeftButton)
    {
        // either select an existing point, or create a new one at this location
        QPointF widgetClickPoint = event->posF();

        selectedPointIndex_ = getSelectedPointIndex(widgetClickPoint);

        if(selectedPointIndex_ == -1)
        {
            // no point selected, create a new one
            osp::vec2f newPoint = widgetPointToPoint(widgetClickPoint);

            // insert into points vector and sort ascending by x
            points_.push_back(newPoint);

            std::stable_sort(points_.begin(), points_.end(), comparePointsByX);

            // set selected point index for the new point
            selectedPointIndex_ = std::find(points_.begin(), points_.end(), newPoint) - points_.begin();

            if(selectedPointIndex_ >= points_.size())
            {
                throw std::runtime_error("TransferFunctionPiecewiseLinearWidget::mousePressEvent(): selected point index out of range");
            }

            // trigger repaint
            repaint();
        }
    }
    else if(event->button() == Qt::RightButton)
    {
        // delete a point if selected (except for first and last points!)
        QPointF widgetClickPoint = event->posF();

        selectedPointIndex_ = getSelectedPointIndex(widgetClickPoint);

        if(selectedPointIndex_ != -1 && selectedPointIndex_ != 0 && selectedPointIndex_ != points_.size() - 1)
        {
            points_.erase(points_.begin() + selectedPointIndex_);

            // trigger repaint
            repaint();

            // emit signal
            emit transferFunctionChanged();
        }

        selectedPointIndex_ = -1;
    }
}

void TransferFunctionPiecewiseLinearWidget::mouseReleaseEvent(QMouseEvent * event)
{
    QWidget::mouseReleaseEvent(event);

    // emit signal if we were manipulating a point
    if(selectedPointIndex_ != -1)
    {
        selectedPointIndex_ = -1;

        emit transferFunctionChanged();
    }
}

void TransferFunctionPiecewiseLinearWidget::mouseMoveEvent(QMouseEvent * event)
{
    QWidget::mouseMoveEvent(event);

    if(selectedPointIndex_ != -1)
    {
        QPointF widgetMousePoint = event->posF();
        osp::vec2f mousePoint = widgetPointToPoint(widgetMousePoint);

        // clamp x value
        if(selectedPointIndex_ == 0)
        {
            // the first point must have x == 0
            mousePoint.x = 0.;
        }
        else if(selectedPointIndex_ == points_.size() - 1)
        {
            // the last point must have x == 1
            mousePoint.x = 1.;
        }
        else
        {
            // intermediate points must have x between their neighbors
            mousePoint.x = std::max(mousePoint.x, points_[selectedPointIndex_ - 1].x);
            mousePoint.x = std::min(mousePoint.x, points_[selectedPointIndex_ + 1].x);
        }

        // clamp y value
        mousePoint.y = std::min(mousePoint.y, 1.f);
        mousePoint.y = std::max(mousePoint.y, 0.f);

        points_[selectedPointIndex_] = mousePoint;

        repaint();

        if(updateDuringChange_ == true)
        {
            // emit signal
            emit transferFunctionChanged();
        }
    }
}

QPointF TransferFunctionPiecewiseLinearWidget::pointToWidgetPoint(const osp::vec2f &point)
{
    return QPointF(point.x * float(width()), (1. - point.y) * float(height()));
}

osp::vec2f TransferFunctionPiecewiseLinearWidget::widgetPointToPoint(const QPointF &widgetPoint)
{
    return osp::vec2f(float(widgetPoint.x()) / float(width()), 1. - float(widgetPoint.y()) / float(height()));
}

int TransferFunctionPiecewiseLinearWidget::getSelectedPointIndex(const QPointF &widgetClickPoint)
{
    for(unsigned int i=0; i<points_.size(); i++)
    {
        QPointF delta = pointToWidgetPoint(points_[i]) - widgetClickPoint;

        if(sqrtf(delta.x()*delta.x() + delta.y()*delta.y()) <= pointPixelRadius_)
        {
            return int(i);
        }
    }

    return -1;
}

float TransferFunctionPiecewiseLinearWidget::getInterpolatedValue(float x)
{
    // boundary cases
    if(x <= 0.)
    {
        return points_[0].y;
    }
    else if(x >= 1.)
    {
        return points_[points_.size() - 1].y;
    }

    // we could make this more efficient...
    for(unsigned int i=0; i<points_.size() - 1; i++)
    {
        if(x <= points_[i + 1].x)////
        {
            float delta = x - points_[i].x;
            float interval = points_[i+1].x - points_[i].x;

            if(delta == 0. || interval == 0.)
            {
                return points_[i].y;
            }

            return points_[i].y + delta / interval * (points_[i+1].y - points_[i].y);
        }
    }

    // we shouldn't get to this point...
    throw std::runtime_error("TransferFunctionPiecewiseLinearWidget::getInterpolatedValue(): error!");
}
