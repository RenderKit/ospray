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

#include "XML.h"

namespace ospray {
  namespace xml {

    std::string toString(const float f) 
    { std::stringstream ss; ss << f; return ss.str(); }

    std::string toString(const vec3f &v) 
    { std::stringstream ss; ss << v.x << " " << v.y << " " << v.z; return ss.str(); }

    Node::~Node()
    {
      for (int i=0;i<prop.size();i++) delete prop[i];
      for (int i=0;i<child.size();i++) delete child[i];
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
        err << "error reading XML file: expecting '" << w << "', but found '" << *s << "'";
        throw std::runtime_error(err.str());
      }
    }
    inline void expect(char *&s, const char w0, const char w1)
    {
      if (*s != w0 && *s != w1) {
        std::stringstream err;
        err << "error reading XML file: expecting '" << w0 << "' or '" << w1 << "', but found '" << *s << "'";
        throw std::runtime_error(err.str());
      }
    }
    inline void consume(char *&s, const char w)
    {
      expect(s,w);
      ++s;
    }
    inline void consume(char *&s, const char *word)
    {
      const char *in = word;
      while (*word) { 
        try {
          consume(s,*word); ++word;
        } catch (...) {
          std::stringstream err;
          err << "error reading XML file: expecting '" << in << "', but could not find it";
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
      mem[end-begin] = 0;

      memcpy(mem,begin,end-begin);
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
        while (isalpha(*s) || isdigit(*s) || *s == '_') {
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

    bool parseProp(char *&s, Prop &prop)
    {
      if (!parseIdentifier(s,prop.name))
        return false;
      skipWhites(s);
      consume(s,'=');
      skipWhites(s);
      expect(s,'"','\'');
      parseString(s,prop.value);
      return true;
    }

    Node *parseNode(char *&s, XMLDoc *doc)
    {
      consume(s,'<');
      Node *node = new Node(doc);
      try {
        if (!parseIdentifier(s,node->name))
          throw std::runtime_error("XML error: could not parse node name");

        skipWhites(s);
      
        Prop prop;
        while (parseProp(s,prop)) {
          node->prop.push_back(new Prop(prop));
          skipWhites(s);
        }

        if (*s == '/') {
          consume(s,"/>");
          return node;
        }

        consume(s,">");

        while (1) {
          skipWhites(s);
          if (*s == '<' && s[1] == '/') {
            consume(s,"</");
            std::string name = "";
            parseIdentifier(s,name);
            if (name != node->name)
              throw std::runtime_error("invalid XML node - started with '<"+node->name+"...'>, but ended with '</"+name+">");
            consume(s,">");
            break;
            // either end of current node
          } else if (*s == '<') {
            // child node
            Node *child = parseNode(s, doc);
            node->child.push_back(child);
          } else if (*s == 0) {
            std::cout << "#osp:xml: warning: xml file ended with still-open nodes (this typically indicates a partial xml file)" << std::endl;
            return node;
          } else {
            if (node->content != "")
              throw std::runtime_error("invalid XML node - two different contents!?");
            // content
            char *begin = s;
            while (*s != '<' && *s != 0) ++s;
            char *end = s;
            while (isspace(end[-1])) --end;
            node->content = makeString(begin,end);
          }
        }
        return node;
      } catch (std::runtime_error e) {
        delete node;
        throw e;
      }
    }

    bool parseHeader(char *&s)
    {
      consume(s,"<?xml");
      if (*s == '?' && s[1] == '>') {
        consume(s,"?>");
        return true;
      }
      if (!isWhite(*s)) return false; ++s;

      skipWhites(s);
    
      Prop headerProp;
      while (parseProp(s,headerProp)) {
        // ignore header prop
        skipWhites(s);
      }
    
      consume(s,"?>");
      return true;
    }

    bool parseXML(XMLDoc *xml, char *s)
    {
      if (s[0] == '<' && s[1] == '?') {
        if (!parseHeader(s))
          throw std::runtime_error("could not parse XML header");
      }
      skipWhites(s);
      while (*s != 0) {
        Node *node = parseNode(s, xml);
        if (node)xml->child.push_back(node);
        skipWhites(s);
      }

      if (*s != 0) throw std::runtime_error("un-parsed junk at end of file"); ++s;
      return xml;
    }

    void Writer::spaces()
    {
      for (int i=0;i<state.size();i++)
        fprintf(xml,"  ");
    }
    
    void Writer::writeProperty(const std::string &name, const std::string &value)
    {
      assert(xml);
      assert(!state.empty());
      State *s = state.top();
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

    XMLDoc *readXML(const std::string &fn)
    {
      FILE *file = fopen(fn.c_str(),"r");
      if (!file) {
        throw std::runtime_error("ospray::XML error: could not open file '"+fn+"'");
      }

      fseek(file,0,SEEK_END);
      long numBytes = ftell(file);
      fseek(file,0,SEEK_SET);
      char *mem = new char[numBytes+1];
      mem[numBytes] = 0;
      fread(mem,1,numBytes,file);
      XMLDoc *xml = new XMLDoc;
      xml->fileName = fn;
      bool valid = false;
      try {
        valid = parseXML(xml,mem);
      } catch (std::runtime_error e) {
        delete[] mem;
        fclose(file);
        throw e;
      }
      delete[] mem;
      fclose(file);

      if (!valid) {
        delete xml;
        return NULL;
      }
      return xml;
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
