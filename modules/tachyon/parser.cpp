/********************************************************************* *\
 * INTEL CORPORATION PROPRIETARY INFORMATION                            
 * This software is supplied under the terms of a license agreement or  
 * nondisclosure agreement with Intel Corporation and may not be copied 
 * or disclosed except in accordance with the terms of that agreement.  
 * Copyright (C) 2014 Intel Corporation. All Rights Reserved.           
 ********************************************************************* */

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
      std::cout << "#osp:tachyon: --- parsing " << fileName << " ---" << std::endl;
      yyin=fopen(fileName.c_str(),"r"); //popen(cmd,"r");
      if (!yyin)
        Error(Loc::current,"#osp:tachyon: can't open file...");

      yyparse();
      fclose(yyin);
    }
  }
}


