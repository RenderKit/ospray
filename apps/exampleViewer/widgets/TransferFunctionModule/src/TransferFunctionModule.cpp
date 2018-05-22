// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#include <exception>
#include <stdexcept>
#include <fstream>
#include "TransferFunctionModule.h"

using namespace tfn;

// The magic number is 'OSTF' in ASCII
const static uint32_t MAGIC_NUMBER = 0x4f535446;
const static uint64_t CURRENT_VERSION = 1;

TransferFunction::TransferFunction(const std::string &fileName) {
  std::ifstream fin(fileName.c_str(), std::ios::binary);
  if (!fin.is_open()) {
    throw std::runtime_error("File " + fileName + " not found");
  }
  // Verify this is actually a tfn::TransferFunction data file
  uint32_t magic = 0;
  if (!fin.read(reinterpret_cast<char*>(&magic), sizeof(uint32_t))) {
    throw std::runtime_error("Failed to read magic number header from " + fileName);
  }
  if (magic != MAGIC_NUMBER) {
    throw std::runtime_error("Read invalid identification header from " + fileName);
  }
  uint64_t version = 0;
  if (!fin.read(reinterpret_cast<char*>(&version), sizeof(uint64_t))) {
    throw std::runtime_error("Failed to read version header from " + fileName);
  }
  // Check if it's a supported version we can parse
  if (version != CURRENT_VERSION) {
    throw std::runtime_error("Got invalid version number from " + fileName);
  }

  uint64_t nameLen = 0;
  if (!fin.read(reinterpret_cast<char*>(&nameLen), sizeof(uint64_t))) {
    throw std::runtime_error("Failed to read nameLength header from " + fileName);
  }
  name.resize(nameLen);
  if (!fin.read(&name[0], nameLen)) {
    throw std::runtime_error("Failed to read name from " + fileName);
  }
  uint64_t numColors = 0;
  if (!fin.read(reinterpret_cast<char*>(&numColors), sizeof(uint64_t))) {
    throw std::runtime_error("Failed to read numColors header from " + fileName);
  }
  uint64_t numOpacities = 0;
  if (!fin.read(reinterpret_cast<char*>(&numOpacities), sizeof(uint64_t))) {
    throw std::runtime_error("Failed to read numOpacities header from " + fileName);
  }
  if (!fin.read(reinterpret_cast<char*>(&dataValueMin), sizeof(double))) {
    throw std::runtime_error("Failed to read dataValueMin header from " + fileName);
  }
  if (!fin.read(reinterpret_cast<char*>(&dataValueMax), sizeof(double))) {
    throw std::runtime_error("Failed to read dataValueMax header from " + fileName);
  }
  if (!fin.read(reinterpret_cast<char*>(&opacityScaling), sizeof(float))) {
    throw std::runtime_error("Failed to read opacityScaling header from " + fileName);
  }
  rgbValues.resize(numColors, vec3f{0.f,0.f,0.f});
  if (!fin.read(reinterpret_cast<char*>(rgbValues.data()), numColors * 3 * sizeof(float))) {
    throw std::runtime_error("Failed to read color values from " + fileName);
  }
  opacityValues.resize(numOpacities, vec2f{0.f,0.f});
  if (!fin.read(reinterpret_cast<char*>(opacityValues.data()), numOpacities * 2 * sizeof(float))) {
    throw std::runtime_error("Failed to read opacity values from " + fileName);
  }
}
TransferFunction::TransferFunction(const std::string &name,
    const std::vector<tfn::vec3f> &rgbValues,
    const std::vector<tfn::vec2f> &opacityValues, const double dataValueMin,
    const double dataValueMax, const float opacityScaling)
  : name(name), rgbValues(rgbValues), opacityValues(opacityValues), dataValueMin(dataValueMin),
  dataValueMax(dataValueMax), opacityScaling(opacityScaling)
{}
void TransferFunction::save(const std::string &fileName) const {
  std::ofstream fout(fileName.c_str(), std::ios::binary);
  if (!fout.is_open()) {
    throw std::runtime_error("Failed to open " + fileName + " for writing");
  }
  if (!fout.write(reinterpret_cast<const char*>(&MAGIC_NUMBER), sizeof(uint32_t))) {
    throw std::runtime_error("Failed to write magic number header to " + fileName);
  }
  if (!fout.write(reinterpret_cast<const char*>(&CURRENT_VERSION), sizeof(uint64_t))) {
    throw std::runtime_error("Failed to write version header to " + fileName);
  }

  const uint64_t nameLen = name.size();
  if (!fout.write(reinterpret_cast<const char*>(&nameLen), sizeof(uint64_t))) {
    throw std::runtime_error("Failed to write nameLength header to " + fileName);
  }
  if (!fout.write(name.data(), nameLen)) {
    throw std::runtime_error("Failed to write name to " + fileName);
  }
  const uint64_t numColors = rgbValues.size();
  if (!fout.write(reinterpret_cast<const char*>(&numColors), sizeof(uint64_t))) {
    throw std::runtime_error("Failed to write numColors header to " + fileName);
  }
  const uint64_t numOpacities = opacityValues.size();
  if (!fout.write(reinterpret_cast<const char*>(&numOpacities), sizeof(uint64_t))) {
    throw std::runtime_error("Failed to write numOpacities header to " + fileName);
  }
  if (!fout.write(reinterpret_cast<const char*>(&dataValueMin), sizeof(double))) {
    throw std::runtime_error("Failed to write dataValueMin header to " + fileName);
  }
  if (!fout.write(reinterpret_cast<const char*>(&dataValueMax), sizeof(double))) {
    throw std::runtime_error("Failed to write dataValueMax header to " + fileName);
  }
  if (!fout.write(reinterpret_cast<const char*>(&opacityScaling), sizeof(float))) {
    throw std::runtime_error("Failed to write opacityScaling header to " + fileName);
  }
  if (!fout.write(reinterpret_cast<const char*>(rgbValues.data()), numColors * 3 * sizeof(float))) {
    throw std::runtime_error("Failed to write color values to " + fileName);
  }
  if (!fout.write(reinterpret_cast<const char*>(opacityValues.data()), numOpacities * 2 * sizeof(float))) {
    throw std::runtime_error("Failed to write opacity values to " + fileName);
  }
}

