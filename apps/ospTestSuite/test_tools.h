// Copyright 2017-2019 Intel Corporation
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

using pixelColorValue = unsigned char;

const pixelColorValue pixelThreshold = 10;
const float errorRate = 0.03;

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

  // helper method to write the rendered image as PPM file
  OsprayStatus writePPM(std::string fileName, const uint32_t *pixel);
  // helper method to write the rendered image as PNG file
  OsprayStatus writePNG(std::string fileName, const uint32_t *pixel);
  // helper method to write the rendered image as HDR file
  OsprayStatus writeHDR(std::string fileName, const float *pixel);
  // helper method to write the image with given format
  OsprayStatus writeImg(std::string fileName, const void *pixel);
  std::string GetFileFormat() const
  {
    return fileFormat;
  }

 public:
  OSPImageTools(vec2i imgSize,
      std::string testName,
      OSPFrameBufferFormat frameBufferFormat);
  ~OSPImageTools() = default;

  // helper method to saved rendered file
  OsprayStatus saveTestImage(const void *pixel);
  // helper method to compare gold image with current framebuffer render
  OsprayStatus compareImgWithBaseline(const uint32_t *testImg);
};
