/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

#pragma once

/*! \file ospray/device/nwlayer.h \brief Defines the basic network layer abstraction */

// ospray 
#include "ospray/device/buffers.h"
#include "ospray/device/command.h"
#include "ospray/api/device.h"
// embree
#include "common/sys/network.h"

namespace ospray {
  namespace nwlayer {

    /*! a "communicator" is the abstraction for the class that can
        send messages/commands to remote clients */
    struct Communicator {
      /* receive from *master* to *all* clients */
      virtual void bcast(const WriteBuffer &args) = 0;
      virtual void sendTo(const uint32 clientID,
                          const WriteBuffer &args) = 0;
      /*! flush the send queue, making sure that the last command was
        properly executed. this may introduce (quite) some delay, so
        be wary of this call */
      virtual void flushSendQueue() = 0;
      /* receive from *master* */
      virtual void recv(CommandTag &cmd, ReadBuffer &args) = 0;
    };

    struct MPICommunicator : public Communicator {
    };
    struct TCPCommunicator : public Communicator {
      std::vector<embree::network::socket_t> remote;
    };


    /*! a network "worker" is the instance that handles a device's API
        calls on a remote node */
    struct RemoteWorker {
      /*! communicator through which we receive commands */
      Communicator        *comm;
      /*! device which we use by default to execute the commands we received */
      ospray::api::Device *device;
      virtual void handleCommand(const CommandTag cmd, ReadBuffer &args) = 0;
      virtual void executeCommands();
    };

    /*! base class for all kinds of devices in which API calls get
      packed up as command streams that then get sent (or broadcast)
      to different workers */
    struct RemoteDevice : public ospray::api::Device { 
    };

    /*! the remote rendering device simply forwards all calls to the
        remote worker (which may use a device of its choice to compute
        the pixels), and returns a (possibly compressed) image back */
    struct RemoteRenderingDevice : public RemoteDevice {
    };

    /*! the MPI *group* device has a set of N nodes that are all in
        the same MPI group and communiate with each other through a
        common intracommunicator. Rank 0 acts as a master and does the
        control/load balancing of the other ranks, but by itself
        doesn't do anything else (if the same node is to also render
        pixels it should simply be added twice; once as rank0, and
        once as rank1. */
    struct MPIGroupDevice {
      struct DeviceInterface : public RemoteDevice
      {
      };
      struct Worker : public RemoteWorker 
      {
      };
    };

    /*! the MPI *cluster* device has a dedicated master node for
        control and load balancing, has all workers running in a
        different MPI group, and has master and workers communicate
        through a MPI-*inter*-communicator (the workers also have a
        intracommunicator among themselves */
    struct MPIClusterDevice {
    };
  }
}
