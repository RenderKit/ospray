Download Precompiled OSPRay Binary Packages
===========================================

Prerequisites
-------------

Your CPU must support at least SSE4.1 or NEON to run OSPRay. The TGZ/ZIP
packages contain most needed 3rd party dependencies. Additionally you
need

- To run the example viewer: OpenGL
- To use the distributed, multi-node rendering feature: Intel® [MPI
  Library](https://software.intel.com/en-us/intel-mpi-library/)

We recommend the latest version of both TBB and Embree libraries.

GPU Runtime Requirements
------------------------

To run OSPRay on Intel GPUs you will need to first have drivers
installed on your system.

### GPU drivers on Linux

Install the latest GPGPU drivers for your Intel GPU from:
<https://dgpu-docs.intel.com/>. Follow the driver installation
instructions for your graphics card.

### GPU drivers on Windows

Install the latest GPGPU drivers for your Intel GPU from:
<https://www.intel.com/content/www/us/en/download/785597/intel-arc-iris-xe-graphics-windows.html>.
Follow the driver installation instructions for your graphics card.


Packages
--------

Packages for x86_64 are provided, OSPRay can be built for ARM64/NEON
using the superbuild (see [Building and Finding OSPRay](#building-and-finding-ospray)).

For Linux we provide OSPRay precompiled for 64\ bit (including the GPU module) as a TGZ archive.
[ospray-<OSPRAY_VERSION>.x86_64.linux.tar.gz](https://github.com/ospray/OSPRay/releases/download/v<OSPRAY_VERSION>/ospray-<OSPRAY_VERSION>.x86_64.linux.tar.gz)  

For Mac OS\ X we provide OSPRay as a ZIP archive:  
[ospray-<OSPRAY_VERSION>.x86_64.macosx.zip](https://github.com/ospray/OSPRay/releases/download/v<OSPRAY_VERSION>/ospray-<OSPRAY_VERSION>.x86_64.macosx.zip)

For Windows we provide OSPRay binaries precompiled for 64\ bit
(including the GPU module) as an MSI installer as well as a ZIP archive.
[ospray-<OSPRAY_VERSION>.x86_64.windows.msi](https://github.com/ospray/OSPRay/releases/download/v<OSPRAY_VERSION>/ospray-<OSPRAY_VERSION>.x86_64.windows.msi)  
[ospray-<OSPRAY_VERSION>.x86_64.windows.zip](https://github.com/ospray/OSPRay/releases/download/v<OSPRAY_VERSION>/ospray-<OSPRAY_VERSION>.x86_64.windows.zip)  

The source code of the latest OSPRay version can be downloaded here:  
[ospray-<OSPRAY_VERSION>.zip](https://github.com/ospray/OSPRay/archive/v<OSPRAY_VERSION>.zip)  
[ospray-<OSPRAY_VERSION>.tar.gz](https://github.com/ospray/OSPRay/archive/v<OSPRAY_VERSION>.tar.gz)

You can also access [previous OSPRay releases](https://github.com/ospray/OSPRay/releases).
