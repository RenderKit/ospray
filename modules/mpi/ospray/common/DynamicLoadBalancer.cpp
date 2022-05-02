// Copyright 2009 Intel Corporation
// SPDX-License-Identifier: Apache-2.0

// This code is for DynamicLoadBalancer using the lifeline scheduling method

#include "DynamicLoadBalancer.h"
#include <math.h>
#include "Messaging.h"
#include "common/Profiling.h"
#include "rkcommon/utility/ArrayView.h"

namespace ospray {
namespace mpi {
using namespace mpicommon;

DynamicLoadBalancer::DynamicLoadBalancer(ObjectHandle myId, int numTasks)
    : MessageHandler(myId)
{
  activeTasks = numTasks;
}
void DynamicLoadBalancer::setActiveTasks(int numTasks)
{
  std::lock_guard<std::mutex> guard(termMutex);
  activeTasks = numTasks;
}

void DynamicLoadBalancer::updateActiveTasks(int term)
{
  std::lock_guard<std::mutex> guard(termMutex);
  activeTasks = activeTasks - term;
}
int DynamicLoadBalancer::getActiveTasks()
{
  int updatedActiveTasks = 0;
  {
    std::lock_guard<std::mutex> guard(termMutex);
    updatedActiveTasks = activeTasks;
  }
  return updatedActiveTasks;
}
void DynamicLoadBalancer::addWork(Work workItem)
{
  std::lock_guard<std::mutex> guard(workMutex);
  workQueue.push_back(workItem);
}

int DynamicLoadBalancer::getWorkSize()
{
  int workSize;
  {
    std::lock_guard<std::mutex> guard(workMutex);
    workSize = workQueue.size();
  }
  return workSize;
}

Work DynamicLoadBalancer::getWorkItemFront()
{
  Work workItem;
  std::lock_guard<std::mutex> guard(workMutex);
  workItem.ntasks = workQueue[0].ntasks;
  workItem.offset = workQueue[0].offset;
  workItem.ownerRank = workQueue[0].ownerRank;
  workQueue.pop_front();

  return workItem;
}

Work DynamicLoadBalancer::getWorkItemBack()
{
  std::lock_guard<std::mutex> guard(workMutex);
  Work workItem = workQueue.back();
  workQueue.pop_back();

  return workItem;
}
// This function takes the rank id and convert it into coords based on the base
// we are using to represent the ranks
std::vector<int> DynamicLoadBalancer::rankToCoords(
    int rank, unsigned int base, unsigned int power)
{
  std::vector<int> result;
  if (rank >= pow(base, power)) {
    postStatusMsg(OSP_LOG_DEBUG) << "ERROR: Requested rank (" << rank
                                 << ") out of bond. Increase base or power.\n";
    return result;
  }

  result.resize(power);
  int left = rank;
  for (unsigned int i = 0; i < power; i++) {
    result[i] = left / pow(base, power - i - 1);
    left -= result[i] * pow(base, power - i - 1);
  }

  return result;
}

// --------------------------------------------------------------------
// This function Takes coords of a rank and returns the rank id
int DynamicLoadBalancer::coordsToRank(std::vector<int> &coords,
    unsigned int base,
    unsigned int power,
    int numRanks)
{
  if (coords.size() != power) {
    postStatusMsg(OSP_LOG_DEBUG)
        << "ERROR: Coords do not match the requested dimensions.\n";
    return -1;
  }

  int result = 0;
  for (unsigned int i = 0; i < power; i++)
    result += coords[i] * pow(base, power - i - 1);

  if (numRanks <= result)
    result -= numRanks;

  return result;
}

// --------------------------------------------------------------------
// This function takes the rank coords and compute the lifelines IDs
std::vector<unsigned int> DynamicLoadBalancer::getLifelineID(
    std::vector<int> &rankCoords,
    unsigned base,
    unsigned int power,
    int numRanks)
{
  std::vector<unsigned int> result;
  if (rankCoords.size() != power) {
    return result;
  }

  int myLifeLineID;
  std::vector<int> lifelineCoords(power);

  // loop over rank lifelines
  for (unsigned int i = 0; i < power; i++) {
    // loop over coords of lifeline i
    for (unsigned int j = 0; j < power; j++) {
      if (i == j) {
        if (int(base - 1) <= rankCoords[i])
          lifelineCoords[j] = 0;
        else
          lifelineCoords[j] = rankCoords[i] + 1;

      } else
        lifelineCoords[j] = rankCoords[j];
    }

    myLifeLineID = coordsToRank(lifelineCoords, base, power, numRanks);
    if (-1 < myLifeLineID)
      result.push_back(myLifeLineID);
  }

  return result;
}

// This function compute the lifelines of the passed rank
std::vector<unsigned int> DynamicLoadBalancer::getMyLifeLines(
    int rank, int numRanks, unsigned int base, unsigned int power)
{
  std::vector<int> ranksCoords = rankToCoords(rank, base, power);
  std::vector<unsigned int> lifelines =
      getLifelineID(ranksCoords, base, power, numRanks);

  return lifelines;
}

// --------------------------------------------------------------------
void DynamicLoadBalancer::incoming(
    const std::shared_ptr<mpicommon::Message> &message)
{
  auto *header = (DynamicLBMessage *)message->data;

  if (header->type == TERMINATED) {
    int numTerm = ((DynamicLBTerminatedMessage *)message->data)->numTerm;
    updateActiveTasks(numTerm);
  } // TERMINATED
  else if (header->type == NEED_WORK) {
    sendWork(header->senderRank);
  } // NEED_WORK
  else if (header->type == RECV_WORK) {
    auto *workMsg = (DynamicLBSendWorkMessage *)message->data;
    int numRecvWork = workMsg->numWorkItems;
    auto *workItems =
        (Work *)(message->data + sizeof(DynamicLBSendWorkMessage));
    Work myWork;
    int workSize;
    for (int i = 0; i < numRecvWork; i++) {
      myWork.ntasks = workItems[i].ntasks;
      myWork.offset = workItems[i].offset;
      myWork.ownerRank = workItems[i].ownerRank;
      addWork(myWork);
    }
    workSize = getWorkSize();
    if (1 < workSize) {
      sendMultiWork();
    }
  } // RECV_WORK
  else {
    postStatusMsg(OSP_LOG_ERROR)
        << "Rank " << workerRank() << " Recv Unknown message\n";
  }
}

// --------------------------------------------------------------------
void DynamicLoadBalancer::requestWork()
{
  const int msgSize = sizeof(DynamicLBMessage);

  int base, power;
  base = 2;
  float p = log2(workerSize());
  if (pow(base, p) - workerSize() == 0)
    power = p;
  else
    power = int(p) + 1;

  myLifelines = getMyLifeLines(workerRank(), workerSize(), base, power);

  for (size_t i = 0; i < myLifelines.size(); i++) {
    if (myLifelines[i] == (unsigned int)workerRank())
      continue;
    else {
      auto msg = std::make_shared<mpicommon::Message>(msgSize);
      auto *header = reinterpret_cast<DynamicLBMessage *>(msg->data);
      header->type = NEED_WORK;
      header->senderRank = workerRank();
      mpi::messaging::sendTo(myLifelines[i], myId, msg);
    }
  }
}
// --------------------------------------------------------------------
void DynamicLoadBalancer::sendMultiWork()
{
  std::lock_guard<std::mutex> lockThieves(thiefMutex);
  if (thiefIds.empty()) {
    return;
  }

  // Try to steal as much work as we can for the thieves, while leaving some for
  // this rank too
  std::vector<Work> stolenWork = getWorkItems([&](size_t workSize) {
    // +1 here to pretend that this rank is also a thief, so that we leave some
    // work for it in the queue
    const size_t workPerRank = workSize / (thiefIds.size() + 1);
    return workPerRank * thiefIds.size();
  });
  if (stolenWork.empty()) {
    return;
  }

  // Each thief will be sent workPerRank work items
  const size_t workPerThief = stolenWork.size() / thiefIds.size();
  size_t nextWorkSet = 0;
  for (auto &thief : thiefIds) {
    sendWorkToThief(thief, stolenWork.data() + nextWorkSet, workPerThief);
    nextWorkSet += workPerThief;
  }
  thiefIds.clear();
}
// --------------------------------------------------------------------
void DynamicLoadBalancer::sendWork(int thiefID)
{
  if (thiefID != -1 && thiefID != workerRank()) {
    std::vector<Work> stolenWork =
        getWorkItems([&](size_t workSize) { return workSize / 2; });
    if (!stolenWork.empty()) {
      sendWorkToThief(thiefID, stolenWork.data(), stolenWork.size());
    } else {
      std::lock_guard<std::mutex> lockThieves(thiefMutex);
      thiefIds.insert(thiefID);
    }
  }
}

void DynamicLoadBalancer::sendWorkToThief(
    int thiefID, const Work *toSend, const size_t numWorkItems)
{
  const int msgSize =
      sizeof(DynamicLBSendWorkMessage) + numWorkItems * sizeof(Work);
  auto msg = std::make_shared<mpicommon::Message>(msgSize);
  auto *header = reinterpret_cast<DynamicLBSendWorkMessage *>(msg->data);
  header->type = RECV_WORK;
  header->senderRank = workerRank();
  header->numWorkItems = numWorkItems;
  auto *workItems =
      reinterpret_cast<Work *>(msg->data + sizeof(DynamicLBSendWorkMessage));
  std::memcpy(workItems, toSend, numWorkItems * sizeof(Work));

  mpi::messaging::sendTo(thiefID, myId, msg);
}

// --------------------------------------------------------------------
void DynamicLoadBalancer::sendTerm(int term)
{
  const int msgSize = sizeof(DynamicLBTerminatedMessage);

  if (0 < term) {
    for (int i = 0; i < workerSize(); i++)
      if (i != workerRank()) {
        auto msg = std::make_shared<mpicommon::Message>(msgSize);
        auto *header =
            reinterpret_cast<DynamicLBTerminatedMessage *>(msg->data);
        header->type = TERMINATED;
        header->senderRank = workerRank();
        header->numTerm = term;
        mpi::messaging::sendTo(i, myId, msg);
      }
  }
}

} // namespace mpi
} // namespace ospray
