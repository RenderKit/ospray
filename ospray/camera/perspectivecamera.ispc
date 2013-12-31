#include "perspectivecamera.ih"

export { PerspectiveCamera_Data };

typedef void *uniform uniVoidPtr;

void PerspectiveCamera_initRay(uniform Camera *uniform camera,
                               varying Ray &ray,
                               const varying vec2f &sample)
{
  uniform PerspectiveCamera *uniform _this = 
    (uniform PerspectiveCamera *uniform)camera;
  const vec3f org = _this->data.org;
  const vec3f dir = _this->data.dir_00
    + sample.x * _this->data.dir_du
    + sample.y * _this->data.dir_dv;
  setRay(ray,org,dir);
}

/*! create a new ispc-side version of a perspectivecamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void PerspectiveCamera_create(void *uniform cClassPtr,
                                      uniVoidPtr &ispcClassPtr,
                                     uniVoidPtr &ispcDataPtr)
{
  uniform PerspectiveCamera *uniform cam = uniform new uniform PerspectiveCamera;
  cam->inherited.cClassPtr = cClassPtr;
  cam->inherited.initRay = PerspectiveCamera_initRay;
  ispcClassPtr = cam;
  ispcDataPtr = &cam->data;
}


