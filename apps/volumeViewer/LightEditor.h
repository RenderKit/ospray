#pragma once

#include <QtGui>
#include <ospray/ospray.h>

class LightEditor : public QWidget {

Q_OBJECT

public:

    LightEditor(OSPLight light);

signals:

    void lightChanged();

protected slots:

    void alphaBetaSliderValueChanged();

protected:

    //! OSPRay light.
    OSPLight light;

    //! UI elements.
    QSlider alphaSlider;
    QSlider betaSlider;
};
