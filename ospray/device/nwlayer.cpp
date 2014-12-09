#include "nwlayer.h"

namespace ospray {
  namespace nwlayer {

    inline std::string toString(long l)
    { std::stringstream ss; ss << l; return ss.str(); }

    void RemoteWorker::handleCommand(const CommandTag cmd, ReadBuffer &args)
    {
      throw std::runtime_error("unknown command "+toString(cmd));
    }

    void RemoteWorker::executeCommands() 
    {
      assert(comm);
      assert(device);
      CommandTag cmd;
      ReadBuffer args;
      while (1) {
        PING;
        comm->recv(cmd,args);
        PRINT(cmd);
        handleCommand(cmd,args);
      }
    }

  } // ::ospray::nwlayer
} // ::ospray
