/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \defgroup ospray_render_obj "Wavefront OBJ" Material-based Renderer

  \brief Implements the Wavefront OBJ Material Model
  
  \ingroup ospray_supported_renderers

  This renderer implementes a shading model roughly based on the
  Wavefront OBJ material format. In particular, this renderer
  implements the Material model as implemented in \ref
  ospray::OBJMaterial, and implements a recursive ray tracer on top of
  this mateiral model.

  Note that this renderer is NOT fully compatible with the Wavefront
  OBJ specifications - in particular, we do not support the different
  illumination models as specified in the 'illum' field, and always
  perform full ray tracing with the given material parameters.

*/

namespace ospray {
};
