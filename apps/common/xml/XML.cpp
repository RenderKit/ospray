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

#include "XML.h"
#include <sstream>

namespace ospray {
  namespace xml {

    std::string toString(const float f)
    { std::stringstream ss; ss << f; return ss.str(); }

    std::string toString(const ospcommon::vec3f &v)
    { std::stringstream ss; ss << v.x << " " << v.y << " " << v.z; return ss.str(); }

    /*! checks if given node has given property */
    bool Node::hasProp(const std::string &propName) const
    {
      return (properties.find(propName) != properties.end());
    }

    /*! return value of property with given name if present, else return 'fallbackValue' */
    std::string Node::getProp(const std::string &propName,
                              const std::string &fallbackValue) const
    {
      if (!hasProp(propName)) return fallbackValue;
      return properties.find(propName)->second;
    }

    /*! return value of property with given name if present; and throw an exception if not */
    std::string Node::getProp(const std::string &propName) const
    {
      if (!hasProp(propName))
        return "";
      // throw std::runtime_error("given xml::Node does have the queried property '"+name+"'");
      return properties.find(propName)->second;
    }

    inline bool isWhite(char s) {
      return
        s == ' ' ||
        s == '\t' ||
        s == '\n' ||
        s == '\r';
    }

    inline void expect(char *&s, const char w)
    {
      if (*s != w) {
        std::stringstream err;
        err << "error reading XML file: expecting '" << w << "', but found '"
            << *s << "'";
        throw std::runtime_error(err.str());
      }
    }

    inline void expect(char *&s, const char w0, const char w1)
    {
      if (*s != w0 && *s != w1) {
        std::stringstream err;
        err << "error reading XML file: expecting '" << w0 << "' or '" << w1
            << "', but found '" << *s << "'";
        throw std::runtime_error(err.str());
      }
    }

    inline void consume(char *&s, const char w)
    {
      expect(s,w);
      ++s;
    }

    inline void consumeComment(char *&s)
    {
      consume(s,'<');
      consume(s,'!');
      while (!((s[0] == 0) ||
               (s[0] == '-' && s[1] == '-' && s[2] == '>')))
        ++s;
      consume(s,'-');
      consume(s,'-');
      consume(s,'>');
    }

    inline void consume(char *&s, const char *word)
    {
      const char *in = word;
      while (*word) {
        try {
          consume(s,*word); ++word;
        } catch (...) {
          std::stringstream err;
          err << "error reading XML file: expecting '" << in
              << "', but could not find it";
          throw std::runtime_error(err.str());
        }
      }
    }

    inline std::string makeString(const char *begin, const char *end)
    {
      if (!begin || !end)
        throw std::runtime_error("invalid substring in osp::xml::makeString");
      if (begin == end) return "";

      char *mem = new char[end-begin+1];
      memcpy(mem,begin,end-begin);
      mem[end-begin] = 0;

      std::string s = mem;
      delete [] mem;
      return s;
    }

    void parseString(char *&s, std::string &value)
    {
      if (*s == '"') {
        consume(s,'"');
        char *begin = s;
        while (*s != '"') {
          if (*s == '\\') ++s;
          ++s;
        }
        char *end = s;
        value = makeString(begin,end);
        consume(s,'"');
      } else {
        consume(s,'\'');
        char *begin = s;
        while (*s != '\'') {
          if (*s == '\\') ++s;
          ++s;
        }
        char *end = s;
        value = makeString(begin,end);
        consume(s,'\'');
      }
    }

    bool parseIdentifier(char *&s, std::string &identifier)
    {
      if (isalpha(*s) || *s == '_') {
        char *begin = s;
        ++s;
        while (isalpha(*s) || isdigit(*s) || *s == '_' || *s == '.') {
          ++s;
        }
        char *end = s;
        identifier = makeString(begin,end);
        return true;
      }
      return false;
    }

    inline void skipWhites(char *&s)
    {
      while (isWhite(*s)) ++s;
    }

    bool parseProp(char *&s, std::string &name, std::string &value)
    {
      if (!parseIdentifier(s,name))
        return false;
      skipWhites(s);
      consume(s,'=');
      skipWhites(s);
      expect(s,'"','\'');
      parseString(s,value);
      return true;
    }

    bool skipComment(char *&s)
    {
      if (*s == '<' && s[1] == '!') {
        consumeComment(s);
        return true;
      }
      return false;
    }

    std::shared_ptr<Node> parseNode(char *&s, XMLDoc *doc)
    {
      consume(s,'<');
      std::shared_ptr<Node> node = std::make_shared<Node>(doc);

      if (!parseIdentifier(s,node->name))
        throw std::runtime_error("XML error: could not parse node name");

      skipWhites(s);

      std::string name, value;
      while (parseProp(s,name,value)) {
        node->properties[name] = value;
        skipWhites(s);
      }

      if (*s == '/') {
        consume(s,"/>");
        return node;
      }

      consume(s,">");

      while (1) {
        skipWhites(s);
        if (skipComment(s))
          continue;
        if (*s == '<' && s[1] == '/') {
          consume(s,"</");
          std::string nodeName;
          parseIdentifier(s, nodeName);
          if (nodeName != node->name) {
            throw std::runtime_error("invalid XML node - started with'<"
                                     + node->name +
                                     "...'>, but ended with '</"+
                                     nodeName + ">");
          }
          consume(s,">");
          break;
          // either end of current node
        } else if (*s == '<') {
          // child node
          std::shared_ptr<Node> child = parseNode(s, doc);
          node->child.push_back(child);
        } else if (*s == 0) {
          std::cout << "#osp:xml: warning: xml file ended with still-open"
            " nodes (this typically indicates a partial xml file)"
                    << std::endl;
          return node;
        } else {
          if (node->content != "") {
            throw std::runtime_error("invalid XML node - two different"
                                     " contents!?");
          }
          // content
          char *begin = s;
          while (*s != '<' && *s != 0) ++s;
          char *end = s;
          while (isspace(end[-1])) --end;
          node->content = makeString(begin,end);
        }
      }
      return node;
    }

    bool parseHeader(char *&s)
    {
      consume(s,"<?xml");
      if (*s == '?' && s[1] == '>') {
        consume(s,"?>");
        return true;
      }
      if (!isWhite(*s)) return false;
      ++s;

      skipWhites(s);

      std::string name, value;
      while (parseProp(s,name,value)) {
        // ignore header prop
        skipWhites(s);
      }

      consume(s,"?>");
      return true;
    }

    void parseXML(std::shared_ptr<XMLDoc> doc, char *s)
    {
      if (s[0] == '<' && s[1] == '?') {
        if (!parseHeader(s))
          throw std::runtime_error("could not parse XML header");
      }
      skipWhites(s);
      while (*s != 0) {
        if (skipComment(s)) {
          skipWhites(s);
          continue;
        }

        std::shared_ptr<Node> node = parseNode(s, doc.get());
        doc->child.push_back(node);
        skipWhites(s);
      }

      if (*s != 0)
        throw std::runtime_error("un-parsed junk at end of file");
    }

    void Writer::spaces()
    {
      for (size_t i = 0; i < state.size(); i++)
        fprintf(xml,"  ");
    }

    void Writer::writeProperty(const std::string &name,
                               const std::string &value)
    {
      assert(xml);
      assert(!state.empty());
      State *s = state.top();
      (void)s;
      assert(s);
      assert(!s->hasContent); // content may not be written before properties
      fprintf(xml," %s=\"%s\"",name.c_str(),value.c_str());
    }

    void Writer::openNode(const std::string &type)
    {
      assert(xml);
      spaces(); fprintf(xml,"<%s",type.c_str());
      State *s = new State;
      s->type = type;
      state.push(s);
    }

    void Writer::closeNode()
    {
      assert(xml);
      assert(!state.empty());
      State *s = state.top();
      assert(s);
      if (s->hasContent)
        fprintf(xml,"</%s>",s->type.c_str());
      else
        fprintf(xml,"/>\n");
      delete s;
      state.pop();
    }

    std::shared_ptr<XMLDoc> readXML(const std::string &fn)
    {
      FILE *file = fopen(fn.c_str(),"r");
      if (!file) {
        throw std::runtime_error("ospray::XML error: could not open file '"
                                 + fn +"'");
      }

      fseek(file,0,SEEK_END);
      ssize_t numBytes =
#ifdef _WIN32
        _ftelli64(file);
#else
        ftell(file);
#endif
      fseek(file,0,SEEK_SET);
      char *mem = new char[numBytes+1];
      try {
        mem[numBytes] = 0;
        auto rc = fread(mem,1,numBytes,file);
        (void)rc;
        std::shared_ptr<XMLDoc> doc = std::make_shared<XMLDoc>();
        doc->fileName = fn;
        parseXML(doc,mem);
        delete[] mem;
        fclose(file);
        return doc;
      } catch (std::runtime_error e) {
        delete[] mem;
        fclose(file);
        throw e;
      }
    }

    Writer::Writer(FILE *xml, FILE *bin)
      : xml(xml), bin(bin)
    {}

    /*! write document header, may only be called once */
    void Writer::writeHeader(const std::string &version)
    {
      assert(xml);
      fprintf(xml,"<?xml version=\"%s\"?>\n",version.c_str());
    }

    /*! write document footer. may only be called once, at end of write */
    void Writer::writeFooter()
    {
      assert(xml);
    }

  } // ::ospray::xml
} // ::ospray
