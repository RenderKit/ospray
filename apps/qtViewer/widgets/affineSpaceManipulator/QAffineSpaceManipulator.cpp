// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// viewer
#include "QAffineSpaceManipulator.h"
#include "HelperGeometry.h"
#include <cmath>

#if __APPLE__
# include "glut.h"
#else
# include "GL/glut.h"
#endif

namespace ospray {
  namespace viewer {

    //! modifier to be used for detecting clicks as 'pick' events
    const Qt::KeyboardModifier pickModifier   = Qt::ShiftModifier;
    //! modifier to be used for detecting clicks as 'strafe' (left/right/up/down) events
    const Qt::KeyboardModifier strafeModifier = Qt::ControlModifier;
    //! modifier to be used for detecting clicks as 'move' (forward/backward) events
    const Qt::KeyboardModifier moveModifier   = Qt::AltModifier;

    //! helper function that projects a given screen coordinate to a virtual sphere of given radius
    vec3f projectToSphere(QPoint pt, vec2i size, float radius)
    {
      int h = std::min(size.x,size.y);
      float fx = (pt.x()-size.x/2.f)/float(h/2.f) / radius;
      float fz = -(pt.y()-size.y/2.f)/float(h/2.f) / radius;
      float fy = 0.f;
      float r = 1.f - fx*fx - fz*fz;
      if (r >= 0.f)
        fy = -sqrtf(r);
      vec3f spherePos =  normalize(vec3f(fx,fy,fz));
      return spherePos;
    }

    QAffineSpaceManipulator::ReferenceFrame::ReferenceFrame()
      : sourcePoint(0,-4,0),
        targetPoint(0,0,0),
        upVector(0,1,0),
        orientation(ospcommon::one)
    {}
        
    void QAffineSpaceManipulator::ReferenceFrame::snapUp()
    {
      if (dot(upVector,upVector) <= 1e-6f) return;
      // vy stays
      linear3f old_orientation = orientation;
      orientation.vx = cross(orientation.vy,upVector);
      orientation.vz = cross(orientation.vx,orientation.vy);
      if (std::isnan(reduce_add(orientation.vx+orientation.vy)))
        orientation = old_orientation;
    }

    QAffineSpaceManipulator::QAffineSpaceManipulator(InteractionMode interactionMode) 
      : size(-1,-1),
        frame(new ReferenceFrame),
        //        interactionMode(QAffineSpaceManipulator::FREE_ROTATION),
        interactionMode(interactionMode),
        motionSpeed(0.f)
    {}

    //! the QT callback that tells us that we have to redraw
    void QAffineSpaceManipulator::paintGL()
    { 
      redraw(); 
    }

    //! the QT callback that tells us that the image got resize
    void QAffineSpaceManipulator::resizeGL(int width, int height)
    {
      glViewport(0, 0, width, height);
      size = vec2i(width,height);
      resize(width,height);
    }

    void QAffineSpaceManipulator::mousePressEvent(QMouseEvent * event)
    {
      lastMousePos = event->pos();
      orientationWhenMouseGotPressed = frame->orientation;
      if (event->buttons() & Qt::LeftButton &&
          event->modifiers() & pickModifier) {
        pick(event);
      }
    }
    void QAffineSpaceManipulator::mouseReleaseEvent(QMouseEvent * event)
    {
      lastMousePos = event->pos();
    }

    inline std::ostream &operator<<(std::ostream &o, const QPoint &q)
    { o << "(" << q.x() << "," << q.y() << ")"; return o; }


    void QAffineSpaceManipulator::strafe(QMouseEvent * event)
    {
      QPoint newPos = event->pos();
      
      const vec3f moveAxisU = frame->orientation.vx;
      const float moveDistU = - 2 * motionSpeed * (newPos.x()-lastMousePos.x()) / float(size.x);
      const vec3f moveAxisV = frame->orientation.vz;
      const float moveDistV = + 2 * motionSpeed * (newPos.y()-lastMousePos.y()) / float(size.y);
      const vec3f moveVec = moveDistU * moveAxisU + moveDistV * moveAxisV;
          
      frame->sourcePoint += moveVec;
      frame->targetPoint += moveVec;

      lastMousePos = event->pos();
      emit affineSpaceChanged(this);
      updateGL();
    }

    /*! rotate around target point, by given angles */
    void QAffineSpaceManipulator::rotateAroundTarget(float angle_x, float angle_y)
    {
      // axes we're rotating in u and v direction, respectively.
      const vec3f uRotationAxis = frame->orientation.vz;
      const vec3f vRotationAxis = frame->orientation.vx;
      const float rotSpeed = 2.f;
      const float du = - angle_x / 300.f;
      const float dv = - angle_y / 300.f;
      linear3f rot
        = linear3f::rotate(vRotationAxis,dv)
        * linear3f::rotate(uRotationAxis,du);
      frame->orientation = rot * frame->orientation;
      frame->snapUp();
      const vec3f vecToTarget = frame->targetPoint - frame->sourcePoint;
      if (interactionMode == FLY) {
        // in FLY mode, the SOURCE point stays, and the target point rotates
        frame->targetPoint = frame->sourcePoint + xfmVector(rot,vecToTarget);
      } else {
        assert(interactionMode == INSPECT);
        frame->sourcePoint = frame->targetPoint - xfmVector(rot,vecToTarget);
      }
    }
    
    void QAffineSpaceManipulator::rotate(QMouseEvent * event)
    {
      QPoint newPos = event->pos();
      
      switch (interactionMode) {
      case FLY: 
      case INSPECT: {
        // axes we're rotating in u and v direction, respectively.
        const vec3f uRotationAxis = frame->orientation.vz;
        const vec3f vRotationAxis = frame->orientation.vx;
        const float rotSpeed = 2.f;
        const float du = - rotSpeed * (newPos.x()-lastMousePos.x()) / float(size.x);
        const float dv = - rotSpeed * (newPos.y()-lastMousePos.y()) / float(size.y);
        linear3f rot
          = linear3f::rotate(vRotationAxis,dv)
          * linear3f::rotate(uRotationAxis,du);
        frame->orientation = rot * frame->orientation;
        frame->snapUp();
        const vec3f vecToTarget = frame->targetPoint - frame->sourcePoint;
        if (interactionMode == FLY) {
          // in FLY mode, the SOURCE point stays, and the target point rotates
          frame->targetPoint = frame->sourcePoint + xfmVector(rot,vecToTarget);
        } else {
          assert(interactionMode == INSPECT);
          frame->sourcePoint = frame->targetPoint - xfmVector(rot,vecToTarget);
        }
      } break;
      case FREE_ROTATION: {
        vec3f oldVec = normalize(projectToSphere(lastMousePos,size,1.f));
        vec3f newVec = normalize(projectToSphere(newPos,size,1.f));
        oldVec = xfmVector((orientationWhenMouseGotPressed),oldVec);
        newVec = xfmVector((orientationWhenMouseGotPressed),newVec);

        vec3f rotAxis = cross(oldVec,newVec);
        if (dot(rotAxis,rotAxis) > 1e-6f) {
          rotAxis = normalize(rotAxis);
          float rotAngle = - acosf(dot(oldVec,newVec));
          linear3f rot = linear3f::rotate(rotAxis,rotAngle);
                
          frame->orientation = frame->orientation * rot;
          const vec3f vecToTarget = frame->targetPoint - frame->sourcePoint;
          frame->sourcePoint = frame->targetPoint - xfmVector(rot,vecToTarget);
        }

      } break;
      default:
        std::cout << "no rotation implemented for this interaction mode" << std::endl;
      };
      lastMousePos = event->pos();
      emit affineSpaceChanged(this);
      updateGL();
    }

    void QAffineSpaceManipulator::move(QMouseEvent * event)
    {
      QPoint newPos = event->pos();
      
      const vec3f moveAxis = frame->orientation.vy;
      float moveDistance = - 10 * motionSpeed * (newPos.y()-lastMousePos.y()) / float(size.y);
      switch (interactionMode) {
      case FLY: {
        /* fly mode: move BOTH source and target positions
           forward/backward along move axis. Since we are moving
           *forward* with mouse, we move in POSITIVE y distance */
        frame->sourcePoint -= moveDistance * moveAxis;
        frame->targetPoint -= moveDistance * moveAxis;
      } break;
      case FREE_ROTATION: 
      case INSPECT: {
        /* inspect mode: pull or push target towards/away from
           viewer; but at most to a given minimum distance (can't
           pull behind us). Since we are PULLING the target
           TOWARDS us, we move with NEGATIVE distance */
        moveDistance = -moveDistance;
        const float targetDistance = length(frame->targetPoint - frame->sourcePoint);
        const float maxAllowedDistance = targetDistance - .1f * motionSpeed;
        if (moveDistance > maxAllowedDistance) moveDistance = maxAllowedDistance;
        frame->sourcePoint += moveDistance * moveAxis;
      } break;
      }
      lastMousePos = event->pos();
      emit affineSpaceChanged(this);
      updateGL();
    }


    //! switch to requested interaction mode
    void QAffineSpaceManipulator::setInteractionMode(QAffineSpaceManipulator::InteractionMode interactionMode)
    { this->interactionMode = interactionMode; }

    /*! toggle-up: switch to given axis (x=0,y=1,z=2) as up-vectors for the rotation.
      when _already_ in up-vector mode for this axis, switch to negative axis. */
    void QAffineSpaceManipulator::toggleUp(int axis)
    {
      vec3f newUp = vec3f(axis==0,axis==1,axis==2);
      if (frame->upVector == newUp)
        newUp = - newUp;
      frame->upVector = newUp;
      frame->snapUp();
    }



    void QAffineSpaceManipulator::mouseMoveEvent(QMouseEvent * event)
    {
      // ==================================================================
      //                          P I C K I N G                            
      // ==================================================================
      if (
          // shift-left-click:
          (event->buttons() & Qt::LeftButton) && (event->modifiers() & pickModifier)
          ) 
        pick(event);
      
      // ==================================================================
      //     S T R A F I N G - straft left/right/up/down in image plane                            
      // ==================================================================
      if (
          // drag middle button (for mice that have one)
          (event->buttons() & Qt::MidButton)
          ||
          // drag mouse with control pressed (for mac-style trackpads)
          ((event->buttons() & Qt::LeftButton) && (event->modifiers() & strafeModifier))
          ) 
        strafe(event);

      // ==================================================================
      //      M O V I N G   -   move forward/backward along camera axis
      // ==================================================================
      if (
          // drag right button (for mice that have one)
          (event->buttons() & Qt::RightButton)
          ||
          // drag mouse with alt-button pressed (for mac-style trackpads)
          ((event->buttons() & Qt::LeftButton) && (event->modifiers() & moveModifier))
          ) 
        move(event);

      // -------------------------------------------------------
      // no modifiers: plain ol' rotate 
      // -------------------------------------------------------
      rotate(event);
      return;
    }


    void QCoordAxisFrameEditor::redraw() 
    {
      static CoordFrameGeometry coordFrameGeometry;
      const vec3f color_bg(.5f);
      // clear background
      glClearColor(color_bg.x,color_bg.y,color_bg.z,0.f);
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      
      glEnable( GL_LIGHTING );

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_LIGHT0);
      glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE );
      GLfloat lightPos[] = {-.2f, -2.f, 1.0f, 0.0f};
      glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
      GLfloat ambient[] =  {0.4f, 0.4f, 0.4f, 1.0f};
      GLfloat diffuse[] =  {1.f, 1.f, 1.0f, 1.0f};
      glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

      glColor3f(1,1,1);

      glViewport(0, 0, (GLsizei) size.x, (GLsizei) size.y);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      gluPerspective(30, (GLfloat) size.x/(GLfloat) size.y, 1.0, 100.0);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      // gluLookAt(0, -5, 0, 0, 0, 0, 0, 0, 1);
      vec3f up = frame->orientation.vz;
      gluLookAt(frame->sourcePoint.x,frame->sourcePoint.y,frame->sourcePoint.z,
                frame->targetPoint.x,frame->targetPoint.y,frame->targetPoint.z,
                up.x,up.y,up.z);

      for (int axisID=0;axisID<3;axisID++) {
        CoordFrameGeometry::Mesh &mesh = coordFrameGeometry.arrow[axisID];
        GLfloat ambient[3], diffuse[3], specular[3];
        for (int j=0;j<3;j++) {
          ambient[j] = .2f*mesh.color[j];
          diffuse[j] = .9f*mesh.color[j];
          specular[j] = .2;
        }
        glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,ambient);
        glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,diffuse);
        glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,specular);

        glBegin(GL_TRIANGLES);
        for (int i=0;i<mesh.index.size();i++) {
          vec3i idx = mesh.index[i];
          glNormal3fv(&mesh.normal[idx.x].x);
          glVertex3fv(&mesh.vertex[idx.x].x);
          glNormal3fv(&mesh.normal[idx.y].x);
          glVertex3fv(&mesh.vertex[idx.y].x);
          glNormal3fv(&mesh.normal[idx.z].x);
          glVertex3fv(&mesh.vertex[idx.z].x);
        }
        glEnd();
      }
      glFlush();
    }


    QCheckeredBallFrameEditor::QCheckeredBallFrameEditor() 
      : color_bg(.5f), color_bright(.8f), color_dark(.2f)
    {
    }

    void QCheckeredBallFrameEditor::redraw()
    {
      static const int Nx = 64;
      static const int Ny = 64;
      static vec3f vtx[Nx][Ny];
      static vec3f col[Nx][Ny];
      static vec2f txt[Nx][Ny];
      static bool verticesGenerated = false;
      if (!verticesGenerated) {
        for (int x=0;x<Nx;x++) {
          for (int y=0;y<Ny;y++) {
            const float t = (y+0.f)/Ny*2.f*M_PI;
            const float f = (x+0.f)/Nx*2.f*M_PI;
            vtx[x][y] = vec3f(cos(t)*sin(f),sin(t)*sin(f),cos(f));
            txt[x][y] = vec2f(x/float(Nx),y/float(Ny));
            vec3f c = (((x/4)+(y/4))%2) ? color_bright : color_dark;
            col[x][y] = vec3f(c);
          }
        }
        verticesGenerated = true;
      }
      
      // clear background
      glClearColor(color_bg.x,color_bg.y,color_bg.z,0.f);
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      
      glEnable( GL_LIGHTING );
      glEnable( GL_CULL_FACE );

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_LIGHT0);
      glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE );
      //glEnable(GL_COLOR_MATERIAL);
      GLfloat lightPos[] = {-1.f, 1.f, 1.0f, 0.0f};
      glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
      GLfloat ambient[] =  {0.2f, 0.2f, 0.2f, 1.0f};
      GLfloat diffuse[] =  {1.f, 1.f, 1.0f, 1.0f};
      glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
      glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

      glColor3f(1,1,1);

      glViewport(0, 0, (GLsizei) size.x, (GLsizei) size.y);
      glMatrixMode(GL_PROJECTION);
      glLoadIdentity();
      gluPerspective(30, (GLfloat) size.x/(GLfloat) size.y, 1.0, 100.0);
      glMatrixMode(GL_MODELVIEW);
      glLoadIdentity();
      gluLookAt(0, 5, 0, 0, 0, 0, 0, 0, 1);
      // gluLookAt(0, 0, 5, 0, 0, 0, 0, 1, 0);


      float matrix[4][4];

      for (int y=0;y<4;y++)
        for (int x=0;x<4;x++)
          matrix[x][y] = (x==y);
      for (int j=0;j<3;j++) {
        matrix[0][j] = frame->orientation.vx[j];
        matrix[1][j] = frame->orientation.vy[j];
        matrix[2][j] = frame->orientation.vz[j];
      }
      glMultMatrixf(&matrix[0][0]);
      //        glPushMatrix();



      glBegin(GL_QUADS);
      for (int x=0;x<Nx;x++)
        for (int y=0;y<Ny;y++) {
          glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,&col[(x+0)%Nx][(y+0)%Ny].x);
          glVertex3fv(&vtx[(x+0)%Nx][(y+0)%Ny].x);
          glVertex3fv(&vtx[(x+1)%Nx][(y+0)%Ny].x);
          glVertex3fv(&vtx[(x+1)%Nx][(y+1)%Ny].x);
          glVertex3fv(&vtx[(x+0)%Nx][(y+1)%Ny].x);
        }
      glEnd();
      glDisable( GL_LIGHTING );
      glDisable( GL_CULL_FACE );
      //      updateGL();
    }

  } // ::viewer
} // ::ospray

