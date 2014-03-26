#include "data.h"
#include <sstream>

namespace ospray {
  size_t sizeOf(OSPDataType type)
  {
    switch (type) {
    case OSP_VOID_PTR: return sizeof(void *);
    case OSP_OBJECT: return sizeof(Data *);
    case OSP_FLOAT:  return sizeof(float);
    case OSP_INT:    return sizeof(int32);
    case OSP_vec2f:  return sizeof(vec2f);
    case OSP_vec3fa: return sizeof(vec3fa);
    case OSP_vec3i:  return sizeof(vec3i);
    case OSP_vec4i:  return sizeof(vec4i);
    default: break;
    };
    std::stringstream err;
    err << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType " << (int)type;
    throw std::runtime_error(err.str());
  }

  Data::Data(size_t numItems, OSPDataType type, void *init, int flags)
    : numItems(numItems), numBytes(numItems*sizeOf(type)), type(type), flags(flags)
  {
    Assert(flags == 0); // nothing else implemented yet ...
    data = new unsigned char[numBytes]; //malloc(numBytes); iw: switch
                                        //to embree::new due to
                                        //alignment
    if (init)
      memcpy(data,init,numBytes);
    else if (type == OSP_OBJECT)
      bzero(data,numBytes);
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
}
