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

#include "VolumeViewer.h"
#include "OpenGLAnnotationRenderer.h"

#ifdef __APPLE__
  #include <OpenGL/gl.h>
#else
  #include <GL/gl.h>
#endif

void OpenGLAnnotationRenderer::render()
{
  glPushAttrib(GL_ENABLE_BIT);
  glEnable(GL_DEPTH_TEST);

  // render volume bounding box
  ospcommon::box3f boundingBox = volumeViewer->getBoundingBox();

  ospcommon::vec3f size(boundingBox.upper - boundingBox.lower);

  glPushMatrix();
  glTranslatef(boundingBox.lower.x, boundingBox.lower.y, boundingBox.lower.z);
  glScalef(size.x, size.y, size.z);

  glColor4f(0.5f, 0.5f, 0.5f, 1.f);

  glBegin(GL_LINES);

  float v1[3], v2[3];

  for (int dim1=0; dim1<3; dim1++) {
    int dim2 = (dim1+1) % 3;
    int dim3 = (dim1+2) % 3;

    for (int i=0; i<=1; i++) {
      for (int j=0; j<=1; j++) {
        v1[dim1] = 0.f;
        v2[dim1] = 1.f;
        v1[dim2] = v2[dim2] = i;
        v1[dim3] = v2[dim3] = j;

        glVertex3fv(v1);
        glVertex3fv(v2);
      }
    }
  }

  glEnd();

  glPopMatrix();

  glPopAttrib();
}
