// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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
#include "ospcommon/common.h"
#include "ospcommon/vec.h"
#include "ospcommon/FileName.h"
// stl
#include <stack>
#include <vector>
#include <memory>
#include <map>

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

    /*! a XML node, consisting of a name, a list of properties, and a
      set of child nodes */
    struct OSPRAY_XML_INTERFACE Node {
      //! constructor
      Node(XMLDoc *doc) : name(""), content(""), doc(doc) {}

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
      std::vector<std::shared_ptr<Node>> child;

      //! pointer to parent doc
      /*! \detailed this points back to the parent xml doc that
          conatined this node. note this is intentionally NOT a ref to
          avoid cyclical dependencies. Ie, do NOT use this unless
          you're sure that the XMLDoc node that contained the given
          node is still around! */
      XMLDoc *doc;
    };

    /*! a entire xml document */
    struct OSPRAY_XML_INTERFACE XMLDoc : public Node {
      //! constructor
      XMLDoc() : Node(this) {}

      //! the name (and path etc) of the file that this doc was read from
      FileName fileName;
    };

    /*! iterator that iterates through all properties of a given node,
      calling the given functor (usually a lambda) with name and
      value.

      use via
      xml::for_each_prop(node,[&](const std::string &name,
                                  const std::string &value){
         doSomthingWith(name,value);
      });
    */
    template<typename Lambda>
    inline void for_each_prop(const Node &node, const Lambda &functor)
    {
      for (auto it = node.properties.begin(); it != node.properties.end(); it++)
        functor(it->first,it->second);
    }

    /*! iterator that iterates through all properties of a given node,
      calling the given functor (usually a lambda) with name and
      value.

      use via
      xml::for_each_child_of(node,[&](const xml::Node &child){
         doSomthingWith(name,value);
      });
    */
    template<typename Lambda>
    inline void for_each_child_of(const Node &node, const Lambda &functor)
    {
      for (auto it = node.child.begin(); it != node.child.end(); it++)
        functor(**it);
    }

    /*! parse an XML file with given file name, and return a pointer
      to it.  In case of any error, this function will free all
      already-allocated data, and throw a std::runtime_error
      exception */
    OSPRAY_XML_INTERFACE std::shared_ptr<XMLDoc> readXML(const std::string &fn);

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
