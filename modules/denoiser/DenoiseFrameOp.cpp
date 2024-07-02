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

void checkError(oidn::DeviceRef oidnDevice)
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
  void createFilterAlpha()
  {
    filterAlpha = denoiser->oidnDevice.newFilter("RT");
    filterAlpha.set("hdr", false);
  }

  Ref<DenoiseFrameOp> denoiser;
  FrameBufferView fbView;
  oidn::FilterRef filter;
  oidn::FilterRef filterAlpha;
};

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpShared
    : public LiveDenoiseFrameOp
{
  LiveDenoiseFrameOpShared(DenoiseFrameOp *denoiser, FrameBufferView &fbView)
      : LiveDenoiseFrameOp(denoiser, fbView),
        buffer(denoiser->oidnDevice.newBuffer(fbView.colorBufferOutput,
            fbView.viewDims.long_product() * sizeof(vec4f))),
        useInput(!denoiser->denoiseAlpha) // negate to trigger syncFilter
  {
    filter.setImage("output",
        buffer,
        oidn::Format::Float3,
        fbView.viewDims.x,
        fbView.viewDims.y,
        0,
        4 * sizeof(float));
    syncFilters();
    setAuxImagesCommit(filter);
  }

  devicert::AsyncEvent process() override
  {
    syncFilters();

    // TODO: Remove oidn::Buffer and copying after switching to OIDN 2.2
    // when alpha is not denoised we copy input buffer to output and then do
    // in-place denoising to preserve alpha
    devicert::AsyncEvent event;
    if (denoiser->drtDevice.getSyclQueuePtr()) {
      if (denoiser->denoiseAlpha)
        // Using SYCL call without SYCL, that's supported in C99 API only
        oidnExecuteSYCLFilterAsync(
            filterAlpha.getHandle(), nullptr, 0, nullptr);
      else
        buffer.writeAsync(0,
            fbView.viewDims.long_product() * sizeof(vec4f),
            fbView.colorBufferInput);

      event = denoiser->drtDevice.createAsyncEvent();
      sycl::event *syclEvent = (sycl::event *)event.getSyclEventPtr();

      oidnExecuteSYCLFilterAsync(filter.getHandle(), nullptr, 0, syclEvent);
    } else {
      event = denoiser->drtDevice.launchHostTask([this]() {
        if (denoiser->denoiseAlpha)
          filterAlpha.execute();
        else
          buffer.write(0,
              fbView.viewDims.long_product() * sizeof(vec4f),
              fbView.colorBufferInput);
        filter.execute();
      });
    }
    return event;
  }

 private:
  void initializeFilterAlpha()
  {
    if (filterAlpha)
      return;
    createFilterAlpha();
    filterAlpha.setImage("color",
        const_cast<vec4f *>(fbView.colorBufferInput),
        oidn::Format::Float,
        fbView.viewDims.x,
        fbView.viewDims.y,
        3 * sizeof(float),
        4 * sizeof(float));
    filterAlpha.setImage("output",
        buffer,
        oidn::Format::Float,
        fbView.viewDims.x,
        fbView.viewDims.y,
        3 * sizeof(float),
        4 * sizeof(float));
    setAuxImagesCommit(filterAlpha);
  }
  void syncFilters()
  {
    if (denoiser->denoiseAlpha)
      initializeFilterAlpha();
    if (useInput == denoiser->denoiseAlpha)
      return;
    useInput = denoiser->denoiseAlpha;
    if (useInput)
      filter.setImage("color",
          const_cast<vec4f *>(fbView.colorBufferInput),
          oidn::Format::Float3,
          fbView.viewDims.x,
          fbView.viewDims.y,
          0,
          4 * sizeof(float));
    else
      filter.setImage("color",
          buffer,
          oidn::Format::Float3,
          fbView.viewDims.x,
          fbView.viewDims.y,
          0,
          4 * sizeof(float));
    filter.commit();
  }

  void setAuxImagesCommit(oidn::FilterRef filter)
  {
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
    filter.commit();
  }

  oidn::BufferRef buffer;
  bool useInput;
};

struct OSPRAY_MODULE_DENOISER_EXPORT LiveDenoiseFrameOpCopy
    : public LiveDenoiseFrameOp
{
  LiveDenoiseFrameOpCopy(DenoiseFrameOp *denoiser, FrameBufferView &fbView)
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

    filter.setImage("color",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        0,
        sizeof(float) * 4);
    filter.setImage("output",
        buffer,
        oidn::Format::Float3,
        fbView.fbDims.x,
        fbView.fbDims.y,
        0,
        sizeof(float) * 4);
    setAuxImagesCommit(filter);

    if (denoiser->denoiseAlpha)
      initializeFilterAlpha();
  }

  devicert::AsyncEvent process() override
  {
    if (denoiser->denoiseAlpha)
      initializeFilterAlpha();

    devicert::AsyncEvent event = denoiser->drtDevice.launchHostTask([this]() {
      // As this path is taken when rendering on CPU and denoising on GPU,
      // the asynchronous OIDN API is used to reduce the number of CPU - GPU
      // synchronization points which would take place on every command in case
      // of using synchronous OIDN API.

      buffer.writeAsync(0, 4 * byteFloatBufferSize, fbView.colorBufferInput);
      if (fbView.normalBuffer)
        buffer.writeAsync(
            byteNormalOffset, 3 * byteFloatBufferSize, fbView.normalBuffer);
      if (fbView.albedoBuffer)
        buffer.writeAsync(
            byteAlbedoOffset, 3 * byteFloatBufferSize, fbView.albedoBuffer);

      filter.executeAsync();
      if (denoiser->denoiseAlpha)
        filterAlpha.executeAsync();

      buffer.readAsync(0, 4 * byteFloatBufferSize, fbView.colorBufferOutput);

      denoiser->oidnDevice.sync();
    });
    return event;
  }

 private:
  void initializeFilterAlpha()
  {
    if (filterAlpha)
      return;
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
    setAuxImagesCommit(filterAlpha);
  }

  void setAuxImagesCommit(oidn::FilterRef filter)
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
    filter.commit();
  }

  oidn::BufferRef buffer;
  size_t byteFloatBufferSize;
  size_t byteNormalOffset;
  size_t byteAlbedoOffset;
};

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

  sharedMem = syclQueuePtr || oidnDevice.get<bool>("systemMemorySupported");
}

void DenoiseFrameOp::commit()
{
  denoiseAlpha = getParam<bool>("denoiseAlpha", false);
}

std::unique_ptr<LiveFrameOpInterface> DenoiseFrameOp::attach(
    FrameBufferView &fbView)
{
  if (sharedMem)
    return rkcommon::make_unique<LiveDenoiseFrameOpShared>(this, fbView);
  return rkcommon::make_unique<LiveDenoiseFrameOpCopy>(this, fbView);
}

std::string DenoiseFrameOp::toString() const
{
  return "ospray::DenoiseFrameOp";
}

} // namespace ospray
