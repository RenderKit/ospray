#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION

#include "ospray_test_tools.h"

extern OSPRayEnvironment * ospEnv;

OsprayStatus writePPM(const std::string& fileName, const osp::vec2i &size, const uint32_t *pixel) {
  std::ofstream outFile(fileName.c_str(), std::ofstream::out | std::ofstream::binary);
  if (!outFile.good()) {
    std::cerr << "Failed to open file " << fileName << std::endl;
    return OsprayStatus::Error;
  }

  outFile << "P6\n"
    << size.x << " " << size.y << "\n"
    << "255\n";

  std::vector<pixelColorValue> out_vec(3 * size.x);
  pixelColorValue* out = out_vec.data();

  for (int y = 0; y < size.y; y++) {
    const pixelColorValue* in = (const pixelColorValue*)(pixel + (size.y-1-y)*size.x);

    for (int x = 0; x < size.x; x++)
      std::memcpy(out + 3*x, in + 4*x, 3);

    outFile.write((const char*)out, 3 * size.x);
  }
  outFile << '\n';

  return OsprayStatus::Ok;
}

OsprayStatus writePNG(const std::string& fileName, const osp::vec2i &size, const uint32_t *pixel) {

  unsigned int bufferLen = 4 * size.x * size.y;
  std::vector<pixelColorValue> writeImage(bufferLen, 255);

  for (int y = 0; y < size.y; ++y) {
    pixelColorValue* lineAdrr = &(writeImage[4*size.x *(size.y-1- y)]);
    std::memcpy(lineAdrr, &(pixel[size.x*y]), 4*sizeof(pixelColorValue)*size.x);
  }
  int retCode = stbi_write_png(fileName.c_str(), size.x, size.y, static_cast<int>(ImgType::RGBA), (const void*)writeImage.data(), 0);
  if (!retCode)
  {
    std::cerr << "Failed to save image: " << fileName << std::endl;
    return OsprayStatus::Fail;
  }
  return OsprayStatus::Ok;
}

OsprayStatus writeHDR(const std::string& fileName, const osp::vec2i &size, const uint32_t *pixel) {
  int retCode = stbi_write_hdr(fileName.c_str(), size.x, size.y, static_cast<int>(ImgType::RGBA), (const float*)pixel);
  if (!retCode)
  {
    std::cerr << "Failed to save image: " << fileName << std::endl;
    return OsprayStatus::Fail;
  }
  return OsprayStatus::Ok;
}

// helper function to write the rendered image
OsprayStatus writeImg(const std::string& fileName, const osp::vec2i &size, const uint32_t *pixel) {
  OsprayStatus writeErr = OsprayStatus::Error;
  const std::string fileFormat = fileName.substr(fileName.find_last_of(".") + 1);
  if (fileFormat == "ppm") {
    writeErr =  writePPM(fileName, size, pixel);
  }else if (fileFormat == "png") {
    writeErr =  writePNG(fileName, size, pixel);
  }else if (fileFormat == "hdr") {
    writeErr =  writeHDR(fileName, size, pixel);
  }else {
    std::cerr << "Unsuporrted file format" << std::endl;
    writeErr =  OsprayStatus::Error;
  }

  return writeErr;
}

// comparare the baseline image wiht the values form the framebuffer
OsprayStatus compareImgWithBaseline(const osp::vec2i &size, const uint32_t *testImg, const std::string &testName) {
  pixelColorValue* testImage = (pixelColorValue*)testImg;

  std::string baselineName = ospEnv->GetBaselineDir() + "/" + testName + ".png";

  int dataX , dataY, dataN;
  pixelColorValue *baselineData = stbi_load(baselineName.c_str(), &dataX, &dataY, &dataN, 4);
  if (!baselineData) {
    std::cerr << "Failed to load image: " << baselineName << std::endl;
    return OsprayStatus::Fail;
  }else if(dataX != size.x || dataY != size.y)
  {
    std::cerr << "Wrong image loaded for: " << baselineName << std::endl;
    stbi_image_free(baselineData);
    return OsprayStatus::Fail;
  }

  unsigned int bufferLen = 4 * size.x * size.y;
  std::vector<pixelColorValue> baselineImage(bufferLen, 255);
  std::vector<pixelColorValue> diffImage(bufferLen, 255);

  for (int y = 0; y < size.y; ++y) {
    pixelColorValue* lineAdrr = &(baselineImage[4*size.x *(size.y-1- y)]);
    std::memcpy(lineAdrr, &(baselineData[4*size.x*y]), 4*sizeof(pixelColorValue)*size.x);
  }

  bool notPerfect = false;
  unsigned incorrectPixels = 0;
  pixelColorValue maxError = 0;
  pixelColorValue minError = 255;
  long long totalError = 0;

  for (int pixel = 0; pixel < size.x * size.y; ++pixel) {
    for (int channel = 0; channel < 3; ++channel) {
      pixelColorValue baselineValue = baselineImage[4*pixel + channel];
      pixelColorValue renderedValue = testImage[4*pixel + channel];
      pixelColorValue diffValue = std::abs((int)baselineValue - (int)renderedValue);

      notPerfect = notPerfect || diffValue;
      maxError = std::max(maxError, diffValue);
      minError = std::min(minError, diffValue);
      totalError += diffValue;
      if (diffValue > pixelThreshold)
        incorrectPixels++;

      diffImage[4*pixel + channel] = diffValue;
    }
  }

  if (notPerfect)
    std::cerr << "[ WARNING  ] " << baselineName << " is not pixel perfect" << std::endl;

  if(incorrectPixels > 0) {
    double meanError = totalError / double(3*size.x*size.y);
    double variance = 0.0;
    for (int pixel = 0; pixel < size.x * size.y; ++pixel)
      for (int channel = 0; channel < 3; ++channel) {
         double diff = diffImage[4*pixel + channel] - meanError;
         variance += diff * diff;
      }
    variance /= (3*size.x*size.y);
    double stdDev = sqrt(variance);

    std::cerr << "[ STATISTIC] Number of errors: " << incorrectPixels << std::endl;
    std::cerr << "[ STATISTIC] Min/Max/Mean/StdDev: "
      << (int)minError << "/"
      << (int)maxError << "/"
      << std::fixed << std::setprecision(2) << meanError << "/"
      << std::fixed << std::setprecision(2) << stdDev
      << std::endl;
  }

  bool failed = (incorrectPixels / double(3*size.x*size.y)) > errorRate;

  if (failed) {
    writeImg(ospEnv->GetFailedDir()+"/"+testName+"_baseline.png", size, (const uint32_t*)baselineImage.data());
    writeImg(ospEnv->GetFailedDir()+"/"+testName+"_rendered.png", size, (const uint32_t*)testImage);
    writeImg(ospEnv->GetFailedDir()+"/"+testName+"_diff.png",     size, (const uint32_t*)diffImage.data());
  }

  stbi_image_free(baselineData);
  if (failed)
    return OsprayStatus::Fail;
  else
    return OsprayStatus::Ok;
}

