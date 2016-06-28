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

#pragma once

#include "ospray/mpi/async/CommLayer.h"
#include "ospray/fb/LocalFB.h"
#include "ospray/common/Thread.h"
#include <queue>

namespace ospray {
  using std::cout;
  using std::endl;

  struct TileDesc;
  struct TileData;

  struct MasterTileMessage;
  template <typename FBType>
  struct MasterTileMessage_FB;
  struct WriteTileMessage;

  struct DistributedFrameBuffer
    : public mpi::async::CommLayer::Object,
      public virtual FrameBuffer
  {
    DistributedFrameBuffer(mpi::async::CommLayer *comm,
                           const vec2i &numPixels,
                           size_t myHandle,
                           ColorBufferFormat colorBufferFormat,
                           bool hasDepthBuffer,
                           bool hasAccumBuffer,
                           bool hasVarianceBuffer);

    ~DistributedFrameBuffer();

    // ==================================================================
    // framebuffer / device interface
    // ==================================================================

    const void *mapDepthBuffer() override;
    const void *mapColorBuffer() override;
    void unmap(const void *mappedMem) override;

    /*! \brief clear (the specified channels of) this frame buffer

      \details for the *distributed* frame buffer, we assume that
      *all* nodes get this command, and that each instance therefore
      can clear only its own tiles without having to tell any other
      node about it
     */
    void clear(const uint32 fbChannelFlags) override;

    // ==================================================================
    // framebuffer-renderer/loadbalancer interface
    // ==================================================================

    /*! framebuffer-renderer/loadbalancer interface: loadbalancer
        calls this function whenever a local node has finished a tile,
        and wants the (distributed) frame buffer to process it */
    void setTile(ospray::Tile &tile) override;

    void startNewFrame();
    void closeCurrentFrame();

    void waitUntilFinished();

    // ==================================================================
    // remaining framebuffer interface
    // ==================================================================

    int32 accumID(const vec2i &) override { return accumId; }
    float tileError(const vec2i &tile) override;
    float endFrame(const float errorThreshold) override;

    // ==================================================================
    // interface for the comm layer, to enable communication between
    // different instances of same object
    // ==================================================================

    //! handle incoming message from commlayer. it's the
    //! recipient's job to properly delete the message.
    void incoming(mpi::async::CommLayer::Message *msg) override;

    //! process a client-to-client write tile message */
    void processMessage(MasterTileMessage *msg);

    //! process a (non-empty) write tile message at the master
    template <typename FBType>
    void processMessage(MasterTileMessage_FB<FBType> *msg);

    //! process a client-to-client write tile message */
    void processMessage(WriteTileMessage *msg);

    // ==================================================================
    // internal helper functions
    // ==================================================================

    /*! this function gets called whenever one of our tiles is done
        writing/compositing/blending/etc; i.e., as soon as we know
        that all the ingredient tile datas for that tile have been
        received from the client(s) that generated them. By the time
        the tile gets called we do know that 'accum' field of the tile
        has been set; it is this function's job to make sure we
        properly call the post-op(s), properly send final color data
        to the master (if required), and properly do the bookkeeping
        that this tile is now done. */
    void tileIsCompleted(TileData *tile);

    //! number of tiles that "I" own
    inline size_t numMyTiles() const { return myTiles.size(); }
    inline bool IamTheMaster() const { return comm->IamTheMaster(); }

    /*! return tile descriptor for given pixel coordinates. this tile
      ! may or may not belong to current instance */
    inline TileDesc *getTileDescFor(const vec2i &coords) const
    { return allTiles[getTileIDof(coords)]; }

    /*! return the tile ID for given pair of coordinates. this tile
        may or may not belong to current instance */
    inline size_t getTileIDof(const vec2i &c) const
    { return (c.x/TILE_SIZE)+(c.y/TILE_SIZE)*numTiles.x; }

    //! \brief common function to help printf-debugging
    /*! \detailed Every derived class should overrride this! */
    std::string toString() const override
    { return "ospray::DistributedFrameBuffer"; }

    typedef enum {
      WRITE_ONCE, ALPHA_BLEND, Z_COMPOSITE
    } FrameMode;

    int accumId;

    //! holds error per tile, for variance estimation / stopping
    float *tileErrorBuffer;

    /*! local frame buffer on the master used for storing the final
        tiles. will be null on all workers, and _may_ be null on the
        master if the master does not have a color buffer */
    Ref<LocalFrameBuffer> localFBonMaster;

    FrameMode frameMode;
    void setFrameMode(FrameMode newFrameMode) ;
    void createTiles();
    TileData *createTile(const vec2i &xy, size_t tileID, size_t ownerID);
    void freeTiles();

    /*! number of tiles written this frame */
    size_t numTilesCompletedThisFrame;

    /*! number of tiles we've (already) sent to the master this frame
        (used to track when current node is done with this frame - we
        are done exactly once we've completed sending the last tile to
        the master) */
    size_t numTilesToMasterThisFrame;

    /*! vector of info for *all* tiles. Each logical tile in the
      screen has an entry here */
    std::vector<TileDesc *> allTiles;

    /*! list of *our* tiles ('our' as in 'that belong to the given
        node'), with the actual data of those tiles */
    std::vector<TileData *> myTiles;

    /*! mutex used to protect all threading-sensitive data in this
        object */
    Mutex mutex;

    //! set to true when the frame becomes 'active', and write tile
    //! messages can be consumed.
    bool frameIsActive;

    /*! set to true when the framebuffer is done for the given
        frame */
    bool frameIsDone;

    //! condition that gets triggered when the frame is done
    Condition frameDoneCond;

    /*! a vector of async messages for the *current* frame that got
        received before that frame actually started, and that we have
        to delay processing until the frame actually starts. note this
        condition can actually happen if another node is just fast
        enough, and sends us the first rendered tile before our node's
        loadbalancer even started working on that frame. */
    std::vector<mpi::async::CommLayer::Message *> delayedMessage;
  };

  // Inlined definitions //////////////////////////////////////////////////////

  template <typename FBType>
  inline void
  DistributedFrameBuffer::processMessage(MasterTileMessage_FB<FBType> *msg)
  {
    if (hasVarianceBuffer && (accumId & 1) == 1)
      tileErrorBuffer[getTileIDof(msg->coords)] = msg->error;

    vec2i numPixels = getNumPixels();

    for (int iy = 0; iy < TILE_SIZE; iy++) {
      int iiy = iy+msg->coords.y;
      if (iiy >= numPixels.y) continue;

      for (int ix = 0; ix < TILE_SIZE; ix++) {
        int iix = ix+msg->coords.x;
        if (iix >= numPixels.x) continue;

        ((FBType*)localFBonMaster->colorBuffer)[iix + iiy*numPixels.x]
          = msg->color[iy][ix];
      }
    }

    // and finally, tell the master that this tile is done
    auto *tileDesc = this->getTileDescFor(msg->coords);
    TileData *td = (TileData*)tileDesc;
    this->tileIsCompleted(td);
  }

} // ::ospray
