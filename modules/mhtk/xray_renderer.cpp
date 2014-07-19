/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#undef NDEBUG

#include "multihit_kernel.h"
#include "xray_renderer.h"
#include "ospray/camera/perspectivecamera.h"
#include "ospray/volume/volume.h"
// ospray stuff
#include "ospray/common/ray.h"
// ispc exports
#include "xray_renderer_ispc.h"

#define STATS(a) 

namespace ospray {
  namespace mhtk {

    /*! \addtogroup mhtk_module_xray
      @{ */
    STATS(int64 numTotalFound=-1);
    STATS(int64 maxFound=-1);
    STATS(int64 numAnyFound=-1);

    // // declaration of ispc-side kernel
    // extern "C" void ispc__mhtk_XRayRenderer_renderTile(void *_tile,
    //                                                    void *_camera,
    //                                                    RTCScene scene);
    
    // /*! \brief create render job for scalar mhtk::xray renderer */
    // TileRenderer::RenderJob *ScalarXRayRenderer::createRenderJob(FrameBuffer *fb)
    // {
    //   STATS(PRINT(numTotalFound));
    //   STATS(PRINT(numAnyFound));
    //   STATS(PRINT(maxFound));
    //   STATS(PRINT(numTotalFound/float(numAnyFound)));
    //   STATS(numTotalFound=0);
    //   STATS(maxFound=0);
    //   STATS(numAnyFound=0);

    //   Model *model = (Model *)getParamObject("model",NULL);
    //   Assert2(model,"null model handle in 'xray' renderer "
    //          "(did you forget to assign a 'model' parameter to the renderer?)");

    //   Camera *camera = (Camera *)getParamObject("camera",NULL);
    //   Assert2(camera,"null camera handle in 'xray' renderer "
    //          "(did you forget to assign a 'camera' parameter to the renderer?)");
    
    //   return new TileJob(camera,model->embreeSceneHandle);
    // }

    ISPCXRayRenderer::ISPCXRayRenderer()
    { 
      ispcEquivalent = ispc::XRayRenderer_create(this); 
    }

    /*! \brief create render job for ispc-based mhtk::xray renderer */
    void ISPCXRayRenderer::commit()
    {
      Renderer::commit();
      model = (Model *)getParamObject("model",NULL);
      Camera *camera = (Camera *)getParamObject("camera",NULL);

      if (model && camera) {
        ispc::XRayRenderer_set(getIE(),model->getIE(),camera->getIE());
      }
    }

    // /*! \brief perform the actual multihit kernel based xray tracing
    //     and shading for a given (individual) ray */
    // inline vec4f traceRayXRay(Ray &ray, 
    //                           RTCScene scene)
    // {
    //   const size_t maxNumHits = 512;
    //   HitInfo hitArray[maxNumHits];
    //   const size_t numHits = multiHitKernel(scene,ray,hitArray,maxNumHits);
    //   STATS(numTotalFound+=numHits);
    //   STATS(numAnyFound+=((numHits>0)?1:0));
    //   STATS(maxFound=std::max(maxFound,(int64)numHits));

    //   // PRINT(numHits);

    //   float color = 0.f;
    //   float addtl = .5f;
    //   for (int i=0;i<numHits;i++) {
    //     vec3f N = normalize(hitArray[i].Ng);
    //     color += (addtl*fabsf(dot(ray.dir,N)));
    //     addtl *= .5f;
    //   }
    //   return vec4f(color);
    // }

    // void ScalarXRayRenderer::commit()
    // {
    //   model = (Model *)getParamObject("model",NULL);
    //   Assert2(model,"null model handle in 'xray' renderer "
    //           "(did you forget to assign a 'model' parameter to the renderer?)");
      
    //   camera = (Camera *)getParamObject("camera",NULL);
    //   Assert2(camera,"null camera handle in 'xray' renderer "
    //           "(did you forget to assign a 'camera' parameter to the renderer?)");

    //   embreeSceneHandle = model->embreeSceneHandle;
    // }

    // /*! \brief rendertile callback for scalar multi-hit kernel xray
    //     renderer */
    // void ScalarXRayRenderer::renderTile(Tile &tile)
    // {
    //   const uint32 size_x = tile.fbSize.x;
    //   const uint32 size_y = tile.fbSize.y;
    //   const uint32 x0 = tile.region.lower.x;
    //   const uint32 y0 = tile.region.lower.y;
    //   for (uint32 frag=0;frag<TILE_SIZE*TILE_SIZE;frag++) {
    //     const uint32  x     = x0 + (frag % TILE_SIZE);
    //     const uint32  y     = y0 + (frag / TILE_SIZE);
    //     if (x < size_x & y < size_y) {
    //       const float screen_u = (x+.5f)*tile.rcp_fbSize.x;
    //       const float screen_v = (y+.5f)*tile.rcp_fbSize.y;
    //       Ray ray;
    //       camera->initRay(ray,vec2f(screen_u,screen_v));
    //       const vec4f col = traceRayXRay(ray,embreeSceneHandle);
    //       tile.r[frag] = col.x;
    //       tile.g[frag] = col.y;
    //       tile.b[frag] = col.z;
    //       tile.a[frag] = col.w;
    //     }
    //   }
    // }


    OSP_REGISTER_RENDERER(ISPCXRayRenderer,mhtk_xray_ispc);
    OSP_REGISTER_RENDERER(ISPCXRayRenderer,mhtk);
    OSP_REGISTER_RENDERER(ISPCXRayRenderer,xray);
    /*! @} */
  }

};

