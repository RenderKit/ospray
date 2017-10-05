#pragma once

#include <stdint.h>
#include <stdio.h>
#include <cstring>
#include <string>
#include <fstream>
#include <vector>
#include <cmath>

#include <ospray/ospray.h>
#include <gtest/gtest.h>
#include <limits>

#include "ospray_environment.h"
#include "../../apps/common/sg/3rdParty/stb_image.h"
#include "../../apps/common/sg/3rdParty/stb_image_write.h"

using pixelColorValue = unsigned char;

const pixelColorValue pixelThreshold = 10;
const float errorRate = 0.03;

enum class OsprayStatus {
  Ok,
  Fail,
  Error,
};

enum ImgType {
    Y=1,
    YA,
    RGB,
    RGBA,
};

// helper function to write the rendered image with given format
OsprayStatus writeImg(const std::string &fileName, const osp::vec2i &size, const void *pixel);
// helper function to write the rendered image as PPM file
OsprayStatus writePPM(const std::string &fileName, const osp::vec2i &size, const uint32_t *pixel);
// helper function to write the rendered image as PNG file
OsprayStatus writePNG(const std::string &fileName, const osp::vec2i &size, const uint32_t *pixel);
// helper function to write the rendered image as HDR file
OsprayStatus writeHDR(const std::string &fileName, const osp::vec2i &size, const float *pixel);
// helper function to compare gold image with current framebuffer render
OsprayStatus compareImgWithBaseline(const osp::vec2i &size, const uint32_t *testImg,const std::string &testName);



