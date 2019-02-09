// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

// ospcomon
#include "../common.h"
#include "../vec.h"
#include "../FileName.h"

// stl
#include <stack>
#include <memory>
#include <map>
#include <vector>

namespace ospcommon {
  namespace xml {

    struct Node;
    struct XMLDoc;

    /*! a XML node, consisting of a name, a list of properties, and a
      set of child nodes */
    struct OSPCOMMON_INTERFACE Node
    {
      Node(XMLDoc *doc) : doc(doc) {}

      /*! checks if given node has given property */
      bool hasProp(const std::string &name) const;

      /*! return value of property with given name if present; and throw an exception if not */
      std::string getProp(const std::string &name) const;

      /*! return value of property with given name if present, else return 'fallbackValue' */
      std::string getProp(const std::string &name, const std::string &fallbackValue) const;

      /*! name of the xml node (i.e., the thing that's in
          "<name>....</name>") */
      std::string name;

      /*! the content string, i.e., the thing that's between
          "<name>..." and "...</name>" */
      std::string content;

      /*! \brief list of xml node properties properties.  '

        \detailed prop'erties in xml nodes are the 'name="value"'
        inside the <node name1="value1" name2="value2"> ... </node>
        description */
      std::map<std::string,std::string> properties;

      /*! list of child nodes */
      std::vector<Node> child;

      //! pointer to parent doc
      /*! \detailed this points back to the parent xml doc that
          contained this node. note this is intentionally NOT a ref to
          avoid cyclical dependencies. Ie, do NOT use this unless
          you're sure that the XMLDoc node that contained the given
          node is still around! */
      XMLDoc *doc {nullptr};
    };

    /*! a entire xml document */
    struct OSPCOMMON_INTERFACE XMLDoc : public Node
    {
      //! constructor
      XMLDoc() : Node(this) {}

      //! the name (and path etc) of the file that this doc was read from
      FileName fileName;
    };

    /*! parse an XML file with given file name, and return a pointer
      to it.  In case of any error, this function will free all
      already-allocated data, and throw a std::runtime_error
      exception */
    OSPCOMMON_INTERFACE std::shared_ptr<XMLDoc> readXML(const std::string &fn);

    /*! helper class for writing sg nodes in XML format */
    struct Writer
    {
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
      struct State
      {
        bool hasContent {false};
        std::string type;
      };

      void spaces();
      std::stack<State*> state;
    };

  } // ::ospray::xml
} // ::ospray
