#include "PrimRef.h"
// #include "Triangle.h"

#include "common/sys/sync/condition.h"
#include "common/sys/sync/atomic.h"
#include "common/sys/ref.h"
#include <vector>

#define MAX_TRIS_PER_BLOCK 1000000

namespace ospray {
  typedef embree::MutexSys Mutex;
  typedef embree::ConditionSys Condition;
  using std::endl;
  using std::cout;

  struct Block {
    bool push(const Prim &prim);
    // bool push(const Triangle &tri);

    // std::vector<Triangle> triangle;
    std::vector<Prim> prim;

    void readFrom(const std::string &fn);
    void writeTo(const std::string &fn);
    void clear() { prim.clear(); }
    // void clear() { triangle.clear(); }
  };

  struct BlockRef : public embree::RefCount {  
    BlockRef() : blockID(nextBlockID++) {};
    virtual ~BlockRef() { 
      // cout << "ERASING " << blockID << endl;
      unlink(getFileName().c_str()); 
    }
    
    size_t blockID;

    static embree::AtomicCounter nextBlockID;
    static std::string fileNameBase;

    // helper functions
    std::string getFileName();
    void saveBlock(Block &block);
    void loadBlock(Block &block);
  };
  
}
