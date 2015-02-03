#include "Material.isph"
#include "brdfs/Lambertian.isph"
#include "textures/Texture.isph"

struct MatteTextured
{
  Material base;
  vec2f s0;                     //!< Offset for texture coordinates.
  vec2f ds;                     //!< Scaling for texture coordinates.
  uniform Texture* uniform Kd;  //!< Texture mapped to the surface.
};

///////////////////////////////////////////////////////////////////////////////
// Implementation

void MatteTextured__shade(const uniform Material *uniform _this,
                          const Ray&                  ray,    
                          const Medium&               currentMedium, 
                          const DifferentialGeometry& dg,            
                          uniform CompositedBRDF&     brdfs)         
{
  const uniform MatteTextured* uniform this = (const uniform MatteTextured* uniform) _this;
  COMPOSITED_BRDF_ADD(brdfs,Lambertian,
                      this->Kd->get(this->Kd,add(mul(this->ds,dg.st),this->s0)));
}

void MatteTextured__Destructor(uniform RefCount* uniform _this)
{
  uniform MatteTextured* uniform this = (uniform MatteTextured* uniform) _this;
  RefCount__DecRef(&this->Kd->base);
  Material__Destructor(_this);
}

inline void MatteTextured__Constructor(uniform MatteTextured *uniform this,
                                       uniform Texture* uniform Kd,
                                       const uniform vec2f& s0,
                                       const uniform vec2f& ds)
{
  Material__Constructor(&this->base,MatteTextured__Destructor,MatteTextured__shade,NULL);
  RefCount__IncRef(&Kd->base);
  this->Kd = Kd;
  this->s0 = s0;
  this->ds = ds;
}

///////////////////////////////////////////////////////////////////////////////
// External API

export void* uniform MatteTextured__new(void* uniform Kd,
                                        const uniform vec2f& s0,
                                        const uniform vec2f& ds)
{
  uniform MatteTextured *uniform this = uniform new uniform MatteTextured;
  MatteTextured__Constructor(this,(uniform Texture* uniform)Kd,s0,ds);
  return this;
}
