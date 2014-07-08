namespace ospray {
  namespace glut3D {
    typedef enum {
      CMD_CREATE_WINDOW, // remote to clietn: create a window
      CMD_RUN_MAIN_LOOP, // remote to client: start the main event loop
      CMD_RESHAPE, // client to remote: glut3Dreshape event
      CMD_POST_REDISPLAY, // remote to client: do a redisplay
      CMD_DISPLAY, 
      CMD_DRAW_PIXELS,                          
      CMD_MOUSE_FUNC,
      CMD_MOTION_FUNC,
      CMD_SPECIAL_FUNC,
      CMD_KEYBOARD_FUNC,
    } RemoteCommand;
  }
}
