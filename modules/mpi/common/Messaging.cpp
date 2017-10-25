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
// ospcommon
#include "ospcommon/utility/DeletedUniquePtr.h"
// stl
#include <unordered_map>

namespace ospray {
  namespace mpi {
    namespace messaging {

      using namespace mpicommon;
      using ospcommon::utility::DeletedUniquePtr;
      using ospcommon::utility::make_deleted_unique;

      // Internal maml message handler for all of OSPRay //////////////////////

      struct ObjectMessageHandler : maml::MessageHandler
      {
        void registerMessageListener(int handleObjID,
                                     maml::MessageHandler *listener);

        void removeMessageListener(int handleObjID);

        void incoming(const std::shared_ptr<Message> &message) override;

        // Data members //

       private:

        std::unordered_map<int, MessageHandler*> objectListeners;
      };

      // Inlined ObjectMessageHandler definitions /////////////////////////////

      inline void ObjectMessageHandler::registerMessageListener(
        int handleObjID,
        MessageHandler *listener
      )
      {
        if (objectListeners.find(handleObjID) != objectListeners.end())
          postStatusMsg() << "WARNING: overwriting an existing listener!";

        objectListeners[handleObjID] = listener;
      }

      inline void ObjectMessageHandler::removeMessageListener(int handleObjID)
      {
        objectListeners.erase(handleObjID);
      }

      inline void ObjectMessageHandler::incoming(
        const std::shared_ptr<Message> &message
      )
      {
        auto obj = objectListeners.find(message->tag);
        if (obj != objectListeners.end()) {
          obj->second->incoming(message);
        } else {
          postStatusMsg() << "WARNING: No destination for incoming message!";
        }
      }

      // Singleton instance (hidden) and helper creation function /////////////

      static DeletedUniquePtr<ObjectMessageHandler> handler;
      static bool handlerValid = false;

      DeletedUniquePtr<ObjectMessageHandler> createHandler()
      {
        auto instance =
            make_deleted_unique<ObjectMessageHandler>(
              [](ObjectMessageHandler *_handler){
                handlerValid = false; delete _handler;
              }
            );

        maml::registerHandlerFor(world.comm, instance.get());
        handlerValid = true;
        return instance;
      }

      // MessageHandler definitions ///////////////////////////////////////////

      MessageHandler::MessageHandler(ObjectHandle handle) : myId(handle)
      {
        registerMessageListener(myId, this);
      }

      MessageHandler::~MessageHandler()
      {
        removeMessageListener(myId);
      }

      // ospray::mpi::messaging definitions ///////////////////////////////////

      void registerMessageListener(int handleObjID,
                                   MessageHandler *listener)
      {
        if (!handlerValid)
          handler = createHandler();

        handler->registerMessageListener(handleObjID, listener);
      }

      void removeMessageListener(int handleObjID)
      {
        if (handlerValid)
          handler->removeMessageListener(handleObjID);
      }

      void enableAsyncMessaging()
      {
        maml::start();
      }

      void sendTo(int globalRank, ObjectHandle object,
                  std::shared_ptr<Message> msg)
      {
        msg->tag = object.objID();
        maml::sendTo(world.comm, globalRank, msg);
      }

      bool asyncMessagingEnabled()
      {
        return maml::isRunning();
      }

      void disableAsyncMessaging()
      {
        maml::stop();
      }

    } // ::ospray::mpi::messaging
  } // ::ospray::mpi
} // ::ospray
