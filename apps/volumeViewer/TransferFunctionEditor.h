#pragma once

#include "ColorMap.h"
#include "TransferFunctionPiecewiseLinearWidget.h"
#include <QtGui>
#include <ospray/ospray.h>

class TransferFunctionEditor : public QWidget
{
Q_OBJECT

public:

    TransferFunctionEditor(OSPTransferFunction transferFunction);

signals:

    void transferFunctionChanged();

public slots:

    void transferFunctionAlphaChanged();

protected slots:

    void setColorMapIndex(int index);

    void setColorValueMin(double value);
    void setColorValueMax(double value);

    void setAlphaValueMin(double value);
    void setAlphaValueMax(double value);

protected:

    void loadColorMaps();

    // color maps
    std::vector<ColorMap> colorMaps_;

    // OSPRay transfer function object
    OSPTransferFunction transferFunction_;

    // transfer function widget for opacity
    TransferFunctionPiecewiseLinearWidget transferFunctionAlphaWidget_;
};
