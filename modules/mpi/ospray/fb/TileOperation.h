// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#pragma once

#include <memory>
#include <vector>
#include "fb/Tile.h"

namespace ospray {

  struct DistributedFrameBuffer;
  struct LiveTileOperation;

  // TODO: Probably should all be under mpi namespace as well

  /* Base class for tile operations which can be attached to a distributed
   * framebuffer. The attach method will be called multiple times on each
   * rank, to make an instance of the live tile operation for each tile owned
   * by the rank.
   *
   * The tileoperation will be kept associated with the specific renderFrame
   * call, and can track additional state as needed by the renderer.
   */
  struct TileOperation
  {
    virtual ~TileOperation() = default;

    /* The attach method is called when the tile operation is first attached
     * to the framebuffer.
     */
    virtual void attach(DistributedFrameBuffer *dfb);

    /* The makeTile method is called to make the live tile operation for each
     * live tile owned by the rank.
     */
    virtual std::shared_ptr<LiveTileOperation> makeTile(
        DistributedFrameBuffer *dfb,
        const vec2i &tileBegin,
        size_t tileID,
        size_t ownerID) = 0;

    virtual std::string toString() const = 0;
  };

  /* Base descriptor for a tile: its position in the frame buffer,
   * its ID and which rank actually owns the tile for running tile operations
   */
  struct TileDesc
  {
    TileDesc(const vec2i &begin, size_t tileID, size_t ownerID);

    virtual ~TileDesc() = default;

    /*! returns whether this tile is one of this particular
        node's tiles */
    virtual bool mine() const
    {
      return false;
    }

    vec2i begin;
    size_t tileID, ownerID;
  };

  /* Base class for a tile operation which the current rank is responsible for
   * executing. Includes scratch space for storing results computed by the
   * tile operation
   */
  struct LiveTileOperation : public TileDesc
  {
    LiveTileOperation(DistributedFrameBuffer *dfb,
                      const vec2i &begin,
                      size_t tileID,
                      size_t ownerID);

    virtual ~LiveTileOperation() override = default;

    // Called at the beginning of each frame
    virtual void newFrame() = 0;

    bool mine() const override
    {
      return true;
    }

    /* Called for each ospray::Tile rendered locally or received over the
     * network for this image tile.
     */
    virtual void process(const ospray::Tile &tile) = 0;

    /* Utility method provided to accumulate the finished tile data from the
     * passed tile into the "finished" tile data, which will be passed on to any
     * ImageOps or written to the framebuffer.
     * TODO: Maybe just manage this on the DFB side?
     */
    void accumulate(const ospray::Tile &tile);

    /* Method to be called when the tile operation is finished and the computed
     * image written to the "finished" member of the struct
     */
    void tileIsFinished();

    DistributedFrameBuffer *dfb;

    // estimated variance of this tile
    float error;

    // TODO: dynamically allocate to save memory when no ACCUM or VARIANCE
    // even more TODO: Tile contains much more data (e.g. AUX), but using only
    // the color buffer here ==> much wasted memory
    // also holds accumulated normal&albedo
    ospray::Tile __aligned(64) accum;
    ospray::Tile __aligned(64) variance;

    /* iw: TODO - have to change this. right now, to be able to give
       the 'postaccum' pixel op a readily normalized tile we have to
       create a local copy (the tile stores only the accum value,
       and we cannot change this) */
    // also holds normalized normal&albedo in AOS format
    // TODO WILL: Is this really such a big deal?
    ospray::Tile __aligned(64) finished;

    //! the rbga32-converted colors
    // TODO: dynamically allocate to save memory when only uint32 / I8 colors
    vec4f __aligned(64) color[TILE_SIZE * TILE_SIZE];
  };

}  // namespace ospray
