/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

#include <QtGui>
#include <ospray/ospray.h>

class TransferFunctionPiecewiseLinearWidget : public QWidget
{
Q_OBJECT

public:

    TransferFunctionPiecewiseLinearWidget();

    // minimum size of the widget
    virtual QSize minimumSizeHint() const { return QSize(240, 180); }

    // get numValues of the transfer function regularly spaced over the entire interval [0, 1]
    std::vector<float> getInterpolatedValuesOverInterval(unsigned int numValues);

    // get transfer function control points
    QVector<QPointF> getPoints() { return points_; }

    // set transfer function control points
    void setPoints(const QVector<QPointF> &points) { points_ = points; repaint(); emit(transferFunctionChanged()); }

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

    // get y value based on linear interpolation of the points_ values for x in [0, 1]
    float getInterpolatedValue(float x);

    // this image shows the color map
    QImage backgroundImage_;

    // the points that define the transfer function, in normalize coordinates ([0,1], [0,1])
    QVector<QPointF> points_;

    // currently selected point
    int selectedPointIndex_;

    // drawing properties
    static float pointPixelRadius_;
    static float linePixelWidth_;

    // if true, will emit update signals during transfer function change
    // otherwise, will wait until change is complete (mouse release)
    static bool updateDuringChange_;
};
