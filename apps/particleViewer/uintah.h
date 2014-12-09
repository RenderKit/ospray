#include "Model.h"

namespace ospray {
  namespace particle {

    //! parse given uintah-format timestep.xml file, and return in a model
    Model *parse__Uintah_timestep_xml(const std::string &s);

  } // ::ospray::particle
} // ::ospray
