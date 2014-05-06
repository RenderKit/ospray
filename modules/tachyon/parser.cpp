#include "parser.h"

extern int  yyparse();
extern void yyerror(const char* msg);
extern FILE *yyin;
extern int yydebug;

namespace ospray {
  namespace tachyon {
    
    Model *parserModel = NULL;

    void importFile(tachyon::Model &model, const std::string &fileName)
    {
      yydebug = 0;
      parserModel = &model;
      Loc::current.name = fileName.c_str();
      Loc::current.line = 1;
      std::cout << " --- parsing " << fileName << " ---" << std::endl;
      // char cmd[1000];
      // sprintf(cmd,"/usr/bin/cpp %s",fileName.c_str());
      yyin=fopen(fileName.c_str(),"r"); //popen(cmd,"r");
      if (!yyin)
        Error(Loc::current,"RayShade importer: could not open input pipe (/usr/bin/cpp)");

      // parser = new Parser;
      yyparse();
      fclose(yyin);
      // Group *world = parser->world;
      // PING;
      // delete parser;
      // parser = NULL;

      // return world;
    }
  }
}


