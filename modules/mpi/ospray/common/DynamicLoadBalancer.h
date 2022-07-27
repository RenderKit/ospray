// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <deque>
#include <mutex>
#include <set>
#include <vector>
#include "../common/Messaging.h"

namespace ospray {
namespace mpi {

enum MSG
{
  TERMINATED = 1000,
  NEED_WORK = 2000,
  VICTIM_NO_WORK = 3000,
  RECV_WORK = 4000

};

struct Work
{
  int ntasks = 0;
  int offset = 0;
  int ownerRank = -1;
};

struct DynamicLBMessage
{
  int type;
  int senderRank;
};

struct DynamicLBTerminatedMessage : DynamicLBMessage
{
  int numTerm;
};

struct DynamicLBSendWorkMessage : DynamicLBMessage
{
  int numWorkItems;
};

struct DynamicLoadBalancer : public mpi::messaging::MessageHandler
{
  DynamicLoadBalancer(ObjectHandle myHandle, int numTasks);
  ~DynamicLoadBalancer() override = default;

  void setActiveTasks(int numTasks);
  void updateActiveTasks(int term);
  int getActiveTasks();
  void addWork(Work workItem);
  int getWorkSize();
  Work getWorkItemFront();
  Work getWorkItemBack();

  void requestWork();
  void sendTerm(int term);
  void incoming(const std::shared_ptr<mpicommon::Message> &message) override;

 private:
  void sendWork(int thiefID);
  void sendMultiWork();
  void sendWorkToThief(
      int thiefID, const Work *toSend, const size_t numWorkItems);

  /* Request a variable size number of work items based on the current size of
   * the workQueue. computeItemsRequested will be called after acquiring the
   * workMutex to compute the number of items to be returned.
   * computeItemsRequested should take a size_t and return a size_t representing
   * the number of items to return. If computeItemsRequested returns 0 or more
   * than workQueue.size(), an empty vector will be returned
   */
  template <typename Fn>
  std::vector<Work> getWorkItems(const Fn computeItemsRequested);

  /*
  The number of lifelines is determined using the following formula:
  base ^ L-1 < #ranks <= base ^ L, where L = number of lifelines
  for example 8 ranks can be represented as one of the following options:
     1- base 2, L = 3: 2 ^ 2 < 8 <= 2 ^ 3
     2- base 3, L = 2, 3 ^ 1 < 8 <= 3 ^ 2
     3- base 4, L = 2, 4 ^ 1 < 8 <= 4 ^ 2

  After choosing the base, rankToCoords creates a virtual cyclic graph, where
  ranks are the graph nodes, and the coords for each graph node(rank) is
  represented in that base. The function rankToCoords returns the coords for
  the passed rank depending on the base and power. For example if the base is
  2 and power then the coords are represented in zeros and ones, the power
  identify the number of digits representing each rank coords, if the power is
  3, then the coords are represented as 3 digits. The function coordsToRank,
  takes the rank coords and return the rank id.
  */
  std::vector<int> rankToCoords(
      int rank, unsigned int base, unsigned int power);
  int coordsToRank(std::vector<int> &coords,
      unsigned int base,
      unsigned int power,
      int numRanks);
  std::vector<unsigned int> getLifelineID(std::vector<int> &coords,
      unsigned base,
      unsigned int power,
      int numRanks);
  std::vector<unsigned int> getMyLifeLines(
      int myRank, int numRanks, unsigned int base, unsigned int power);

  std::vector<unsigned int> myLifelines;
  std::set<int> thiefIds;
  std::deque<Work> workQueue;
  int activeTasks;
  std::mutex termMutex;
  std::mutex workMutex;
  std::mutex thiefMutex;
}; // struct DynamicLoadBalancer

template <typename Fn>
std::vector<Work> DynamicLoadBalancer::getWorkItems(
    const Fn computeItemsRequested)
{
  std::vector<Work> reqWorkItems;
  std::lock_guard<std::mutex> guard(workMutex);
  const size_t numReqWorkItems = computeItemsRequested(workQueue.size());
  if (numReqWorkItems == 0 || numReqWorkItems > workQueue.size()) {
    return reqWorkItems;
  }
  for (size_t i = 0; i < numReqWorkItems; i++) {
    Work workItem = workQueue.back();
    workQueue.pop_back();
    reqWorkItems.push_back(workItem);
  }
  return reqWorkItems;
}

} // namespace mpi
} // namespace ospray
