// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

// ospcommon
#include "ospcommon/tasking/parallel_for.h"
#include "common/Data.h"
// ospray
#include "VisItDistributedRaycast.h"
#include "common/DistributedModel.h"
#include "render/MPILoadBalancer.h"
#include "fb/DistributedFrameBuffer.h"
// STL standard
#include <algorithm>

namespace ospray {
    namespace visit {

	struct RegionInfo
	{
	    int currentRegion;
	    bool *regionVisible;

	    RegionInfo() : currentRegion(0), regionVisible(nullptr) {}
	};

	TileInfo::TileInfo(const Tile& src) {	   
	    std::copy(src.r, src.r + TILE_SIZE * TILE_SIZE, r);
	    std::copy(src.g, src.g + TILE_SIZE * TILE_SIZE, g);
	    std::copy(src.b, src.b + TILE_SIZE * TILE_SIZE, b);
	    std::copy(src.a, src.a + TILE_SIZE * TILE_SIZE, a);
	    depth = *(std::min_element(src.z, src.z + TILE_SIZE * TILE_SIZE));
	    regionSize[0] = src.region.lower.x;
	    regionSize[1] = src.region.lower.y;
	    regionSize[2] = src.region.upper.x;
	    regionSize[3] = src.region.upper.y;
	    fbSize[0] = src.fbSize.x;
	    fbSize[1] = src.fbSize.y;
	    visible = true;
	}

	// VisItDistributedRaycastRenderer definitions /////////////////////////////////

	VisItDistributedRaycastRenderer::VisItDistributedRaycastRenderer()
	{
	    DistributedRaycastRenderer();
	    std::cout << "#osp: creating VisIt distributed raycast renderer!" << std::endl;
	}

	void VisItDistributedRaycastRenderer::commit()
	{
	    DistributedRaycastRenderer::commit();
	    tileRetriever = getVoidPtr("tileRetriever", nullptr);
	}

	float VisItDistributedRaycastRenderer::renderFrame(FrameBuffer *fb,
							   const uint32 channelFlags)
	{
	    using namespace mpicommon;
	    using namespace ospray::mpi;

	    auto *dfb = dynamic_cast<DistributedFrameBuffer *>(fb);
	    dfb->setFrameMode(DistributedFrameBuffer::ALPHA_BLEND);
	    if (tileRetriever == nullptr) { dfb->startNewFrame(errorThreshold); }
	    dfb->beginFrame();

	    auto *perFrameData = beginFrame(dfb);
	    // This renderer doesn't use per frame data, since we sneak in some tile
	    // info in this pointer.
	    assert(!perFrameData);
	    DistributedModel *distribModel = dynamic_cast<DistributedModel*>(model);
	    const size_t numRegions = distribModel->myRegions.size() + distribModel->othersRegions.size();

	    // Initialize the tile list
	    TileRegionList tileInfoList(dfb->getTotalTiles());

	    tasking::parallel_for(dfb->getTotalTiles(), [&](int taskIndex) {
		    const size_t numTiles_x = fb->getNumTiles().x;
		    const size_t tile_y = taskIndex / numTiles_x;
		    const size_t tile_x = taskIndex - tile_y*numTiles_x;
		    const vec2i tileID(tile_x, tile_y);
		    const int32 accumID = fb->accumID(tileID);
		    const bool tileOwner = (taskIndex % numGlobalRanks()) == globalRank();

		    if (dfb->tileError(tileID) <= errorThreshold) {
			return;
		    }

		    // The first 0..myRegions.size() - 1 entries are for my regions,
		    // the following entries are for other nodes regions
		    RegionInfo regionInfo;
		    regionInfo.regionVisible = STACK_BUFFER(bool, numRegions);
		    std::fill(regionInfo.regionVisible, regionInfo.regionVisible + numRegions, false);

		    Tile __aligned(64) tile(tileID, dfb->size, accumID);

		    // We use the task of rendering the first region to also fill out the block visiblility list
		    const int NUM_JOBS = (TILE_SIZE * TILE_SIZE) / RENDERTILE_PIXELS_PER_JOB;
		    tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
			    renderTile(&regionInfo, tile, tIdx);
			});

		    // initialize tile information list
		    tileInfoList[taskIndex] = std::vector<TileInfo>(0);

		    if (regionInfo.regionVisible[0]) {
			tile.generation = 1;
			tile.children = 0;
			if (tileRetriever == nullptr) {
			    fb->setTile(tile);
			} 
			else {
			    tileInfoList[taskIndex].emplace_back(tile);
			}			

		    }

		    // If we own the tile send the background color and the count of children for the
		    // number of regions projecting to it that will be sent.
		    if (tileOwner) {
			tile.generation = 0;
			tile.children = std::count(regionInfo.regionVisible, regionInfo.regionVisible + numRegions, true);
			std::fill(tile.r, tile.r + TILE_SIZE * TILE_SIZE, bgColor.x);
			std::fill(tile.g, tile.g + TILE_SIZE * TILE_SIZE, bgColor.y);
			std::fill(tile.b, tile.b + TILE_SIZE * TILE_SIZE, bgColor.z);
			std::fill(tile.a, tile.a + TILE_SIZE * TILE_SIZE, 1.0);
			std::fill(tile.z, tile.z + TILE_SIZE * TILE_SIZE, std::numeric_limits<float>::infinity());
			if (tileRetriever == nullptr) {
			    fb->setTile(tile);
			} 
			else {
			    tileInfoList[taskIndex].emplace_back(tile);
			}			

		    }

		    // Render the rest of our regions that project to this tile and ship them off
		    tile.generation = 1;
		    tile.children = 0;
		    for (size_t bid = 1; bid < distribModel->myRegions.size(); ++bid) {
			if (!regionInfo.regionVisible[bid]) {
			    continue;
			}
			regionInfo.currentRegion = bid;
			tasking::parallel_for(NUM_JOBS, [&](int tIdx) {
				renderTile(&regionInfo, tile, tIdx);
			    });
			if (tileRetriever == nullptr) {
			    fb->setTile(tile);
			} 
			else {
			    tileInfoList[taskIndex].emplace_back(tile);
			}			

		    }
		});

	    if (tileRetriever == nullptr) { dfb->waitUntilFinished(); }
	    else { (*static_cast<TileRetriever*>(tileRetriever))(tileInfoList); }

	    endFrame(nullptr, channelFlags);
	    return dfb->endFrame(errorThreshold);
	}

	std::string VisItDistributedRaycastRenderer::toString() const
	{
	    return "ospray::visit::VisItDistributedRaycastRenderer";
	}

	OSP_REGISTER_RENDERER(VisItDistributedRaycastRenderer, visitrc);

    } // ::ospray::visit
} // ::ospray

