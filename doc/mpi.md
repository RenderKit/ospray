Parallel Rendering with MPI
===========================

OSPRay has the ability to scale to multiple nodes in a cluster via MPI.
This enables applications to take advantage of larger compute and memory
resources when available.

Prerequisites for MPI Mode
--------------------------

In addition to the standard build requirements of OSPRay, you must have
the following items available in your environment in order to build&run
OSPRay in MPI mode:

-   An MPI enabled multi-node environment, such as an HPC cluster
-   An MPI implementation you can build against (i.e. Intel MPI,
    MVAPICH2, etc...)

Enabling the MPI Module in your Build
-------------------------------------

To build the MPI module the CMake option `OSPRAY_MODULE_MPI` must be
enabled, which can be done directly on the command line (with
`-DOSPRAY_MODULE_MPI=ON`) or through a configuration dialog (`ccmake`,
`cmake-gui`), see also [Compiling OSPRay].

This will trigger CMake to go look for an MPI implementation in your
environment. You can then inspect the CMake value of `MPI_LIBRARY` to
make sure that CMake found your MPI build environment correctly.

This will result in an OSPRay module being built. To enable using it,
applications will need to either link `libospray_module_mpi`, or call

    ospLoadModule("mpi");

before initializing OSPRay.

Modes of Using OSPRay's MPI Features
------------------------------------

OSPRay provides two ways of using MPI to scale up rendering: offload and
distributed.

### Offload Rendering

The "offload" rendering mode is where a single (not-distributed) calling
application treats the OSPRay API the same as with local rendering.
However, OSPRay uses multiple MPI connected nodes to evenly distribute
frame rendering work, where each node contains a full copy of all scene
data. This method is most effective for scenes which can fit into
memory, but are very expensive to render: for example, path tracing with
many samples-per-pixel is very compute heavy, making it a good situation
to use the offload feature. This can be done with any application which
already uses OSPRay for local rendering without the need for any code
changes.

When doing MPI offload rendering, applications can optionally enable
dynamic load balancing, which can be beneficial in certain contexts.
This load balancing refers to the distribution of tile rendering work
across nodes: thread-level load balancing on each node is still dynamic
with the thread tasking system. The options for enabling/controlling the
dynamic load balacing features on the `mpi_offload` device are found in
the table below, which can be changed while the application is running.
Please note that these options will likely only pay off for scenes which
have heavy rendering load (e.g. path tracing a non-trivial scene) and
have a lot of variance in how expensive each tile is to render.

  Type  Name                  Default Description
  ----- -------------------- -------- --------------------------------------
  bool  dynamicLoadBalancer     false whether to use dynamic load balancing
  ----- -------------------- -------- --------------------------------------
  : Parameters specific to the `mpi_offload` device

### Distributed Rendering

The "distributed" rendering mode is where a MPI distributed application
(such as a scientific simulation) uses OSPRay collectively to render
frames. In this case, the API expects all calls (both created objects
and parameters) to be the same on every application rank, except each
rank can specify arbitrary geometries and volumes. Each renderer will
have its own limitations on the topology of the data (i.e. overlapping
data regions, concave data, etc.), but the API calls will only differ
for scene objects. Thus all other calls (i.e. setting camera, creating
framebuffer, rendering frame, etc.) will all be assumed to be identical,
but only rendering a frame and committing the model must be in
lock-step. This mode targets using all available aggregate memory for
very large scenes and for "in-situ" visualization where the data is
already distributed by a simulation app.

Running an Application with the "offload" Device
------------------------------------------------

As an example, our sample viewer can be run as a single application
which offloads rendering work to multiple MPI processes running on
multiple machines.

The example apps are setup to be launched in two different setups. In
either setup, the application must initialize OSPRay with the offload
device. This can be done by creating an "`mpi_offload`" device and
setting it as the current device (via the `ospSetCurrentDevice()`
function), or passing either "`--osp:mpi`" or "`--osp:mpi-offload`" as a
command line parameter to `ospInit()`. Note that passing a command line
parameter will automatically call `ospLoadModule("mpi")` to load the MPI
module, while the application will have to load the module explicitly if
using `ospNewDevice()`.

### Single MPI Launch

OSPRay is initialized with the `ospInit()` function call which takes
command line arguments in and configures OSPRay based on what it finds.
In this setup, the app is launched across all ranks, but workers will
never return from `ospInit()`, essentially turning the application into
a worker process for OSPRay. Here's an example of running the
ospVolumeViewer data-replicated, using `c1`-`c4` as compute nodes and
`localhost` the process running the viewer itself:

    mpirun -perhost 1 -hosts localhost,c1,c2,c3,c4 ./ospExampleViewer <scene file> --osp:mpi

### Separate Application&Worker Launches

The second option is to explicitly launch the app on rank 0 and worker
ranks on the other nodes. This is done by running `ospray_mpi_worker` on
worker nodes and the application on the display node. Here's the same
example above using this syntax:

    mpirun -perhost 1 -hosts localhost ./ospExampleViewer <scene file> --osp:mpi \
      : -hosts c1,c2,c3,c4 ./ospray_mpi_worker

This method of launching the application and OSPRay worker separately
works best for applications which do not immediately call `ospInit()` in
their `main()` function, or for environments where application
dependencies (such as GUI libraries) may not be available on compute
nodes.

Running an Application with the "distributed" Device
----------------------------------------------------

Applications using the new distributed device should initialize OSPRay
by creating (and setting current) an "`mpi_distributed`" device or pass
`"--osp:mpi-distributed"` as a command line argument to `ospInit()`.
Note that due to the semantic differences the distributed device gives
the OSPRay API, it is not expected for applications which can already
use the offload device to correctly use the distributed device without
changes to the application.

The following additional parameter can be set on the `mpi_distributed`
device.

  ------- ----------------- ----------------------------------------------
  Type    Name              Description
  ------- ----------------- ----------------------------------------------
  `void*` worldCommunicator  A pointer to the `MPI_Comm` which should be
                             used as OSPRay's world communicator. This will
                             set how many ranks OSPRay should expect to
                             participate in rendering. The default is
                             `MPI_COMM_WORLD` where all ranks are expected
                             to participate in rendering.
  ------ ------------ ----------------------------------------------------
  : Parameters for the `mpi_distributed` device.

By setting the `worldCommunicator` parameter to a different communicator
than `MPI_COMM_WORLD` the client application can tune how OSPRay is run
within its processes. The default uses `MPI_COMM_WORLD` and thus expects
all processes to also participate in rendering, thus if a subset of processes
do not call collectives like `ospRenderFrame` the application would hang.

For example, an MPI parallel application may be
run with one process per-core, however OSPRay is multithreaded and will
perform best when run with one process per-node. By splitting `MPI_COMM_WORLD`
the application can create a communicator with one rank per-node to then
run OSPRay on one process per-node. The remaining ranks on each node
can then aggregate their data to the OSPRay process for rendering.

There are also two optional parameters available on the OSPModel created
using the distributed device, which can be set to tell OSPRay about your
application's data distribution.

  ---------- ----------------- ----------------------------------------------
  Type       Name              Description
  ---------- ----------------- ----------------------------------------------
  `box3f[]`  regions           [data] array of boxes which bound the data owned by
                               the current rank, used for sort-last compositing.
                               The global set of regions specified by all ranks
                               must be disjoint for correct compositing.
        
  `box3f[]`  ghostRegions      Optional [data] array of boxes which bound the ghost data on
                               each rank. Using these shared data between nodes
                               can be used for computing secondary ray effects
                               such as ambient occlusion. If specifying ghostRegions,
                               there should be one ghostRegion for each region.
  ------ ------------ -------------------------------------------------------
  : Parameters for the distributed OSPModel

See the distributed device examples in the MPI module for examples.

The renderer supported when using the distributed device is the `mpi_raycast` renderer.
This renderer is an experimental renderer and currently only supports ambient occlusion
(on the local data only). To compute correct ambient occlusion across the distributed
data the application is responsible for replicating ghost data and specifying the
ghostRegions and regions as described above.

  ---------- ----------------- -------- -------------------------------------
  Type       Name               Default Description
  ---------- ----------------- -------- -------------------------------------
  int        aoSamples                0  number of rays per sample to compute
                                         ambient occlusion
  ------ ------------ -------------------------------------------------------
  : Parameters for the distributed OSPModel

