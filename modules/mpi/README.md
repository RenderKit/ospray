OSPRay MPI Module
=================

This is release v2.0.0 (alpha) of Intel® OSPRay's MPI Module. For changes and new
features see the [changelog](CHANGELOG.md). Visit http://www.ospray.org
for more information.

OSPRay MPI Module Overview
==========================

Intel OSPRay is an **o**pen source, **s**calable, and **p**ortable
**ray** tracing engine for high-performance, high-fidelity visualization
on Intel Architecture CPUs. OSPRay is part of the [Intel oneAPI
Rendering Toolkit](https://software.intel.com/en-us/rendering-framework)
and is released under the permissive [Apache 2.0
license](http://www.apache.org/licenses/LICENSE-2.0).

The purpose of the MPI module for OSPRay is to provide distributed rendering
capabilities for OSPRay. The module enables image- and data-parallel rendering
across HPC clusters using MPI, allowing applications to transparently distribute
rendering work, or to render data sets which are too large to fit in memory on
a single machine.

OSPRay internally builds on top of [Intel
Embree](https://embree.github.io/) and [ISPC (Intel SPMD Program
Compiler)](https://ispc.github.io/), and fully exploits modern
instruction sets like Intel SSE4, AVX, AVX2, and AVX-512 to achieve high
rendering performance, thus a CPU with support for at least SSE4.1 is
required to run OSPRay.
The MPI module requires an MPI library which supports at least
`MPI_THREAD_SERIALIZED`.


OSPRay Support and Contact
--------------------------
OSPRay is under active development, and though we do our best to
guarantee stable release versions a certain number of bugs,
as-yet-missing features, inconsistencies, or any other issues are still
possible. Should you find any such issues please report them immediately
via [OSPRay's GitHub Issue
Tracker](https://github.com/ospray/OSPRay/issues) (or, if you should
happen to have a fix for it, you can also send us a pull request).
For bugs specific to the MPI module and distributed rendering, please report
them to the [MPI Module's GitHub Issue Tracker](https://github.com/ospray/module_mpi/issues)
For missing features please contact us via email at
<a href="mailto:ospray@googlegroups.com" class="email">ospray@googlegroups.com</a>.

For recent news, updates, and announcements, please see our complete
[news/updates](https://www.ospray.org/news.html) page.

Join our [mailing
list](https://groups.google.com/forum/#!forum/ospray-announce/join) to
receive release announcements and major news regarding OSPRay.

[![Join the chat at
https://gitter.im/ospray/ospray](https://ospray.github.io/images/gitter_badge.svg)](https://gitter.im/ospray/ospray?utm_source=badge&utm_medium=badge&utm_content=badge)

Building the MPI Module
=======================

The latest MPI module sources are always available at the [MPI Module GitHub
repository](https://github.com/ospray/module_mpi). The default `master`
branch should always point to the latest bugfix release.

Prerequisites
-------------

OSPRay v2.0.0 (alpha) is required, and can be built from source following the
instructions on the [OSPRay GitHub Repository](http://github.com/ospray/ospray).
After getting the OSPRay source code clone this module into the `modules/`
subdirectory of OSPRay, and enable the MPI module by setting
`OSPRAY_MODULE_MPI=ON` in CMake. When building OSPRay with
`OSPRAY_ENABLE_APPS=ON` the MPI module's [tutorial apps](tutorials/) will be built,
which demonstrate the various features of the module.


Documentation
=============

The MPI module provides two OSPRay devices to allow applications to leverage
distributed rendering capabilities. The `mpiOffload` device provides transparent
image-parallel rendering, where the same OSPRay application written for local
rendering can be replicated across multiple nodes to distribute the rendering
work. The `mpiDistributed` device allows MPI distributed
applications to use OSPRay for distributed rendering, where each rank can
render and independent piece of a global data set, or hybrid rendering where
ranks partially or completely share data.


MPI Offload Rendering
---------------------

The `mpiOffload` device can be used to distribute image rendering tasks across
a cluster without requiring modifications to the application itself. Existing
applications using OSPRay for local rendering simply be passed command line
arguments to load the module and indicate that the `mpiOffload` device should
be used for image-parallel rendering. To load the module, pass
`--osp:module:mpi`, to select the MPIOffloadDevice, pass
`--osp:device:mpiOffload`.
For example, the `ospExamples` application can be run as:

```
mpirun -n <N> ./ospExamples --osp:module:mpi --osp:device:mpiOffload
```

and will automatically distribute the image rendering tasks among the
corresponding `N` nodes. Note that in this configuration rank 0 will act as
a master/application rank, and will run the user application code but not
perform rendering locally. Thus a minimum of 2 ranks are required, one master
to run the application and one worker to perform the rendering. Running with
3 ranks for example would now distribute half the image rendering work to rank 1
and half to rank 2.

If more control is required over the placement of ranks to nodes, or you want
to run a worker rank on the master node as well you can run the application
and the `ospray_mpi_worker` program through MPI's MPMD mode. The
`ospray_mpi_worker` will load the MPI module and select the offload device
by default.

```
mpirun -n 1 ./ospTutorialBoxes --osp:module:mpi --osp:device:mpi \
  : -n <N> ./ospray_mpi_worker
```

Finally, you can also run the workers in a server mode on a remote machine
and connect your application to them over a socket. This allows remote
rendering on a large cluster while displaying on a local machine (e.g., a laptop)
where the two devices may not be able to connect over MPI. First, launch
the workers in `mpi-listen` mode:

```
mpirun -n <N> ./ospray_mpi_worker --osp:deviceparam mpiMode mpi-listen
```

The workers will print out a port number to connect to, e.g.,
`#osp: Listening on port #####` You can then run your application in the
`mpi-connect` mode, and pass the host name of the first worker rank and this
port number to the device:

```
./ospTutorialBoxes --osp:module:mpi --osp:device:mpi \
  --osp:deviceparam mpiMode mpi-connect \
  --osp:deviceparam host <worker rank 0 host>:<port>
```

MPI Distributed Rendering
-------------------------

While MPI Offload rendering is used to transparently distribute rendering work
without requiring modification to the application, MPI Distributed rendering
is targetted at use of OSPRay within MPI-parallel applications. The MPI
distributed device can be selected by loading the `mpi` module, and manually
creating and using an instance of the `mpiDistributed` device.

```c
ospLoadModule("mpi");

OSPDevice mpiDevice = ospNewDevice("mpiDistributed");
ospDeviceCommit(mpiDevice);
ospSetCurrentDevice(mpiDevice);
```

Your application can either initialize MPI before-hand, ensuring that
`MPI_THREAD_SERIALIZED` or higher is supported, or allow the device to
initialize MPI on commit. When using the distributed device each rank can
specify independent local data using the OSPRay API, as if rendering locally.
However, when calling `ospRenderFrameAsync` the ranks will work collectively
to render the data. The distributed device supports both image-parallel,
where the data is replicated, and data-parallel, where the data is distributed,
rendering modes. The `mpiDistributed` device will by default use each rank in
`MPI_COMM_WORLD` as a render worker; however, it can also take a specific MPI
communicator to use as the world communicator. Only those ranks in the specified
communicator will participate in rendering.

| Type  | Name              | Default             | Description                |
|:------|:------------------|--------------------:|:---------------------------|
| void* | worldCommunicator |    MPI\_COMM\_WORLD | The MPI communicator which the OSPRay workers should treat as their world

: Parameters specific to the distributed `mpiDistributed` Device.

| Type         | Name    | Default| Description                                |
|:-------------|:--------|-------:|:-------------------------------------------|
| OSPBox3f\[\] | regions |    NULL| A list of bounding boxes which bound the owned local data to be rendered by the rank |

: Parameters specific to the distributed `OSPWorld`.


| Type         | Name    | Default| Description                                |
|:-------------|:--------|-------:|:-------------------------------------------|
| aoSamples    | int     |      0 | The number of AO samples to take per-pixel |
| aoRadius     | float   |   1e20f| The AO ray length to use. Note that if the AO ray would have crossed a rank boundary and ghost geometry is not available, there  will be visible artifacts in the shading. |

: Parameters specific to the `mpiRaycast` renderer.


### Image Parallel Rendering in the MPI Distributed Device

If all ranks specify the exact same data, the distributed device can be used
for image-parallel rendering. This works identical to the offload device,
except that the MPI-aware application is able to load data in parallel on each
rank rather than loading on the master and shipping data out to the workers.
When a parallel file system is available, this can improve data load times.
Image-parallel rendering is selected by specifying the same data on each rank,
and using any of the existing local renderers (e.g., scivis, pathtracer).
See [ospMPIDistributedTutorialReplicatedData](tutorials/ospMPIDistributedTutorialReplicatedData.cpp)
for an example.

### Data Parallel Rendering in the MPI Distributed Device

The MPI Distributed device also supports data-parallel rendering with sort-last
compositing. Each rank can specify a different piece of data, as long as the
bounding boxes of each rank's data are non-overlapping. The rest of the
scene setup is similar to local rendering; however, for distributed rendering
only the `mpiRaycast` renderer is supported. This renderer implements a subset
of the `scivis` rendering features which are suitable for implementation in
a distributed environment.

By default the aggregate bounding box of the instances in the local world will
be used as the bounds of that rank's data. However, when using ghost zones for
volume interpolation, geometry or ambient occlusion, each rank's data can
overlap. To clip these non-owned overlap regions out a set of `regions` can
pass as a parameter to the `OSPWorld` being rendered. Each rank can specify
one or more non-overlapping `box3f`'s which bound the portions of its local
data which it is reponsible for rendering.
See the [ospMPIDistributedTutorialStructuredVolume](tutorials/ospMPIDistributedTutorialStructuredVolume.cpp)
for an example.

Finally, the MPI distributed device also supports hybrid-parallel rendering,
where multiple ranks can share a single piece of data. For each shared piece
of data the rendering work will be assigned image-parallel among the ranks.
Partially-shared regions are determined by finding those ranks specifying data
with the same bounds (or `regions`) and merging them. See the
[ospMPIDistributedTutorialPartiallyReplicatedData](tutorials/ospMPIDistributedTutorialPartiallyReplicatedData.cpp)
for an example.


Interaction With User Modules
----------------------------

The MPI Offload rendering mode trivially supports user modules, with the caveat
that attempting to share data directly with the application (e.g., passing a `void*`
or other tricks to the module) will not work in a distributed environment.
Instead, use the `ospNewSharedData` API to share data from the application with
OSPRay, which will in turn be copied over the network to the workers.

The MPI Distributed device also supports user modules, as all that is required
for compositing the distributed data are the bounds of each rank's local data.


