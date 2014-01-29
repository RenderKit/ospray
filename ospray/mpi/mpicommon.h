#pragma once

#include <mpi.h>
#include "../common/ospcommon.h"

namespace ospray {

  /*! Helper class for MPI Programming */
  namespace mpi {
    //! abstraction for an MPI group. 
    /*! it's the responsiblity of the respective mpi setup routines to
      fill in the proper values */
    struct Group {
      /*! whether the current process/thread is a member of this
        gorup */
      bool     containsMe; 
      /*! communictor for this group. intercommunicator if i'm a
        member of this gorup; else it's an intracommunicator */
      MPI_Comm comm; 
      /*! my rank in this group if i'm a member; else set to
        MPI_ROOT */
      int rank; 
      /*! size of this group if i'm a member, else size of remote
        group this intracommunicaotr refers to */
      int size; 

      Group() : size(-1), rank(-1), comm(MPI_COMM_NULL), containsMe(false) {};
      void makeIntercomm() 
      { MPI_Comm_rank(comm,&rank); MPI_Comm_size(comm,&size); containsMe = true; }
      void makeIntracomm()
      { containsMe = false; rank = MPI_ROOT; MPI_Comm_remote_size(comm,&size); }
    };

    extern Group world; //! MPI_COMM_WORLD
    extern Group app; /*! for workers: intracommunicator to app
                        for app: intercommunicator among app processes
                      */
    extern Group worker; /*!< group of all ospray workers (often the
                           world root is reserved for either app or
                           load balancing, and not part of the worker
                           group */

    void init(int *ac, const char **av);
  };

}
