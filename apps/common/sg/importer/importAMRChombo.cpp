// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wextended-offsetof"

#include "SceneGraph.h"
#include "../volume/AMRVolume.h"
#include "../volume/Volume.h"
// hdf
#include "hdf5.h"
// ospcommon
#include "ospcommon/range.h"
#include "ospcommon/box.h"

// core ospray
#include "ospray/common/OSPCommon.h"

namespace ospray {
  namespace amr {

    //! stores AMR Level data
    struct Level
    {
      Level(int _levelID) : levelID(_levelID){}

      int levelID;
      // apparently, the refinement factor of this level
      double dt;
      /*! the 'outputGhost' variable, specifying the number of ghost cells on
       * all sides */
      vec3i numGhostCells;

      inline vec3i boxSize(const int boxID) const
      {
        return boxes[boxID].size() + 1;
      }

      inline double getValue(int boxID, int compID, const vec3i &coordNoGhost)
      {
        assert(compID >= 0);
        const vec3i sizeNoGhosts   = boxSize(boxID);
        const vec3i sizeWithGhosts = sizeNoGhosts + 2 * numGhostCells;
        size_t brickOfs            = offsets[boxID] +
                          compID * (size_t)sizeWithGhosts.x *
                              (size_t)sizeWithGhosts.y *
                              (size_t)sizeWithGhosts.z;
        const vec3i coordWithGhosts = coordNoGhost + numGhostCells;
        return data[brickOfs + coordWithGhosts.x +
                    sizeWithGhosts.x * (coordWithGhosts.y +
                                        sizeWithGhosts.y * coordWithGhosts.z)];
      }

      std::vector<box3i> boxes;
      std::vector<int64_t> offsets;
      std::vector<double> data;

      box3f getWorldBounds(const int boxID) const;
      box3f getWorldBounds() const;
    };

    //! struct AMR used for storing intermediate AMR representation for
    //!   file loading
    struct AMR
    {
      std::vector<Level *> level;
      //! array of component names
      std::vector<std::string> component;

      std::string voxelType="float"; // for now only floats are created by importer

      static AMR *parse(const std::string &fileName, int maxLevel = 1 << 30);
      box3f getWorldBounds() const;
    };

    //! parses out level data from hdf5 files
    void parseBoxes(hid_t file, Level *level)
    {
      char dataName[1000];
      sprintf(dataName, "level_%i/boxes", level->levelID);
      hid_t data = H5Dopen(file, dataName, H5P_DEFAULT);
      if (data < 0)
        throw std::runtime_error("does not exist");
      hid_t space        = H5Dget_space(data);
      const size_t nDims = H5Sget_simple_extent_ndims(space);
      hsize_t *dims = STACK_BUFFER(hsize_t, nDims);
      H5Sget_simple_extent_dims(space, dims, nullptr);
      size_t numBoxes = dims[0];

      // create compound data type for a box
      hid_t boxType = H5Tcreate(H5T_COMPOUND, sizeof(box3i));

      H5Tinsert(boxType, "lo_i", HOFFSET(box3i, lower.x), H5T_NATIVE_INT);
      H5Tinsert(boxType, "lo_j", HOFFSET(box3i, lower.y), H5T_NATIVE_INT);
      H5Tinsert(boxType, "lo_k", HOFFSET(box3i, lower.z), H5T_NATIVE_INT);
      H5Tinsert(boxType, "hi_i", HOFFSET(box3i, upper.x), H5T_NATIVE_INT);
      H5Tinsert(boxType, "hi_j", HOFFSET(box3i, upper.y), H5T_NATIVE_INT);
      H5Tinsert(boxType, "hi_k", HOFFSET(box3i, upper.z), H5T_NATIVE_INT);

      level->boxes.resize(numBoxes);
      H5Dread(data, boxType, H5S_ALL, H5S_ALL, H5P_DEFAULT, &level->boxes[0]);
      H5Dclose(data);
    }

    //! parses out refinement levels for amr data
    void parseOffsets(hid_t file, Level *level)
    {
      char dataName[1000];
      sprintf(dataName, "level_%i/data:offsets=0", level->levelID);
      hid_t data = H5Dopen(file, dataName, H5P_DEFAULT);
      if (data < 0)
        throw std::runtime_error("does not exist");
      hid_t space        = H5Dget_space(data);
      const size_t nDims = H5Sget_simple_extent_ndims(space);
      hsize_t *dims = STACK_BUFFER(hsize_t, nDims);
      H5Sget_simple_extent_dims(space, dims, nullptr);
      size_t numOffsets = dims[0];

      level->offsets.resize(numOffsets);
      H5Dread(data,
              H5T_NATIVE_INT64,
              H5S_ALL,
              H5S_ALL,
              H5P_DEFAULT,
              &level->offsets[0]);
      H5Dclose(data);
    }

    //! parse scalar data from hdf5 file
    void parseData(hid_t file, Level *level)
    {
      char dataName[1000];
      sprintf(dataName, "level_%i/data:datatype=0", level->levelID);
      hid_t data         = H5Dopen(file, dataName, H5P_DEFAULT);
      hid_t space        = H5Dget_space(data);
      const size_t nDims = H5Sget_simple_extent_ndims(space);
      hsize_t *dims = STACK_BUFFER(hsize_t, nDims);
      H5Sget_simple_extent_dims(space, dims, nullptr);
      size_t numData = dims[0];

      level->data.resize(numData);
      H5Dread(data,
              H5T_NATIVE_DOUBLE,
              H5S_ALL,
              H5S_ALL,
              H5P_DEFAULT,
              &level->data[0]);

      H5Dclose(data);
    }

    //! parse attributes in hdf5 amr data
    void parseDataAttributes(hid_t file,
                             Level *level,
                             const std::string &levelName)
    {
      const std::string dataAttrName =
          "/" + levelName + "/" + "data_attributes";
      hid_t attr_ghost = H5Aopen_by_name(
          file, dataAttrName.c_str(), "outputGhost", H5P_DEFAULT, H5P_DEFAULT);
      hid_t ghostType  = H5Aget_type(attr_ghost);
      H5Aread(attr_ghost, ghostType, &level->numGhostCells);
      H5Aclose(attr_ghost);
    }

    //! parse individual level in amr data
    void parseLevel(hid_t file, Level *level)
    {
      char levelName[1000];
      sprintf(levelName, "level_%i", level->levelID);
      hid_t attr_dt =
          H5Aopen_by_name(file, levelName, "dx", H5P_DEFAULT, H5P_DEFAULT);
      H5Aread(attr_dt, H5T_NATIVE_DOUBLE, &level->dt);
      H5Aclose(attr_dt);

      parseDataAttributes(file, level, levelName);
      parseBoxes(file, level);
      parseData(file, level);
      parseOffsets(file, level);

      ospLogF(1) << "read input level #" << level->levelID << ", cellWidth is "
                 << level->dt << std::endl;
    }

    //! parse hdf5 file
    AMR *AMR::parse(const std::string &fileName, int maxLevel)
    {
      AMR *cd           = new AMR;
      char *maxLevelEnv = getenv("AMR_MAX_LEVEL");
      if (maxLevelEnv) {
        maxLevel = atoi(maxLevelEnv);
      } else {
      }
      hid_t file = H5Fopen(fileName.c_str(), H5F_ACC_RDONLY, H5P_DEFAULT);
      if (!file)
        throw std::runtime_error("could not open AMR HDF file '" + fileName +
                                 "'");

      // read components ...
      int numComponents = -1;
      {
        hid_t attr_num_components = H5Aopen_by_name(
            file, "/", "num_components", H5P_DEFAULT, H5P_DEFAULT);
        H5Aread(attr_num_components, H5T_NATIVE_INT, &numComponents);
        H5Aclose(attr_num_components);
      }
      if (numComponents < 1)
        throw std::runtime_error("could not parse num components");
      for (ssize_t i = 0; i < numComponents; i++) {
        char compName[10000];
        sprintf(compName, "component_%li", i);

        hid_t att        = H5Aopen_name(file, compName);
        hid_t ftype      = H5Aget_type(att);

        size_t len = H5Tget_size(ftype);
        char *comp = STACK_BUFFER(char, (len + 1));
        comp[len] = 0;

        hid_t type = H5Tget_native_type(ftype, H5T_DIR_ASCEND);
        H5Aread(att, type, comp);
        H5Aclose(att);

        cd->component.push_back(comp);
      }

      unsigned long long numObjectsInFile;
      H5Gget_num_objs(file, &numObjectsInFile);

      for (size_t objID = 0; objID < numObjectsInFile; objID++) {
        const int MAX_NAME_SIZE = 1000;
        char name[MAX_NAME_SIZE];
        H5Gget_objname_by_idx(file, objID, name, MAX_NAME_SIZE);

        if (objID == 0) {
          if (strcmp(name, "Chombo_global")) {
            std::cout << name << std::endl;
            throw std::runtime_error(
                "missing 'Chombo_global' object - apparently this is not a "
                "chombo file!?");
          }
          continue;
        }
        int levelID;
        int rc = sscanf(name, "level_%d", &levelID);
        if (rc == 1) {
          if (levelID > maxLevel) {
            ospLogF(1) << "#osp:amr: skipping amr level #" << levelID
                       << " as instructed by amr_MAX_LEVEL envvar" << std::endl;
            continue;
          }

          Level *level = new Level(levelID);
          parseLevel(file, level);
          cd->level.push_back(level);
          continue;
        }
        ospLogF(1) << "#osp:qtv:amr: unknown HDF5 block '" << name << "'"
                   << std::endl;
      }

      H5Fclose(file);
      return cd;
    }

    //! get bounds for leaf level
    box3f Level::getWorldBounds(const int boxID) const
    {
      return box3f(float(dt) * vec3f(boxes[boxID].lower),
                   float(dt) * vec3f(boxes[boxID].upper + vec3i(1)));
    }

    //! get bounds for a level
    box3f Level::getWorldBounds() const
    {
      box3f bounds = getWorldBounds(0);
      for (size_t i = 1; i < boxes.size(); i++)
        bounds.extend(getWorldBounds(i));
      return bounds;
    }

    //! get bounds of enditre amr struct
    box3f AMR::getWorldBounds() const
    {
      box3f bounds = empty;
      for (size_t i = 0; i < level.size(); i++)
        bounds.extend(level[i]->getWorldBounds());
      return bounds;
    }

  }  // ::ospray::amr

  namespace sg {

    //! parse Chombo hdf5 file into AMRVolume node
    void parseAMRChomboFile(std::shared_ptr<sg::AMRVolume> &node,
                            const FileName &fileName,
                            const std::string &desiredComponent = "",
                            const range1f *clampRange = nullptr,
                            int maxLevel = 1 << 30)
    {
      amr::AMR *amr = ospray::amr::AMR::parse(fileName.str(), maxLevel);
      assert(!amr->level.empty());

      box3i rootLevelBounds = empty;
      for (size_t i = 0; i < amr->level[0]->boxes.size(); i++)
        rootLevelBounds.extend(amr->level[0]->boxes[i]);
      assert(rootLevelBounds.lower == vec3i(0));

      node->child("bounds") = amr->getWorldBounds();
      (*node)["voxelType"] = amr->voxelType;

      node->componentID = -1;
      for (size_t i = 0; i < amr->component.size(); i++) {
        if (amr->component[i] == desiredComponent) {
          node->componentID = i;
        }
      }
      if (node->componentID < 0) {
        if (desiredComponent == "") {
          ospLogF(1) << "no component specified - defaulting to component 0"
                     << std::endl;
          node->componentID = 0;
        } else
          throw std::runtime_error("could not find desird component '" +
                                   desiredComponent + "'");
      }

      for (size_t levelID = 0; levelID < amr->level.size(); levelID++) {
        amr::Level *level = amr->level[levelID];
        ospLogF(1) << " - level: " << levelID << " : " << level->boxes.size()
                   << " boxes" << std::endl;
        for (size_t brickID = 0; brickID < level->boxes.size(); brickID++) {
          AMRVolume::BrickInfo bi;
          bi.box   = level->boxes[brickID];
          bi.dt    = level->dt;
          bi.level = levelID;

          node->brickInfo.push_back(bi);

          size_t numValues = bi.size().product();
          float *f         = new float[numValues];
          node->brickPtrs.push_back(f);
          for (int iz = 0; iz < bi.size().z; iz++)
            for (int iy = 0; iy < bi.size().y; iy++)
              for (int ix = 0; ix < bi.size().x; ix++) {
                vec3i coord(ix, iy, iz);
                double v = level->getValue(brickID, node->componentID, coord);
                if (clampRange)
                  v = clampRange->clamp(v);
                node->valueRange.extend(v);
                *f++ = v;
              }
        }
        level->data.clear();
      }
      ospLogF(1) << "found " << node->brickInfo.size() << " bricks"
                 << std::endl;
    }

    // Import HDF5 CHOMBO files ///////////////////////////////////////////////

    void importCHOMBO(const std::shared_ptr<Node> &world,
                      const FileName &fileName)
    {
      auto node = sg::createNode("amr", "AMRVolume")->nodeAs<sg::AMRVolume>();
      parseAMRChomboFile(node, fileName);
      node->child("transferFunction")["valueRange"] = node->valueRange.toVec2f();
      world->add(node);
    }

  }  // ::ospray::sg
}  // ::ospray

#pragma clang diagnostic pop
