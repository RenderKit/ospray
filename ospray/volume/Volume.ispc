#include "ospray/volume/Volume.ih"

export void Volume_setGammaCorrection(void *uniform _self, const uniform vec2f &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->gammaCorrection = value;
}

export void Volume_setGradientShadingEnabled(void *uniform _self, const uniform bool &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->gradientShadingEnabled = value;
}

export void Volume_setNumIsovalues(void *uniform _self, const uniform int value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->numIsovalues = value;
}

export void Volume_setIsovalues(void *uniform _self, uniform float *uniform values)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->isovalues = values;
}

export void Volume_setSamplingRate(void *uniform _self, const uniform float value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->samplingRate = value;
}

export void Volume_setTransferFunction(void *uniform _self, void *uniform value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->transferFunction = (TransferFunction *uniform) value;
}

export void Volume_setVolumeClippingBox(void *uniform _self, const uniform box3f &value)
{
  uniform Volume *uniform self = (uniform Volume *uniform)_self;
  self->volumeClippingBox = value;
}
