// Copyright 2017 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <stdint.h>
#include <stdio.h>
#include <cmath>
#include <cstring>
#include <fstream>
#include <limits>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "environment.h"

#include "stb_image.h"
#include "stb_image_write.h"

#include "rkcommon/utility/multidim_index_sequence.h"

using pixelColorValue = float;

const pixelColorValue pixelThreshold = 0.05f;
const float errorRate = 0.0004f;
const float sumThreshold = 9.0f; // XXX disabled, should be 0.00005f;
const float sumThresholdRatio = 0.1f;

enum class OsprayStatus
{
  Ok,
  Fail,
  Error,
};

enum ImgType
{
  Y = 1,
  YA,
  RGB,
  RGBA,
};

class OSPImageTools
{
 protected:
  vec2i size;
  std::string fileFormat;
  std::string imgName;

  std::string GetFileFormat() const
  {
    return fileFormat;
  }
  // helper method to write the rendered image as PNG file
  OsprayStatus writePNG(std::string fileName, const uint32_t *pixel);
  // helper method to write the rendered image as HDR file
  OsprayStatus writeHDR(std::string fileName, const float *pixel);
  // helper method to write the image with given format
  OsprayStatus writeImage(std::string fileName, const void *pixel);
  OsprayStatus writeImg(std::string fileName,
      std::string typeName,
      const void *pixel,
      bool killAlpha = false);
  // average pixels over some window
  OsprayStatus verifyBaselineImage(const int sizeX,
      const int sizeY,
      const void *baselineImage,
      const std::string &baselineName);
  vec4f getAveragedPixel(const vec4f *image,
      vec2i pixelIndex,
      const rkcommon::index_sequence_2D &imageIndices);
  // compare gold image with fb but with storage type abstraction
  template <typename T>
  OsprayStatus compareImgWithBaselineTmpl(const T *testImage,
      const T *baselineImage,
      const std::string &baselineName,
      const bool writeImages,
      const bool denoised, // relax test that assume uniformly distributed noise
      const float pixelConversionFactor = 1.0f / 255.f);

 public:
  OSPImageTools(vec2i imgSize,
      std::string testName,
      OSPFrameBufferFormat frameBufferFormat);
  ~OSPImageTools() = default;

  // helper method to saved rendered file
  OsprayStatus saveTestImage(const void *pixel);
  // helper method to compare gold image with current framebuffer render
  OsprayStatus compareImgWithBaseline(
      const void *testImage, const bool denoised = false);
};
