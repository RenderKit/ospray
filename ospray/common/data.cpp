#include "data.h"
#include <sstream>

namespace ospray {
  size_t sizeOf(OSPDataType type)
  {
    switch (type) {
    case OSP_VOID_PTR: return sizeof(void *);
    case OSP_OBJECT:  return sizeof(Data *);
    case OSP_FLOAT:   return sizeof(float);
    case OSP_uint8:   return sizeof(uint8);
    case OSP_int8:    return sizeof(int8);
    case OSP_uint32:  return sizeof(uint32);
    case OSP_int32:   return sizeof(int32);
    case OSP_vec2f:   return sizeof(vec2f);
    case OSP_vec3f:   return sizeof(vec3f);
    case OSP_vec3fa:  return sizeof(vec3fa);
    case OSP_vec3i:   return sizeof(vec3i);
    case OSP_vec3ui:  return sizeof(vec3ui);
    case OSP_vec4uc:  return sizeof(uint32);
    case OSP_vec4i:   return sizeof(vec4i);
    default: break;
    };
    std::stringstream err;
    err << __FILE__ << ":" << __LINE__ << ": unknown OSPDataType " << (int)type;
    throw std::runtime_error(err.str());
  }

  Data::Data(size_t numItems, OSPDataType type, void *init, int flags)
    : numItems(numItems), numBytes(numItems*sizeOf(type)), type(type), flags(flags)
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
