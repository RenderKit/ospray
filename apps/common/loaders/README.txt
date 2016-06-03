
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

      A set of loaders for common file types that are domain-agnostic.
      All loaders are subtypes of generic classes which specify common
      interfaces.  In general, applications should invoke the methods
      defined in these generic interfaces.  A specific loader is chosen
      at runtime based on the extension in the file name passed to the
      generic method.

      Generic Methods:

        OSPObject *ObjectFile::importObjects(const char *filename)

            A file containing state for one or more OSPRay objects.
            The return type is a NULL-terminated list of OSPObject.

        OSPVolume VolumeFile::importVolume(const char *filename, OSPVolume volume)

            A file potentially containing structured or unstructured
            volumetric data.

      Supported File Formats:

        OSP (file extension ".osp")

            An XML implementation of the ObjectFile interface.  This
            loader initializes but does not commit the OSPRay state for
            the objects, enabling the application to make further changes.
            The introspection functions in the OSPRay API (ospGet*) can
            be used to get information about the loaded objects and their
            parameters.

        RAW (file extension ".raw")

            An implementation of the VolumeFile interface for volumetric
            data stored as a brick of values.  This file type is not self-
            describing and so requires additional meta-data, typically in
            the form of an OSP file.

