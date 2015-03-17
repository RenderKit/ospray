
                 INTEL CORPORATION PROPRIETARY INFORMATION

    This software is supplied under the terms of a license agreement or
    nondisclosure agreement with Intel Corporation and may not be copied
    or disclosed except in accordance with the terms of that agreement.
    Copyright (C) 2014 Intel Corporation. All Rights Reserved.

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

