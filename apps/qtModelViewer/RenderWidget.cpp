/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

// viewer
#include "RenderWidget.h"
#include <glut.h>
#include <sys/times.h>

namespace ospray {
  namespace viewer {

    //! helper function that projects a given screen coordinate to a virtual sphere of given radius
    vec3f projectToSphere(QPoint pt, vec2i size, float radius)
    {
      PRINT(pt.x()); PRINT(pt.y());
      int h = std::min(size.x,size.y);
      float fx = (pt.x()-size.x/2.f)/float(h/2.f) / radius;
      float fy = ((size.y-pt.y())-size.y/2.f)/float(h/2.f) / radius;
      float fz = 0.f;
      float r = 1.f - fx*fx - fy*fy;
      if (r >= 0.f)
        fz = sqrtf(r);
      return normalize(vec3f(fx,-fz,fy));
    }

    RotationWidget::Camera::Camera()
      : from(0,-4,0),
        at(0,0,0),
        up(0,0,1)
    {
      frame = embree::one;
    }
        
    RotationWidget::RotationWidget() 
      : size(-1,-1),
        camera(new Camera),
        //        interactionMode(RotationWidget::FREE_ROTATION)
        interactionMode(RotationWidget::INSPECT)
    {}

    void RotationWidget::cameraChanged(RotationWidget *which) { PING; }

    //! the QT callback that tells us that we have to redraw
    void RotationWidget::paintGL()
    { redraw(); }

    //! the QT callback that tells us that the image got resize
    void RotationWidget::resizeGL(int width, int height)
    { 
      size = vec2i(width,height);
      resize(width,height);
    }

    const Qt::KeyboardModifier moveModifier = Qt::AltModifier;
    const Qt::KeyboardModifier pickModifier = Qt::ShiftModifier;
    const Qt::KeyboardModifier strafeModifier = Qt::ControlModifier;

    void RotationWidget::mousePressEvent(QMouseEvent * event)
    {
      lastMousePos = event->pos();
      startDragPos = event->pos();
      struct tms t;
      startDragTime = times(&t);
      
      if (event->buttons() & Qt::LeftButton &&
          event->modifiers() & pickModifier) {
        pick(event);
      }
    }
    void RotationWidget::mouseReleaseEvent(QMouseEvent * event)
    {
      PING;
      lastMousePos = event->pos();
    }

    inline std::ostream &operator<<(std::ostream &o, const QPoint &q)
    { o << "(" << q.x() << "," << q.y() << ")"; return o; }

    void RotationWidget::mouseMoveEvent(QMouseEvent * event)
    {
      // PRINT((int*)(int)event->buttons());
      // PRINT((int*)(int)event->modifiers());
      QPoint newPos = event->pos();
      if (event->buttons() & Qt::LeftButton) {
        // mouse was draggd with left button pressed.
        if (event->modifiers() & pickModifier) {
          // picking - do nothing; pick events are handled in *clicking*, not in *dragging*.
        } else if (event->modifiers() & Qt::AltModifier) {
          std::cout << "ALT " << newPos << std::endl;
        } else if (event->modifiers() & Qt::ControlModifier) {
          std::cout << "CONTROL " << newPos << std::endl;
        } else if (event->modifiers() & Qt::MetaModifier) {
          std::cout << "META " << newPos << std::endl;
        } else {
          // -------------------------------------------------------
          // no modifiers: plain ol' rotate 
          // -------------------------------------------------------
          float du = (newPos.x()-lastMousePos.x()) / float(size.x);
          float dv = (newPos.y()-lastMousePos.y()) / float(size.y);
          switch (interactionMode) {
          case FLY:
          case INSPECT: {
            float rotSpeed = 2.f;
            linear3f rot
              = linear3f::rotate(vec3f(1,0,0),rotSpeed*dv)
              * linear3f::rotate(vec3f(0,0,1),rotSpeed*du);
            camera->frame = rot * camera->frame;
          } break;
          case FREE_ROTATION: {
            if (newPos != lastMousePos) {
            vec3f oldVec = normalize(projectToSphere(lastMousePos,size,.9f));
            vec3f newVec = normalize(projectToSphere(newPos,size,.9f));
            PRINT(oldVec);
            PRINT(newVec);
            linear3f oldFrame, newFrame;
            vec3f cr = cross(oldVec,newVec);
            if (dot(cr,cr) > 1e-6f) {
              oldFrame.vz = newFrame.vz = normalize(cr);
              oldFrame.vx = normalize(oldVec);
              newFrame.vx = normalize(newVec);
              oldFrame.vy = cross(oldFrame.vz,oldFrame.vx);
              newFrame.vy = cross(newFrame.vz,newFrame.vx);
              linear3f rot = newFrame * rcp(oldFrame);
              camera->frame = rot * camera->frame;

              affine3f rotFrom = affine3f::translate(-camera->at) * affine3f(rot) * affine3f::translate(camera->at);
              camera->from = xfmPoint(rotFrom,camera->from);
            }
            }
          } break;
          default:
            std::cout << "no rotation implemented for this interaction mode" << endl;
          };
          updateGL();
        }
      }
      lastMousePos = event->pos();
    }


    struct CoordFrameGeometry {
      struct Quad {
        vec3f vtx[4], nor[4];
      };
      struct Mesh {
        vec3f color;
        //      std::vector<vec3fa> color;
        std::vector<vec3fa> vertex;
        std::vector<vec3fa> normal;
        std::vector<vec3i>  index;
        
        void addQuad(const affine3f &xfm, const Quad &quad)
        {
          vec3i idx0 = vec3i(vertex.size());
          for (int i=0;i<4;i++) {
            vertex.push_back(vec3fa(xfmPoint(xfm,quad.vtx[i])));
            normal.push_back(vec3fa(normalize(xfmVector(xfm,quad.nor[i]))));
          }
          index.push_back(idx0+vec3i(0,1,2));
          index.push_back(idx0+vec3i(0,2,3));
        }
      };
      
      Mesh arrow[3];
      float shaftThickness;
      float headLength;
      int numSegments;
      
      CoordFrameGeometry()
        : shaftThickness(.1), headLength(.2f), numSegments(16)
      {
        makeArrow(arrow[0],vec3f(1,0,0));
        makeArrow(arrow[1],vec3f(0,1,0));
        makeArrow(arrow[2],vec3f(0,0,1));
      }
      void makeArrow(Mesh &mesh,const vec3f &axis) 
      {
        PING;
        mesh.color = axis;
        affine3f xfm = embree::frame(axis);
        std::swap(xfm.l.vx,xfm.l.vz);
        PRINT(xfm);
        Quad quad; 
        for (int i=0;i<numSegments;i++) {
          const float f0 = (i+0.f)/numSegments * 2.f * M_PI;
          const float f1 = (i+1.f)/numSegments * 2.f * M_PI;
          const float u0 = cosf(f0), u1 = cosf(f1);
          const float v0 = sinf(f0), v1 = sinf(f1);
          
          quad.vtx[0] = vec3f(0,shaftThickness*u0,shaftThickness*v0);
          quad.vtx[1] = vec3f(1.f-headLength,shaftThickness*u0,shaftThickness*v0);
          quad.vtx[2] = vec3f(1.f-headLength,shaftThickness*u1,shaftThickness*v1);
          quad.vtx[3] = vec3f(0,shaftThickness*u1,shaftThickness*v1);
          quad.nor[0] = quad.nor[1] = vec3f(0,u0,v0);
          quad.nor[2] = quad.nor[3] = vec3f(0,u1,v1);
          mesh.addQuad(xfm,quad);

          quad.vtx[0] = vec3f(1.f-headLength,headLength*u0,headLength*v0);
          quad.vtx[3] = vec3f(1.f-headLength,headLength*u1,headLength*v1);
          quad.nor[0] = quad.nor[1] = quad.nor[2] = quad.nor[3] = vec3f(-1,0,0);
          mesh.addQuad(xfm,quad);

          quad.vtx[1] = quad.vtx[2] = vec3f(1.f,0,0);
          quad.nor[0] = quad.nor[1] = vec3f(+1,u0,v0);
          quad.nor[2] = quad.nor[3] = vec3f(+1,u1,v1);
          mesh.addQuad(xfm,quad);
        }
      }
    };


    void CoordFrameRotationEditor::redraw() 
    {
      PING;
      static CoordFrameGeometry coordFrameGeometry;
      PING;
      const vec3f color_bg(.5f);
      // clear background
      glClearColor(color_bg.x,color_bg.y,color_bg.z,0.f);
      glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
      
      glEnable( GL_LIGHTING );
      //      glEnable( GL_CULL_FACE );

      glEnable(GL_DEPTH_TEST);
      glEnable(GL_LIGHT0);
      glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE );
      //glEnable(GL_COLOR_MATERIAL);
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
      vec3f up = camera->frame.vz;
      gluLookAt(camera->from.x,camera->from.y,camera->from.z,
                camera->at.x,camera->at.y,camera->at.z,
                up.x,up.y,up.z);
      PING;

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
          vec3f idx = mesh.index[i];
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


    CheckeredSphereRotationEditor::CheckeredSphereRotationEditor() 
      : color_bg(.5f), color_bright(.8f), color_dark(.2f)
    {
    }

    void CheckeredSphereRotationEditor::redraw()
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
          matrix[0][j] = camera->frame.vx[j];
          matrix[1][j] = camera->frame.vy[j];
          matrix[2][j] = camera->frame.vz[j];
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

    OSPRayRenderWidget::OSPRayRenderWidget()
      : ospRenderer(NULL), ospModel(NULL), ospCamera(NULL), ospFrameBuffer(NULL)
    {
      float coords[4][4] = { { 0.f,0.f,0.f,0.f }, { 1.f,0.f,0.f,0.f }, { 0.f,1.f,0.f,0.f }, { 0.f,0.f,1.f,0.f } };
      OSPData colorData = ospNewData(4,OSP_FLOAT4,coords);
      OSPData coordData = ospNewData(4,OSP_FLOAT4,coords);
      int   index[3][3]  = { { 1,2,0 }, { 2,3,0 }, { 3,1,0 } };
      OSPData indexData = ospNewData(3,OSP_INT3,index);

      OSPGeometry triMesh = ospNewTriangleMesh();
      assert(triMesh);
      ospSetData(triMesh,"vertex",coordData);
      ospSetData(triMesh,"vertex.color",coordData);
      ospSetData(triMesh,"index",indexData);
      ospCommit(triMesh);
      
      ospCamera = ospNewCamera("perspective");
      updateOSPRayCamera();

      ospModel = ospNewModel();
      assert(ospModel);
      ospAddGeometry(ospModel,triMesh);
      ospCommit(ospModel);

      //const char *rendererType = "eyeLight";
      const char *rendererType = "eyeLight_vertexColor";
      // const char *rendererType = "testFrame";
      ospRenderer = ospNewRenderer(rendererType);
      assert(ospRenderer);
      if (!ospRenderer)
        throw std::runtime_error("#ospQTV: could not create renderer");

      ospSetObject(ospRenderer,"world",ospModel);
      ospSetObject(ospRenderer,"camera",ospCamera);
      ospCommit(ospRenderer);
    }
    
    //! the QT callback that tells us that we have to redraw
    void OSPRayRenderWidget::redraw()
    { 
      if (!ospFrameBuffer) return;
      if (!ospRenderer) return;
      
      updateOSPRayCamera();
      ospRenderFrame(ospFrameBuffer, ospRenderer);
      
      uint32 *mappedFB = (unsigned int *)ospMapFrameBuffer(ospFrameBuffer);      
      glDrawPixels(size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, mappedFB);      
      ospUnmapFrameBuffer(mappedFB, ospFrameBuffer);
    }
    
    //! the QT callback that tells us that the image got resize
    void OSPRayRenderWidget::resize(int width, int height)
    { 
      // RotationWidget::resizeGL(width,height);
      if (ospFrameBuffer)
        ospFreeFrameBuffer(ospFrameBuffer);
      ospFrameBuffer = ospNewFrameBuffer(size,OSP_RGBA_I8);
      assert(ospFrameBuffer);
    }

    //! update the ospray camera (ospCamera) from the widget camera (this->camera)
    void OSPRayRenderWidget::updateOSPRayCamera()
    {
      assert(camera);
      assert(ospCamera);
      ospSetVec3f(ospCamera,"pos",camera->from);
      ospSetVec3f(ospCamera,"dir",camera->at-camera->from);
      ospSetVec3f(ospCamera,"up",camera->frame.vz);
      ospSetf(ospCamera,"aspect",size.x/float(size.y));
      ospCommit(ospCamera);      
    }

  } // ::viewer
} // ::ospray

