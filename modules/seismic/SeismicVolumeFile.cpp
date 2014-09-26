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
#include "SeismicVolumeFile.h"

namespace ospray {

    SeismicVolumeFile::SeismicVolumeFile(const std::string &filename) : filename(realpath(filename.c_str(), NULL)) {

        //! We use the file extension to determine if this is a header or volume file.
        std::string extension = filename.substr(filename.find_last_of(".") + 1);

        if(extension == "smch" || extension == "seismich") {

            //! Read attributes from XML volume header file.
            readHeader();

            //! Verify a data file has been specified.
            std::string dataFilename = header["dataFilename"];  exitOnCondition(dataFilename.empty(), "no dataFilename found in volume header");

            //! Open seismic data file.
            exitOnCondition(openSeismicDataFile(dataFilename) != true, "could not open seismic data file");
        }
        else {

            //! No header provided; the given filename is the volume filename.
            //! Open seismic data file.
            exitOnCondition(openSeismicDataFile(filename) != true, "could not open seismic data file");
        }
    }

    vec3i SeismicVolumeFile::getVolumeDimensions() {

        vec3i volumeDimensions(dimensions[0], dimensions[1], dimensions[2]);

        //! Range check.
        exitOnCondition(reduce_min(volumeDimensions) <= 0, "invalid volume dimensions");  return(volumeDimensions);
    }

    void SeismicVolumeFile::getVoxelData(void **buffer) {

        //! This method not yet implemented, waiting on final file loader API.
        NOTIMPLEMENTED;
    }

    void SeismicVolumeFile::getVoxelData(StructuredVolume *volume) {

        //! Volume statistics.
        float dataMin = FLT_MAX;
        float dataMax = -FLT_MAX;

        //! Properties of the volume.
        vec3i volumeDimensions = getVolumeDimensions();

        //! Default trace fields used for coordinates in each dimension.
        std::string traceCoordinate2Field("CdpNum");
        std::string traceCoordinate3Field("FieldRecNum");

        //! The volume header can override these defaults.
        if(header.count("traceCoordinate2Field") == 1) traceCoordinate2Field = header["traceCoordinate2Field"];
        if(header.count("traceCoordinate3Field") == 1) traceCoordinate3Field = header["traceCoordinate3Field"];

        if(ospray::logLevel >= 1) std::cout << "OSPRay::SeismicVolumeFile trace coordinate fields = " << traceCoordinate2Field << " " << traceCoordinate3Field << std::endl;

        //! Get location of these fields in trace header.
        FIELD_TAG traceCoordinate2Tag = cdds_member(inputBinTag, 0, traceCoordinate2Field.c_str());
        FIELD_TAG traceCoordinate3Tag = cdds_member(inputBinTag, 0, traceCoordinate3Field.c_str());
        exitOnCondition(traceCoordinate2Tag == -1 || traceCoordinate3Tag == -1, "could not get trace header coordinate information");

        //! Allocate trace array; the trace length is given by the trace header size and the first dimension.
        //! Note that FreeDDS converts all trace data to floats.
        float * traceBuffer = (float *)malloc((traceHeaderSize + dimensions[0]) * sizeof(float));

        //! Copy voxel data into the volume a trace at a time.
        //! There may not exist traces for all (i2, i3) grid locations.
        size_t traceCount = 0;

        while(cddx_read(inputBinTag, traceBuffer, 1) == 1) {

            //! Get logical coordinates.
            int coordinate2, coordinate3;
            cdds_geti(inputBinTag, traceCoordinate2Tag, traceBuffer, &coordinate2, 1);
            cdds_geti(inputBinTag, traceCoordinate3Tag, traceBuffer, &coordinate3, 1);

            //! Copy trace into the volume.
            volume->setRegion(&traceBuffer[traceHeaderSize], vec3i(0, coordinate2-origin[1], coordinate3-origin[2]), vec3i(volumeDimensions.x, 1, 1));

            //! Accumulate statistics.
            for (int i1=0; i1<volumeDimensions.x; i1++) {

                dataMin = std::min(float(traceBuffer[traceHeaderSize + i1]), dataMin);
                dataMax = std::max(float(traceBuffer[traceHeaderSize + i1]), dataMax);
            }

            traceCount++;
        }

        //! Print statistics.
        if(ospray::logLevel >= 1) {
            
            std::cout << "OSPRay::SeismicVolumeFile read " << traceCount << " traces. " << volumeDimensions.y*volumeDimensions.z - traceCount << " missing trace(s)." << std::endl;
            std::cout << "OSPRay::SeismicVolumeFile volume data range = " << dataMin << " " << dataMax << std::endl;
        }

        //! Clean up.
        free(traceBuffer);

        //! Close the seismic data file.
        cdds_close(inputBinTag);
    }

    vec3f SeismicVolumeFile::getVoxelSpacing() {

        vec3f voxelSpacing(deltas[0], deltas[1], deltas[2]);

        //! Range check.
        exitOnCondition(voxelSpacing.x < 0.0f || voxelSpacing.y < 0.0f || voxelSpacing.z < 0.0f, "invalid voxel spacing");  return(voxelSpacing);
    }

    std::string SeismicVolumeFile::getVoxelType() {

        //! Note: FreeDDS converts sample types int, short, long, float, and double to float.
        return std::string("float");
    }

    void SeismicVolumeFile::readHeader() {

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

    bool SeismicVolumeFile::openSeismicDataFile(std::string dataFilename) {

        //! Open seismic data file, keeping trace header information.
        //! Sample types int, short, long, float, and double will be converted to float.
        inputBinTag = cddx_in2("in", dataFilename.c_str(), "seismic");
        exitOnCondition(inputBinTag < 0, "could not open data file " + dataFilename);

        //! Get volume origin.
        exitOnCondition(cdds_scanf("origin.axis(1)", "%f", &origin[0]) != 1, "could not find origin of dimension 1");
        exitOnCondition(cdds_scanf("origin.axis(2)", "%f", &origin[1]) != 1, "could not find origin of dimension 2");
        exitOnCondition(cdds_scanf("origin.axis(3)", "%f", &origin[2]) != 1, "could not find origin of dimension 3");

        if(ospray::logLevel >= 1) std::cout << "OSPRay::SeismicVolumeFile volume origin = " << origin[0] << " " << origin[1] << " " << origin[2] << std::endl;

        //! Get voxel spacing (deltas).
        exitOnCondition(cdds_scanf("delta.axis(1)", "%f", &deltas[0]) != 1, "could not find delta of dimension 1");
        exitOnCondition(cdds_scanf("delta.axis(2)", "%f", &deltas[1]) != 1, "could not find delta of dimension 2");
        exitOnCondition(cdds_scanf("delta.axis(3)", "%f", &deltas[2]) != 1, "could not find delta of dimension 3");

        if(ospray::logLevel >= 1) std::cout << "OSPRay::SeismicVolumeFile volume deltas = " << deltas[0] << " " << deltas[1] << " " << deltas[2] << std::endl;

        //! Get seismic volume dimensions and volume size.
        exitOnCondition(cdds_scanf("size.axis(1)", "%d", &dimensions[0]) != 1, "could not find size of dimension 1");
        exitOnCondition(cdds_scanf("size.axis(2)", "%d", &dimensions[1]) != 1, "could not find size of dimension 2");
        exitOnCondition(cdds_scanf("size.axis(3)", "%d", &dimensions[2]) != 1, "could not find size of dimension 3");

        if(ospray::logLevel >= 1) std::cout << "OSPRay::SeismicVolumeFile volume dimensions = " << dimensions[0] << " " << dimensions[1] << " " << dimensions[2] << std::endl;

        int volumeSize = cdds_axis_prod(1);  exitOnCondition(volumeSize <= 0, "could not find volume size");

        //! Verify the data is 3D (and not of higher dimension).
        exitOnCondition(dimensions[0]*dimensions[1]*dimensions[2] != volumeSize, "volume must be three dimensions");

        //! Get trace header information.
        FIELD_TAG traceSamplesTag = cdds_member(inputBinTag, 0, "Samples");
        traceHeaderSize = cdds_index(inputBinTag, traceSamplesTag, DDS_FLOAT);
        exitOnCondition(traceSamplesTag == -1 || traceHeaderSize == -1, "could not get trace header information");

        if(ospray::logLevel >= 1) std::cout << "OSPRay::SeismicVolumeFile trace header size = " << traceHeaderSize << std::endl;

        //! Check that volume dimensions are valid. If not, perform a scan of all trace headers to find actual dimensions.
        if(dimensions[0] <= 1 || dimensions[1] <= 1 || dimensions[2] <= 1) {

            if(ospray::logLevel >= 1) std::cout << "OSPRay::SeismicVolumeFile improper volume dimensions found, scanning volume file for proper values." << std::endl;

            exitOnCondition(scanSeismicDataFileForDimensions() != true, "failed scan of seismic data file");
        }

        return true;
    }

    bool SeismicVolumeFile::scanSeismicDataFileForDimensions() {

        //! Default trace fields used for coordinates in each dimension.
        std::string traceCoordinate2Field("CdpNum");
        std::string traceCoordinate3Field("FieldRecNum");

        //! The volume header can override these defaults.
        if(header.count("traceCoordinate2Field") == 1) traceCoordinate2Field = header["traceCoordinate2Field"];
        if(header.count("traceCoordinate3Field") == 1) traceCoordinate3Field = header["traceCoordinate3Field"];

        if(ospray::logLevel >= 1) std::cout << "OSPRay::SeismicVolumeFile trace coordinate fields = " << traceCoordinate2Field << " " << traceCoordinate3Field << std::endl;

        //! Get location of these fields in trace header.
        FIELD_TAG traceCoordinate2Tag = cdds_member(inputBinTag, 0, traceCoordinate2Field.c_str());
        FIELD_TAG traceCoordinate3Tag = cdds_member(inputBinTag, 0, traceCoordinate3Field.c_str());
        exitOnCondition(traceCoordinate2Tag == -1 || traceCoordinate3Tag == -1, "could not get trace header coordinate information");

        //! Allocate trace array; the trace length is given by the trace header size and the first dimension.
        //! Note that FreeDDS converts all trace data to floats.
        float * traceBuffer = (float *)malloc((traceHeaderSize + dimensions[0]) * sizeof(float));

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
        dimensions[1] = maxDimension2 - minDimension2 + 1;
        dimensions[2] = maxDimension3 - minDimension3 + 1;

        //! Update origin.
        origin[1] = minDimension2;
        origin[2] = minDimension3;

        if(ospray::logLevel >= 1) std::cout << "OSPRay::SeismicVolumeFile updated volume dimensions = " << dimensions[0] << " x " << dimensions[1] << " (" << minDimension2 << " --> " << maxDimension2 << ") x " << dimensions[2] << " (" << minDimension3 << " --> " << maxDimension3 << ")"  << std::endl;

        //! Clean up.
        free(traceBuffer);

        //! Seek back to the beginning of the file.
        cdds_lseek(inputBinTag, 0, 0, SEEK_SET);

        return true;
    }

    //! Module initialization function
    extern "C" void ospray_init_module_seismic() {

        std::cout << "loaded module 'seismic'." << std::endl;
    }

} // namespace ospray
