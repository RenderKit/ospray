//
//                 INTEL CORPORATION PROPRIETARY INFORMATION
//
//    This software is supplied under the terms of a license agreement or
//    nondisclosure agreement with Intel Corporation and may not be copied
//    or disclosed except in accordance with the terms of that agreement.
//    Copyright (C) 2014 Intel Corporation. All Rights Reserved.
//

#include <string>
#include <string.h>
#include "apps/common/fileio/OSPObjectFile.h"

namespace ospray {

    void OSPObjectFile::importAttributeFloat(const tinyxml2::XMLNode *node, OSPObject parent) {

        //! The attribute value is encoded in a string.
        const char *text = node->ToElement()->GetText();  float value = 0.0f;  char guard[8];

        //! Get the attribute value.
        exitOnCondition(sscanf(text, "%f %7s", &value, guard) != 1, "malformed XML element '" + std::string(node->ToElement()->Name()) + "'");

        //! Set the attribute on the parent object.
        ospSet1f(parent, node->ToElement()->Name(), value);

    }

    void OSPObjectFile::importAttributeFloat2(const tinyxml2::XMLNode *node, OSPObject parent) {

        //! The attribute value is encoded in a string.
        const char *text = node->ToElement()->GetText();  vec2f value = vec2f(0.0f);  char guard[8];

        //! Get the attribute value.
        exitOnCondition(sscanf(text, "%f %f %7s", &value.x, &value.y, guard) != 2, "malformed XML element '" + std::string(node->ToElement()->Name()) + "'");

        //! Set the attribute on the parent object.
        ospSetVec2f(parent, node->ToElement()->Name(), value);

    }

    void OSPObjectFile::importAttributeFloat3(const tinyxml2::XMLNode *node, OSPObject parent) {

        //! The attribute value is encoded in a string.
        const char *text = node->ToElement()->GetText();  vec3f value = vec3f(0.0f);  char guard[8];

        //! Get the attribute value.
        exitOnCondition(sscanf(text, "%f %f %f %7s", &value.x, &value.y, &value.z, guard) != 3, "malformed XML element '" + std::string(node->ToElement()->Name()) + "'");

        //! Set the attribute on the parent object.
        ospSetVec3f(parent, node->ToElement()->Name(), value);

    }

    void OSPObjectFile::importAttributeInteger(const tinyxml2::XMLNode *node, OSPObject parent) {

        //! The attribute value is encoded in a string.
        const char *text = node->ToElement()->GetText();  int value = 0;  char guard[8];

        //! Get the attribute value.
        exitOnCondition(sscanf(text, "%d %7s", &value, guard) != 1, "malformed XML element '" + std::string(node->ToElement()->Name()) + "'");

        //! Set the attribute on the parent object.
        ospSet1i(parent, node->ToElement()->Name(), value);

    }

    void OSPObjectFile::importAttributeInteger3(const tinyxml2::XMLNode *node, OSPObject parent) {

        //! The attribute value is encoded in a string.
        const char *text = node->ToElement()->GetText();  vec3i value = vec3i(0);  char guard[8];

        //! Get the attribute value.
        exitOnCondition(sscanf(text, "%d %d %d %7s", &value.x, &value.y, &value.z, guard) != 3, "malformed XML element '" + std::string(node->ToElement()->Name()) + "'");

        //! Set the attribute on the parent object.
        ospSetVec3i(parent, node->ToElement()->Name(), value);

    }

    void OSPObjectFile::importAttributeString(const tinyxml2::XMLNode *node, OSPObject parent) {

        //! Get the attribute value.
        const char *value = node->ToElement()->GetText();

        //! Error check.
        exitOnCondition(strlen(value) == 0, "malformed XML element '" + std::string(node->ToElement()->Name()) + "'");

        //! Set the attribute on the parent object.
        ospSetString(parent, node->ToElement()->Name(), value);

    }

    OSPObjectCatalog OSPObjectFile::importLight(const tinyxml2::XMLNode *root) {

        //! Create the OSPRay object.
        OSPLight light = ospNewLight(NULL, root->ToElement()->Attribute("type"));

        //! Iterate over object attributes.
        for (const tinyxml2::XMLNode *node = root->FirstChild() ; node ; node = node->NextSibling()) importLightAttribute(node, light);

        //! Catalog the object attributes to allow introspection by the application.
        return(new ObjectCatalog(root->ToElement()->Attribute("name"), (ManagedObject *) light));

    }

    void OSPObjectFile::importLightAttribute(const tinyxml2::XMLNode *node, OSPLight light) {

        //! Light color.
        if (!strcmp(node->ToElement()->Name(), "color")) return(importAttributeFloat3(node, light));

        //! Light direction.
        if (!strcmp(node->ToElement()->Name(), "direction")) return(importAttributeFloat3(node, light));

        //! Light half angle for spot lights.
        if (!strcmp(node->ToElement()->Name(), "halfAngle")) return(importAttributeFloat(node, light));

        //! Light position.
        if (!strcmp(node->ToElement()->Name(), "position")) return(importAttributeFloat3(node, light));

        //! Light illumination distance cutoff.
        if (!strcmp(node->ToElement()->Name(), "range")) return(importAttributeFloat(node, light));

        //! Error check.
        exitOnCondition(true, "unrecognized XML element type '" + std::string(node->ToElement()->Name()) + "'");

    }

    OSPLoadDataMode OSPObjectFile::importLoadDataMode(const tinyxml2::XMLNode *node) {

        //! The flag is optionally encoded in a string.
        const char *text = node->ToElement()->Attribute("mode");

        //! Load volume data immediately, data will be copied from the host to the device.
        if (text && !strcmp(text, "loadDataImmediate")) return(OSP_LOAD_DATA_IMMEDIATE);

        //! Defer the load of volume data to the device, no data copy is necessary.
        if (text == NULL || !strcmp(text, "loadDataDeferred")) return(OSP_LOAD_DATA_DEFERRED);

        //! Error check.
        exitOnCondition(true, "malformed XML element '" + std::string(node->ToElement()->Name()) + "'");

        //! The program will never get here.
        return(OSP_LOAD_DATA_DEFERRED);

    }

    OSPObjectCatalog OSPObjectFile::importObject(const tinyxml2::XMLNode *node) {

        //! OSPRay light object.
        if (!strcmp(node->ToElement()->Name(), "light")) return(importLight(node));

        //! OSPRay triangle mesh object.
        if (!strcmp(node->ToElement()->Name(), "triangleMesh")) return(importTriangleMesh(node));

        //! OSPRay volume object.
        if (!strcmp(node->ToElement()->Name(), "volume")) return(importVolume(node));

        //! No other object types are currently supported.
        exitOnCondition(true, "unrecognized XML element type '" + std::string(node->ToElement()->Name()) + "'");  return(NULL);

    }

    OSPObjectCatalog OSPObjectFile::importObjects() {

        //! The XML document container.
        tinyxml2::XMLDocument xml(true, tinyxml2::COLLAPSE_WHITESPACE);

        //! Read the XML object file.
        exitOnCondition(xml.LoadFile(filename.c_str()) != tinyxml2::XML_SUCCESS, "unable to read object file '" + filename + "'");

        //! A catalog of the OSPRay objects and attributes contained in the file.
        ObjectCatalog *catalog = new ObjectCatalog();  std::vector<OSPObjectCatalog> entries;

        //! Iterate over the object entries, skip the XML declaration and comments.
        for (const tinyxml2::XMLNode *node = xml.FirstChild() ; node ; node = node->NextSibling()) if (node->ToElement()) entries.push_back(importObject(node));

        //! Copy the object entries into the catalog.
        catalog->entries = new OSPObjectCatalog[entries.size() + 1]();  memcpy(catalog->entries, &entries[0], entries.size() * sizeof(OSPObjectCatalog));

        //! The populated catalog.
        return(catalog);

    }

    OSPObjectCatalog OSPObjectFile::importVolume(const tinyxml2::XMLNode *root) {

        //! Create the OSPRay object.
        OSPVolume volume = ospNewVolume("bricked");

        //! Iterate over object attributes.
        for (const tinyxml2::XMLNode *node = root->FirstChild() ; node ; node = node->NextSibling()) importVolumeAttribute(node, volume);

        //! Catalog the object attributes to allow introspection by the application.
        return(new ObjectCatalog(root->ToElement()->Attribute("name"), (ManagedObject *) volume));

    }

    void OSPObjectFile::importVolumeAttribute(const tinyxml2::XMLNode *node, OSPVolume volume) {

        //! Volume size in voxels per dimension.
        if (!strcmp(node->ToElement()->Name(), "dimensions")) return(importAttributeInteger3(node, volume));

        //! File containing a volume specification and / or voxel data.
        if (!strcmp(node->ToElement()->Name(), "filename")) return(importVolumeFile(node, volume));

        //! Gamma correction coefficient and exponent.
        if (!strcmp(node->ToElement()->Name(), "gammaCorrection")) return(importAttributeFloat2(node, volume));

        //! Sampling rate for ray casting based renderers.
        if (!strcmp(node->ToElement()->Name(), "samplingRate")) return(importAttributeFloat(node, volume));

        //! Subvolume offset from origin within the full volume.
        if (!strcmp(node->ToElement()->Name(), "subvolumeOffsets")) return(importAttributeInteger3(node, volume));

        //! Subvolume dimensions within the full volume.
        if (!strcmp(node->ToElement()->Name(), "subvolumeDimensions")) return(importAttributeInteger3(node, volume));

        //! Subvolume steps in each dimension; can be used to subsample the volume.
        if (!strcmp(node->ToElement()->Name(), "subvolumeSteps")) return(importAttributeInteger3(node, volume));

        //! Voxel value range.
        if (!strcmp(node->ToElement()->Name(), "voxelRange")) return(importAttributeFloat2(node, volume));

        //! Voxel spacing in world coordinates (currently unused).
        if (!strcmp(node->ToElement()->Name(), "voxelSpacing")) return(importAttributeFloat3(node, volume));

        //! Voxel type string.
        if (!strcmp(node->ToElement()->Name(), "voxelType")) return(importAttributeString(node, volume));

        //! Error check.
        exitOnCondition(true, "unrecognized XML element type '" + std::string(node->ToElement()->Name()) + "'");

    }

    void OSPObjectFile::importVolumeFile(const tinyxml2::XMLNode *node, OSPVolume volume) {

        //! Defer load of the volume file contents until object commit.
        if (importLoadDataMode(node) == OSP_LOAD_DATA_DEFERRED) return(importAttributeString(node, volume));

        //! The volume file name.
        const char *filename = node->ToElement()->GetText();

        //! Load the volume file contents into host memory.
//      VolumeFile::importVolumeToHostMemory(filename, (Volume *) volume);

    }

} // namespace ospray

