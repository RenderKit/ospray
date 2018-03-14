FIU Dataset: Shadows and Ambient Occlusion vs OpenGL-like local shading
=======================================================================

(click on images to see higher-res versions)

[![](demos/fiu/fiu-gl-small.jpg)](demos/fiu/fiu-gl.png)

FIU with local (OpenGL-like) Shading
:   Without shadows and/or ambient occlusion (i.e., using a OpenGL-like
    local shading model), the FIU dataset looks like this.

\
[![](demos/fiu/fiu-obj-small.jpg)](demos/fiu/fiu-obj.png)

FIU with ray traced shadows
:   Simply adding shadows already greatly enhances the user's perception
    of depth (e.g., the relative positions of streamlines and base
    geometry, the actual shape of the geometry, etc).

\
[![](demos/fiu/fiu-ao-small.jpg)](demos/fiu/fiu-ao.png)

FIU with OSPRay's Ambient Occlusion Renderer
:   Ambient Occlusion further increases perception of depth. In OSPRay,
    enabling (true, ray-cast) Ambient Occlusion is as simple as
    selecting the "ao" renderer.


