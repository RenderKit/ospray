Parallel Rendering with MPI
===========================

OSPRay has the ability to scale to multiple nodes in a cluster via MPI.
This enables applications to take advantage of larger compute and memory
resources when available.

Prerequisites for MPI Mode
--------------------------

In addition to the standard build requirements of OSPRay, you must have
the following items available in your environment in order to
build & run OSPRay in MPI mode:

-   An MPI enabled multi-node environment, such as an HPC cluster
-   An MPI implementation you can build against (i.e., Intel MPI,
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
memory, but are expensive to render: for example, path tracing with
many samples-per-pixel is compute heavy, making it a good situation
to use the offload feature. This can be done with any application which
already uses OSPRay for local rendering without the need for any code
changes.

When doing MPI offload rendering, applications can optionally enable
dynamic load balancing, which can be beneficial in certain contexts.
This load balancing refers to the distribution of tile rendering work
across nodes: thread-level load balancing on each node is still dynamic
with the thread tasking system. The options for enabling/controlling the
dynamic load balancing features on the `mpi_offload` device are found in
the table below, which can be changed while the application is running.
Please note that these options will likely only pay off for scenes which
have heavy rendering load (e.g., path tracing a non-trivial scene) and
have much variance in how expensive each tile is to render.

  Type  Name                  Default  Description
  ----- -------------------- --------  --------------------------------------
  bool  dynamicLoadBalancer     false  whether to use dynamic load balancing
  ----- -------------------- --------  --------------------------------------
  : Parameters specific to the `mpi_offload` device.

### Distributed Rendering

The "distributed" rendering mode is where a MPI distributed application
(such as a scientific simulation) uses OSPRay collectively to render
frames. In this case, the API expects all calls (both created objects
and parameters) to be the same on every application rank, except each
rank can specify arbitrary geometries and volumes. Each renderer will
have its own limitations on the topology of the data (i.e., overlapping
data regions, concave data, etc.), but the API calls will only differ
for scene objects. Thus all other calls (i.e., setting camera, creating
framebuffer, rendering frame, etc.) will all be assumed to be identical,
but only rendering a frame and committing the model must be in
lock-step. This mode targets using all available aggregate memory for
huge scenes and for "in-situ" visualization where the data is
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

### Separate Application & Worker Launches

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

  ------ ------------------ ----------------------------------------------------
  Type   Name               Description
  ------ ------------------ ----------------------------------------------------
  void*  worldCommunicator  A pointer to the `MPI_Comm` which should be used as
                            OSPRay's world communicator. This will set how many
                            ranks OSPRay should expect to participate in
                            rendering. The default is `MPI_COMM_WORLD` where all
                            ranks are expected to participate in rendering.
  ------ ------------------ ----------------------------------------------------
  : Parameters for the `mpi_distributed` device.

By setting the `worldCommunicator` parameter to a different communicator
than `MPI_COMM_WORLD` the client application can tune how OSPRay is run
within its processes. The default uses `MPI_COMM_WORLD` and thus expects
all processes to also participate in rendering, thus if a subset of
processes do not call collectives like `ospRenderFrame` the application
would hang.

For example, an MPI parallel application may be run with one process
per-core, however OSPRay is multithreaded and will perform best when run
with one process per-node. By splitting `MPI_COMM_WORLD` the application
can create a communicator with one rank per-node to then run OSPRay on
one process per-node. The remaining ranks on each node can then
aggregate their data to the OSPRay process for rendering.

The model used by the distributed device takes three additional
parameters, to allow users to express their data distribution to OSPRay.
All models should be disjoint to ensure correct sort-last compositing.
Geometries used in the distributed MPI renderer can make use of the
[SciVis renderer]'s [OBJ material].

  ------ ------------- ---------------------------------------------------------
  Type   Name          Description
  ------ ------------- ---------------------------------------------------------
  int    id            An integer that uniquely identifies this piece of
                       distributed data. For example, in a common case of one
                       sub-brick per-rank, this would just be the region's MPI
                       rank. Multiple ranks can specify models with the same ID,
                       in which case the rendering work for the model will be
                       shared among them.

  vec3f  region.lower  Override the original model geometry + volume bounds with
                       a custom lower bound position. This can be used to clip
                       geometry in the case the objects cross over to another
                       region owned by a different node. For example, rendering
                       a set of spheres with radius.

  vec3f  region.upper  Override the original model geometry + volume bounds with
                       a custom upper bound position.
  ------ ------------- ---------------------------------------------------------
  : Parameters for the distributed `OSPModel`.

The renderer supported when using the distributed device is the
`mpi_raycast` renderer. This renderer is an experimental renderer and
currently only supports ambient occlusion (on the local data only, with
optional ghost data). To compute correct ambient occlusion across the
distributed data the application is responsible for replicating ghost
data and specifying the ghost models and models as described above.
Note that shadows and ambient occlusion are computed on the local geometries,
in the `model` and the corresponding `ghostModel` in the ghost model array,
if any where set.

  -------------------- ---------------------- --------  ------------------------
  Type                 Name                    Default  Description
  -------------------- ---------------------- --------  ------------------------
  OSPModel/OSPModel[]  model                      NULL  the [model] to render,
                                                        can optionally be a
                                                        [data] array of multiple
                                                        models

  OSPModel/OSPModel[]  ghostModel                 NULL  the optional [model]
                                                        containing the ghost
                                                        geometry for ambient
                                                        occlusion; when setting
                                                        a [data] array for both
                                                        `model` and
                                                        `ghostModel`, each
                                                        individual ghost model
                                                        shadows only its
                                                        corresponding model

  OSPLight[]           lights                           [data] array with
                                                        handles of the [lights]

  int                  aoSamples                     0  number of rays per
                                                        sample to compute
                                                        ambient occlusion

  bool                 aoTransparencyEnabled     false  whether object
                                                        transparency is
                                                        respected when computing
                                                        ambient occlusion
                                                        (slower)

  bool                 oneSidedLighting           true  if true, backfacing
                                                        surfaces (wrt. light
                                                        source) receive no
                                                        illumination
  -------------------- ---------------------- --------  ------------------------
  : Parameters for the `mpi_raycast` renderer.

See the distributed device examples in the MPI module for examples.

