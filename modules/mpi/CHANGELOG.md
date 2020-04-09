Version History
---------------

### Changes in v2.1.0:

-   Add suport for `ospGetTaskDuration` to query the render time of a
    asynchronous (or synchronous) renderFrame call
-   Use flush bcasts to allow us to use non-owning views for data
    transfer. Note that shared `ospData` with strides is currently
    transmitted as whole
-   Fix member variable type for bcast
-   Fix incorrect data size computation in `offload` device
-   Fix large data chunking support for MPI Bcast

### Changes in v2.0.0:

-   The MPI module is now provided separately from the main OSPRay
    repository
-   Users can now extend OSPRay with custom distributed renderers and
    compositing operations, by extending
    `ospray::mpi::DistributedRenderer` and the
    `ospray::mpi::TileOperation`, respectively. See the
    `ospray::mpi::DistributedRaycastRenderer` for an example to start
    from.
-   The MPI Offload device can now communicate over sockets, allowing
    for remote rendering on clusters in the listen/connect mode
-   Data and commands are now sent asynchronously to the MPI workers in
    the Offload device, overlapping better with application work. The
    number of data copies performed has also been significantly reduced,
    and should improve load times
-   The MPI Distributed device will now infer the rank's local data
    bounds based on the volumes and geometry specified if no bounding
    boxes are specified
-   When specifying custom bounds on each rank IDs are no longer
    required, and ranks sharing data will be determined by finding any
    specifying the same bounding boxes. This will also be done if no
    bounds are specified, allowing automatic image and hybrid parallel
    rendering.
-   The MPI Distributed device can now be used for image-parallel
    rendering (e.g., same as offload), where now each application can
    load data in parallel. See the
    `ospMPIDistributedTutorialReplicatedData` for an example.
