#pragma once

#include "ospray/rv_module.h"

namespace ospray {
  namespace rv {
    /*! a resitor geometry 'model': one set of resistors with attribtues etc */
    struct ResistorModel {
      const Resistor *const resistor;
      const float    *const attribute;

      const uint32          numResistors;
      //! embree geometry ID
      const id_t            geomID;

      ResistorModel(const id_t     geomID,
                    const uint32   numResistors, 
                    const Resistor resistor[],
                    const float    attribute[])
        : geomID(geomID),
          numResistors(numResistors),
          resistor(resistor),
          attribute(attribute)
      {}
    };
  }
}
