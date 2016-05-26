// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// own
#include "Importer.h"
// ospcommon
#include "common/FileName.h"
// tinyxml
#include "TinyXML2.h"
// std
#include <stdexcept>
#include <libgen.h>

namespace ospray {
  namespace vv_importer {

    void importVolumeRAW(const FileName &fileName, Volume *volume);
    void importVolumeRM(const FileName &fileName, Volume *volume);

    void importVolume(const FileName &fileName, Volume *volume)
    {
      const std::string ext = fileName.ext();
      if (ext == "raw" || ext == "gz") {
        importVolumeRAW(fileName,volume);
      } else if (ext == "bob") {
        importVolumeRM(fileName, volume);
      } else {
        throw std::runtime_error("unknown volume format '"+ext+"'");
      }

      // post-checks
      assert(volume->handle != NULL);
      assert(!volume->bounds.empty());
    }

    vec2i parseInt2(const tinyxml2::XMLNode *node)
    {
      vec2i v;
      int rc = sscanf(node->ToElement()->GetText(),"%i %i",&v.x,&v.y);
      assert(rc == 2);
      return v;
    }
    vec3i parseInt3(const tinyxml2::XMLNode *node)
    {
      vec3i v;
      int rc = sscanf(node->ToElement()->GetText(),"%i %i %i",&v.x,&v.y,&v.z);
      assert(rc == 3);
      return v;
    }

    float parseFloat1(const tinyxml2::XMLNode *node)
    {
      float v;
      int rc = sscanf(node->ToElement()->GetText(),"%f",&v);
      assert(rc == 1);
      return v;
    }
    vec2f parseFloat2(const tinyxml2::XMLNode *node)
    {
      vec2f v;
      int rc = sscanf(node->ToElement()->GetText(),"%f %f",&v.x,&v.y);
      assert(rc == 2);
      return v;
    }
    vec3f parseFloat3(const tinyxml2::XMLNode *node)
    {
      vec3f v;
      int rc = sscanf(node->ToElement()->GetText(),"%f %f %f",&v.x,&v.y,&v.z);
      assert(rc == 3);
      return v;
    }

    void importVolume(const FileName &orgFileName, Group *group, const tinyxml2::XMLNode *root)
    {
      const char *dpFromEnv = getenv("OSPRAY_DATA_PARALLEL");
      
      Volume *volume = new Volume;
      if (dpFromEnv) {
        // Create the OSPRay object.
        std::cout << "#osp.loader: found OSPRAY_DATA_PARALLEL env-var, "
                  << "#osp.loader: trying to use data _parallel_ mode..." << std::endl;
        osp::vec3i blockDims;
        int rc = sscanf(dpFromEnv,"%dx%dx%d",&blockDims.x,&blockDims.y,&blockDims.z);
        if (rc !=3)
          throw std::runtime_error("could not parse OSPRAY_DATA_PARALLEL env-var. Must be of format <X>x<Y>x<>Z (e.g., '4x4x4'");
        volume->handle = ospNewVolume("data_distributed_volume");
        if (volume->handle == NULL)
          throw std::runtime_error("#loaders.ospObjectFile: could not create volume ...");
        ospSetVec3i(volume->handle,"num_dp_blocks",blockDims);
      } else {
        // Create the OSPRay object.
        std::cout << "#osp.loader: no OSPRAY_DATA_PARALLEL dimensions set, "
                  << "#osp.loader: assuming data replicated mode is desired" << std::endl;
        std::cout << "#osp.loader: to use data parallel mode, set OSPRAY_DATA_PARALLEL env-var to <X>x<Y>x<Z>" << std::endl;
        std::cout << "#osp.loader: where X, Y, and Z are the desired _number_ of data parallel blocks" << std::endl;
        volume->handle = ospNewVolume("block_bricked_volume");
      }
      if (volume->handle == NULL)
        throw std::runtime_error("#loaders.ospObjectFile: could not create volume ...");
      
      // Temporary storage for the file name attribute if specified.
      const char *volumeFilename = NULL;
      
      // Iterate over object attributes.
      for (const tinyxml2::XMLNode *node = root->FirstChild() ; node ; node = node->NextSibling()) {
        
        // Volume size in voxels per dimension.
        if (!strcmp(node->ToElement()->Name(), "dimensions")) 
          { volume->dimensions = parseInt3(node);  continue; }
        
        // File containing a volume specification and / or voxel data.
        if (!strcmp(node->ToElement()->Name(), "filename")) 
          { volumeFilename = node->ToElement()->GetText();  continue; }
        
        // Grid origin in world coordinates.
        if (!strcmp(node->ToElement()->Name(), "gridOrigin")) 
          { volume->gridOrigin = parseFloat3(node);  continue; }
        
        // Grid spacing in each dimension in world coordinates.
        if (!strcmp(node->ToElement()->Name(), "gridSpacing"))
          { volume->gridSpacing = parseFloat3(node);  continue; }
        
        // Sampling rate for ray casting based renderers.
        if (!strcmp(node->ToElement()->Name(), "samplingRate")) 
          { volume->samplingRate = parseFloat1(node);  continue; }
        
        // Subvolume offset from origin within the full volume.
        if (!strcmp(node->ToElement()->Name(), "subvolumeOffsets")) 
          { volume->subVolumeOffsets = parseInt3(node);  continue; }
        
        // Subvolume dimensions within the full volume.
        if (!strcmp(node->ToElement()->Name(), "subvolumeDimensions")) 
          { volume->subVolumeDimensions = parseInt3(node);  continue; }
        
        // Subvolume steps in each dimension; can be used to subsample the volume.
        if (!strcmp(node->ToElement()->Name(), "subvolumeSteps")) 
          { volume->subVolumeSteps = parseInt3(node);  continue; }
        
        // Voxel value range.
        if (!strcmp(node->ToElement()->Name(), "voxelRange")) 
          { volume->voxelRange = parseFloat2(node);  continue; }
        
        // Voxel type string.
        if (!strcmp(node->ToElement()->Name(), "voxelType")) 
          { volume->voxelType = node->ToElement()->GetText();  continue; }
        
        // Error check.
        exitOnCondition(true, "unrecognized XML element type '" + 
                        std::string(node->ToElement()->Name()) + "'");
      }
      
      // Load the contents of the volume file if specified.
      if (volumeFilename != NULL) {
        
        // Some implementations of 'dirname()' are destructive.
        char *duplicateFilename = strdup(orgFileName.str().c_str());
        
        // The volume file path is absolute.
        if (volumeFilename[0] == '/') 
          importVolume(volumeFilename, volume);
        else 
          importVolume((std::string(dirname(duplicateFilename)) + "/" + volumeFilename).c_str(), volume);
        
        // Free the temporary character array.
        if (duplicateFilename != NULL) 
          free(duplicateFilename);
      }
      
      group->volume.push_back(volume);
    }
    
    void importTriangleMesh(Group *group, const tinyxml2::XMLNode *node)
    {
      throw std::runtime_error("importTriangleMesh: not yet implemented");
    }

    void importLight(Group *group, const tinyxml2::XMLNode *node)
    {
      throw std::runtime_error("importLight: not yet implemented");
    }
    
    void importObject(const FileName &orgFileName, Group *group, const tinyxml2::XMLNode *node)
    {
      // OSPRay light object.
      if (!strcmp(node->ToElement()->Name(), "light")) 
        importLight(group,node);
      
      // OSPRay triangle mesh object.
      else if (!strcmp(node->ToElement()->Name(), "triangleMesh")) 
        importTriangleMesh(group,node);
      
      // OSPRay volume object.
      else if (!strcmp(node->ToElement()->Name(), "volume")) 
        importVolume(orgFileName,group,node);
      
      // No other object types are currently supported.
      else 
        exitOnCondition(true, "unrecognized XML element type '" 
                        + std::string(node->ToElement()->Name()) + "'");  
    }

    void importOSP(const FileName &fileName, Group *existingGroupToAddTo)
    {
      // The XML document container.
      tinyxml2::XMLDocument xml(true, tinyxml2::COLLAPSE_WHITESPACE);
      
      // Read the XML object file.
      exitOnCondition(xml.LoadFile(fileName.str().c_str()) != tinyxml2::XML_SUCCESS, 
                      "unable to read object file '" + fileName.str() + "'");
      
      Group *group = existingGroupToAddTo ? existingGroupToAddTo : new Group;

      // Iterate over the object entries, skip the XML declaration and comments.
      for (const tinyxml2::XMLNode *node = xml.FirstChild() ; node ; node = node->NextSibling()) 
        if (node->ToElement()) 
          importObject(fileName,group,node); 

      PRINT(group->volume.size());
      PRINT(group->geometry.size());

      // post-checks:
      for (int i=0;i<group->volume.size();i++) {
        assert(group->volume[i]->handle != NULL);
        assert(!group->volume[i]->bounds.empty());
      }
    }
  }
}
