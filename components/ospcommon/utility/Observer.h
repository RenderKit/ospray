// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "../common.h"
#include "TimeStamp.h"

#include <algorithm>
#include <vector>

namespace ospcommon {
  namespace utility {

    struct Observer;

    // Something that an observer looks at
    //
    // NOTE(jda) - This can either be used as a base class or as a stand-alone
    //             member of a class. It is up to the user to decide best how to
    //             use this abstraction.
    struct Observable
    {
      Observable() = default;
      virtual ~Observable();

      void notifyObservers();

    private:

      friend Observer;

      void registerObserver(Observer &newObserver);
      void removeObserver(Observer &toRemove);

      TimeStamp lastNotified;

      std::vector<Observer*> observers;
    };

    // Something that looks an an observable instance.
    //
    // NOTE(jda) - I think this makes more sense for objects to hold an instance
    //             of an Observer and avoid _being_ and observer.
    struct Observer
    {
      Observer(Observable &observee);
      ~Observer();

      bool wasNotified();

    private:

      friend Observable;

      TimeStamp lastObserved;
      Observable *observee {nullptr};
    };

    // Inlined definitions ////////////////////////////////////////////////////

    // Observable //

    inline Observable::~Observable()
    {
      for (auto *observer : observers)
        observer->observee = nullptr;
    }

    inline void Observable::notifyObservers()
    {
      lastNotified.renew();
    }

    inline void Observable::registerObserver(Observer &newObserver)
    {
      observers.push_back(&newObserver);
    }

    inline void Observable::removeObserver(Observer &toRemove)
    {
      auto &o = observers;
      o.erase(std::remove(o.begin(), o.end(), &toRemove), o.end());
    }

    // Observer //

    inline Observer::Observer(Observable &_observee) : observee(&_observee)
    {
      observee->registerObserver(*this);
    }

    inline Observer::~Observer()
    {
      if (observee)
        observee->removeObserver(*this);
    }

    inline bool Observer::wasNotified()
    {
      if (!observee)
        return false;

      bool notified = lastObserved < observee->lastNotified;

      if (notified)
        lastObserved.renew();

      return notified;
    }

  } // ::ospcommon::utility
} // ::ospcommon
