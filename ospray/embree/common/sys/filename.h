// ======================================================================== //
// Copyright 2009-2015 Intel Corporation                                    //
//                                                                          //
// Licensed under the Apache License, Version 2.0 (the "License");          //
// you may not use this file except in compliance with the License.         //
// You may obtain a copy of the License at                                  //
//                                                                          //
//     http://www.apache.org/licenses/LICENSE-2.0                           //
//                                                                          //
// Unless required by applicable law or agreed to in writing, software      //
// distributed under the License is distributed on an "AS IS" BASIS,        //
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. //
// See the License for the specific language governing permissions and      //
// limitations under the License.                                           //
// ======================================================================== //

#pragma once

#include "platform.h"

namespace embree
{
  /*! Convenience class for handling file names and paths. */
  class FileName
  {
  public:

    /*! create an empty filename */
    OSPRAY_EMBREE_INTERFACE FileName();

    /*! create a valid filename from a string */
    OSPRAY_EMBREE_INTERFACE FileName(const char* filename);

    /*! create a valid filename from a string */
    OSPRAY_EMBREE_INTERFACE FileName(const std::string& filename);

    /*! auto convert into a string */
    OSPRAY_EMBREE_INTERFACE operator std::string() const { return filename; }

    /*! returns a string of the filename */
    OSPRAY_EMBREE_INTERFACE const std::string str() const { return filename; }

    /*! returns a c-string of the filename */
    OSPRAY_EMBREE_INTERFACE const char* c_str() const { return filename.c_str(); }

    /*! returns the path of a filename */
    OSPRAY_EMBREE_INTERFACE FileName path() const;

    /*! returns the file of a filename  */
    OSPRAY_EMBREE_INTERFACE std::string base() const;

    /*! returns the base of a filename without extension */
    OSPRAY_EMBREE_INTERFACE std::string name() const;

    /*! returns the file extension */
    OSPRAY_EMBREE_INTERFACE std::string ext() const;

    /*! drops the file extension */
    OSPRAY_EMBREE_INTERFACE FileName dropExt() const;

    /*! replaces the file extension */
    OSPRAY_EMBREE_INTERFACE FileName setExt(const std::string& ext = "") const;

    /*! adds file extension */
    OSPRAY_EMBREE_INTERFACE FileName addExt(const std::string& ext = "") const;

    /*! concatenates two filenames to this/other */
    OSPRAY_EMBREE_INTERFACE FileName operator +(const FileName& other) const;

    /*! concatenates two filenames to this/other */
    OSPRAY_EMBREE_INTERFACE FileName operator +(const std::string& other) const;

    /*! removes the base from a filename (if possible) */
    OSPRAY_EMBREE_INTERFACE FileName operator -(const FileName& base) const;

    /*! output operator */
    OSPRAY_EMBREE_INTERFACE friend std::ostream& operator<<(std::ostream& cout, const FileName& filename);

  private:
    std::string filename;
  };

}
