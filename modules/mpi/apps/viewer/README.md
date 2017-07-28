# ospDistribViewerDemo

This app demonstrates how to write an distributed scivis style
interactive renderer using the distributed MPI device. Note that because
OSPRay uses sort-last compositing it is up to the user to ensure
that the data distribution across the nodes is suitable. Specifically,
each nodes' data must be convex and disjoint. This renderer
supports multiple volumes and geometries per-node, to ensure they're
composited correctly you specify a list of bounding regions to the
model, within these regions can be arbitrary volumes/geometries
and each rank can have as many regions as needed. As long as the
regions are disjoint/convex the data will be rendered correctly.
In this demo we either generate a volume, or load a RAW volume file
if one is passed on the commandline.

## Loading a RAW Volume

To load a RAW volume you must specify the filename (`-f <file>`), the
data type (`-dtype <dtype>`), the dimensions (`-dims X Y Z`) and the
value range for the transfer function (`-range MIN MAX`). For example,
to run on the [CSAFE dataset from the demos page](http://www.ospray.org/demos.html#csafe-heptane-gas-dataset)
you would pass the following arguments:

```
mpirun -np N ./ospDistribViewerDemo \
    -f <path to csafe>/csafe-heptane-302-volume.raw \
    -dtype uchar -dims 302 302 302 -range 0 255
```

The volume file will then be chunked up into an `X x Y x Z` grid such that
`N = X * Y * Z`. See `loadVolume` in [gensv/generateSciVis.cpp](../gensv/generateSciVis.cpp)
for an example of how
to properly load a volume distributed across ranks with correct specification of brick positions
and ghost voxels for interpolation at boundaries. If no volume file data is passed a volume will be
generated instead, in that case see `makeVolume`.

## Geometry

The viewer can also display some randomly generated sphere geometry if you
pass `-spheres N` where `N` is the number of spheres to generate per-node.
These spheres will be generated inside the bounding box of the region's volume
data.

In the case that you have geometry crossing the boundary of nodes
and are replicating it on both nodes to render (ghost zones, etc.)
the region will be used by the renderer to clip rays against allowing
to split the object between the two nodes, with each rendering half.
This will keep the regions rendered by each rank disjoint and thus
avoid any artifacts. For example, if a sphere center is on the border
between two nodes, each would render half the sphere and the halves
would be composited to produce the final complete sphere in the image.

## App-initialized MPI

Passing the `-appMPI` flag will have the application initialize MPI instead of
letting OSPRay do it internally when creating the MPI distributed device. In this
case OSPRay will not finalize MPI when cleaning up the device, allowing the application
to use OSPRay for some work, shut it down and recreate everything later if needed
for additional computation, without accidentally shutting down its MPI communication.

## Interactive Viewer

Rank 0 will open an interactive window with GLFW and display the rendered image.
When the application state needs to update (e.g camera or transfer function changes),
this information is broadcasted out to the other nodes to update their scene data.

