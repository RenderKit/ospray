#include "PrimStream.h"

namespace ospray {

  void PrimStream::flush()
  {
    if (numPrims == 0) return;

    size_t myID = (size_t)pthread_self();
    // printf("ID %li DOING flush\n",myID);

    Ref<BlockRef> newBlock = new BlockRef;
    // printf("ID %li saving\n",myID);
    newBlock->saveBlock(buffer);
    // printf("ID %li pushing\n",myID);
    blockRefs.push_back(newBlock);
    // printf("ID %li bla\n",myID);

    buffer.clear();
  }
  
  void PrimStream::push(const Prim &prim)
  {
    bounds.extend(prim.bounds);
    numPrims++;
    if (buffer.push(prim)) return;

    flush();
    buffer.push(prim);
  }
  
}
