
    Copyright 2014-2016 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.


    Description:

        A file loader for seismic volume data based on the FreeDDS library.

    FreeDDS:

       "The Data Dictionary System is an I/O System (and much more) capable of
        handling multi-dimensional seismic datasets (up to 9 dimensions; e.g.
        'axis = t offset shot x y') in various formats including usp (BP Amoco's
        Un*x Seismic Processing), segy (Society of Exploration Geophysicists
        SEG-Y format), segy1 (SEG's SEG-Y Rev 1.0 format), sep (Stanford
        Exploration Project's format), su (Colorado School of Mine's Seismic
        Un*x format) as well as other flexible trace definitions (with or without
        trace headers) such as fcube.  DDS allows different processing systems to
        be used together by piping directly together as well as through actual
        emulation of different processing systems."

        http://freeusp.org/DDS/download.php


