/*! \file renderer.ispc Defines ISPC side of RV module renderer */

// ospray stuff
#include "fb/framebuffer.ih"
#include "camera/perspectivecamera.ih"
#include "common/ray.ih"
#include "render/util.ih"
#include "common/math/bbox.ih"
// embree stuff
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_ray.isph"
#include "embree2/rtcore_geometry.isph"
#include "embree2/rtcore_geometry_user.isph"

void rvIntersect(void* uniform ptr,       /*!< pointer to user data */
                  varying RTCRay& ray,     /*!< ray to intersect */
                  uniform size_t item      /*< item to intersect */)
{
  print("intersecting %\n",item);
  ray.primID = item;
}

struct Layer {
  vec3f color;
  float lower_z, upper_z;
};

uniform Layer *uniform rv_layer;

struct Resistor {
  box2f  coordinate;   //!< coordinates, in nm
  uint32 layerID;      //!< layer ID that this resistor is in - provides y coords
  uint32 netID;        //!< net that this resistor is in
};

struct ResistorModel {
  uniform uint32 numResistors;
  uniform Resistor *uniform resistor;
  uniform float *uniform attribute;
};


void rvGetBounds(void* uniform ptr,              /*!< pointer to user data */
                 uniform size_t item,            /*!< item to calculate bounds for */
                 uniform RTCBounds &bounds_o     /*!< returns calculated bounds */)
{
  const uniform ResistorModel *uniform model
    = (const uniform ResistorModel *uniform )ptr;
  const uniform Resistor &res = model->resistor[item];
  bounds_o.lower_x = res.coordinate.lower.x;
  bounds_o.lower_y = res.coordinate.lower.y;
  bounds_o.upper_x = res.coordinate.upper.x;
  bounds_o.upper_y = res.coordinate.upper.y;
  
  bounds_o.lower_z = rv_layer[res.layerID].lower_z;
  bounds_o.upper_z = rv_layer[res.layerID].upper_z;
}

// export void ispc__rv__setIntersectFct(uniform RTCScene scene,
//                                       uniform unsigned int geomID)
// {
//   rtcSetIntersectFunction(scene,geomID,rvIntersect);
// }
export void ispc__rv__createModel(uniform RTCScene embreeScene,
                                  uniform uint32 ID,
                                  uniform uint32 numResistors,
                                  uniform Resistor *uniform resistor,
                                  uniform float *uniform attribute)
{
}

export void ispc__RVRenderer_renderTile(void *uniform _tile, 
                                        void *uniform _camera, 
                                        uniform RTCScene scene)
{
  uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;
  uniform Camera *uniform camera = (uniform Camera *uniform)_camera;
  tile->format = TILE_FORMAT_RGBA8 | TILE_FORMAT_FLOAT4;
  const uniform uint32 size_x = tile->fbSize.x;
  const uniform uint32 size_y = tile->fbSize.y;
  const uniform uint32 x0 = tile->region.lower.x;
  const uniform uint32 y0 = tile->region.lower.y;
  for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32 frag = i*programCount+programIndex;
    const uint32  x     = x0 + (frag % TILE_SIZE);
    const uint32  y     = y0 + (frag / TILE_SIZE);
    if (x < size_x & y < size_y) {
      const float screen_u = (x+.5f)/size_x;
      const float screen_v = (y+.5f)/size_y;
      Ray ray;
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
      // ISPC issue #703. Switch to 'nice' code once ISPC #703 is fixed.
      // print("ray.dir % % %\n",ray.dir.x,ray.dir.y,ray.dir.z);


      // print("org_x %\n",ray.org.x);
      // print("org_y %\n",ray.org.y);
      // print("org_z %\n",ray.org.z);
      // print("dir_x %\n",ray.dir.x);
      // print("dir_y %\n",ray.dir.y);
      // print("dir_z %\n",ray.dir.z);

#if 0
      rtcIntersect(scene,(varying RTCRay&)ray);
#else
      rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
#endif
      const vec4f col = make_vec4f(0.f);
      if (ray.primID >= 0) {
        print("hit %\n",ray.primID);
        make_vec4f(make_random_color(ray.primID)); 
      }
      // if (ray.geomID >= 0)
      //   print("col % % %\n",col.x,col.y,col.z);
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.t;
      tile->rgba8[i] = cvt_uint32(col);
    }
  }  
}

