%error-verbose
/* supress shift-reduces conflict message for dangling else */
/* one for 'if', one for 'uif' */
 //%expect 2

%{

#include "model.h"
#include "loc.h"

  using namespace ospray;
  using namespace ospray::particle;

// #define NOTIMPLEMENTED \
//         Error(Loc::current, "Unimplemented parser functionality %s:%d", \
//         __FILE__, __LINE__);

extern int yylex();
void yyerror(const char *s);

extern char *yytext;
 namespace ospray {
   namespace particle {
     extern Model *parserModel;
   }
 }
%}

%union {
  int                intVal;
  float              floatVal;
  char              *identifierVal;
  std::vector<ospray::particle::Model::Atom> *atomVector;
  ospray::vec3f                     *Vec3f;
}

// DUMMY
%token TOKEN_INT_CONSTANT TOKEN_IDENTIFIER TOKEN_FLOAT_CONSTANT TOKEN_TIMESTEP
%token TOKEN_ITEM TOKEN_NUMBER TOKEN_OF TOKEN_ATOMS TOKEN_BOX TOKEN_BOUNDS TOKEN_ITEMS
%token TOKEN_type TOKEN_x TOKEN_y TOKEN_z
;

%type <intVal>    Int
%type <floatVal>  Float
%type <atomVector> atom_list vec3_list


%start world
%%

world
: item_timestep item_num_atoms item_box_bounds item_atoms
| vec3_list
{
  parserModel->atom = *$1;
  delete $1;
}
;

item_timestep: TOKEN_ITEM ':' TOKEN_TIMESTEP Int 

item_num_atoms: TOKEN_ITEM ':' TOKEN_NUMBER TOKEN_OF TOKEN_ATOMS Int ;

item_box_bounds: TOKEN_ITEM ':' TOKEN_BOX TOKEN_BOUNDS Float Float Float Float Float Float ;

item_atoms: TOKEN_ITEM ':' TOKEN_ATOMS TOKEN_type TOKEN_x TOKEN_y TOKEN_z atom_list 
{
  parserModel->atom = *$8;
  delete $8;
}

vec3_list
: vec3_list Float Float Float
{ 
  $$ = $1; 
  ospray::particle::Model::Atom a; 
  a.type = 0;
  a.position.x = $2;
  a.position.y = $3;
  a.position.z = $4;
  $$->push_back(a);
}
| 
{ $$ = new std::vector<ospray::particle::Model::Atom> ; }
;

atom_list
: atom_list Int Float Float Float
{ 
  $$ = $1; 
  ospray::particle::Model::Atom a; 
  a.type = $2;
  a.position.x = $3;
  a.position.y = $4;
  a.position.z = $5;
  $$->push_back(a);
}
| 
{ $$ = new std::vector<ospray::particle::Model::Atom> ; }
;

Float
: TOKEN_FLOAT_CONSTANT { $$ = yylval.floatVal; }
| Int                  { $$ = $1; }
;

Int: TOKEN_INT_CONSTANT { $$ = yylval.intVal; }
;

%%
#include <stdio.h>

void yyerror(const char *s)
{
  std::cout << "=======================================================" << std::endl;
  PRINT(yytext);
  Error(Loc::current, s);
}

