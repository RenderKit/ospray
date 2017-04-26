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

#include "Messaging.h"
// stl
#include <unordered_map>

namespace ospray {
  namespace mpi {
    namespace messaging {

      // Internal maml message handler for all of OSPRay //////////////////////

      struct ObjectMessageHandler : maml::MessageHandler
      {
        void registerMessageListener(int handleObjID,
                                     maml::MessageHandler *listener);

        void incoming(const std::shared_ptr<maml::Message> &message) override;

        // Data members //

        std::unordered_map<int, maml::MessageHandler*> objectListeners;
      };

      // Inlined ObjectMessageHandler definitions /////////////////////////////

      inline void ObjectMessageHandler::registerMessageListener(
        int handleObjID,
        maml::MessageHandler *listener
      )
      {
        if (objectListeners.find(handleObjID) != objectListeners.end())
          postErrorMsg() << "WARNING: overwriting an existing object listener!";

        objectListeners[handleObjID] = listener;
      }

      inline void ObjectMessageHandler::incoming(
        const std::shared_ptr<maml::Message> &message
      )
      {
        auto obj = objectListeners.find(message->tag);
        if (obj != objectListeners.end()) {
          obj->second->incoming(message);
        } else {
          postErrorMsg() << "No destination for incoming message!";
        }
      }

      // Singleton instance (hidden) and helper creation function /////////////

      std::unique_ptr<ObjectMessageHandler> createHandler()
      {
        auto instance = ospcommon::make_unique<ObjectMessageHandler>();
        maml::registerHandlerFor(world.comm, instance.get());
        return instance;
      }

      static std::unique_ptr<ObjectMessageHandler> handler;

      // ospray::mpi::messaging definitions ///////////////////////////////////

      void registerMessageListener(int handleObjID,
                                   maml::MessageHandler *listener)
      {
        if (!handler.get())
          handler = createHandler();

        handler->registerMessageListener(handleObjID, listener);
      }

      void sendTo(int globalRank, ObjectHandle object,
                  std::shared_ptr<maml::Message> msg)
      {
        msg->tag = object.objID();
        maml::sendTo(world.comm, globalRank, msg);
      }

    } // ::ospray::mpi::messaging
  } // ::ospray::mpi
} // ::ospray
