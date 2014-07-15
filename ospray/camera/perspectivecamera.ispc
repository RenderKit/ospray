/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#include "perspectivecamera.ih"

void PerspectiveCamera_initRay(uniform Camera *uniform camera,
                               varying Ray &ray,
                               const varying CameraSample &sample)
{
  uniform PerspectiveCamera *uniform _this = 
    (uniform PerspectiveCamera *uniform)camera;
  const vec3f org = _this->org;
  const vec3f dir = _this->dir_00
    + sample.screen.x * _this->dir_du
    + sample.screen.y * _this->dir_dv;
  setRay(ray,org,normalize(dir));
}

/*! create a new ispc-side version of a perspectivecamera - with given
    pointer to the c-class version - and return both class pointer and
    pointer to internal data back via the supplied reference
    parameters */
export void *uniform PerspectiveCamera_create(void *uniform cppE)
{
  uniform PerspectiveCamera *uniform cam = uniform new uniform PerspectiveCamera;
  cam->inherited.cppEquivalent = cppE;
  cam->inherited.initRay = PerspectiveCamera_initRay;
  cam->inherited.doesDOF = false;
  return cam;
}

export void PerspectiveCamera_set(void *uniform _camera, 
                                  const uniform vec3f &org,
                                  const uniform vec3f &dir_00,
                                  const uniform vec3f &dir_du,
                                  const uniform vec3f &dir_dv)
{
  uniform PerspectiveCamera *uniform camera = (uniform PerspectiveCamera *uniform)_camera;
  camera->org    = org;
  camera->dir_00 = dir_00;
  camera->dir_du = dir_du;
  camera->dir_dv = dir_dv;
}


// #include "perspectivecamera.ih"

// export { PerspectiveCamera_Data };

// typedef void *uniform uniVoidPtr;

// void PerspectiveCamera_initRay(uniform Camera *uniform camera,
//                                varying Ray &ray,
//                                const varying vec2f &screen,
//                                const varying vec2f &lens
//                                )
// {
//   uniform PerspectiveCamera *uniform _this = 
//     (uniform PerspectiveCamera *uniform)camera;
//   const vec3f org = _this->data.org;
//   const vec3f dir = _this->data.dir_00
//     + screen.x * _this->data.dir_du
//     + screen.y * _this->data.dir_dv;
//   setRay(ray,org,normalize(dir));
// }

// /*! create a new ispc-side version of a perspectivecamera - with given
//     pointer to the c-class version - and return both class pointer and
//     pointer to internal data back via the supplied reference
//     parameters */
// export void PerspectiveCamera_create(void *uniform cClassPtr,
//                                       uniVoidPtr &ispcClassPtr,
//                                      uniVoidPtr &ispcDataPtr)
// {
//   uniform PerspectiveCamera *uniform cam = uniform new uniform PerspectiveCamera;
//   cam->inherited.cClassPtr = cClassPtr;
//   cam->inherited.initRay = PerspectiveCamera_initRay;
//   ispcClassPtr = cam;
//   ispcDataPtr = &cam->data;
// }


