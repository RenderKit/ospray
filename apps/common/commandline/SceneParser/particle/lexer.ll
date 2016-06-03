%{

  /*
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
*/ 

#define YY_MAIN 1
#define YY_NEVER_INTERACTIVE 0

#include "Model.h"
#include "loc.h"
#include "parser_y.hpp" // (iw) use auto-generated one, not checked-in one
#include <string>

static void lCComment();
static void lCppComment();
static void lHandleCppHash();

 using namespace ospray;
 using namespace ospray::particle;

#define register /**/
%}

%option nounput
%option noyywrap

WHITESPACE [ \t\r]+
INT_NUMBER (([0-9]+)|(0x[0-9a-fA-F]+))
FLOAT_NUMBER (([0-9]+|(([0-9]+\.[0-9]*f?)|(\.[0-9]+)))([eE][-+]?[0-9]+)?f?)|([-]?0x[01]\.?[0-9a-fA-F]+p[-+]?[0-9]+f?)

IDENT [a-zA-Z_][a-zA-Z_0-9\-]*

%%
ITEM            { return TOKEN_ITEM; }
NUMBER            { return TOKEN_NUMBER; }
OF            { return TOKEN_OF; }
ITEMS            { return TOKEN_ITEMS; }
TIMESTEP            { return TOKEN_TIMESTEP; }
ATOMS            { return TOKEN_ATOMS; }
BOX            { return TOKEN_BOX; }
BOUNDS            { return TOKEN_BOUNDS; }
type            { return TOKEN_type; }
x            { return TOKEN_x; }
y            { return TOKEN_y; }
z            { return TOKEN_z; }

{IDENT} { 
  yylval.identifierVal = strdup(yytext);
  return TOKEN_IDENTIFIER; 
}

{INT_NUMBER} { 
    char *endPtr = NULL;
    unsigned long long val = strtoull(yytext, &endPtr, 0);
    yylval.intVal = (int32_t)val;
    if (val != (unsigned int)yylval.intVal)
        Warning(Loc::current, "32-bit integer has insufficient bits to represent value %s (%llx %llx)",
                yytext, yylval.intVal, (unsigned long long)val);
    return TOKEN_INT_CONSTANT; 
}
-{INT_NUMBER} { 
    char *endPtr = NULL;
    unsigned long long val = strtoull(yytext, &endPtr, 0);
    yylval.intVal = (int32_t)val;
    return TOKEN_INT_CONSTANT; 
}



{FLOAT_NUMBER} { yylval.floatVal = atof(yytext); return TOKEN_FLOAT_CONSTANT; }
-{FLOAT_NUMBER} { yylval.floatVal = atof(yytext); return TOKEN_FLOAT_CONSTANT; }

":" { return ':'; }
"," { return ','; }
"{" { return '{'; }
"}" { return '}'; }
"[" { return '['; }
"]" { return ']'; }

{WHITESPACE} { /* nothing */ }
\n { Loc::current.line++; }

^#.*\n { 
  /*lHandleCppHash(); */ 
  Loc::current.line++;
   }

. { Error(Loc::current, "Illegal character: %c (0x%x)", yytext[0], int(yytext[0])); }

%%


// static void lHandleCppHash() {
//     char *src, prev = 0;

//     if (yytext[0] != '#' || yytext[1] != ' ') {
//         Error(Loc::current, "Malformed cpp file/line directive: %s\n", yytext);
//         exit(1);
//     }
//     Loc::current.line = strtol(&yytext[2], &src, 10) - 1;
//     if (src == &yytext[2] || src[0] != ' ' || src[1] != '"') {
//         Error(Loc::current, "Malformed cpp file/line directive: %s\n", yytext);
//         exit(1);
//     }
//     std::string filename;
//     for (src += 2; *src != '"' || prev == '\\'; ++src) {
//         filename.push_back(*src);
//     }
//     Loc::current.name = strdup(filename.c_str());
// }


