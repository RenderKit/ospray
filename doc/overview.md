OSPRay Overview
===============

OSPRay is an **o**pen source, **s**calable, and **p**ortable **ray**
tracing engine for high-performance, high-fidelity visualization on
Intel® Architecture CPUs. OSPRay is released under the permissive
[Apache 2.0 license](http://www.apache.org/licenses/LICENSE-2.0).

The purpose of OSPRay is to provide an open, powerful, and easy-to-use
rendering library that allows one to easily build applications that use
ray tracing based rendering for interactive applications (including both
surface- and volume-based visualizations). OSPRay is completely
CPU-based, and runs on anything from laptops, to workstations, to
compute nodes in HPC systems.

OSPRay internally builds on top of [Embree] and [ISPC (Intel® SPMD
Program Compiler)](https://ispc.github.io/), and fully utilizes modern
instruction sets like Intel® SSE4, AVX, AVX2, and AVX-512 to achieve
high rendering performance, thus a CPU with support for at least SSE4.1
is required to run OSPRay.


OSPRay Support and Contact
--------------------------

OSPRay is under active development, and though we do our best to
guarantee stable release versions a certain number of bugs,
as-yet-missing features, inconsistencies, or any other issues are
still possible. Should you find any such issues please report
them immediately via [OSPRay's GitHub Issue
Tracker](https://github.com/ospray/OSPRay/issues) (or, if you should
happen to have a fix for it,you can also send us a pull request); for
missing features please contact us via email at
<ospray@googlegroups.com>.

For recent news, updates, and announcements, please see our complete
[news/updates] page.

Join our [mailing
list](https://groups.google.com/forum/#!forum/ospray-announce/join) to
receive release announcements and major news regarding OSPRay.

