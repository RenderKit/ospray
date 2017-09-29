#include "ospray_test_tools.h"

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

  int failed = 0;
  for (int y = 0; y < size.y; ++y) {
    for (int x = 0; baseline.good() && x < size.x; ++x) {
      char* baselinePixel = (char*)(baselineImage.data() + 4 * ((size.y-1-y)*size.x + x));
      char* renderedPixel = (char*)(testImage + 4 * ((size.y-1-y)*size.x + x));
      baseline.read(baselinePixel, 3);
      failed |= std::memcmp(baselinePixel, renderedPixel, 2);
    }
  }

  if (!baseline.good()) {
    std::cerr << "File " << baselineName << " is corrupted" << std::endl;
    return OsprayStatus::Error;
  }

  unsigned colorRenderError = 0;
  pixelColorValue maxError = 0;
  pixelColorValue minError = 255;
  float meanError = 0;

  if (failed) {
    std::vector<pixelColorValue> diffImage(bufferLen, 255);

    std::cerr << "[ WARNING  ] " << baselineName << " is not pixel perfect" << std::endl;
    for (int idx = 0; idx < size.x * size.y; ++idx) {
      for ( int colorIdx = 0; colorIdx < 3; ++colorIdx) {
        diffImage[4*idx + colorIdx] = std::abs((int)(testImage[4*idx + colorIdx]) - (int)(baselineImage[4*idx + colorIdx]));

        if(diffImage[4*idx + colorIdx] > pixelThreshold)
        {
          maxError = diffImage[4 *idx + colorIdx] > maxError ? diffImage[4*idx + colorIdx] : maxError;
          minError = diffImage[4 *idx + colorIdx] < minError ? diffImage[4*idx + colorIdx] : minError;
          meanError += diffImage[4*idx + colorIdx];
          colorRenderError++;
        }
      }
    }

    writeImg(ospEnv->GetFailedDir()+"/"+testName+"_baseline.ppm", size, (const uint32_t*)baselineImage.data());
    writeImg(ospEnv->GetFailedDir()+"/"+testName+"_rendered.ppm", size, (const uint32_t*)testImage);
    writeImg(ospEnv->GetFailedDir()+"/"+testName+"_diff.ppm",     size, (const uint32_t*)diffImage.data());
  }

  if(colorRenderError) {
    meanError = (meanError/float(colorRenderError))/3.f;
    std::cerr << "[ STATISTIC] Number of errors: " << colorRenderError << std::endl;
    std::cerr << "[ STATISTIC] Min/Max/Mean: "
      << (int)minError << "/"
      << (int)maxError << "/"
      << std::fixed << std::setprecision(2) << meanError
      << std::endl;
  }
  if (failed != 0 && colorRenderError/float(size.x*size.y) > errorRate)
  {
    return OsprayStatus::Fail;
  }
  else
  {
    return OsprayStatus::Ok;
  }
}

