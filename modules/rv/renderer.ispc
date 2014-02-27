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

// void rvIntersect1(void* uniform ptr,       /*!< pointer to user data */
//                   uniform RTCRay1& ray,     /*!< ray to intersect */
//                   uniform size_t item      /*< item to intersect */)
// {
//   print("intersecting %\n",item);
//   // ray.primID = item;
// }


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
#if 0
  print("bounds %: lo % % % -",item,
        bounds_o.lower_x,
        bounds_o.lower_y,
        bounds_o.lower_z);
  print(" hi % % %\n",
        bounds_o.upper_x,
        bounds_o.upper_y,
        bounds_o.upper_z);
#endif
}

void rvIntersect(void* uniform ptr,       /*!< pointer to user data */
                 varying RTCRay& _ray,     /*!< ray to intersect */
                 // varying Ray &ray,
                 uniform size_t item      /*< item to intersect */)
{
  varying Ray *uniform ray = (varying Ray *uniform)&_ray;
  // print("INTERSECTING %\n",item);
  const uniform ResistorModel *uniform model
    = (const uniform ResistorModel *uniform )ptr;
  const uniform Resistor &res = model->resistor[item];

  const float x0 = (res.coordinate.lower.x - ray->org.x)*rcp(ray->dir.x);
  const float y0 = (res.coordinate.lower.y - ray->org.y)*rcp(ray->dir.y);
  const float z0 = (rv_layer[res.layerID].lower_z - ray->org.z)*rcp(ray->dir.z);

  const float x1 = (res.coordinate.upper.x - ray->org.x)*rcp(ray->dir.x);
  const float y1 = (res.coordinate.upper.y - ray->org.y)*rcp(ray->dir.y);
  const float z1 = (rv_layer[res.layerID].upper_z - ray->org.z)*rcp(ray->dir.z);

  const float t0 = max(max(ray->t0, 
                           min(x0,x1)),
                       max(min(y0,y1),
                           min(z0,z1)));
  
  const float t1 = min(min(ray->t, 
                           max(x0,x1)),
                       min(max(y0,y1),
                           max(z0,z1)));
  
  if (t0 <= t1)  {
    //    ray->geomID = 
    ray->primID = item;
    ray->t = t0;
  }
}


// export void ispc__rv__setIntersectFct(uniform RTCScene scene,
//                                       uniform unsigned int geomID)
// {
//   rtcSetIntersectFunction(scene,geomID,rvIntersect);
// }
export void ispc__rv__createModel(uniform RTCScene embreeScene,
                                  uniform uint32 geomID,
                                  uniform uint32 numResistors,
                                  uniform Resistor *uniform resistor,
                                  uniform float *uniform attribute)
{
  uniform ResistorModel *uniform model = uniform new uniform ResistorModel;
  model->numResistors = numResistors;
  model->resistor = resistor;
  model->attribute = attribute;
  rtcSetUserData(embreeScene,geomID,model);
  rtcSetBoundsFunction(embreeScene,geomID,rvGetBounds);
  rtcEnable(embreeScene,geomID);

  // void *uniform ptr = (void*uniform)&rvIntersect;
  // print("setting intersect fct IN MODULE % %, scene %\n",geomID,(void*uniform)ptr,embreeScene);
  rtcSetIntersectFunction(embreeScene,geomID,rvIntersect);
  // rtcSetIntersectFunction1(embreeScene,geomID,rvIntersect1);
  print("#osp:rv: (ISPC) model created\n");
}

vec4f shade(const Ray &ray)
{
  if (ray.primID < 0) return make_vec4f(0.f);
  return make_vec4f(make_random_color(ray.primID)); 
}

const uniform float rv_pixel_x[4] = { 0.f, 0.5f, 0.25f, 0.75f };
const uniform float rv_pixel_y[4] = { 0.f, 1.f/3.f, 2.f/3.f, 1.f/9.f };

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
  const float inv_size_x = rcp(size_x);
  const float inv_size_y = rcp(size_y);
  for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32 frag = i*programCount+programIndex;
    const uint32  x     = x0 + (frag % TILE_SIZE);
    const uint32  y     = y0 + (frag / TILE_SIZE);
    if (x < size_x & y < size_y) {
      Ray ray;
#if 1
      vec4f col = make_vec4f(0.f);
      for (int i=0;i<4;i++) {
        const float screen_u = (x+rv_pixel_x[i])*inv_size_x;
        const float screen_v = (y+rv_pixel_y[i])*inv_size_y;
        camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
        rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
        col = col + shade(ray);
      }
      col = 0.25f * col;
#else
      const float screen_u = (x+.5f)/size_x;
      const float screen_v = (y+.5f)/size_y;
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
      rtcIntersect(scene,*((varying RTCRay *uniform)&ray));
      vec4f col = shade(ray); 
#endif
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.t;
      tile->rgba8[i] = cvt_uint32(col);
    }
  }  
}

