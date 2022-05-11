// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <condition_variable>
#include <memory>
#include "../common/Messaging.h"
#include "DistributedFrameBuffer_TileMessages.h"
#include "fb/LocalFB.h"
#include "fb/SparseFB.h"
#include "render/Renderer.h"
#include "rkcommon/containers/AlignedVector.h"

namespace ospray {
struct TileDesc;
struct TileOperation;
struct LiveTileOperation;

class DistributedTileError : public TaskError
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

  ~DistributedFrameBuffer() override;

  // ==================================================================
  // framebuffer / device interface
  // ==================================================================

  void commit() override;

  const void *mapBuffer(OSPFrameBufferChannel channel) override;

  void unmap(const void *mappedMem) override;

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

  // Return the number of render tasks in the x and y direction
  // This is the kernel launch dims to render the image
  vec2i getNumRenderTasks() const override;

  uint32_t getTotalRenderTasks() const override;

  vec2i getGlobalNumTiles() const;

  uint32_t getGlobalTotalTiles() const;

  /* Get render task IDs will return the render task IDs for layer 0,
   * the tiles owned by the DFB for compositing.
   */
  utility::ArrayView<uint32_t> getRenderTaskIDs() override;

  // Task error is not valid for the DFB, as error is tracked at a per-tile
  // level. Use tileError to get rendering error
  float taskError(const uint32_t taskID) const override;

  float tileError(const uint32_t tileID);

  /*! framebuffer-renderer/loadbalancer interface: loadbalancer
      calls this function whenever a local node has finished a tile,
      and wants the (distributed) frame buffer to process it */
  void setTile(const ispc::Tile &tile);

  void startNewFrame(const float errorThreshold);

  void closeCurrentFrame();

  void waitUntilFinished();

  void endFrame(const float errorThreshold, const Camera *camera) override;

  void setTileOperation(
      std::shared_ptr<TileOperation> tileOp, const Renderer *renderer);

  std::shared_ptr<TileOperation> getTileOperation();

  const Renderer *getLastRenderer() const;

  mpicommon::Group getMPIGroup();
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

  void setSparseFBLayerCount(size_t numLayers);

  size_t getSparseLayerCount() const;

  SparseFrameBuffer *getSparseFBLayer(size_t l);

  void cancelFrame() override;

  float getCurrentProgress() const override;

 private:
  friend struct LiveTileOperation;

  bool hasIDBuf() const;

  // ==================================================================
  // internal helper functions
  // ==================================================================

  /*! this function gets called whenever one of our tiles is done
      writing/compositing/blending/etc; i.e., as soon as we know
      that all the ingredient tile data for that tile have been
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

  // Data members ///////////////////////////////////////////////////////////

  // The communicator used by this DFB instance for collectives, to avoid
  // conflicts with other framebuffers
  mpicommon::Group mpiGroup;

  // Total number of tiles that the framebuffer is divided into, including those
  // not owned by this sparsefb
  vec2i totalTiles;

  // Total number of render tasks that the framebuffer is divided into,
  // including those not owned by this sparsefb
  vec2i numRenderTasks;

  std::vector<char> tileGatherBuffer;

  std::vector<char> compressedResults;

  // Locations of the compressed tiles in the tileGatherBuffer
  std::vector<uint32_t> tileBufferOffsets;

  uint32_t nextTileWrite{0};

  std::mutex finalTileBufferMutex;

  /*! holds error per tile and adaptive regions, for variance estimation /
      stopping */
  DistributedTileError tileErrorRegion;

  /*! local frame buffer on the master used for storing the final
      tiles. will be null on all workers, and _may_ be null on the
      master if the master does not have a color buffer */
  std::unique_ptr<LocalFrameBuffer> localFBonMaster;

  /* SparseFrameBuffer layers in the DFB, layer 0 stores the tiles owned by DFB
   * for compositing, and layers 1-N are those added by the renderer. For
   * renderers whose local rendering work is dynamic/data-dependent (e.g.,
   * data-parallel rendering), layers can be allocated and resized as needed to
   * avoid having to reallocate the set of SparseFrameBuffer(s) needed each
   * frame.
   */
  std::vector<std::shared_ptr<SparseFrameBuffer>> layers;

  /*! #tiles we've (already) sent to / received by the master this frame
      (used to track when current node is done with this frame - we are done
      exactly once we've completed sending / receiving the last tile to / by
      the master) */
  size_t numTilesCompletedThisFrame{0};

  /*! The total number of tiles completed by all workers during this frame,
      to track progress for the user's progress callback. NOTE: This is
      not the numTilesCompletedThisFrame , which tracks how many tiles
      this rank has finished of the ones it's responsible for completing */
  size_t globalTilesCompletedThisFrame{0};

  /*! The number of tiles the master is expecting to receive from each rank */
  std::vector<int> numTilesExpected;

  /* protected numTilesCompletedThisFrame to ensure atomic update and compare
   */
  std::mutex numTilesMutex;

  /* vector of info for *all* tiles. Each logical tile in the    screen has an
   * entry here. allTiles owns the lifetime for tiles in myTiles as well, which
   * is a subset of those in allTiles containing the live tile operations for
   * the tiles this rank owns */
  std::vector<std::unique_ptr<TileDesc>> allTiles;

  /* list of *our* tiles ('our' as in 'that belong to the given node'), with the
    actual data of those tiles. NOTE: for msvc15 issue with shared_ptr, the tile
    operation lifetime is managed in allTiles which holds a unique_ptr for all
    tiles (both ones we own, and don't). */
  std::vector<LiveTileOperation *> myTiles;

  std::shared_ptr<TileOperation> tileOperation = nullptr;
  const Renderer *lastRenderer = nullptr;

  /*! mutex used to protect all threading-sensitive data in this
      object */
  std::mutex mutex;

  //! set to true when the frame becomes 'active', and write tile
  //! messages can be consumed.
  std::atomic<bool> frameIsActive{false};

  /*! set to true when the framebuffer is done for the given
      frame */
  bool frameIsDone{false};

  int renderingProgressTiles{0};
  std::chrono::steady_clock::time_point lastProgressReport;

  std::atomic<float> frameProgress{0.f};

  //! condition that gets triggered when the frame is done
  std::condition_variable frameDoneCond;

  /*! Gather all tile errors in the optimized FB_NONE case to send them out
      in the single gatherv. */
  std::mutex tileErrorsMutex;
  std::vector<vec2i> tileIDs;
  std::vector<float> tileErrors;
};

} // namespace ospray
