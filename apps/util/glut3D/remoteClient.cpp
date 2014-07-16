/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

// embree
#include "common/sys/platform.h"

// glut3d
#include "remoteGlut3D.h"
#include <string>
#include "remoteCommon.h"

#if defined(__WIN32__)
#include <winsock2.h>
#include <io.h>
typedef int socklen_t;
#define SHUT_RDWR SD_BOTH
#else 
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h> 
#define SOCKET int
#define closesocket close
#endif

/*! ignore if not supported */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0 
#endif


#include "ospray/common/ospcommon.h"
#include /*embree*/"common/math/affinespace.h"

#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

namespace ospray {
  //! dedicated namespace for 3D glut viewer widget
  namespace glut3D {

    /*! ID of glut window on local display client */
    int windowID = -1;
    /*! socket to remote glut3D instance */
    int sockID = -1; 

    void sockRecv(void *ptr, int size)
    {
      while (size) {
        int num = recv(sockID,ptr,size,0);
        size -= num;
        ptr = (void*)((char*)ptr + num);
      }
    }
    void sockSend(void *ptr, int size)
    {
      while (size) {
        int num = send(sockID,ptr,size,0);
        size -= num;
        ptr = (void*)((char*)ptr + num);
      }
    }

    void runOutstandingRemoteCommands();

    void remoteIdle(void)
    {
      runOutstandingRemoteCommands();
    }

    void remoteKeyboard(unsigned char key, int32 x, int32 y)
    {
      int cmd = CMD_KEYBOARD_FUNC;
      sockSend(&cmd,sizeof(int));
      sockSend(&key,sizeof(key));
      sockSend(&x,sizeof(int));
      sockSend(&y,sizeof(int));
    }
    void remoteSpecial(int32 key, int32 x, int32 y)
    {
      int cmd = CMD_SPECIAL_FUNC;
      sockSend(&cmd,sizeof(int));
      sockSend(&key,sizeof(key));
      sockSend(&x,sizeof(int));
      sockSend(&y,sizeof(int));
    }
    void remoteMotionFunc(int32 x, int32 y)
    {
      int cmd = CMD_MOTION_FUNC;
      sockSend(&cmd,sizeof(int));
      sockSend(&x,sizeof(int));
      sockSend(&y,sizeof(int));
    }

    void remoteMouseFunc(int32 whichButton, int32 released, int32 x, int32 y)
    {
      int cmd = CMD_MOUSE_FUNC;
      sockSend(&cmd,sizeof(int));
      sockSend(&whichButton,sizeof(int));
      sockSend(&released,sizeof(int));
      sockSend(&x,sizeof(int));
      sockSend(&y,sizeof(int));
    }

    void remoteDisplay( void )
    {
      int cmd = CMD_DISPLAY;
      sockSend(&cmd,sizeof(int));
    }

    void remoteReshape(int32 x, int32 y)
    {
      PING;
      int cmd = CMD_RESHAPE;
      sockSend(&cmd,sizeof(int));
      sockSend(&x,sizeof(int));
      sockSend(&y,sizeof(int));
    }

    void usage() 
    {
      std::cout << "usage: ospRemoteGlut3D <host> <port>" << std::endl;
      std::cout << "(using the <host> <port> reported by the remote app's output when using --osp-remote-glut)" << std::endl;
      
      exit(1);
    }
    std::string stringOf(int i)
    {
      std::stringstream ss;
      ss << i; 
      return ss.str();
    }
    void establishConnection(const std::string &host, int port)
    {
      std::cout << "$remoteGlut3DClient: establishing connection to "
                << host << ":" << port << std::endl;
      

      /*! create a new socket */
      sockID = ::socket(AF_INET, SOCK_STREAM, 0);
      if (sockID < 0) throw std::runtime_error("cannot create socket");
      
      /*! perform DNS lookup */
      struct hostent* server = ::gethostbyname(host.c_str());
      if (server == NULL) throw std::runtime_error("server "+host+" not found");
      
      /*! perform connection */
      struct sockaddr_in serv_addr;
      memset((char*)&serv_addr, 0, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = (unsigned short) htons(port);
      memcpy((char*)&serv_addr.sin_addr.s_addr, (char*)server->h_addr, server->h_length);
      
      if (::connect(sockID,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0) {
        throw std::runtime_error("connection to "+std::string(host)+":"+stringOf(port)+" failed - did you start the remote glut3D service yet?");
      }

      { int flag = 1; ::setsockopt(sockID, IPPROTO_TCP, TCP_NODELAY, (const char*)&flag, sizeof(int)); }

      std::cout << "$remoteGlut3DClient: GOT connection..." << std::endl;
      sleep(1);
    }

    void runOutstandingRemoteCommands()
    {
      // wait for calls from remote glut
      while (1) {
        int cmd;
        int ret = recv(sockID,&cmd,sizeof(int),MSG_DONTWAIT);
        if (ret <= 0) {
          return;
        }
        switch(cmd) {
        case CMD_CREATE_WINDOW: {
          PING;
          vec2i size;
          int fs;
          int len;
          sockRecv(&size.x,sizeof(int));
          sockRecv(&size.y,sizeof(int));
          sockRecv(&fs,sizeof(int));
          sockRecv(&len,sizeof(int));
          char *title = (char*)malloc(len+1);
          title[len] = 0;
          sockRecv(title,len);

          PRINT(size);
          PRINT(title);


          glutInitWindowSize( size.x, size.y );
          windowID = glutCreateWindow(title);
          glutSetWindow(windowID);

          glutDisplayFunc( remoteDisplay );
          glutReshapeFunc( remoteReshape );
          glutKeyboardFunc(remoteKeyboard );
          glutSpecialFunc( remoteSpecial );
          glutMotionFunc(  remoteMotionFunc );
          glutMouseFunc(   remoteMouseFunc );
          glutIdleFunc(    remoteIdle );
          
          if (fs)
            glutFullScreen();

        } break;
        case CMD_POST_REDISPLAY: {
          glutPostRedisplay();
        } break;
        case CMD_DRAW_PIXELS: {
          vec2i size;
          sockRecv(&size,sizeof(size));
          int *pixels = new int[size.x*size.y];
          sockRecv(pixels,size.x*size.y*sizeof(int));
          glDrawPixels(size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
          glutSwapBuffers();
          delete[] pixels;
        } break;
        case CMD_RUN_MAIN_LOOP:
          PING;
          glutMainLoop();
          break;
        default:
          throw std::runtime_error("$remoteGlut3DClient: Unknown remote glut command : "+stringOf(cmd));
        }
      }

    }

    void runRemoteClient(const std::string &host, int port)
    {
      establishConnection(host,port);

      while (1) {
        runOutstandingRemoteCommands();
        // at some point this will go to the main loop, and not return.
        usleep(100);
      }
    }

  }
}

int main(int ac, char **av)
{
  if (ac != 3)
    ospray::glut3D::usage();

  glutInit(&ac,av);
  assert(ac == 3);
  const std::string host = av[1];
  const int port = atoi(av[2]);
  ospray::glut3D::runRemoteClient(host,port);
  return 0;
}
