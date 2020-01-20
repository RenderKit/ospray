// Copyright 2017-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "test_tools.h"

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

OsprayStatus OSPImageTools::writePPM(
    std::string fileName, const uint32_t *pixel)
{
  std::ofstream outFile(
      fileName.c_str(), std::ofstream::out | std::ofstream::binary);
  if (!outFile.good()) {
    std::cerr << "Failed to open file " << fileName << std::endl;
    return OsprayStatus::Error;
  }

  outFile << "P6\n"
          << size.x << " " << size.y << "\n"
          << "std::numeric_limits<pixelColorValue>::max()\n";

  std::vector<pixelColorValue> out_vec(3 * size.x);
  pixelColorValue *out = out_vec.data();

  for (int y = 0; y < size.y; y++) {
    const pixelColorValue *in =
        (const pixelColorValue *)(pixel + (size.y - 1 - y) * size.x);

    for (int x = 0; x < size.x; x++)
      std::memcpy(out + 3 * x, in + ImgType::RGBA * x, 3);

    outFile.write((const char *)out, 3 * size.x);
  }
  outFile << '\n';

  return OsprayStatus::Ok;
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
    writeErr = writePPM(fileName, (const uint32_t *)pixel);
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

// compare the baseline image with the values form the framebuffer
OsprayStatus OSPImageTools::compareImgWithBaseline(const uint32_t *testImg)
{
  pixelColorValue *testImage = (pixelColorValue *)testImg;
  std::string baselineName =
      ospEnv->GetBaselineDir() + "/" + imgName + GetFileFormat();

  int dataX, dataY, dataN;
  pixelColorValue *baselineData =
      stbi_load(baselineName.c_str(), &dataX, &dataY, &dataN, ImgType::RGBA);
  if (!baselineData) {
    std::cerr << "Failed to load image: " << baselineName << std::endl;
    return OsprayStatus::Fail;
  } else if (dataX != size.x || dataY != size.y) {
    std::cerr << "Wrong image loaded for: " << baselineName << std::endl;
    stbi_image_free(baselineData);
    return OsprayStatus::Fail;
  }

  unsigned int bufferLen = ImgType::RGBA * size.x * size.y;
  std::vector<pixelColorValue> baselineImage(
      bufferLen, std::numeric_limits<pixelColorValue>::max());
  std::vector<pixelColorValue> diffImage(
      bufferLen, std::numeric_limits<pixelColorValue>::max());

  for (int y = 0; y < size.y; ++y) {
    pixelColorValue *lineAdrr =
        &(baselineImage[ImgType::RGBA * size.x * (size.y - 1 - y)]);
    std::memcpy(lineAdrr,
        &(baselineData[ImgType::RGBA * size.x * y]),
        ImgType::RGBA * sizeof(pixelColorValue) * size.x);
  }

  bool notPerfect = false;
  unsigned incorrectPixels = 0;
  pixelColorValue maxError = 0;
  pixelColorValue minError = std::numeric_limits<pixelColorValue>::max();
  long long totalError = 0;

  for (int pixel = 0; pixel < size.x * size.y; ++pixel) {
    for (int channel = 0; channel < 3; ++channel) {
      pixelColorValue baselineValue =
          baselineImage[ImgType::RGBA * pixel + channel];
      pixelColorValue renderedValue =
          testImage[ImgType::RGBA * pixel + channel];
      pixelColorValue diffValue =
          std::abs((int)baselineValue - (int)renderedValue);

      notPerfect = notPerfect || diffValue;
      maxError = std::max(maxError, diffValue);
      minError = std::min(minError, diffValue);
      totalError += diffValue;
      if (diffValue > pixelThreshold)
        incorrectPixels++;

      diffImage[ImgType::RGBA * pixel + channel] = diffValue;
    }
  }

  if (notPerfect)
    std::cerr << "[ WARNING  ] " << baselineName << " is not pixel perfect"
              << std::endl;

  if (incorrectPixels > 0) {
    double meanError = totalError / double(3 * size.x * size.y);
    double variance = 0.0;
    for (int pixel = 0; pixel < size.x * size.y; ++pixel)
      for (int channel = 0; channel < 3; ++channel) {
        double diff = diffImage[ImgType::RGBA * pixel + channel] - meanError;
        variance += diff * diff;
      }
    variance /= (3 * size.x * size.y);
    double stdDev = sqrt(variance);

    std::cerr << "[ STATISTIC] Number of errors: " << incorrectPixels
              << std::endl;
    std::cerr << "[ STATISTIC] Min/Max/Mean/StdDev: " << (int)minError << "/"
              << (int)maxError << "/" << std::fixed << std::setprecision(2)
              << meanError << "/" << std::fixed << std::setprecision(2)
              << stdDev << std::endl;
  }

  bool failed = (incorrectPixels / double(3 * size.x * size.y)) > errorRate;

  if (failed) {
    writeImg(ospEnv->GetFailedDir() + "/" + imgName + "_baseline",
        (const uint32_t *)baselineImage.data());
    writeImg(ospEnv->GetFailedDir() + "/" + imgName + "_rendered",
        (const uint32_t *)testImage);
    writeImg(ospEnv->GetFailedDir() + "/" + imgName + "_diff",
        (const uint32_t *)diffImage.data());
  }

  stbi_image_free(baselineData);
  if (failed)
    return OsprayStatus::Fail;
  else
    return OsprayStatus::Ok;
}
