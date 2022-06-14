// Copyright 2017 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "test_tools.h"
#include "rkcommon/utility/SaveImage.h"

extern OSPRayEnvironment *ospEnv;

OSPImageTools::OSPImageTools(
    vec2i imgSize, std::string testName, OSPFrameBufferFormat frameBufferFormat)
    : size(imgSize), imgName(testName)
{
  switch (frameBufferFormat) {
  case OSP_FB_RGBA8:
    fileFormat = ".ppm";
    break;
  case OSP_FB_SRGBA:
    fileFormat = ".png";
    break;
  case OSP_FB_RGBA32F:
    fileFormat = ".hdr";
    break;
  default:
    fileFormat = ".err";
    break;
  }
}

OsprayStatus OSPImageTools::writePNG(
    std::string fileName, const uint32_t *pixel)
{
  unsigned int bufferLen = ImgType::RGBA * size.x * size.y;
  std::vector<pixelColorValue> writeImage(
      bufferLen, std::numeric_limits<pixelColorValue>::max());

  for (int y = 0; y < size.y; ++y) {
    pixelColorValue *lineAdrr =
        &(writeImage[ImgType::RGBA * size.x * (size.y - 1 - y)]);
    std::memcpy(lineAdrr,
        &(pixel[size.x * y]),
        ImgType::RGBA * sizeof(pixelColorValue) * size.x);
  }
  int retCode = stbi_write_png(fileName.c_str(),
      size.x,
      size.y,
      ImgType::RGBA,
      (const void *)writeImage.data(),
      0);
  if (!retCode) {
    std::cerr << "Failed to save image: " << fileName << std::endl;
    return OsprayStatus::Fail;
  }
  return OsprayStatus::Ok;
}

OsprayStatus OSPImageTools::writeHDR(std::string fileName, const float *pixel)
{
  unsigned int bufferLen = ImgType::RGBA * size.x * size.y;
  std::vector<float> writeImage(bufferLen, std::numeric_limits<float>::max());

  for (int y = 0; y < size.y; ++y) {
    float *lineAdrr = &(writeImage[ImgType::RGBA * size.x * (size.y - 1 - y)]);
    std::memcpy(lineAdrr,
        &(pixel[size.x * y]),
        ImgType::RGBA * sizeof(pixelColorValue) * size.x);
  }
  int retCode = stbi_write_hdr(
      fileName.c_str(), size.x, size.y, ImgType::RGBA, writeImage.data());
  if (!retCode) {
    std::cerr << "Failed to save image: " << fileName << std::endl;
    return OsprayStatus::Fail;
  }
  return OsprayStatus::Ok;
}

// helper function to write the rendered image
OsprayStatus OSPImageTools::writeImg(std::string fileName, const void *pixel)
{
  OsprayStatus writeErr = OsprayStatus::Error;
  fileName += GetFileFormat();
  if (GetFileFormat() == ".ppm") {
    rkcommon::utility::writePPM(
        fileName, size.x, size.y, (const uint32_t *)pixel);
    writeErr = OsprayStatus::Ok;
  } else if (GetFileFormat() == ".png") {
    writeErr = writePNG(fileName, (const uint32_t *)pixel);
  } else if (GetFileFormat() == ".hdr") {
    writeErr = writeHDR(fileName, (const float *)pixel);
  } else {
    std::cerr << "Unsuporrted file format" << std::endl;
    writeErr = OsprayStatus::Error;
  }

  return writeErr;
}

OsprayStatus OSPImageTools::saveTestImage(const void *pixel)
{
  return writeImg(ospEnv->GetBaselineDir() + "/" + imgName, pixel);
}

vec4f OSPImageTools::getAveragedPixel(const vec4i *image,
    vec2i pixelIndex,
    const rkcommon::index_sequence_2D &imageIndices)
{
  vec4i p(0);
  unsigned int count = 0;
  rkcommon::index_sequence_2D indices(vec2i(5));
  for (vec2i id : indices) {
    vec2i pid = pixelIndex + (id - vec2i(2));
    if ((reduce_min(pid) < 0) || (reduce_min(size - pid) < 1))
      continue;

    p += image[imageIndices.flatten(pid)];
    count++;
  }
  return p / float(count);
}

// compare the baseline image with the values form the framebuffer
OsprayStatus OSPImageTools::compareImgWithBaseline(const uint32_t *testImg)
{
  vec4uc *testImage = (vec4uc *)testImg;
  std::string baselineName =
      ospEnv->GetBaselineDir() + "/" + imgName + GetFileFormat();

  int dataX, dataY, dataN;
  stbi_set_flip_vertically_on_load(true);
  vec4uc *baselineImage = (vec4uc *)stbi_load(
      baselineName.c_str(), &dataX, &dataY, &dataN, ImgType::RGBA);
  if (!baselineImage) {
    std::cerr << "Failed to load image: " << baselineName << std::endl;
    return OsprayStatus::Fail;
  } else if (dataX != size.x || dataY != size.y) {
    std::cerr << "Wrong image loaded for: " << baselineName << std::endl;
    stbi_image_free(baselineImage);
    return OsprayStatus::Fail;
  }

  bool notPerfect = false;
  long long totalError = 0;
  rkcommon::index_sequence_2D imageIndices(size);
  std::vector<vec4uc> diffAbsImage(imageIndices.total_indices());
  {
    // Prepare temporary diff image with signed integers
    std::vector<vec4i> diffImage(imageIndices.total_indices());
    for (vec2i i : imageIndices) {
      const unsigned int pixelIndex = imageIndices.flatten(i);
      const vec4i baselineValue = baselineImage[pixelIndex];
      const vec4i renderedValue = testImage[pixelIndex];
      diffImage[pixelIndex] = baselineValue - renderedValue;
    }

    for (vec2i i : imageIndices) {
      const unsigned int pixelIndex = imageIndices.flatten(i);
      const vec4uc diffValue = abs(diffImage[pixelIndex]);
      const vec4i diffAvgValue =
          abs(getAveragedPixel(diffImage.data(), i, imageIndices));

      // Only count errors if above specified threshold, this removes blurred
      // noise
      const int pixelError = reduce_add(diffAvgValue);
      if (pixelError > pixelThreshold)
        totalError += pixelError;

      // Not a perfect match if any difference detected
      notPerfect = notPerfect || reduce_add(diffValue);

      // Set values for output diff image
      diffAbsImage[pixelIndex] = diffValue;
      diffAbsImage[pixelIndex].w = std::numeric_limits<unsigned char>::max();
    }
  }

  if (notPerfect)
    std::cerr << "[ WARNING  ] " << baselineName << " is not pixel perfect"
              << std::endl;

  double meanError = totalError / double(4 * size.x * size.y);
  if (totalError) {
    std::cerr << "[ STATISTIC] Total/mean error: " << totalError << "/"
              << std::fixed << std::setprecision(3) << meanError << std::endl;
  }

  bool failed = meanError > errorRate;
  if (failed) {
    writeImg(ospEnv->GetFailedDir() + "/" + imgName + "_baseline",
        (const uint32_t *)baselineImage);
    writeImg(ospEnv->GetFailedDir() + "/" + imgName + "_rendered",
        (const uint32_t *)testImage);
    writeImg(ospEnv->GetFailedDir() + "/" + imgName + "_diff",
        (const uint32_t *)diffAbsImage.data());
  }

  stbi_image_free(baselineImage);
  if (failed)
    return OsprayStatus::Fail;
  else
    return OsprayStatus::Ok;
}
