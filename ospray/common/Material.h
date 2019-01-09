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

#include "Managed.h"

namespace ospray {

  struct Texture2D;

  /*! \brief helper structure to store a uniform or textured material parameter */
  template <typename T>
  struct MaterialParam
  {
    T factor;
    Texture2D* map;
    affine2f xform;
    linear2f rot;
  };

  using MaterialParam1f = MaterialParam<float>;
  using MaterialParam3f = MaterialParam<vec3f>;

  /*! \brief implements the basic abstraction for anything that is a 'material'.

    Note that different renderers will probably define different materials, so the same "logical" material (such a as a "diffuse gray" material) may look differently */
  struct OSPRAY_SDK_INTERFACE Material : public ManagedObject
  {
    virtual ~Material() override = default;
    virtual std::string toString() const override;
    virtual void commit() override;

    /*! \brief helper function to combine multiple texture transformation parameters

       The following parameters (prefixed with "texture_name.") are combined
       into one transformation matrix:

       name         type   description
       transform    vec4f  interpreted as 2x2 matrix (linear part), column-major
       rotation     float  angle in degree, counterclock-wise, around center (0.5, 0.5)
       scale        vec2f  enlarge texture, relative to center (0.5, 0.5)
       translation  vec2f  move texture in positive direction (right/up)

       The transformations are applied in the given order. Rotation, scale and
       translation are interpreted "texture centric", i.e. their effect seen by
       an user are relative to the texture (although the transformations are
       applied to the texture coordinates).
     */
    affine2f getTextureTransform(const char* texture_name);

    /*! \brief helper function to get a uniform or texture material parameter */
    MaterialParam1f getMaterialParam1f(const char *name, float valIfNotFound);
    MaterialParam3f getMaterialParam3f(const char *name, vec3f valIfNotFound);

    /*! \brief creates an abstract material class of given type

      The respective material type must be a registered material type
      in either ospray proper or any already loaded module. For
      material types specified in special modules, make sure to call
      ospLoadModule first. */
    static Material *createInstance(const char *renderer_type,
                                    const char *material_type);
  };

#define OSP_REGISTER_MATERIAL(renderer_name, InternalClass, external_name)     \
  OSP_REGISTER_OBJECT(::ospray::Material, material, \
                      InternalClass, renderer_name##__##external_name)

} // ::ospray
