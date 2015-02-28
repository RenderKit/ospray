
// // ======================================================================== //
// // Copyright 2009-2014 Intel Corporation                                    //
// //                                                                          //
// // Licensed under the Apache License, Version 2.0 (the "License");          //
// // you may not use this file except in compliance with the License.         //
// // You may obtain a copy of the License at                                  //
// //                                                                          //
// //     http://www.apache.org/licenses/LICENSE-2.0                           //
// //                                                                          //
// // Unless required by applicable law or agreed to in writing, software      //
// // distributed under the License is distributed on an "AS IS" BASIS,        //
// // WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// // See the License for the specific language governing permissions and      //
// // limitations under the License.                                           //
// // ======================================================================== //

// #include "ospray/mpi/DistributedFrameBuffer.h"

// namespace ospray {
  
// #define DFB_OBJECT_ID 1000
// #define ASYNC_COMM_TAG 1001  
  
//   void testDFB_testScreen(int &ac, char **av)
//   {
//     vec2i fbSize(4000,3000);

//     mpi::init(&ac,(const char**)av);

//     mpi::async::CommLayer commLayer;
//     mpi::async::Group *world = mpi::async::createGroup("world",MPI_COMM_WORLD,&commLayer,ASYNC_COMM_TAG);


//     TiledFrameBuffer *dfb = ospray::createDistributedFrameBuffer(&commLayer,fbSize,DFB_OBJECT_ID);
//     commLayer.registerObject(dfb,DFB_OBJECT_ID);
//     // CheckSumAndBounceNewRandomMessage consumer;

//     // mpi::async::Group *world = mpi::async::createGroup("world",MPI_COMM_WORLD,
//     //                                                    &consumer,ASYNC_TAG);
//     // srand(world->rank*13*17*23+2342556);
    
//     // mpi::world.barrier();
//     // char hostname[1000];
//     // gethostname(hostname,1000);
//     // printf("#osp:mpi(%2i/%i): async comm test on %s\n",mpi::world.rank,mpi::world.size,hostname); fflush(0);
//     // mpi::world.barrier();
//     // double t0 = getSysTime();

//     // for (int i=0;i<mpi::world.size * NUM_START_MESSAGES_PER_NODE;i++) {
//     //   // int tgt = (world->rank+i) % world->size;
//     //   int tgt = rand() % world->size;
//     //   sendRandomMessage(mpi::Address(world,tgt));
//     // }

//     // sleep(NUM_SECONDS);
//     // //usleep(NUM_SECONDS*1000000LL);
    
//     // mutex.lock();
//     // doNotSendAnyMoreMessages = true;
//     // for (int i=0;i<world->size;i++) {
//     //   int *msg = (int*)malloc(sizeof(int));
//     //   *msg = -1;
//     //   mpi::async::send(mpi::Address(world,i),msg,sizeof(int));
//     // }
//     // printf("#osp:mpi(%2i/%i): done notices sent\n",mpi::world.rank,mpi::world.size); fflush(0);
//     // while (numTerminated < mpi::world.size)
//     //   cond.wait(mutex);
//     // mutex.unlock();
//     // double t1 = getSysTime();
//     // double MBs = numBytesReceived / (1024*1024.f);
//     // double secs = t1-t0;
//     // printf("#osp:mpi(%2i/%i): received %li msgs of %5.3lfMB in %2.1lfsecs, that's %5.3lfMB/sec\n",
//     //        mpi::world.rank,mpi::world.size,numMessagesReceived,MBs,secs,MBs/secs); fflush(0);
           
//     // mpi::world.barrier();
//     // usleep(1000);
//     mpi::async::shutdown();
//   }
  
// } // ::ospray

// int main(int ac, char **av)
// {
//   ospray::testDFB_testScreen(ac,av);
//   return 0;
// }

