//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include <stdarg.h>
#include <vector>
#include "apps/common/fileio/TinyXML2.h"
#include "apps/common/fileio/RawVolumeFile.h"

namespace ospray {

    void RawVolumeFile::getKeyValueStrings() {

        //! The XML document container.
        tinyxml2::XMLDocument xml(true, tinyxml2::COLLAPSE_WHITESPACE);

        //! Read the XML volume header file.
        exitOnCondition(xml.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS, "unable to read volume header file '" + filename + "'");

        //! The XML node containing the volume specification.
        tinyxml2::XMLNode *root = xml.FirstChildElement("volume");  exitOnCondition(root == NULL, "no volume XML tag found in header file '" + filename + "'");

        //! Iterate over elements comprising the XML volume specification node.
        for (tinyxml2::XMLElement *element = root->FirstChildElement() ; element != NULL ; element = element->NextSiblingElement()) {

            //! Store the key value strings comprising this XML element.
            header[element->Name()] = element->GetText();

            //! Elements may contain zero or more attributes.
            const tinyxml2::XMLAttribute *attribute = element->FirstAttribute();

            //! Iterate over attributes in this XML element.
            while (attribute != NULL) header[std::string(element->Name()) + " " + attribute->Name()] = attribute->Value(), attribute = attribute->Next();

        }

    }

    vec3i RawVolumeFile::getVolumeDimensions() {

        //! Verify dimensions have been specified.
        exitOnCondition(header.find("dimensions") == header.end(), "no dimensions found in volume header");

        //! Scan the volume dimensions from a string.
        vec3i volumeDimensions(0);  sscanf(header["dimensions"].c_str(), "%d %d %d", &volumeDimensions.x, &volumeDimensions.y, &volumeDimensions.z);

        //! Range check.
        exitOnCondition(reduce_min(volumeDimensions) <= 0, "invalid volume dimensions");  return(volumeDimensions);

    }

    void RawVolumeFile::getVoxelData(void **buffer) {

        //! Verify a file name has been given for the actual volume data.
        exitOnCondition(header.find("dataFile") == header.end(), "no voxel data file name found in volume header");

        //! Attempt to open the volume data file.
        FILE *file = openVoxelDataFile();  exitOnCondition(!file, "unable to locate volume file '" + header["dataFile"] + "'");

        //! Offset into the volume data file.
        size_t offset = 0;  sscanf(header["dataFile offset"].c_str(), "%zu", &offset);  fseek(file, offset, SEEK_SET);

        //! Properties of the volume.
        size_t voxelCount = getVolumeDimensions().x * getVolumeDimensions().y * getVolumeDimensions().z;  size_t voxelSize = getVoxelSizeInBytes();

        //! If the pointer is not NULL it is assumed the caller allocated memory of the correct size.
        if (*buffer == NULL) *buffer = malloc(voxelCount * voxelSize);

        //! Copy voxel data into the volume.
        size_t voxelsRead = fread(*buffer, voxelSize, voxelCount, file);

        //! The end of the file may have been reached unexpectedly.
        exitOnCondition(voxelsRead != voxelCount, "end of volume data file reached before read completed");

    }

    void RawVolumeFile::getVoxelData(StructuredVolume *volume) {

        //! Verify a file name has been given for the actual volume data.
        exitOnCondition(header.find("dataFile") == header.end(), "no voxel data file name found in volume header");

        //! Attempt to open the volume data file.
        FILE *file = openVoxelDataFile();  exitOnCondition(!file, "unable to locate volume file '" + header["dataFile"] + "'");

        //! Offset into the volume data file.
        size_t offset = 0;  sscanf(header["dataFile offset"].c_str(), "%zu", &offset);  fseek(file, offset, SEEK_SET);

        //! Properties of the volume.
        vec3i volumeDimensions = getVolumeDimensions();  size_t voxelSize = getVoxelSizeInBytes();

        //! Allocate memory for a single row of voxel data.
        void *buffer = malloc(volumeDimensions.x * voxelSize);

        //! Copy voxel data into the volume a row at a time.
        for (size_t k=0 ; k < volumeDimensions.z ; k++) for (size_t j=0 ; j < volumeDimensions.y ; j++) {

            //! Read a row of voxels from the volume file.
            size_t voxelsRead = fread(buffer, voxelSize, volumeDimensions.x, file);

            //! The end of the file may have been reached unexpectedly.
            exitOnCondition(voxelsRead != volumeDimensions.x, "end of volume data file reached before read completed");

            //! Copy voxel data into the volume.
            volume->setRegion(buffer, vec3i(0, j, k), vec3i(volumeDimensions.x, 1, 1));

        //! Clean up.
        } free(buffer);  fclose(file);

    }

    size_t RawVolumeFile::getVoxelSizeInBytes() {

        //! Voxel type string.
        std::string voxelType = getVoxelType();

        //! Separate out the base type and vector width.
        char kind[voxelType.size()];  unsigned int width = 1;  sscanf(voxelType.c_str(), "%[^0-9]%u", kind, &width);

        //! Range check.
        if (width != 1 && width != 2 && width != 3 && width != 4) return(0);

        //! Unsigned character scalar and vector types.
        if (!strcmp(kind, "uchar")) return(sizeof(unsigned char) * width);

        //! Floating point scalar and vector types.
        if (!strcmp(kind, "float")) return(sizeof(float) * width);

        //! Unsigned integer scalar and vector types.
        if (!strcmp(kind, "uint")) return(sizeof(unsigned int) * width);

        //! Signed integer scalar and vector types.
        if (!strcmp(kind, "int")) return(sizeof(int) * width);

        //! Unknown type.
        return(0);

    }

    vec3f RawVolumeFile::getVoxelSpacing() {

        //! Voxel spacing defaults to 1.0.
        vec3f voxelSpacing(1.0f);  sscanf(header["voxelSpacing"].c_str(), "%f %f %f", &voxelSpacing.x, &voxelSpacing.y, &voxelSpacing.z);

        //! Range check.
        exitOnCondition(voxelSpacing.x < 0.0f || voxelSpacing.y < 0.0f || voxelSpacing.z < 0.0f, "invalid voxel spacing");  return(voxelSpacing);

    }

    std::string RawVolumeFile::getVoxelType() {

        //! Verify a voxel type has been specified.
        std::string voxelType = header["voxelType"];  exitOnCondition(voxelType.empty(), "no voxel type found in volume header");  return(voxelType);

    }

    FILE *RawVolumeFile::openVoxelDataFile() {

        //! Look for the volume data file at the path given in the volume header file.
        FILE *file = fopen(header["dataFile"].c_str(), "rb");  if (file != NULL) return(file);

        //! Extract the volume header file path.
        size_t length = filename.find_last_of("/");  std::string path = (length < std::string::npos) ? filename.substr(0, length) : "";

        //! Look for the volume data file in the directory containing the volume header file.
        file = fopen((path + "/" + header["dataFile"]).c_str(), "rb");  return(file);

    }

} // namespace ospray

