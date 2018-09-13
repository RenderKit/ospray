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

#pragma once

#include <memory>

// ospray
#include "ospray/fb/LocalFB.h"
// ospray_mpi
#include "../common/Messaging.h"
// std
#include <condition_variable>

namespace ospray {
  struct TileDesc;
  struct TileData;

  struct AllTilesDoneMessage;
  struct MasterTileMessage;
  template <typename ColorT>
  struct MasterTileMessage_FB;
  template <typename ColorT>
  struct MasterTileMessage_FB_Depth;
  template <typename ColorT>
  struct MasterTileMessage_FB_Depth_Aux;
  struct WriteTileMessage;

  /*! color buffer and depth buffer on master */
  enum COMMANDTAG {
    /*! command tag that identifies a CommLayer::message as a write
      tile command. this is a command using for sending a tile of
      new samples to another instance of the framebuffer (the one
      that actually owns that tile) for processing and 'writing' of
      that tile on that owner node. */
    WORKER_WRITE_TILE = 1 << 1,
    /*! command tag used for sending 'final' tiles from the tile
        owner to the master frame buffer. Note that we *do* send a
        message back ot the master even in cases where the master
        does not actually care about the pixel data - we still have
        to let the master know when we're done. */
    MASTER_WRITE_TILE_I8 = 1 << 2,
    MASTER_WRITE_TILE_F32 = 1 << 3,
    /*! command tag used for sending 'final' tiles from the tile
        owner to the master frame buffer. We do send only one message
        after all worker tiles are processed. This only applies for
        the FB_NONE case where the master does not care about the
        pixel data. */
    WORKER_ALL_TILES_DONE  = 1 << 4,
    // Modifier to indicate the tile also has depth values
    MASTER_TILE_HAS_DEPTH = 1,
    // Indicates that the tile additionally also has normal and/or albedo values
    MASTER_TILE_HAS_AUX = 1 << 5,
    // abort rendering the current frame
    CANCEL_RENDERING = 1 << 6,
  };

  class DistributedTileError : public TileError
  {
    public:
      DistributedTileError(const vec2i &numTiles);
      void sync(); // broadcast tileErrorBuffer to all workers
  };

  struct DistributedFrameBuffer : public mpi::messaging::MessageHandler,
                                  public FrameBuffer
  {
    DistributedFrameBuffer(const vec2i &numPixels,
                           ObjectHandle myHandle,
                           ColorBufferFormat,
                           const uint32 channels,
                           bool masterIsAWorker = false);

    ~DistributedFrameBuffer() override ;

    // ==================================================================
    // framebuffer / device interface
    // ==================================================================

    const void *mapBuffer(OSPFrameBufferChannel channel) override;
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

    void startNewFrame(const float errorThreshold);
    void closeCurrentFrame();

    void waitUntilFinished();

    int32 accumID(const vec2i &) override;
    float tileError(const vec2i &tile) override;
    void  beginFrame() override;
    float endFrame(const float errorThreshold) override;

    enum FrameMode { WRITE_MULTIPLE, ALPHA_BLEND, Z_COMPOSITE };

    void setFrameMode(FrameMode newFrameMode) ;

    // ==================================================================
    // interface for maml messaging, enables communication between
    // different instances of same object
    // ==================================================================

    //! handle incoming message from commlayer. it's the
    //! recipient's job to properly delete the message.
    void incoming(const std::shared_ptr<mpicommon::Message> &message) override;

    //! process an client-to-master all-tiles-are-done message */
    void processMessage(AllTilesDoneMessage *msg, ospcommon::byte_t* data);
    //! process a (non-empty) write tile message at the master
    template <typename ColorT>
    void processMessage(MasterTileMessage_FB<ColorT> *msg);

    //! process a client-to-client write tile message */
    void processMessage(WriteTileMessage *msg);

    size_t ownerIDFromTileID(size_t tileID) const;

    // signal the workers whether to cancel 
    bool continueRendering() const { return !cancelRendering; }

  private:

    friend struct TileData;
    friend struct WriteMultipleTile;
    friend struct AlphaBlendTile_simple;
    friend struct ZCompositeTile;

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
    /*! This function is called when a master write tile is completed, on the
        master process. It only marks on the master that the tile is done, and
        checks if we've completed rendering the frame. */
    void finalizeTileOnMaster(TileData *tile);

    //! number of tiles that "I" own
    size_t numMyTiles() const;

    //! \brief common function to help printf-debugging
    /*! \detailed Every derived class should overrride this! */
    std::string toString() const override;

    /*! return tile descriptor for given pixel coordinates. this tile
      ! may or may not belong to current instance */
    TileDesc *getTileDescFor(const vec2i &coords) const;

    /*! return the tile ID for given pair of coordinates. this tile
        may or may not belong to current instance */
    size_t getTileIDof(const vec2i &c) const;

    void createTiles();
    TileData *createTile(const vec2i &xy, size_t tileID, size_t ownerID);
    void freeTiles();

    /*! atomic update and check if frame is complete with given tiles */
    bool isFrameComplete(size_t numTiles);

    /*! Offloads processing of incoming message to tasking system */
    void scheduleProcessing(const std::shared_ptr<mpicommon::Message> &message);

    /*! Compose and send all-tiles-done message including tile errors. */
    void sendAllTilesDoneMessage();

    void sendCancelRenderingMessage();

    // Data members ///////////////////////////////////////////////////////////

    int32 *tileAccumID; //< holds accumID per tile, for adaptive accumulation
    int32 *tileInstances; //< how many copies of this tile are active, usually 1

    /*! holds error per tile and adaptive regions, for variance estimation /
        stopping */
    DistributedTileError tileErrorRegion;

    /*! local frame buffer on the master used for storing the final
        tiles. will be null on all workers, and _may_ be null on the
        master if the master does not have a color buffer */
    std::unique_ptr<LocalFrameBuffer> localFBonMaster;

    FrameMode frameMode;

    /*! #tiles we've (already) sent to / received by the master this frame
        (used to track when current node is done with this frame - we are done
        exactly once we've completed sending / receiving the last tile to / by
        the master) */
    size_t numTilesCompletedThisFrame;

    /* protected numTilesCompletedThisFrame to ensure atomic update and compare */
    std::mutex numTilesMutex;

    /*! vector of info for *all* tiles. Each logical tile in the
      screen has an entry here */
    std::vector<TileDesc *> allTiles;

    /*! list of *our* tiles ('our' as in 'that belong to the given
        node'), with the actual data of those tiles */
    std::vector<TileData *> myTiles;

    /*! mutex used to protect all threading-sensitive data in this
        object */
    std::mutex mutex;

    //! set to true when the frame becomes 'active', and write tile
    //! messages can be consumed.
    bool frameIsActive;

    /*! set to true when the framebuffer is done for the given
        frame */
    bool frameIsDone;

    bool cancelRendering; // signal the workers whether to cancel 

    bool masterIsAWorker {false};

    //! condition that gets triggered when the frame is done
    std::condition_variable frameDoneCond;

    /*! a vector of async messages for the *current* frame that got
        received before that frame actually started, and that we have
        to delay processing until the frame actually starts. note this
        condition can actually happen if another node is just fast
        enough, and sends us the first rendered tile before our node's
        loadbalancer even started working on that frame. */
    std::vector<std::shared_ptr<mpicommon::Message>> delayedMessage;

    /*! Gather all tile errors in the optimized FB_NONE case to send them out
        in the single AllTilesDoneMessage. */
    std::mutex tileErrorsMutex;
    std::vector< vec2i > tileIDs;
    std::vector< float > tileErrors;
  };

} // ::ospray
