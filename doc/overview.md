OSPRay Overview
===============

Intel® OSPRay is an **o**pen source, **s**calable, and **p**ortable
**ray** tracing engine for high-performance, high-fidelity visualization
on Intel Architecture CPUs, Intel Xe GPUs, and ARM64 CPUs. OSPRay is part
of the [Intel Rendering Toolkit (Render
Kit)](https://software.intel.com/en-us/rendering-framework) and is
released under the permissive [Apache 2.0
license](http://www.apache.org/licenses/LICENSE-2.0).

The purpose of OSPRay is to provide an open, powerful, and easy-to-use
rendering library that allows one to easily build applications that use
ray tracing based rendering for interactive applications (including both
surface- and volume-based visualizations). OSPRay runs on anything from
laptops, to workstations, to compute nodes in HPC systems.

OSPRay internally builds on top of Intel [Embree], Intel [Open VKL], and
Intel [Open Image Denoise]. The CPU implementation is based on
Intel [ISPC (Implicit SPMD Program Compiler)](https://ispc.github.io/)
and fully exploits modern instruction sets like Intel SSE4, AVX, AVX2,
AVX-512 and NEON to achieve high rendering performance. Hence, a CPU
with support for at least SSE4.1 is required to run OSPRay on x86_64
architectures, or a CPU with support for NEON is required to run OSPRay
on ARM64 architectures.

OSPRay's GPU implementation (beta status) is based on the
[SYCL](https://www.khronos.org/sycl/) cross-platform programming
language implemented by [Intel oneAPI Data Parallel C++
(DPC++)](https://www.intel.com/content/www/us/en/developer/tools/oneapi/data-parallel-c-plus-plus.html)
and currently supports Intel Arc™ GPUs on Linux and Windows, and Intel
Data Center GPU Flex and Max Series on Linux, exploiting ray tracing
hardware support.


OSPRay Support and Contact
--------------------------

OSPRay is under active development, and though we do our best to
guarantee stable release versions a certain number of bugs,
as-yet-missing features, inconsistencies, or any other issues are still
possible. For any such requests or findings please use [OSPRay's GitHub
Issue Tracker](https://github.com/ospray/OSPRay/issues) (or, if you
should happen to have a fix for it, you can also send us a pull
request).

To receive release announcements simply ["Watch" the OSPRay
repository](https://github.com/ospray/OSPRay) on GitHub.

