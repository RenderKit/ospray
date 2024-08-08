// Copyright 2017 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "test_tools.h"
#include <fstream>
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
    fileFormat = ".pfm";
    break;
  default:
    fileFormat = ".err";
    break;
  }
}

OsprayStatus OSPImageTools::writePNG(
    std::string fileName, const uint32_t *pixel)
{
  stbi_flip_vertically_on_write(true);
  int retCode = stbi_write_png(
      fileName.c_str(), size.x, size.y, ImgType::RGBA, (const void *)pixel, 0);
  if (!retCode) {
    std::cerr << "Failed to save image: " << fileName << std::endl;
    return OsprayStatus::Fail;
  }
  return OsprayStatus::Ok;
}

OsprayStatus OSPImageTools::writeHDR(std::string fileName, const float *pixel)
{
  stbi_flip_vertically_on_write(true);
  int retCode =
      stbi_write_hdr(fileName.c_str(), size.x, size.y, ImgType::RGBA, pixel);
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
  } else if (GetFileFormat() == ".pfm") {
    try {
      rkcommon::utility::writePFM(fileName, size.x, size.y, (const vec4f *)pixel);
      writeErr = OsprayStatus::Ok;
    } catch (...) {
      writeErr = OsprayStatus::Fail;
    }
  } else {
    std::cerr << "Unsupported file format" << std::endl;
    writeErr = OsprayStatus::Error;
  }

  return writeErr;
}

OsprayStatus OSPImageTools::verifyBaselineImage(const int sizeX,
    const int sizeY,
    const void *baselineImage,
    const std::string &baselineName)
{
  // Check if baseline image is suitable
  if (!baselineImage) {
    std::cerr << "Failed to load image: " << baselineName << std::endl;
    return OsprayStatus::Fail;
  } else if (sizeX != size.x || sizeY != size.y) {
    std::cerr << "Wrong image loaded for: " << baselineName << std::endl;
    return OsprayStatus::Fail;
  }
  return OsprayStatus::Ok;
}

vec4f OSPImageTools::getAveragedPixel(const vec4f *image,
    vec2i pixelIndex,
    const rkcommon::index_sequence_2D &imageIndices)
{
  vec4f p(0.f);
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
template <typename T>
OsprayStatus OSPImageTools::compareImgWithBaselineTmpl(const T *testImage,
    const T *baselineImage,
    const std::string &baselineName,
    const bool writeImages,
    const float pixelConversionFactor)
{
  bool notPerfect = false;
  double totalError = 0.;

  rkcommon::index_sequence_2D imageIndices(size);
  std::vector<T> diffAbsImage(imageIndices.total_indices());
  {
    // Prepare temporary diff image with floats
    std::vector<vec4f> diffImage(imageIndices.total_indices());
    for (vec2i i : imageIndices) {
      const unsigned int pixelIndex = imageIndices.flatten(i);
      const vec4f baselineValue = baselineImage[pixelIndex];
      const vec4f renderedValue = testImage[pixelIndex];
      diffImage[pixelIndex] = baselineValue - renderedValue;
    }

    for (vec2i i : imageIndices) {
      const unsigned int pixelIndex = imageIndices.flatten(i);
      const T diffValue = abs(diffImage[pixelIndex]);
      const vec4f diffAvgValue =
          abs(getAveragedPixel(diffImage.data(), i, imageIndices));

      // Only count errors if above specified threshold, this removes blurred
      // noise
      const float pixelError = reduce_add(diffAvgValue) * pixelConversionFactor;
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
  if (failed && writeImages) {
    writeImg(
        ospEnv->GetFailedDir() + "/" + imgName + "_baseline", baselineImage);
    writeImg(ospEnv->GetFailedDir() + "/" + imgName + "_rendered", testImage);
    writeImg(
        ospEnv->GetFailedDir() + "/" + imgName + "_diff", diffAbsImage.data());
  }

  if (failed)
    return OsprayStatus::Fail;
  else
    return OsprayStatus::Ok;
}

OsprayStatus OSPImageTools::saveTestImage(const void *pixel)
{
  return writeImg(ospEnv->GetBaselineDir() + "/" + imgName, pixel);
}

vec4f *loadPF4(std::string fileName, int &sizeX, int &sizeY)
{
  std::ifstream ifs;

  ifs.open(fileName, std::ifstream::in | std::ifstream::binary);
  if (!ifs.good())
    return nullptr;

  std::string header;
  std::getline(ifs, header);
  if (!ifs.good() || header != "PF4")
    return nullptr;

  ifs >> sizeX >> sizeY;
  std::getline(ifs, header); // eat newline after size information
  if (!ifs.good())
    return nullptr;

  std::getline(ifs, header);
  if (!ifs.good() || header != "-1.0")
    return nullptr;

  size_t sz = sizeX * sizeY;
  vec4f *img = new vec4f[sz];

  ifs.read((char*)img, sz * sizeof(float) * 4);
  if (!ifs.good())
    return nullptr;

  ifs.close();
  return img;
}

// compare the baseline image with the values form the framebuffer
OsprayStatus OSPImageTools::compareImgWithBaseline(const void *testImage)
{
  const std::string baselineNameBase = ospEnv->GetBaselineDir() + "/" + imgName;
  std::string nextCandidate = baselineNameBase + GetFileFormat();
  int candidate = 0;
  struct stat buffer; // for file check
  stbi_set_flip_vertically_on_load(true);
  int dataX, dataY, dataN;
  OsprayStatus compErr = OsprayStatus::Error;
  bool lastCandidate = false;

  while (compErr != OsprayStatus::Ok && !lastCandidate) {
    const std::string baselineName = nextCandidate;
    // look ahead for a next candidate, otherwise write diff images on failure
    nextCandidate =
        baselineNameBase + "_v" + std::to_string(++candidate) + GetFileFormat();
    lastCandidate = stat(nextCandidate.c_str(), &buffer) == -1;

    if (GetFileFormat() == ".png") {
      vec4uc *baselineImage = (vec4uc *)stbi_load(
          baselineName.c_str(), &dataX, &dataY, &dataN, ImgType::RGBA);
      compErr = verifyBaselineImage(dataX, dataY, baselineImage, baselineName);
      if (compErr == OsprayStatus::Ok)
        compErr = compareImgWithBaselineTmpl<vec4uc>(
            (vec4uc *)testImage, baselineImage, baselineName, lastCandidate);
      if (baselineImage)
        stbi_image_free(baselineImage);
    } else if (GetFileFormat() == ".hdr") {
      vec4f *baselineImage = (vec4f *)stbi_loadf(
          baselineName.c_str(), &dataX, &dataY, &dataN, ImgType::RGBA);
      compErr = verifyBaselineImage(dataX, dataY, baselineImage, baselineName);
      if (compErr == OsprayStatus::Ok)
        compErr = compareImgWithBaselineTmpl<vec4f>((vec4f *)testImage,
            baselineImage,
            baselineName,
            lastCandidate,
            255.0f);
      if (baselineImage)
        stbi_image_free(baselineImage);
    } else if (GetFileFormat() == ".pfm") {
      vec4f *baselineImage = loadPF4(baselineName, dataX, dataY);
      compErr = verifyBaselineImage(dataX, dataY, baselineImage, baselineName);
      if (compErr == OsprayStatus::Ok)
        compErr = compareImgWithBaselineTmpl<vec4f>((vec4f *)testImage,
            baselineImage,
            baselineName,
            lastCandidate,
            255.0f);
      if (baselineImage)
        delete[] baselineImage;
    } else {
      std::cerr << "Unsupported file format" << std::endl;
      compErr = OsprayStatus::Error;
    }
  }

  return compErr;
}
