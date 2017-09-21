#include "ospray_test_tools.h"

#include <cmath>

extern OSPRayEnvironment * ospEnv;

// helper function to write the rendered image as PPM file
OsprayStatus writeImg(const std::string& fileName, const osp::vec2i &size, const uint32_t *pixel) {
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

// comparare the baseline image wiht the values form the framebuffer
OsprayStatus compareImgWithBaseline(const osp::vec2i &size, const uint32_t *testImg, const std::string &testName) {
  pixelColorValue* testImage = (pixelColorValue*)testImg;

  std::string baselineName = ospEnv->GetBaselineDir() + testName + ".ppm";
  std::ifstream baseline(baselineName.c_str(), std::ifstream::in | std::ifstream::binary);
  if (!baseline.good()) {
    std::cerr << "Failed to open file " << baselineName << std::endl;
    return OsprayStatus::Error;
  }

  //  ignore 3 newlines
  for (int ignoredNewlines = 0; ignoredNewlines < 3; ++ignoredNewlines) {
    while (!baseline.eof() && baseline.get() != '\n') { }
  }

  unsigned int bufferLen = 4 * size.x * size.y;
  std::vector<pixelColorValue> baselineImage(bufferLen, 255);
  std::vector<pixelColorValue> diffImage(bufferLen, 0);

  for (int y = 0; y < size.y; ++y) {
    for (int x = 0; x < size.x; ++x) {
      int index = 4*((size.y-y-1) * size.x + x);
      pixelColorValue* pixelAddr = &(baselineImage[index]);
      baseline.read((char*)pixelAddr, 3);
    }
  }

  if (!baseline.good()) {
    std::cerr << "File " << baselineName << " is corrupted" << std::endl;
    return OsprayStatus::Error;
  }

  bool notPerfect = false;
  unsigned incorrectPixels = 0;
  pixelColorValue maxError = 0;
  pixelColorValue minError = 255;
  long long totalError = 0;

  for (int pixel = 0; pixel < size.x * size.y; ++pixel) {
    for (int channel = 0; baseline.good() && channel < 3; ++channel) {
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
    writeImg(ospEnv->GetFailedDir()+"/"+testName+"_baseline.ppm", size, (const uint32_t*)baselineImage.data());
    writeImg(ospEnv->GetFailedDir()+"/"+testName+"_rendered.ppm", size, (const uint32_t*)testImage);
    writeImg(ospEnv->GetFailedDir()+"/"+testName+"_diff.ppm",     size, (const uint32_t*)diffImage.data());
  }

  if (failed)
    return OsprayStatus::Fail;
  else
    return OsprayStatus::Ok;
}

