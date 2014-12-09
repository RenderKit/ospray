// ospray
#include "Data.h"
// stl
#include <sstream>

namespace ospray {

  Data::Data(size_t numItems, OSPDataType type, void *init, int flags)
    : numItems(numItems), numBytes(numItems * sizeOf(type)), type(type), flags(flags)
  {
    /* two notes here:
       a) i'm using embree's 'new' to enforce alignment
       b) i'm adding 16 bytes to size to enforce 4-float padding (which embree
          requires in some buffers 
    */
    if (flags & OSP_DATA_SHARED_BUFFER) {
      Assert2(init != NULL, "shared buffer is NULL");
      data = init;
    } else {
      data = new unsigned char[numBytes+16]; 
      if (init)
        memcpy(data,init,numBytes);
      else if (type == OSP_OBJECT)
        bzero(data,numBytes);
    }

    managedObjectType = OSP_DATA;

    // std::cout << "checksum when creating data array" << std::endl;
    // PRINT(numBytes);
    // PRINT((int*)computeCheckSum(init,numBytes));
  }

  Data::~Data() 
  { 
    if (type == OSP_OBJECT) {
      Data **child = (Data **)data;
      for (int i=0;i<numItems;i++)
        if (child[i]) child[i]->refDec();
    }
    free(data); 
  }

} // ::ospray
