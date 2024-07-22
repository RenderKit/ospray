// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "DenoiseFrameOp.h"
#include "common/DeviceRT.h"
#include "fb/FrameBufferView.h"

namespace ospray {

// error callback
void checkError(void * /*userPtr*/, oidn::Error error, const char *errorMessage)
{
  if (error != oidn::Error::None && error != oidn::Error::Cancelled) {
    throw std::runtime_error(
        "Error running OIDN: " + std::string(errorMessage));
  }
}

void checkError(oidn::DeviceRef &oidnDevice)
{
  const char *errorMessage = nullptr;
  auto error = oidnDevice.getError(errorMessage);
  checkError(nullptr, error, errorMessage);
}

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOp
    : public LiveFrameOpInterface
{
  LiveDenoiseFrameOp(DenoiseFrameOp *denoiser, FrameBufferView &fbView)
      : denoiser(denoiser),
        fbView(fbView),
        filter(denoiser->oidnDevice.newFilter("RT"))
  {
    filter.set("hdr", true);
  }

 protected:
  void updateFilters();
  virtual void initFilterAlphaImages() = 0;
  virtual void initFilterAuxImages() = 0;
  virtual void updateFilterInput() = 0;
  virtual void updateAuxImages(oidn::FilterRef &) = 0;

  Ref<DenoiseFrameOp> denoiser;
  FrameBufferView fbView;
  oidn::FilterRef filter;
  oidn::FilterRef filterAlpha;
  oidn::FilterRef filterAlbedo;
  oidn::FilterRef filterNormal;
  bool prefilter{false};
};

void LiveDenoiseFrameOp::updateFilters()
{
  prefilter = denoiser->quality == OIDN_QUALITY_HIGH;

  if (prefilter) {
    if (fbView.normalBuffer && !filterNormal) {
      filterNormal = denoiser->oidnDevice.newFilter("RT");
      filterNormal.set("quality", OIDN_QUALITY_HIGH);
    }
    if (fbView.albedoBuffer && !filterAlbedo) {
      filterAlbedo = denoiser->oidnDevice.newFilter("RT");
      filterAlbedo.set("quality", OIDN_QUALITY_HIGH);
    }

    initFilterAuxImages();

    if (fbView.normalBuffer)
      filterNormal.commit();
    if (fbView.albedoBuffer)
      filterAlbedo.commit();
  }

  filter.set("quality", denoiser->quality);
  filter.set("cleanAux", prefilter);
  updateFilterInput();
  updateAuxImages(filter);

  if (denoiser->denoiseAlpha) {
    if (!filterAlpha) {
      filterAlpha = denoiser->oidnDevice.newFilter("RT");
      filterAlpha.set("hdr", false);
      initFilterAlphaImages();
    }

    filterAlpha.set("quality", denoiser->quality);
    filterAlpha.set("cleanAux", prefilter);
    updateAuxImages(filterAlpha);
  }

  filter.commit();
  if (denoiser->denoiseAlpha)
    filterAlpha.commit();
}

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpShared
    : public LiveDenoiseFrameOp
{
  LiveDenoiseFrameOpShared(DenoiseFrameOp *, FrameBufferView &);

 protected:
  // when alpha is not denoised we copy input buffer to output and then do
  // in-place denoising to preserve alpha
  oidn::BufferRef bufferOut; // shared buffer aliasing colorBufferOutput

 private:
  void initFilterAlphaImages() override;
  void initFilterAuxImages() override;
  void updateFilterInput() override;
  void updateAuxImages(oidn::FilterRef &) override;

  oidn::BufferRef bufferAux; // scratch buffer when prefiltering
  size_t byteAlbedoOffset;
};

LiveDenoiseFrameOpShared::LiveDenoiseFrameOpShared(
    DenoiseFrameOp *denoiser, FrameBufferView &fbView)
    : LiveDenoiseFrameOp(denoiser, fbView),
      bufferOut(denoiser->oidnDevice.newBuffer(fbView.colorBufferOutput,
          fbView.viewDims.long_product() * sizeof(vec4f)))
{
  filter.setImage("output",
      bufferOut,
      oidn::Format::Float3,
      fbView.viewDims.x,
      fbView.viewDims.y,
      0,
      4 * sizeof(float));

  updateFilters();
}

void LiveDenoiseFrameOpShared::initFilterAlphaImages()
{
  filterAlpha.setImage("color",
      const_cast<vec4f *>(fbView.colorBufferInput),
      oidn::Format::Float,
      fbView.viewDims.x,
      fbView.viewDims.y,
      3 * sizeof(float),
      4 * sizeof(float));
  filterAlpha.setImage("output",
      bufferOut,
      oidn::Format::Float,
      fbView.viewDims.x,
      fbView.viewDims.y,
      3 * sizeof(float),
      4 * sizeof(float));
}

void LiveDenoiseFrameOpShared::initFilterAuxImages()
{
  if (!bufferAux) {
    const size_t byteBufferSize = 3 * sizeof(float) * fbView.fbDims.product();
    size_t sz = 0;
    if (fbView.normalBuffer)
      sz += byteBufferSize;
    if (fbView.albedoBuffer) {
      byteAlbedoOffset = sz;
      sz += byteBufferSize;
    }
    bufferAux = denoiser->oidnDevice.newBuffer(sz, oidn::Storage::Device);
  }

  if (fbView.normalBuffer) {
    filterNormal.setImage("normal",
        const_cast<vec3f *>(fbView.normalBuffer),
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y);
    filterNormal.setImage("output",
        bufferAux,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y);
  }
  if (fbView.albedoBuffer) {
    filterAlbedo.setImage("albedo",
        const_cast<vec3f *>(fbView.albedoBuffer),
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y);
    filterAlbedo.setImage("output",
        bufferAux,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        byteAlbedoOffset);
  }
}

void LiveDenoiseFrameOpShared::updateFilterInput()
{
  if (denoiser->denoiseAlpha)
    filter.setImage("color",
        const_cast<vec4f *>(fbView.colorBufferInput),
        oidn::Format::Float3,
        fbView.viewDims.x,
        fbView.viewDims.y,
        0,
        4 * sizeof(float));
  else
    filter.setImage("color",
        bufferOut, // in-place filtering of copied input
        oidn::Format::Float3,
        fbView.viewDims.x,
        fbView.viewDims.y,
        0,
        4 * sizeof(float));
}

void LiveDenoiseFrameOpShared::updateAuxImages(oidn::FilterRef &filter)
{
  if (prefilter) {
    if (fbView.normalBuffer)
      filter.setImage("normal",
          bufferAux,
          oidn::Format::Float3,
          fbView.viewDims.x,
          fbView.viewDims.y);
    if (fbView.albedoBuffer)
      filter.setImage("albedo",
          bufferAux,
          oidn::Format::Float3,
          fbView.viewDims.x,
          fbView.viewDims.y,
          byteAlbedoOffset);
  } else {
    if (fbView.normalBuffer)
      filter.setImage("normal",
          const_cast<vec3f *>(fbView.normalBuffer),
          oidn::Format::Float3,
          fbView.viewDims.x,
          fbView.viewDims.y);
    if (fbView.albedoBuffer)
      filter.setImage("albedo",
          const_cast<vec3f *>(fbView.albedoBuffer),
          oidn::Format::Float3,
          fbView.viewDims.x,
          fbView.viewDims.y);
  }
}

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpSharedSycl
    : public LiveDenoiseFrameOpShared
{
  LiveDenoiseFrameOpSharedSycl(
      DenoiseFrameOp *denoiser, FrameBufferView &fbView)
      : LiveDenoiseFrameOpShared(denoiser, fbView)
  {}

  devicert::AsyncEvent process() override;
};

devicert::AsyncEvent LiveDenoiseFrameOpSharedSycl::process()
{
  updateFilters();

  // Using SYCL calls without SYCL, that's supported in C99 API only

  if (prefilter) {
    if (filterNormal)
      oidnExecuteSYCLFilterAsync(filterNormal.getHandle(), nullptr, 0, nullptr);
    if (filterAlbedo)
      oidnExecuteSYCLFilterAsync(filterAlbedo.getHandle(), nullptr, 0, nullptr);
  }

  if (denoiser->denoiseAlpha)
    oidnExecuteSYCLFilterAsync(filterAlpha.getHandle(), nullptr, 0, nullptr);
  else
    bufferOut.writeAsync(0,
        fbView.viewDims.long_product() * sizeof(vec4f),
        fbView.colorBufferInput);

  devicert::AsyncEvent event = denoiser->drtDevice.createAsyncEvent();

  oidnExecuteSYCLFilterAsync(
      filter.getHandle(), nullptr, 0, (sycl::event *)event.getSyclEventPtr());

  return event;
}

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpSharedCpu
    : public LiveDenoiseFrameOpShared
{
  LiveDenoiseFrameOpSharedCpu(DenoiseFrameOp *denoiser, FrameBufferView &fbView)
      : LiveDenoiseFrameOpShared(denoiser, fbView)
  {}
  devicert::AsyncEvent process() override;
};

devicert::AsyncEvent LiveDenoiseFrameOpSharedCpu::process()
{
  updateFilters();

  devicert::AsyncEvent event = denoiser->drtDevice.launchHostTask([this]() {
    if (prefilter) {
      if (filterNormal)
        filterNormal.execute();
      if (filterAlbedo)
        filterAlbedo.execute();
    }
    if (denoiser->denoiseAlpha)
      filterAlpha.execute();
    else
      bufferOut.write(0,
          fbView.viewDims.long_product() * sizeof(vec4f),
          fbView.colorBufferInput);
    filter.execute();
  });
  return event;
}

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpCopy
    : public LiveDenoiseFrameOp
{
  LiveDenoiseFrameOpCopy(DenoiseFrameOp *, FrameBufferView &);

  devicert::AsyncEvent process() override;

 private:
  void initFilterAlphaImages() override;
  void initFilterAuxImages() override;
  void updateFilterInput() override;
  void updateAuxImages(oidn::FilterRef &) override;

  oidn::BufferRef buffer;
  size_t byteFloatBufferSize;
  size_t byteNormalOffset;
  size_t byteAlbedoOffset;
};

LiveDenoiseFrameOpCopy::LiveDenoiseFrameOpCopy(
    DenoiseFrameOp *denoiser, FrameBufferView &fbView)
    : LiveDenoiseFrameOp(denoiser, fbView)
{
  byteFloatBufferSize = sizeof(float) * fbView.fbDims.product();
  size_t sz = 4 * byteFloatBufferSize;

  if (fbView.normalBuffer) {
    byteNormalOffset = sz;
    sz += 3 * byteFloatBufferSize;
  }
  if (fbView.albedoBuffer) {
    byteAlbedoOffset = sz;
    sz += 3 * byteFloatBufferSize;
  }
  buffer = denoiser->oidnDevice.newBuffer(sz, oidn::Storage::Device);

  filter.setImage("output",
      buffer,
      oidn::Format::Float3,
      fbView.fbDims.x,
      fbView.fbDims.y,
      0,
      sizeof(float) * 4);

  updateFilters();
}

devicert::AsyncEvent LiveDenoiseFrameOpCopy::process()
{
  updateFilters();

  // As this path is taken when rendering on CPU and denoising on GPU,
  // the asynchronous OIDN API is used to reduce the number of CPU - GPU
  // synchronization points which would take place on every command in case
  // of using synchronous OIDN API.
  devicert::AsyncEvent event = denoiser->drtDevice.launchHostTask([this]() {
    buffer.writeAsync(0, 4 * byteFloatBufferSize, fbView.colorBufferInput);
    if (fbView.normalBuffer)
      buffer.writeAsync(
          byteNormalOffset, 3 * byteFloatBufferSize, fbView.normalBuffer);
    if (fbView.albedoBuffer)
      buffer.writeAsync(
          byteAlbedoOffset, 3 * byteFloatBufferSize, fbView.albedoBuffer);

    if (prefilter) {
      if (filterNormal)
        filterNormal.executeAsync();
      if (filterAlbedo)
        filterAlbedo.executeAsync();
    }

    filter.executeAsync();
    if (denoiser->denoiseAlpha)
      filterAlpha.executeAsync();

    buffer.readAsync(0, 4 * byteFloatBufferSize, fbView.colorBufferOutput);

    denoiser->oidnDevice.sync();
  });
  return event;
}

void LiveDenoiseFrameOpCopy::initFilterAlphaImages()
{
  filterAlpha.setImage("color",
      buffer,
      oidn::Format::Float,
      fbView.fbDims.x,
      fbView.fbDims.y,
      sizeof(float) * 3,
      sizeof(float) * 4);
  filterAlpha.setImage("output",
      buffer,
      oidn::Format::Float,
      fbView.fbDims.x,
      fbView.fbDims.y,
      sizeof(float) * 3,
      sizeof(float) * 4);
}

void LiveDenoiseFrameOpCopy::initFilterAuxImages()
{
  if (fbView.normalBuffer) {
    filterNormal.setImage("normal",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        byteNormalOffset);
    filterNormal.setImage("output",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        byteNormalOffset);
  }
  if (fbView.albedoBuffer) {
    filterAlbedo.setImage("albedo",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        byteAlbedoOffset);
    filterAlbedo.setImage("output",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        byteAlbedoOffset);
  }
}

void LiveDenoiseFrameOpCopy::updateFilterInput()
{
  filter.setImage("color",
      buffer,
      oidn::Format::Float3,
      fbView.fbDims.x,
      fbView.fbDims.y,
      0,
      sizeof(float) * 4);
}

void LiveDenoiseFrameOpCopy::updateAuxImages(oidn::FilterRef &filter)
{
  if (fbView.normalBuffer)
    filter.setImage("normal",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        byteNormalOffset);
  if (fbView.albedoBuffer)
    filter.setImage("albedo",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        byteAlbedoOffset);
}

DenoiseFrameOp::DenoiseFrameOp(devicert::Device &device) : drtDevice(device)
{
  // Get appropriate SYCL command queue for post-processing from device
  sycl::queue *syclQueuePtr = (sycl::queue *)drtDevice.getSyclQueuePtr();
  if (syclQueuePtr) {
    // Using SYCL call without SYCL, that's supported in C99 API only
    oidnDevice = oidnNewSYCLDevice(syclQueuePtr, 1);
  } else {
    oidnDevice = oidn::newDevice();
  }
  checkError(oidnDevice);
  oidnDevice.setErrorFunction(checkError);

  if (device.isDebug())
    oidnDevice.set("verbose", 2);

  oidnDevice.commit();
}

void DenoiseFrameOp::commit()
{
  quality = getParam<uint32_t>("quality", OSP_DENOISER_QUALITY_MEDIUM);
  denoiseAlpha = getParam<bool>("denoiseAlpha", false);
}

std::unique_ptr<LiveFrameOpInterface> DenoiseFrameOp::attach(
    FrameBufferView &fbView)
{
  if (drtDevice.getSyclQueuePtr())
    return rkcommon::make_unique<LiveDenoiseFrameOpSharedSycl>(this, fbView);

  if (oidnDevice.get<bool>("systemMemorySupported"))
    return rkcommon::make_unique<LiveDenoiseFrameOpSharedCpu>(this, fbView);

  return rkcommon::make_unique<LiveDenoiseFrameOpCopy>(this, fbView);
}

std::string DenoiseFrameOp::toString() const
{
  return "ospray::DenoiseFrameOp";
}

} // namespace ospray
