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

    void transferFunctionAlphasChanged();

protected slots:

    void setColorMapIndex(int index);

    void setDataValueMin(double value);
    void setDataValueMax(double value);

protected:

    void loadColorMaps();

    // color maps
    std::vector<ColorMap> colorMaps_;

    // OSPRay transfer function object
    OSPTransferFunction transferFunction_;

    // transfer function widget for opacity
    TransferFunctionPiecewiseLinearWidget transferFunctionAlphaWidget_;
};
