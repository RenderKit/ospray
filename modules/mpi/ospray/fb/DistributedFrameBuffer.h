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

#include <condition_variable>
#include <memory>
#include "../common/Messaging.h"
#include "DistributedFrameBuffer_TileMessages.h"
#include "ospcommon/containers/AlignedVector.h"
#include "ospray/fb/LocalFB.h"
#include "ospray/render/Renderer.h"

namespace ospray {
  struct TileDesc;
  struct TileOperation;
  struct LiveTileOperation;

  class DistributedTileError : public TileError
  {
    mpicommon::Group group;

   public:
    DistributedTileError(const vec2i &numTiles, mpicommon::Group group);
    // broadcast tileErrorBuffer to all workers in this group
    void sync();
  };

  struct DistributedFrameBuffer : public mpi::messaging::MessageHandler,
                                  public FrameBuffer
  {
    DistributedFrameBuffer(const vec2i &numPixels,
                           ObjectHandle myHandle,
                           ColorBufferFormat,
                           const uint32 channels);

    // ==================================================================
    // framebuffer / device interface
    // ==================================================================

    const void *mapBuffer(OSPFrameBufferChannel channel) override;
    void unmap(const void *mappedMem) override;

    void commit() override;

    /*! \brief clear (the specified channels of) this frame buffer

      \details for the *distributed* frame buffer, we assume that
      *all* nodes get this command, and that each instance therefore
      can clear only its own tiles without having to tell any other
      node about it
     */
    void clear() override;

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
    void endFrame(const float errorThreshold, const Camera *camera) override;

    void setTileOperation(std::shared_ptr<TileOperation> tileOp,
                          const Renderer *renderer);
    std::shared_ptr<TileOperation> getTileOperation();
    const Renderer *getLastRenderer() const;

    // ==================================================================
    // interface for maml messaging, enables communication between
    // different instances of same object
    // ==================================================================

    //! handle incoming message from commlayer. it's the
    //! recipient's job to properly delete the message.
    void incoming(const std::shared_ptr<mpicommon::Message> &message) override;

    //! process a (non-empty) write tile message at the master
    template <typename ColorT>
    void processMessage(MasterTileMessage_FB<ColorT> *msg);

    //! process a client-to-client write tile message */
    void processMessage(WriteTileMessage *msg);

    size_t ownerIDFromTileID(size_t tileID) const;

    void reportTimings(std::ostream &os);

   private:
    using RealMilliseconds = std::chrono::duration<double, std::milli>;
    std::vector<RealMilliseconds> queueTimes;
    std::vector<RealMilliseconds> workTimes;
    RealMilliseconds finalGatherTime, masterTileWriteTime, waitFrameFinishTime,
        compressTime, decompressTime, preGatherDuration;
    double compressedPercent;
    std::mutex statsMutex;

    std::vector<char> compressedBuf;
    std::vector<char> tileGatherResult;
    std::vector<char> tileGatherBuffer;
    std::vector<char> compressedResults;
    std::atomic<size_t> nextTileWrite;

    friend struct LiveTileOperation;

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
    void tileIsFinished(LiveTileOperation *tile);

    /*! This function is called when a worker reports how many tiles it's
        completed to the master, to update the user's progress callback.
        This is only used if the user has set a progress callback. */
    void updateProgress(ProgressMessage *msg);

    //! number of tiles that "I" own
    size_t numMyTiles() const;

    //! \brief common function to help printf-debugging
    /*! \detailed Every derived class should override this! */
    std::string toString() const override;

    /*! return tile descriptor for given pixel coordinates. this tile
      ! may or may not belong to current instance */
    TileDesc *getTileDescFor(const vec2i &coords) const;

    /*! return the tile ID for given pair of coordinates. this tile
        may or may not belong to current instance */
    size_t getTileIDof(const vec2i &c) const;

    void createTiles();

    /*! atomic update and check if frame is complete with given tiles */
    bool isFrameComplete(size_t numTiles);

    /*! Offloads processing of incoming message to tasking system */
    void scheduleProcessing(const std::shared_ptr<mpicommon::Message> &message);

    /*! Gather the final tiles from the other ranks to the master rank to
     * copy into the framebuffer */
    void gatherFinalTiles();

    /*! Gather the tile IDs and error info from the other ranks to the master,
     * for OSP_FB_NONE rendering, where we only track that info on the master */
    void gatherFinalErrors();

    void sendCancelRenderingMessage();

    // Data members ///////////////////////////////////////////////////////////

    // The communicator used by this DFB instance for collectives, to avoid
    // conflicts with other framebuffers
    mpicommon::Group mpiGroup;

    // Holds accumID per tile, for adaptive accumulation
    ospcommon::containers::AlignedVector<uint32_t> tileAccumID;

    /*! holds error per tile and adaptive regions, for variance estimation /
        stopping */
    DistributedTileError tileErrorRegion;

    /*! local frame buffer on the master used for storing the final
        tiles. will be null on all workers, and _may_ be null on the
        master if the master does not have a color buffer */
    std::unique_ptr<LocalFrameBuffer> localFBonMaster;

    /*! #tiles we've (already) sent to / received by the master this frame
        (used to track when current node is done with this frame - we are done
        exactly once we've completed sending / receiving the last tile to / by
        the master) */
    size_t numTilesCompletedThisFrame;

    /*! The total number of tiles completed by all workers during this frame,
        to track progress for the user's progress callback. NOTE: This is
        not the numTilesCompletedThisFrame , which tracks how many tiles
        this rank has finished of the ones it's responsible for completing */
    size_t globalTilesCompletedThisFrame;

    /*! The number of tiles the master is expecting to receive from each rank */
    std::vector<size_t> numTilesExpected;

    /* protected numTilesCompletedThisFrame to ensure atomic update and compare
     */
    std::mutex numTilesMutex;

    /*! vector of info for *all* tiles. Each logical tile in the
      screen has an entry here */
    std::vector<std::shared_ptr<TileDesc>> allTiles;

    /*! list of *our* tiles ('our' as in 'that belong to the given
        node'), with the actual data of those tiles */
    std::vector<std::shared_ptr<LiveTileOperation>> myTiles;

    std::shared_ptr<TileOperation> tileOperation = nullptr;
    const Renderer *lastRenderer                 = nullptr;

    /*! mutex used to protect all threading-sensitive data in this
        object */
    std::mutex mutex;

    //! set to true when the frame becomes 'active', and write tile
    //! messages can be consumed.
    std::atomic<bool> frameIsActive;

    /*! set to true when the framebuffer is done for the given
        frame */
    bool frameIsDone;

    int renderingProgressTiles;
    std::chrono::high_resolution_clock::time_point lastProgressReport;

    //! condition that gets triggered when the frame is done
    std::condition_variable frameDoneCond;

    /*! Gather all tile errors in the optimized FB_NONE case to send them out
        in the single gatherv. */
    std::mutex tileErrorsMutex;
    std::vector<vec2i> tileIDs;
    std::vector<float> tileErrors;
  };

}  // namespace ospray
