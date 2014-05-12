#include "device.h"

namespace ospray {
  namespace coi {
    using std::cout; 
    using std::endl;
    
    struct COIDevice : public ospray::api::Device {
    };
      
    ospray::api::Device *createCoiDevice(int *ac, const char **av)
    {
      cout << "=======================================================" << endl;
      cout << "#osp:mic: trying to create coi device" << endl;
      cout << "=======================================================" << endl;
      
      return NULL;
    }
  }
}


