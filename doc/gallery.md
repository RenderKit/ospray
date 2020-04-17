OSPRay Gallery
==============

This page contains a few sample screenshots of different renderings done
with OSPRay. If *you* have created any notable images through OSPRay
and would like to share them on this page, please [send us an
email](mailto:ospray@googlegroups.com).


<div class="exhibit">
<div class="caption">
<h2>Moana Island Scene Screen Capture</h2>
[![Moana Island Scene](gallery/moana_video.jpg)](https://www.youtube.com/watch?v=p0EJo0pZ3QI "Moana Island Scene")
<p>Real-time screen capture of OSPRay rendering the moana island scene.</p>
</div>
</div>

<div class="exhibit">
<div class="caption">
<h2>DKRZ Weather Visualization in ParaView</h2>
[![DKRZ](gallery/dkrz_video.jpg)](https://youtu.be/aNz22XzjIV0 "DKRZ")
<p>Path traced volumetric rendering of a clouds in a climate simulation.</p>
</div>
</div>

<div class="exhibit">
<div class="caption">
<h2>Rhino Viewport OSPRay plugin</h2>
[![Rhino Viewport](gallery/rhino_video.jpg)](https://vimeo.com/405661583 "Rhino Viewport")
<p>Real-time screen capture of the "osprey" Rhino plugin by Darby Johnston.</p>
</div>
</div>

<div class="exhibit">
<div class="caption">
<h2>AMR Binary Black Hole Merger</h2>
[![Cosmos Video](gallery/cosmos_video.png)](https://vimeo.com/237987637 "Cosmos Video")
<p>Simulation of two black holes colliding using GRChombo in reference to a LIGO event.  Simulation was conducted by
[the Centre for Theoretical Cosmology (CTC)](http://www.ctc.cam.ac.uk/).  Images were 
rendered in ParaView by directly sampling the AMR structure using OSPRay.</p>
</div>
</div>

<div class="exhibit">
<div class="caption">
<h2>NASA Ames Parachute Simulation</h2>
[![NASA Parachute Video](gallery/nasa_parachute_video.png)](https://vimeo.com/237987416 "NASA Parachute Video")
<p>Video rendered using OSPRay by Tim Sandstrom of NASA Ames.</p>
</div>
</div>

Bentley Virtual Showroom
------------------------------------------------

<div class="left">
[![](gallery/bentley-thumb.jpg)](gallery/bentley.jpg)
</div>

Photorealistic rendering of a Bentley virtual showroom using OSPRay's path tracer.

Moana Island Scene
------------------------------------------------

<div class="left">
[![](gallery/moana1-thumb.jpg)](gallery/moana1.jpg)
[![](gallery/moana2-thumb.jpg)](gallery/moana2.jpg)
[![](gallery/moana3-thumb.jpg)](gallery/moana3.jpg)
[![](gallery/moana4-thumb.jpg)](gallery/moana4.jpg)
</div>

The Moana Island Scene rendered in OSPRay, publicly released by
[Disney](https://www.technology.disneyanimation.com/islandscene).
The scene consists of 39.3 million instances, 261.1 million unique quads,
 and 82.4 billion instanced quads.  The cloud
dataset was also added to the scene to highlight OSPRay's
 path traced volume rendering capabilities.  A video of
the scene being rendered interactively can be found [here](https://www.youtube.com/watch?v=p0EJo0pZ3QI&t=11s).


10TB Walls Dataset
------------------------------------------------

<div class="left">
[![](gallery/walls_4k_00-thumb.jpg)](gallery/walls_4k_00.png)
[![](gallery/walls_4k_01-thumb.jpg)](gallery/walls_4k_01.png)
[![](gallery/walls_4k_05-thumb.jpg)](gallery/walls_4k_05.png)
[![](gallery/walls_4k_11-thumb.jpg)](gallery/walls_4k_11.png)
[![](gallery/walls_4k_21-thumb.jpg)](gallery/walls_4k_21.png)
</div>

Some of the forty time steps of the `walls` data set, which is a
simulation of the formation of domain walls in the early universe by
[the Centre for Theoretical Cosmology (CTC)](http://www.ctc.cam.ac.uk/)
in cooperation with [SGI](http://www.sgi.com/). Each volume has a
resolution of 4096^3^, thus the total data set is *10TB* large. With
OSPRay running on a SGI UV300 system you can interactively explore the
data -- watch the [movie](gallery/walls_4k.mp4). For more background
read the related [blog post by
SGI](http://blog.sgi.com/accelerating-scientific-discovery-through-visualization/).

OSPRay Parallel Rendering on TACC's "Stallion" Display Wall
-----------------------------------------------------------

[![](gallery/ospray_stallion-thumb.jpg)](displaywall.html)

This shows a photo of OSPRay parallel rendering on TACC's 320 MPixel
"Stallion" Display wall.

[Read more.](displaywall.html)


VMD/Tachyon Screenshots
-----------------------

This section shows some screenshots from the Tachyon module applied to
models exported from the widely used "Visual Molecular Dynamics" (VMD)
tool. In that workflow, a VMD user uses the VMD command
`export Tachyon mymodel.tachy` to export the VMD model in tachyon
format, then uses the `ospTachyon mymodel.tachy` viewer on those models.

[![](gallery/ospTachyon-glpf-thumb.jpg)](gallery/ospTachyon-glpf.png)

VMD "GLPF" model; original model courtesy John Stone, UIUC.\

[![](gallery/ospTachyon-organelle-thumb.jpg)](gallery/ospTachyon-organelle.png)

VMD "Organelle", using vdW-representation via ospray spheres. Model
courtesy Carsten Kutzner, MPI BPC, Göttingen.\

[![](gallery/ospTachyon-ribosome-thumb.jpg)](gallery/ospTachyon-ribosome.png)

VMD "ribosome", with balls, sticks, ribbons, and quicksurfs. Model
courtesy Carsten Kutzner, MPI BPC, Göttingen.

Other
-----------------------

[![](gallery/obj-xfrog-thumb.jpg)](gallery/obj-xfrog.png)

The "xfrog" model of 1.7 billion (instanced) triangles and transparency
textures. Model originally created using XFrog, model courtesy Oliver
Deussen, University of Konstanz.\