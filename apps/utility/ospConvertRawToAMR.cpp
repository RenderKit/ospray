// Copyright 2009-2019 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#include "rawToAMR.h"

static const std::string description = R"description(
Creates an adaptive mesh refinement (AMR) representation of the provided
structured volume data.

The input structured volume data is used as the highest refinement level
(i.e. the finest resolution data). From this level, data that does not meet
the user-provided variance threshold is discarded. The average of the
discarded block is used in the next level down.

This simulates the effect that an adaptive simulation would have created
regions of higher refinement in the highly varying portions of the volume.
)description";

static const std::string usage =
    R"usage([-h | --help] input_volume variable_type x_dim y_dim z_dim num_levels
    block_size refinement_factor threshold output_basename
)usage";

static const std::string help = R"help(
input_volume        Structured volume in binary brick-of-data format
                    (string filepath)
variable_type       Type of structured volume, must be exactly one of:
                        float
                    (string)
x_dim               X dimensions of the structured volume grid (int)
y_dim               Y dimensions of the structured volume grid (int)
z_dim               Z dimensions of the structured volume grid (int)
num_levels          Number of refinement levels to simulate (int)
block_size          Size of blocks at each level in terms of cells. Blocks
                    are defined as cubes with this number of cells per edge.
                    Note that refinement levels change the width of the
                    cells, NOT the width of the blocks. Blocks will always
                    be this provided size in cell extents, but the width of
                    the cells will be smaller/larger depending on level.
                    For example, a block of 4x4x4 cells at the highest
                    refinement level will be converted into a single cell
                    at the next lower level, which would be part of that
                    level's block of 4x4x4 cells. (int)
refinement_factor   How much larger/smaller, in terms of cell extents, each
                    level's grid will be. Note that this is independent of
                    block_size above! For example, if the input data is a
                    512^3 grid, and block_size is 4^3, the highest
                    refinement level will have a 128^3 block grid. If this
                    value is 2, the next level will have a 64^3 block grid,
                    but will still be comprised of 4^3 blocks. That is, the
                    number of cells decreases from 512^3 to 256^3, and the
                    cell width increases to accommodate. (int)
threshold           Variance threshold used to determine whether a block
                    belongs to a higher refinement level. If the variance
                    within a block is above this threshold, it remains at
                    the current level. Otherwise, if the variance is low,
                    this block would have not have been selected for mesh
                    refinement by a numerical simulation, so this block is
                    discarded at this level. (variable_type, converted to
                    float)
output_basename     Basename for the output files. This application creates
                    three files with different extensions, with this basename
                    in common. (string)
)help";

using namespace ospcommon::math;

static std::string inFileName;
static std::string format;
static vec3i inDims;
static int numLevels;
static int blockSize;
static int refinementLevel;
static float threshold;
static std::string outFileBase;

bool parseArguments(int argc, char **argv)
{
  if (argc != 11) {
    if (argc > 1
        && (std::string(argv[1]) == "-h" || std::string(argv[1]) == "--help")) {
      std::cerr << description << std::endl;
      std::cerr << "Usage: " << argv[0] << " " << usage << std::endl;
      std::cerr << help << std::endl;
    } else {
      std::cerr << argc - 1 << " argument"
                << ((argc == 1 || argc > 2) ? "s " : " ");
      std::cerr << "provided, but 10 are needed" << std::endl;
      std::cerr << "Usage: " << argv[0] << " " << usage << std::endl;
    }
    return false;
  }

  inFileName = argv[1];
  format = argv[2];
  inDims.x = atoi(argv[3]);
  inDims.y = atoi(argv[4]);
  inDims.z = atoi(argv[5]);
  numLevels = atoi(argv[6]);
  blockSize = atoi(argv[7]);
  refinementLevel = atoi(argv[8]);
  threshold = atof(argv[9]);
  outFileBase = argv[10];

  return true;
}

int main(int argc, char **argv)
{
  bool parseSucceed = parseArguments(argc, argv);
  if (!parseSucceed) {
    return 1;
  }

  // ALOK: TODO:
  // support more than float
  std::vector<float> in;
  if (format == "float") {
    in = ospray::amr::mmapRAW<float>(inFileName, inDims);
  } else {
    throw std::runtime_error("unsupported input voxel format");
  }

  std::vector<box3i> blockBounds;
  std::vector<int> refinementLevels;
  std::vector<float> cellWidths;
  std::vector<std::vector<float>> brickData;

  ospray::amr::makeAMR(in,
      inDims,
      numLevels,
      blockSize,
      refinementLevel,
      threshold,
      blockBounds,
      refinementLevels,
      cellWidths,
      brickData);
  ospray::amr::outputAMR(outFileBase,
      blockBounds,
      refinementLevels,
      cellWidths,
      brickData,
      blockSize);

  return 0;
}
