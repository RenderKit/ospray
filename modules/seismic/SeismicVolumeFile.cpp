// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
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

#include <stdarg.h>
#include <vector>
#include "SeismicVolumeFile.h"

OSPVolume SeismicVolumeFile::importVolume(OSPVolume volume) {

  //! Set voxel type of the volume. Note that FreeDDS converts sample types of int, short, long, float, and double to float.
  ospSetString(volume, "voxelType", "float");

  //! Open seismic data file and populate attributes.
  exitOnCondition(openSeismicDataFile(volume) != true, "unable to open file '" + filename + "'");

  //! Import the voxel data from the file into the volume.
  exitOnCondition(importVoxelData(volume) != true, "error importing voxel data.");

  //! Return the volume.
  return volume;
}

bool SeismicVolumeFile::openSeismicDataFile(OSPVolume volume) {

  //! Open seismic data file, keeping trace header information.
  //! Sample types int, short, long, float, and double will be converted to float.
  inputBinTag = cddx_in2("in", filename.c_str(), "seismic");
  exitOnCondition(inputBinTag < 0, "could not open data file " + filename);

  //! Get seismic volume dimensions and volume size.
  exitOnCondition(cdds_scanf("size.axis(1)", "%d", &dimensions.x) != 1, "could not find size of dimension 1");
  exitOnCondition(cdds_scanf("size.axis(2)", "%d", &dimensions.y) != 1, "could not find size of dimension 2");
  exitOnCondition(cdds_scanf("size.axis(3)", "%d", &dimensions.z) != 1, "could not find size of dimension 3");

  if(verbose) std::cout << toString() << " volume dimensions = " << dimensions.x << " " << dimensions.y << " " << dimensions.z << std::endl;

  //! Get voxel spacing (deltas).
  exitOnCondition(cdds_scanf("delta.axis(1)", "%f", &deltas.x) != 1, "could not find delta of dimension 1");
  exitOnCondition(cdds_scanf("delta.axis(2)", "%f", &deltas.y) != 1, "could not find delta of dimension 2");
  exitOnCondition(cdds_scanf("delta.axis(3)", "%f", &deltas.z) != 1, "could not find delta of dimension 3");

  if(verbose) std::cout << toString() << " volume deltas = " << deltas.x << " " << deltas.y << " " << deltas.z << std::endl;

  //! Get trace header information.
  FIELD_TAG traceSamplesTag = cdds_member(inputBinTag, 0, "Samples");
  traceHeaderSize = cdds_index(inputBinTag, traceSamplesTag, DDS_FLOAT);
  exitOnCondition(traceSamplesTag == -1 || traceHeaderSize == -1, "could not get trace header information");

  if(verbose) std::cout << toString() << " trace header size = " << traceHeaderSize << std::endl;

  //! Check that volume dimensions are valid. If not, perform a scan of all trace headers to find actual dimensions.
  if(reduce_min(dimensions) <= 1) {
    if(verbose) std::cout << toString() << " improper volume dimensions found, scanning volume file for proper values." << std::endl;

    exitOnCondition(scanSeismicDataFileForDimensions(volume) != true, "failed scan of seismic data file");
  }

  //! Check if a subvolume of the volume has been specified.
  //! Subvolume parameters: subvolumeOffsets, subvolumeDimensions, subvolumeSteps.
  //! The subvolume defaults to full dimensions (allowing for just subsampling, for example.)
  subvolumeOffsets = osp::vec3i(0);
  ospGetVec3i(volume, "subvolumeOffsets", &subvolumeOffsets);
  exitOnCondition(reduce_min(subvolumeOffsets) < 0 || reduce_max(subvolumeOffsets - dimensions) >= 0, "invalid subvolume offsets");

  subvolumeDimensions = dimensions - subvolumeOffsets;
  ospGetVec3i(volume, "subvolumeDimensions", &subvolumeDimensions);
  exitOnCondition(reduce_min(subvolumeDimensions) < 1 || reduce_max(subvolumeDimensions - (dimensions - subvolumeOffsets)) > 0, "invalid subvolume dimension(s) specified");

  subvolumeSteps = osp::vec3i(1);
  ospGetVec3i(volume, "subvolumeSteps", &subvolumeSteps);
  exitOnCondition(reduce_min(subvolumeSteps) < 1 || reduce_max(subvolumeSteps - (dimensions - subvolumeOffsets)) >= 0, "invalid subvolume steps");

  if(reduce_max(subvolumeOffsets) > 0 || subvolumeDimensions != dimensions || reduce_max(subvolumeSteps) > 1)
    useSubvolume = true;
  else
    useSubvolume = false;

  //! The dimensions of the volume to be imported.
  volumeDimensions = osp::vec3i(subvolumeDimensions.x / subvolumeSteps.x + (subvolumeDimensions.x % subvolumeSteps.x != 0),
                                subvolumeDimensions.y / subvolumeSteps.y + (subvolumeDimensions.y % subvolumeSteps.y != 0),
                                subvolumeDimensions.z / subvolumeSteps.z + (subvolumeDimensions.z % subvolumeSteps.z != 0));

  //! Range check.
  exitOnCondition(reduce_min(volumeDimensions) <= 0, "invalid volume dimensions");

  if(verbose) std::cout << toString() << " subvolume dimensions = " << volumeDimensions.x << " " << volumeDimensions.y << " " << volumeDimensions.z << std::endl;

  //! Set dimensions of the volume.
  ospSetVec3i(volume, "dimensions", volumeDimensions);

  //! The grid spacing of the volume to be imported.
  gridSpacing = deltas;

  if(useSubvolume) {
    gridSpacing.x *= float(subvolumeSteps.x);
    gridSpacing.y *= float(subvolumeSteps.y);
    gridSpacing.z *= float(subvolumeSteps.z);
  }

  //! Set voxel spacing of the volume if not already provided.
  if(!ospGetVec3f(volume, "gridSpacing", &gridSpacing))
    ospSetVec3f(volume, "gridSpacing", gridSpacing);

  return true;
}

bool SeismicVolumeFile::scanSeismicDataFileForDimensions(OSPVolume volume) {

  //! Trace fields used for coordinates in each dimension; defaults can be overridden.
  std::string traceCoordinate2Field = "CdpNum";
  std::string traceCoordinate3Field = "FieldRecNum";

  char * temp;
  if(ospGetString(volume, "traceCoordinate2Field", &temp))
    traceCoordinate2Field = std::string(temp);

  if(ospGetString(volume, "traceCoordinate3Field", &temp))
    traceCoordinate3Field = std::string(temp);

  if(verbose) std::cout << toString() << " trace coordinate fields = " << traceCoordinate2Field << " " << traceCoordinate3Field << std::endl;

  //! Get location of these fields in trace header.
  FIELD_TAG traceCoordinate2Tag = cdds_member(inputBinTag, 0, traceCoordinate2Field.c_str());
  FIELD_TAG traceCoordinate3Tag = cdds_member(inputBinTag, 0, traceCoordinate3Field.c_str());
  exitOnCondition(traceCoordinate2Tag == -1 || traceCoordinate3Tag == -1, "could not get trace header coordinate information");

  //! Allocate trace array; the trace length is given by the trace header size and the first dimension.
  //! Note that FreeDDS converts all trace data to floats.
  float * traceBuffer = (float *)malloc((traceHeaderSize + dimensions.x) * sizeof(float));
  exitOnCondition(traceBuffer == NULL, "failed to allocate trace buffer");

  //! Range in second and third dimensions
  int minDimension2, maxDimension2;
  int minDimension3, maxDimension3;

  //! Iterate through all traces.
  size_t traceCount = 0;

  while(cddx_read(inputBinTag, traceBuffer, 1) == 1) {

    //! Get logical coordinates.
    int coordinate2, coordinate3;
    cdds_geti(inputBinTag, traceCoordinate2Tag, traceBuffer, &coordinate2, 1);
    cdds_geti(inputBinTag, traceCoordinate3Tag, traceBuffer, &coordinate3, 1);

    //! Update dimension bounds.
    if(traceCount == 0) {
      minDimension2 = maxDimension2 = coordinate2;
      minDimension3 = maxDimension3 = coordinate3;
    }
    else {
      minDimension2 = std::min(minDimension2, coordinate2);
      maxDimension2 = std::max(maxDimension2, coordinate2);

      minDimension3 = std::min(minDimension3, coordinate3);
      maxDimension3 = std::max(maxDimension3, coordinate3);
    }

    traceCount++;
  }

  //! Update dimensions.
  dimensions.y = maxDimension2 - minDimension2 + 1;
  dimensions.z = maxDimension3 - minDimension3 + 1;
  exitOnCondition(dimensions.y <= 1 || dimensions.z <= 1, "could not determine volume dimensions");

  if(verbose) std::cout << toString() << " updated volume dimensions = " << dimensions.x << " x " << dimensions.y << " (" << minDimension2 << " --> " << maxDimension2 << ") x " << dimensions.z << " (" << minDimension3 << " --> " << maxDimension3 << ")"  << std::endl;

  //! Clean up.
  free(traceBuffer);

  //! Seek back to the beginning of the file.
  cdds_lseek(inputBinTag, 0, 0, SEEK_SET);

  return true;
}

bool SeismicVolumeFile::importVoxelData(OSPVolume volume) {

  //! Trace fields used for coordinates in each dimension; defaults can be overridden.
  std::string traceCoordinate2Field = "CdpNum";
  std::string traceCoordinate3Field = "FieldRecNum";

  char * temp;
  if(ospGetString(volume, "traceCoordinate2Field", &temp))
    traceCoordinate2Field = std::string(temp);

  if(ospGetString(volume, "traceCoordinate3Field", &temp))
    traceCoordinate3Field = std::string(temp);

  if(verbose) std::cout << toString() << " trace coordinate fields = " << traceCoordinate2Field << " " << traceCoordinate3Field << std::endl;

  //! Get location of these fields in trace header. If we can't find them, ignore trace header information.
  bool ignoreTraceHeaders = false;

  FIELD_TAG traceCoordinate2Tag = cdds_member(inputBinTag, 0, traceCoordinate2Field.c_str());
  FIELD_TAG traceCoordinate3Tag = cdds_member(inputBinTag, 0, traceCoordinate3Field.c_str());

  if(traceHeaderSize > 0 && (traceCoordinate2Tag == -1 || traceCoordinate3Tag == -1)) {
    ignoreTraceHeaders = true;
    if(verbose)
      std::cout << toString() << " could not get trace header coordinate information, ignoring trace header information." << std::endl;
  }

  //! Allocate trace array; the trace length is given by the trace header size and the first dimension.
  //! Note that FreeDDS converts all trace data to floats.
  float * traceBuffer = (float *)malloc((traceHeaderSize + dimensions.x) * sizeof(float));
  exitOnCondition(traceBuffer == NULL, "failed to allocate trace buffer");

  //! Get origin in dimension 2 and 3 from first trace if we have a trace header; otherwise assume it is (0, 0).
  int origin2 = 0;
  int origin3 = 0;

  if(!ignoreTraceHeaders && traceHeaderSize > 0) {

    exitOnCondition(cddx_read(inputBinTag, traceBuffer, 1) != 1, "could not read first trace");
    cdds_geti(inputBinTag, traceCoordinate2Tag, traceBuffer, &origin2, 1);
    cdds_geti(inputBinTag, traceCoordinate3Tag, traceBuffer, &origin3, 1);

    if(verbose) std::cout << toString() << " trace coordinate origin: (" << origin2 << ", " << origin3 << ")" << std::endl;

    //! Seek back to the beginning of the file.
    cdds_lseek(inputBinTag, 0, 0, SEEK_SET);
  }

  //! Copy voxel data into the volume a trace at a time.
  size_t traceCount = 0;

  //! If no subvolume is specified and we have trace header information, read all the traces. Missing traces are allowed.
  //! Otherwise, read only the specified subvolume, skipping over traces as needed. Missing traces NOT allowed.
  if(!useSubvolume && !ignoreTraceHeaders && traceHeaderSize > 0) {

    while(cddx_read(inputBinTag, traceBuffer, 1) == 1) {

      //! Get logical coordinates.
      int coordinate2, coordinate3;
      cdds_geti(inputBinTag, traceCoordinate2Tag, traceBuffer, &coordinate2, 1);
      cdds_geti(inputBinTag, traceCoordinate3Tag, traceBuffer, &coordinate3, 1);

      //! Validate coordinates.
      exitOnCondition(coordinate2-origin2 < 0 || coordinate2-origin2 >= dimensions.y || coordinate3-origin3 < 0 || coordinate3-origin3 >= dimensions.z, "invalid trace coordinates found");

      //! Copy trace into the volume.
      ospSetRegion(volume, &traceBuffer[traceHeaderSize], osp::vec3i(0, coordinate2-origin2, coordinate3-origin3), osp::vec3i(volumeDimensions.x, 1, 1));

      traceCount++;
    }
  }
  else {

    //! Allocate trace buffer for subvolume data.
    float * traceBufferSubvolume = (float *)malloc(volumeDimensions.x * sizeof(float));

    //! Iterate through the grid of traces of the subvolume, seeking as necessary.
    for(long i3=subvolumeOffsets.z; i3<subvolumeOffsets.z+subvolumeDimensions.z; i3+=subvolumeSteps.z) {

      //! Seek to offset if necessary.
      if(i3 == subvolumeOffsets.z && subvolumeOffsets.z != 0)
        cdds_lseek(inputBinTag, 0, subvolumeOffsets.z * dimensions.y, SEEK_CUR);

      for(long i2=subvolumeOffsets.y; i2<subvolumeOffsets.y+subvolumeDimensions.y; i2+=subvolumeSteps.y) {

        //! Seek to offset if necessary.
        if(i2 == subvolumeOffsets.y && subvolumeOffsets.y != 0)
          cdds_lseek(inputBinTag, 0, subvolumeOffsets.y, SEEK_CUR);

        //! Read trace.
        exitOnCondition(cddx_read(inputBinTag, traceBuffer, 1) != 1, "unable to read trace");

        //! Get logical coordinates.
        int coordinate2 = i2;
        int coordinate3 = i3;

        if(!ignoreTraceHeaders && traceHeaderSize > 0) {
          cdds_geti(inputBinTag, traceCoordinate2Tag, traceBuffer, &coordinate2, 1);
          cdds_geti(inputBinTag, traceCoordinate3Tag, traceBuffer, &coordinate3, 1);
        }

        //! Some trace headers will have improper coordinate information; correct coordinate3 if necessary.
        if(coordinate2-origin2 >= dimensions.y) {
          coordinate3 = origin3 + (coordinate2-origin2) / dimensions.y;
          coordinate2 -= (coordinate3-origin3) * dimensions.y;
        }

        //! Validate we have the expected coordinates.
        if((coordinate2-origin2 != i2 || coordinate3-origin3 != i3) && verbose)
          std::cout << toString() << " found incorrect coordinate(s): (" << coordinate2-origin2 << ", " << coordinate3-origin3 << ") != (" << i2 << ", " << i3 << ")" << std::endl;

        exitOnCondition(coordinate2-origin2 != i2, "found invalid coordinate (dimension 2)");
        exitOnCondition(coordinate3-origin3 != i3, "found invalid coordinate (dimension 3)");

        //! Resample trace for the subvolume.
        for(long i1=subvolumeOffsets.x; i1<subvolumeOffsets.x+subvolumeDimensions.x; i1+=subvolumeSteps.x)
          traceBufferSubvolume[(i1 - subvolumeOffsets.x) / subvolumeSteps.x] = traceBuffer[traceHeaderSize + i1];

        //! Copy subsampled trace into the volume.
        ospSetRegion(volume, &traceBufferSubvolume[0], osp::vec3i(0, (i2 - subvolumeOffsets.y) / subvolumeSteps.y, (i3 - subvolumeOffsets.z) / subvolumeSteps.z), osp::vec3i(volumeDimensions.x, 1, 1));

        traceCount++;

        //! Skip traces (second dimension)
        if(i2 >= subvolumeOffsets.y + subvolumeDimensions.y - subvolumeSteps.y)
          cdds_lseek(inputBinTag, 0, dimensions.y - i2 - 1, SEEK_CUR);
        else
          cdds_lseek(inputBinTag, 0, subvolumeSteps.y - 1, SEEK_CUR);
      }

      //! Skip traces (third dimension)
      if(i3 >= subvolumeOffsets.z + subvolumeDimensions.z - subvolumeSteps.z)
        cdds_lseek(inputBinTag, 0, (dimensions.z - i3 - 1) * dimensions.y, SEEK_CUR);
      else
        cdds_lseek(inputBinTag, 0, (subvolumeSteps.z - 1) * dimensions.y, SEEK_CUR);
    }

    //! Clean up.
    free(traceBufferSubvolume);
  }

  //! Print statistics.
  if(verbose)
    std::cout << toString() << " read " << traceCount << " traces. " << volumeDimensions.y*volumeDimensions.z - int(traceCount) << " missing trace(s)." << std::endl;

  //! Clean up.
  free(traceBuffer);

  //! Close the seismic data file.
  cdds_close(inputBinTag);

  return true;
}

//! Module initialization function
extern "C" void ospray_init_module_seismic() {

  std::cout << "loaded module 'seismic'." << std::endl;
}
