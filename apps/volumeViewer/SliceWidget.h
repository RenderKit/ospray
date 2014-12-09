#pragma once

#include <QtGui>
#include <ospray/ospray.h>

class SliceWidget : public QFrame
{
Q_OBJECT

public:

    SliceWidget(std::vector<OSPModel> models, osp::box3f volumeBounds);
    ~SliceWidget();

signals:

    void sliceChanged();

public slots:

    void apply();
    void load(std::string filename = std::string());

protected slots:

    void save();
    void autoApply();
    void originSliderValueChanged(int value);
    void setAnimation(bool set);
    void animate();

protected:

    //! OSPRay models.
    std::vector<OSPModel> models;

    //! Bounding box of the volume.
    osp::box3f volumeBounds;

    //! OSPRay triangle mesh for the slice.
    OSPTriangleMesh triangleMesh;

    //! UI elements.
    QCheckBox autoApplyCheckBox;

    QDoubleSpinBox originXSpinBox;
    QDoubleSpinBox originYSpinBox;
    QDoubleSpinBox originZSpinBox;

    QDoubleSpinBox normalXSpinBox;
    QDoubleSpinBox normalYSpinBox;
    QDoubleSpinBox normalZSpinBox;

    QSlider originSlider;
    QPushButton originSliderAnimateButton;
    QTimer originSliderAnimationTimer;
    int originSliderAnimationDirection;
};
