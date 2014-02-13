// ospray proper - has to come first (for mpi.h)
#include "../../ospray/mpi/mpicommon.h"
// ospray api, for the viewer
#include "ospray/ospray.h"
// DTM
#include "dtm.h"

namespace ospray {
  namespace dtm {
    using std::endl;
    using std::cout;

    std::string inputFile = "";
    
    void usage(const std::string &msg="") 
    {
      // make sure we emit error messages only on root rank 0
      if (mpi::app.rank != 0)
        return;

      cout << endl;
      if (msg != "") cout << "Error: " << msg << endl << endl;
      cout << "usage:" << endl;
      cout << " mpirun <...> ./dtmViewer --osp:mpi-launch <launchscript> inputFiles.dtm.idx" << endl;
      cout << endl;
      exit(0);
    }


    struct Brick {
      vec3i coord;
      int   numTris;
      int   owner;
    };
    std::vector<Brick*> brickList;
    
    void loadIndexFile(const char *idxFileName)
    {
      FILE *idxFile = fopen(idxFileName,"r");
      if (!idxFile) usage("could not open input file"+std::string(idxFileName));
      
      const int LINESZ = 10000;
      char line[LINESZ];
      while (fgets(line,10000,idxFile) && !feof(idxFile)) {
        Brick *brick = new Brick;
        sscanf(line,"cell %i %i %i numtris %i",
               &brick->coord.x,
               &brick->coord.y,
               &brick->coord.z,
               &brick->numTris);
        brickList.push_back(brick);
      }
    }
    void parseArgs(int ac, const char **av)
    {
      if (ac == 2)
        inputFile = av[1];
      if (inputFile == "")
        usage("no input file specified");
    }
    
    void runViewer()
    {
      PING;
      while(1) sleep(10);
      // glutXyz()....
    }

    void runRemoteData()
    {
      PING;
      while(1) sleep(10);
    }
  
    void main(int ac, const char **av)
    {
      // initialize ospray such that rank0 can issue api calls (rank0
      // runs the viewer), and all other ranks provide DTM data (and
      // do the rendering work.
      ospInit(&ac,av);

      if (mpi::app.size == -1)
      { // make sure we're emitting error messages only from rank 0
        // here. use explicit MPI call here; ospray::mpi:: stuff may
        // not be initialized, yet
        MPI_Init(&ac,(char ***)&av);
        int rank;
        MPI_Comm_rank(MPI_COMM_WORLD,&rank);
        if (rank == 0)
          printf("OSPRay MPI mode not initialized ... \n"
                 "did you call with '--osp:mpi-lauch' or '--osp:mpi-listen[:<filename>]'?\n");
        exit(-1);
      }
      
      if (mpi::app.size == 1)
        usage("dtmViewer must be started in an mpi process with at least 2 processes");

      parseArgs(ac,av);
      if (mpi::world.rank == 0) {
        runViewer();
      } else {
        runRemoteData(); 
      }
    }
  }
}

int main(int ac, const char **av)
{
  ospray::dtm::main(ac,av);
  return 0;
}
