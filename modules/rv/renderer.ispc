/*! \file renderer.ispc Defines ISPC side of RV module renderer */

// ospray stuff
#include "ospray/fb/framebuffer.ih"
#include "ospray/camera/perspectivecamera.ih"
#include "ospray/common/ray.ih"
#include "ospray/render/util.ih"
#include "ospray/math/bbox.ih"
// embree stuff
#include "embree2/rtcore_scene.isph"
#include "embree2/rtcore_ray.isph"
#include "embree2/rtcore_geometry.isph"
#include "embree2/rtcore_geometry_user.isph"

/*! describes a layer. *MUST* be identical in data layout to ospray::rv::Layer class */
struct RVLayer {
  vec3f color;
  float lower_z, upper_z;
};

typedef enum {
  RENDER_BY_LAYER,
  RENDER_BY_NET,
  RENDER_BY_ATTRIBUTE
} RenderMode;

struct Resistor {
  box2f  coordinate;   //!< coordinates, in nm
  uint32 layerID;      //!< layer ID that this resistor is in - provides y coords
  uint32 netID;        //!< net that this resistor is in
};

struct ResistorModel {
  uniform Resistor *uniform resistor;
  uniform float    *uniform attribute;

  uniform uint32            numResistors;
  uniform uint32            numAttributesPerResistor;
  uniform uint32            numAttributesTotal; // == numResistors * numAttributesPerResistor...
  uniform int32             geomID;
};

// "inherit" from ospray::ray
struct RVRay {
  Ray inherited;
  // the model we've hit
  const uniform ResistorModel *varying model;
};

struct ColorRange {
  vec3f lo_color;
  float lo_value;
  vec3f hi_color;
  float hi_value;
};

/*! @{ \note These variable(s) are used on both C++ and ISPC
    side. Since ISPC doesn't know about namespaces we have to make
    them global, and add a namespace-like "rv_"-prefix to all
    variables to resolve naming conflicts. Also, for this 'sharing' of
    data to work we have to make *absolutely* sure that the ISPC- and
    C++-side definitions of the respective classes (eg, "Layer" are
    exactly the same) */

//! num attributes per resistor
uniform uint32           rv_numAttributesPerResistor;
//! num layers
uniform uint64           rv_numLayers = -1;
//! list of layers
uniform RVLayer *uniform rv_layer = NULL;
//! camera 
uniform Camera  *uniform rv_camera = NULL;
//! layer enable flags
uniform bool    *uniform rv_layerEnabled = NULL;
//! embree scene
uniform RTCScene         rv_scene = NULL;
//! shade mode
uniform RenderMode       rv_shadeMode = RENDER_BY_LAYER;
//! for shade by attrib mode: ID of attribute to use for shading
uniform uint32           rv_shadeByAttrib_attribID = 0;
//! for shade by attrib mode: color range to be used for shading
uniform ColorRange       rv_shadeByAttrib_colorRange;

/*! @} */



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
                 uniform size_t item      /*< item to intersect */)
{
  varying Ray *uniform ray = (varying Ray *uniform)&_ray;
  const uniform ResistorModel *uniform model
    = (const uniform ResistorModel *uniform )ptr;
  const uniform Resistor &res = model->resistor[item];

  if (!rv_layerEnabled[res.layerID]) return;

  const float x0 = (res.coordinate.lower.x - ray->org.x)*rcpf(ray->dir.x);
  const float y0 = (res.coordinate.lower.y - ray->org.y)*rcpf(ray->dir.y);
  const float z0 = (rv_layer[res.layerID].lower_z - ray->org.z)*rcpf(ray->dir.z);

  const float x1 = (res.coordinate.upper.x - ray->org.x)*rcpf(ray->dir.x);
  const float y1 = (res.coordinate.upper.y - ray->org.y)*rcpf(ray->dir.y);
  const float z1 = (rv_layer[res.layerID].upper_z - ray->org.z)*rcpf(ray->dir.z);

  const float t0 = max(max(ray->t0, 
                           min(x0,x1)),
                       max(min(y0,y1),
                           min(z0,z1)));
  
  const float t1 = min(min(ray->t, 
                           max(x0,x1)),
                       min(max(y0,y1),
                           max(z0,z1)));
  
  if (t0 <= t1)  {
    varying RVRay *uniform rvray = (varying RVRay *uniform)&_ray;
    rvray->model  = model;

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
export void ispc__rv__createModel(uniform uint32 geomID,
                                  uniform uint32 numResistors,
                                  uniform Resistor *uniform resistor,
                                  uniform float *uniform attribute)
{
  uniform ResistorModel *uniform model = uniform new uniform ResistorModel;
  model->numResistors = numResistors;
  model->resistor = resistor;
  model->attribute = attribute;
  rtcSetUserData(rv_scene,geomID,model);
  rtcSetBoundsFunction(rv_scene,geomID,rvGetBounds);
  rtcEnable(rv_scene,geomID);

  rtcSetIntersectFunction(rv_scene,geomID,rvIntersect);
  print("#osp:rv: (ISPC) model created\n");
}

vec4f shade(const RVRay &ray)
{
  if (ray.inherited.primID < 0) return make_vec4f(0.f);

  const uniform ResistorModel *model = ray.model;
  const int resID = ray.inherited.primID;
  const Resistor *res = &model->resistor[resID];

  // first, need a normal: take normal of side where we're closest to
  const vec3f hitPos = ray.inherited.org + ray.inherited.t * ray.inherited.dir;
  const float dist_x = min(abs(hitPos.x-res->coordinate.lower.x),
                           abs(hitPos.x-res->coordinate.upper.x));
  const float dist_y = min(abs(hitPos.y-res->coordinate.lower.y),
                           abs(hitPos.y-res->coordinate.upper.y));
  const float dist_z = min(abs(hitPos.z-rv_layer[res->layerID].lower_z),
                           abs(hitPos.z-rv_layer[res->layerID].upper_z));
  vec3f normal = make_vec3f(1.f,0.f,0.f);
  if (dist_y < min(dist_x,dist_z)) 
    normal = make_vec3f(0.f,1.f,0.f);
  if (dist_z < min(dist_x,dist_y)) 
    normal = make_vec3f(0.f,0.f,1.f);
  const float shadeFactor = 0.3f+0.8f*abs(dot(normal,ray.inherited.dir));

  if (rv_shadeMode == RENDER_BY_LAYER) {
    return shadeFactor*make_vec4f(rv_layer[res->layerID].color); 
  } else if (rv_shadeMode == RENDER_BY_NET) {
    return shadeFactor*make_vec4f(rv_layer[res->netID].color); 
  } else /* RENDER_BY_ATTRIBUTE */ {
    const int attrNo = rv_shadeByAttrib_attribID + resID*rv_numAttributesPerResistor;
    const float attrib = model->attribute[attrNo];
    uniform ColorRange &range = rv_shadeByAttrib_colorRange;
    const float f = (attrib - range.lo_value)*rcpf(range.hi_value-range.lo_value+1e-6f);
    const vec3f col = (1.f-f)*range.lo_color + f*(range.hi_color);
    return shadeFactor*make_vec4f(col); //ray.inherited.primID)); 
  }
}

const uniform float rv_pixel_x[4] = { 0.f, 0.5f, 0.25f, 0.75f };
const uniform float rv_pixel_y[4] = { 0.f, 1.f/3.f, 2.f/3.f, 1.f/9.f };

export void ispc__RVRenderer_renderTile(void *uniform _tile)
{
  uniform Tile   *uniform tile   = (uniform Tile *uniform)_tile;
  uniform Camera *uniform camera = rv_camera; ///(uniform Camera *uniform)_camera;
  tile->format = TILE_FORMAT_RGBA8 | TILE_FORMAT_FLOAT4;
  const uniform uint32 size_x = tile->fbSize.x;
  const uniform uint32 size_y = tile->fbSize.y;
  const uniform uint32 x0 = tile->region.lower.x;
  const uniform uint32 y0 = tile->region.lower.y;
  const float inv_size_x = rcpf(size_x);
  const float inv_size_y = rcpf(size_y);
  for (uniform uint32 i=0;i<TILE_SIZE*TILE_SIZE/programCount;i++) {
    const uint32 frag = i*programCount+programIndex;
    const uint32  x     = x0 + (frag % TILE_SIZE);
    const uint32  y     = y0 + (frag / TILE_SIZE);
    if (x < size_x & y < size_y) {
      RVRay ray;
#if 1
      vec4f col = make_vec4f(0.f);
      for (int i=0;i<4;i++) {
        const float screen_u = (x+rv_pixel_x[i])*inv_size_x;
        const float screen_v = (y+rv_pixel_y[i])*inv_size_y;
        camera->initRay(camera,ray.inherited,make_vec2f(screen_u,screen_v));
        rtcIntersect(rv_scene,*((varying RTCRay *uniform)&ray));
        col = col + shade(ray);
      }
      col = 0.25f * col;
#else
      const float screen_u = (x+.5f)/size_x;
      const float screen_v = (y+.5f)/size_y;
      camera->initRay(camera,ray,make_vec2f(screen_u,screen_v));
      rtcIntersect(rv_scene,*((varying RTCRay *uniform)&ray));
      vec4f col = shade(ray); 
#endif
      tile->r[i] = col.x;
      tile->g[i] = col.y;
      tile->b[i] = col.z;
      tile->a[i] = col.w;
      tile->z[i] = ray.inherited.t;
      tile->rgba8[i] = cvt_uint32(col);
    }
  }  
}

