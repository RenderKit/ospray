#include "mpi.h"
#include "unistd.h"

int main(int ac, char **av)
{
   int rank,size;
   MPI_Init(&ac,&av);
   MPI_Comm_rank(MPI_COMM_WORLD,&rank);
   MPI_Comm_size(MPI_COMM_WORLD,&size);
   printf("mpi world: %i/%i\n",rank,size);
   sleep(3);
   return 0;
}
