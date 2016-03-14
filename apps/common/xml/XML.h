// ======================================================================== //
// Copyright 2009-2016 Intel Corporation                                    //
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

// // ospray
// #include "ospray/common/OSPCommon.h"
// // embree
// #include "common/sys/filename.h"

// ospcomon
#include "common/common.h"
#include "common/vec.h"
#include "common/FileName.h"
// stl
#include <stack>
#include <vector>

#ifdef _WIN32
#  ifdef ospray_xml_EXPORTS
#    define OSPRAY_XML_INTERFACE __declspec(dllexport)
#  else
#    define OSPRAY_XML_INTERFACE __declspec(dllimport)
#  endif
#else
#  define OSPRAY_XML_INTERFACE
#endif

namespace ospray {
  namespace xml {

    struct Node;
    using ospcommon::FileName;
    struct XMLDoc;

    /*! 'prop'erties in xml nodes are the 'name="value"' inside the
      <node name1="value1" name2="value2"> ... </node> description */
    struct Prop {
      std::string name;
      std::string value;
    };

    /*! a XML node, consisting of a name, a list of properties, and a
      set of child nodes */
    struct Node {
      //! constructor
      Node(XMLDoc *doc) : name(""), content(""), doc(doc) {}
      
      //! destructor
      virtual ~Node();

      inline bool hasProp(const std::string &name) const {
        for (int i=0;i<prop.size();i++) 
          if (prop[i]->name == name) return true;
        return false;
      }
      inline std::string getProp(const std::string &name) const {
        for (int i=0;i<prop.size();i++) 
          if (prop[i]->name == name) return prop[i]->value; 
        return "";
      }

      /*! find properly with given name, and return as long ('l')
        int. return undefined if prop does not exist */
      inline size_t getPropl(const std::string &name) const
      { return atol(getProp(name).c_str()); }
      
      /*! name of the xml node (i.e., the thing that's in
          "<name>....</name>") */
      std::string name;

      /*! the content string, i.e., the thing that's between
          "<name>..." and "...</name>" */
      std::string content;

      /*! list of xml node properties properties */
      std::vector<Prop *> prop;

      /*! list of child nodes */
      std::vector<Node *> child;

      //! pointer to parent doc 
      /*! \detailed this points back to the parent xml doc that
          conatined this node. note this is intentionally NOT a ref to
          avoid cyclical dependencies */
      XMLDoc *doc;
    };

    /*! a entire xml document */
    struct XMLDoc : public Node {
      //! constructor
      XMLDoc() : Node(this) {}

      //! the name (and path etc) of the file that this doc was read from
      FileName fileName;
    };
    
    /*! parse an XML file with given file name, and return a pointer
      to it.  In case of any error, this function will free all
      already-allocated data, and throw a std::runtime_error
      exception */
    OSPRAY_XML_INTERFACE XMLDoc *readXML(const std::string &fn);

      
    /*! @{ */
    //! \brief helper function(s) to convert data tyeps into strings 
    std::string toString(const int64_t value);
    std::string toString(const float value);
    std::string toString(const ospcommon::vec3f &value);
    /*! @} */
    
    /*! helper class for writing sg nodes in XML format */
    struct Writer {
      Writer(FILE *xml, FILE *bin);

      /*! write document header, may only be called once */
      void writeHeader(const std::string &version);
      /*! write document footer. may only be called once, at end of write */
      void writeFooter();

      //! open a new xml node with given node type */
      void openNode(const std::string &type);
      void writeProperty(const std::string &name, const std::string &value);
      void writeContent(const std::string &name,  const std::string &value);
      //! close last open node type */
      void closeNode();
      /*! align output pos on binary file to given alignment */
      void alignData(size_t alignment);
      /*! write given data into data file, and return offset value at
          which it was written */
      size_t writeData(const void *ptr, size_t size);
      FILE *xml, *bin;
    private:
      struct State {
        bool hasContent;
        std::string type;
        State() : hasContent(false), type("") {};
      };
      void spaces();
      std::stack<State*> state;
    };

  } // ::ospray::xml
} // ::ospray
