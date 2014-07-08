#include "remoteGlut3D.h"
#include <string>

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

#include <turbojpeg.h>

/*! ignore if not supported */
#ifndef MSG_NOSIGNAL
#define MSG_NOSIGNAL 0 
#endif

#include "remoteCommon.h"

namespace ospray {
  namespace glut3D {

    RemoteGlut3D *RemoteGlut3D::instance = NULL;

    void RemoteGlut3D::sockRecv(void *ptr, int size)
    {
      while (size) {
        int num = recv(sockID,ptr,size,0);
        size -= num;
        ptr = (void*)((char*)ptr + num);
      }
    }
    void RemoteGlut3D::sockSend(const void *ptr, int size)
    {
      while (size) {
        int num = send(sockID,ptr,size,0);
        size -= num;
        ptr = (void*)((char*)ptr + num);
      }
    }

    RemoteGlut3D::RemoteGlut3D()
    {
      std::cout << "#remoteGlut3D: STARTING UP." << std::endl;
      
      int port = 55556;
      
      int listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);            
      if (listenSocket < 0) throw std::runtime_error("cannot create socket");
      
      // int flag = 1;
      // ::setsockopt(listenSocket, SOL_SOCKET, SO_REUSEADDR, 
      //              (const char*)&flag, sizeof(int)); 
      
      /*! bind socket to port */
      struct sockaddr_in serv_addr;
      memset((char *) &serv_addr, 0, sizeof(serv_addr));
      serv_addr.sin_family = AF_INET;
      serv_addr.sin_port = (unsigned short) htons(port);
      serv_addr.sin_addr.s_addr = INADDR_ANY;

      int RETRIES = 100;
      bool found = false;
      for (int i=0;i<RETRIES;i++)
        if (::bind(listenSocket, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0) 
          continue;
        else
          found = true;
      
      if (!found)
        {
          std::stringstream ss;
          ss << "binding to port "<<port<<" failed";
          throw std::runtime_error(ss.str());
        }
      
      /*! listen to port, up to 5 pending connections */
      if (::listen(listenSocket,5) < 0)
        throw std::runtime_error("listening on socket failed");

      std::cout << "#remoteGlut3D: LISTENING FOR CONNECTIONS ON PORT " << ntohs(serv_addr.sin_port) << std::endl;

      /*! accept incoming connection */
      struct sockaddr_in addr;
      socklen_t len = sizeof(addr);
      sockID = ::accept(listenSocket, (struct sockaddr *) &addr, &len);
      if (sockID < 0) throw std::runtime_error("cannot accept connection");

      { int flag = 1; ::setsockopt(listenSocket,IPPROTO_TCP,TCP_NODELAY,
                                   (char*)&flag,sizeof(int)); }
      
      std::cout << "#remoteGlut3D: GOT CONNECTION" << std::endl;
    }

    void RemoteGlut3D::postRedisplay()
    {
      int cmd = CMD_POST_REDISPLAY;
      sockSend(&cmd,sizeof(cmd));
    }

    void RemoteGlut3D::drawPixels(const vec2i &size, const uint32 *pixels)
    {
      if (!pixels)
        return;
      int cmd = CMD_DRAW_PIXELS;                
      sockSend(&cmd,sizeof(cmd));
      sockSend(&size,sizeof(size));
      sockSend(pixels,sizeof(int)*size.x*size.y);
    }

    void RemoteGlut3D::mainLoop()
    {
      int cmd = CMD_RUN_MAIN_LOOP;
      sockSend(&cmd,sizeof(cmd));

      // now, do the main loop:
      while (1) {
        sockRecv(&cmd,sizeof(cmd));
        // PRINT(cmd);
        switch (cmd) {
        case CMD_MOUSE_FUNC: {
          int released; 
          int whichButton;
          vec2i where;
          sockRecv(&whichButton,sizeof(whichButton));
          sockRecv(&released,sizeof(released));
          sockRecv(&where,sizeof(where));
          if (Glut3DWidget::activeWindow)
            Glut3DWidget::activeWindow->mouseButton(whichButton,released,where);

        } break;
        case CMD_MOTION_FUNC: {
          vec2i where;
          sockRecv(&where,sizeof(where));
          if (Glut3DWidget::activeWindow)
            Glut3DWidget::activeWindow->motion(where);

        } break;
        case CMD_KEYBOARD_FUNC: {
          unsigned char key;
          sockRecv(&key,sizeof(key));
          vec2i where;
          sockRecv(&where,sizeof(where));
          if (Glut3DWidget::activeWindow)
            Glut3DWidget::activeWindow->keypress(key,where);

        } break;
        case CMD_SPECIAL_FUNC: {
          unsigned int key;
          sockRecv(&key,sizeof(key));
          vec2i where;
          sockRecv(&where,sizeof(where));
          if (Glut3DWidget::activeWindow)
            Glut3DWidget::activeWindow->specialkey(key,where);

        } break;
        case CMD_DISPLAY: {
          if (Glut3DWidget::activeWindow)
            Glut3DWidget::activeWindow->display();
        } break;
        case CMD_RESHAPE: {
          vec2i size;
          sockRecv(&size,sizeof(size));
          if (Glut3DWidget::activeWindow)
            Glut3DWidget::activeWindow->reshape(size);
        } break;
        default:
          FATAL("$remoteGlut3D: unknown command "<<cmd);
        }
      }
    }
   
    void RemoteGlut3D::createWindow(const char *title, 
                                    const vec2i &size,
                                    bool fullScreen)
    {
      struct {
        int cmd, sx, sy, fs, titleSz;
      } data;
      data.cmd = CMD_CREATE_WINDOW;
      data.sx = size.x;
      data.sy = size.y;
      data.fs = fullScreen;
      data.titleSz = strlen(title);
      
      sockSend(&data,sizeof(data));
      sockSend(title,strlen(title));
    }

  }
}
