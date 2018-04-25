// ======================================================================== //
// Copyright 2009-2018 Intel Corporation                                    //
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

#include "FileName.h"
#include "sysinfo.h"

namespace ospcommon
{
#ifdef _WIN32
  const char path_sep = '\\';
#else
  const char path_sep = '/';
#endif

  /*! create an empty filename */
  FileName::FileName () {}

  /*! create a valid filename from a string */
  FileName::FileName (const char* in) {
    filename = in;
    for (size_t i=0; i<filename.size(); i++)
      if (filename[i] == '\\' || filename[i] == '/')
        filename[i] = path_sep;
    while (!filename.empty() && filename[filename.size()-1] == path_sep)
      filename.resize(filename.size()-1);
  }

  /*! create a valid filename from a string */
  FileName::FileName (const std::string& in) {
    filename = in;
    for (size_t i=0; i<filename.size(); i++)
      if (filename[i] == '\\' || filename[i] == '/')
        filename[i] = path_sep;
    while (!filename.empty() && filename[filename.size()-1] == path_sep)
      filename.resize(filename.size()-1);
  }
  
  /*! returns path to home folder */
  FileName FileName::homeFolder() 
  {
#ifdef _WIN32
    const char* home = getenv("UserProfile");
#else
    const char* home = getenv("HOME");
#endif
    if (home) return home;
    return "";
  }

  /*! returns path to executable */
  FileName FileName::executableFolder() {
    return FileName(getExecutableFileName()).path();
  }

  /*! returns the path */
  std::string FileName::path() const {
    size_t pos = filename.find_last_of(path_sep);
    if (pos == std::string::npos) return "";	
    return filename.substr(0,pos+1);
  }

  /*! returns the basename */
  std::string FileName::base() const {
    size_t pos = filename.find_last_of(path_sep);
    if (pos == std::string::npos) return filename;
    return filename.substr(pos+1);
  }

  /*! returns the extension */
  std::string FileName::ext() const {
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) return "";
    return filename.substr(pos+1);
  }

  /*! returns the extension */
  FileName FileName::dropExt() const {
    size_t pos = filename.find_last_of('.');
    if (pos == std::string::npos) return filename;
    return filename.substr(0,pos);
  }

  /*! returns the basename without extension */
  std::string FileName::name() const {
    size_t start = filename.find_last_of(path_sep);
    if (start == std::string::npos) start = 0; else start++;
    size_t end = filename.find_last_of('.');
    if (end == std::string::npos || end < start) end = filename.size();
    return filename.substr(start, end - start);
  }

  /*! replaces the extension */
  FileName FileName::setExt(const std::string& ext) const {
    size_t start = filename.find_last_of(path_sep);
    if (start == std::string::npos) start = 0; else start++;
    size_t end = filename.find_last_of('.');
    if (end == std::string::npos || end < start) return FileName(filename+ext);
    return FileName(filename.substr(0,end)+ext);
  }

  /*! adds the extension */
  FileName FileName::addExt(const std::string& ext) const {
    return FileName(filename+ext);
  }

  /*! concatenates two filenames to this/other */
  FileName FileName::operator +( const FileName& other ) const {
    if (filename == "") return FileName(other);
    else return FileName(filename + path_sep + other.filename);
  }

  /*! concatenates two filenames to this/other */
  FileName FileName::operator +( const std::string& other ) const {
    return operator+(FileName(other));
  }

  /*! removes the base from a filename (if possible) */
  FileName FileName::operator -( const FileName& base ) const {
    size_t pos = filename.find_first_of(base);
    if (pos == std::string::npos) return *this;
    return FileName(filename.substr(pos+1));
  }

  /*! == operator */
  bool operator== (const FileName& a, const FileName& b) {
    return a.filename == b.filename;
  }
  
  /*! != operator */
  bool operator!= (const FileName& a, const FileName& b) {
    return a.filename != b.filename;
  }

  /*! output operator */
  std::ostream& operator<<(std::ostream& cout, const FileName& filename) {
    return cout << filename.filename;
  }
}
