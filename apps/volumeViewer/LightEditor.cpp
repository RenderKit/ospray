#include "LightEditor.h"

LightEditor::LightEditor(OSPLight light) : light(light) {

    //! Make sure we have an existing transfer light.
    if (!light) throw std::runtime_error("LightEditor: must be constructed with an existing light");

    //! Setup UI elments.
    QVBoxLayout * layout = new QVBoxLayout();
    setLayout(layout);

    //! Form layout.
    QWidget * formWidget = new QWidget();
    QFormLayout * formLayout = new QFormLayout();
    formWidget->setLayout(formLayout);
    layout->addWidget(formWidget);

    //! Add sliders for spherical angles alpha and beta for light direction.
    alphaSlider.setOrientation(Qt::Horizontal);
    betaSlider.setOrientation(Qt::Horizontal);
    formLayout->addRow("alpha", &alphaSlider);
    formLayout->addRow("beta", &betaSlider);

    //! Connect signals and slots for sliders.
    connect(&alphaSlider, SIGNAL(valueChanged(int)), this, SLOT(alphaBetaSliderValueChanged()));
    connect(&betaSlider, SIGNAL(valueChanged(int)), this, SLOT(alphaBetaSliderValueChanged()));
}

void LightEditor::alphaBetaSliderValueChanged() {

    //! Get alpha value in [-180, 180] degrees.
    float alpha = -180.0f + float(alphaSlider.value() - alphaSlider.minimum()) / float(alphaSlider.maximum() - alphaSlider.minimum()) * 360.0f;

    //! Get beta value in [-90, 90] degrees.
    float beta = -90.0f + float(betaSlider.value() - betaSlider.minimum()) / float(betaSlider.maximum() - betaSlider.minimum()) * 180.0f;

    //! Compute unit vector.
    float lightX = cos(alpha * M_PI/180.0f) * cos(beta * M_PI/180.0f);
    float lightY = sin(alpha * M_PI/180.0f) * cos(beta * M_PI/180.0f);
    float lightZ = sin(beta * M_PI/180.0f);

    //! Update OSPRay light direction.
    ospSet3f(light, "direction", lightX, lightY, lightZ);

    //! Commit and emit signal.
    ospCommit(light);
    emit lightChanged();
}
