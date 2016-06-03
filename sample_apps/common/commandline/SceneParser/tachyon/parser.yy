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

%error-verbose

%{

#include "SceneParser/tachyon/Model.h"
#include "SceneParser/tachyon/Loc.h"

#include "common/vec.h"

  extern int yydebug;

  using namespace ospray;
  using namespace ospcommon;

extern int yylex();
void yyerror(const char *s);

extern char *yytext;
 namespace ospray {
   namespace tachyon {
     extern Model *parserModel;
   }
 }
%}

%union {
  int                intVal;
  float              floatVal;
  char              *identifierVal;
  std::vector<ospcommon::vec3fa> *vec3fVector;
  std::vector<int>               *intVector;
  ospray::tachyon::VertexArray   *vertexArray;
  ospray::tachyon::Texture       *texture;
  ospray::tachyon::DirLight      *dirLight;
  ospray::tachyon::PointLight    *pointLight;
  ospcommon::vec3f               *Vec3f;
}

// DUMMY
%token TOKEN_INT_CONSTANT TOKEN_IDENTIFIER TOKEN_FLOAT_CONSTANT TOKEN_OTHER_STRING
%token TOKEN_BEGIN_SCENE TOKEN_END_SCENE TOKEN_RESOLUTION TOKEN_SHADER_MODE TOKEN_END_SHADER_MODE TOKEN_TRANS_VMD TOKEN_FOG_VMD
%token TOKEN_Camera TOKEN_End_Camera TOKEN_Projection TOKEN_Orthographic TOKEN_Zoom TOKEN_Aspectratio TOKEN_Antialiasing TOKEN_Raydepth TOKEN_Center TOKEN_Viewdir TOKEN_Updir
%token TOKEN_Directional_Light TOKEN_Direction TOKEN_Color TOKEN_Background TOKEN_Density TOKEN_End TOKEN_Start TOKEN_Fog TOKEN_Exp2
%token TOKEN_FCylinder TOKEN_Base TOKEN_Apex TOKEN_Rad
%token TOKEN_Texture TOKEN_Ambient TOKEN_Diffuse TOKEN_Specular TOKEN_Opacity TOKEN_Phong TOKEN_Plastic TOKEN_Phong_size TOKEN_TexFunc TOKEN_TransMode TOKEN_R3D
%token TOKEN_STri TOKEN_V0 TOKEN_V1 TOKEN_V2 TOKEN_N0 TOKEN_N1 TOKEN_N2
%token TOKEN_Sphere
%token TOKEN_VertexArray TOKEN_End_VertexArray TOKEN_Numverts
%token TOKEN_Colors TOKEN_Coords TOKEN_Normals TOKEN_TriStrip TOKEN_TriMesh
%token TOKEN_Ambient_Color TOKEN_Rescale_Direct TOKEN_Samples TOKEN_Ambient_Occlusion
%token TOKEN_Background_Gradient TOKEN_UpDir TOKEN_TopVal TOKEN_BottomVal TOKEN_TopColor TOKEN_BottomColor
%token TOKEN_Light TOKEN_Attenuation TOKEN_linear TOKEN_quadratic TOKEN_constant
%token TOKEN_Start_ClipGroup TOKEN_End_ClipGroup TOKEN_NumPlanes
;


%type <intVal>    Int
%type <floatVal>  Float
%type <identifierVal> Identifier
%type <vec3fVector> vec3f_vector
%type <intVector> int_vector
%type <vertexArray> vertexarray_body
%type <texture> texture texture_body
%type <dirLight> directional_light_body
%type <pointLight> light_body

%type <Vec3f> vec3f

%start world
%%

world: TOKEN_BEGIN_SCENE scene TOKEN_END_SCENE {
 }
;

scene
: /* eol */
| scene resolution
| scene shader_mode
| scene camera
| scene directional_light
| scene fog
| scene background
| scene fcylinder
| scene stri
| scene sphere
| scene vertexarray
| scene background_gradient
| scene light
| scene start_clipgroup
| scene end_clipgroup
;


start_clipgroup
: TOKEN_Start_ClipGroup TOKEN_NumPlanes Int clipGroup_planes
;

clipGroup_planes
: /* eol */
| clipGroup_planes Float Float Float Float
;

end_clipgroup
: TOKEN_End_ClipGroup
;

vertexarray: TOKEN_VertexArray TOKEN_Numverts Int vertexarray_body TOKEN_End_VertexArray
{
  assert($4->normal.size() == 0 || $4->normal.size() == $4->coord.size());
  assert($4->color.size() == 0 || $4->color.size() == $4->coord.size());
  ospray::tachyon::parserModel->addVertexArray($4);
}
;


light
: TOKEN_Light light_body
{ ospray::tachyon::parserModel->addPointLight(*$2); delete $2; }
;

light_body
: /* eol */ { $$ = new ospray::tachyon::PointLight; }
| light_body TOKEN_Center vec3f
{ $$ = $1; $1->center = *$3; delete $3; }
| light_body TOKEN_Color vec3f
{ $$ = $1; $1->color = *$3; delete $3; }
| light_body TOKEN_Rad Float
{ $$ = $1; $1->color = vec3f($3); }
| light_body TOKEN_Attenuation TOKEN_constant Float TOKEN_linear Float TOKEN_quadratic Float
{
  $$ = $1;
  $$->atten.constant  = $4;
  $$->atten.linear    = $6;
  $$->atten.quadratic = $8;
}
;


vertexarray_body
: /* eol */ { $$ = new ospray::tachyon::VertexArray; }
| vertexarray_body TOKEN_Coords vec3f_vector
{ $$ = $1; $1->coord = *$3; delete $3; }
| vertexarray_body TOKEN_Normals vec3f_vector
{ $$ = $1; $1->normal = *$3; delete $3; }
| vertexarray_body TOKEN_Colors vec3f_vector
{ $$ = $1; $1->color = *$3; delete $3; }
| vertexarray_body texture
{ $$ = $1; $1->textureID = ospray::tachyon::parserModel->addTexture($2); }
| vertexarray_body TOKEN_TriStrip Int int_vector
{
  $$ = $1;
  for (int i=2;i<$4->size();i++) {
    vec3i tri((*$4)[i-2],(*$4)[i-1],(*$4)[i]);
    $1->triangle.push_back(tri);
  }
  delete $4;
}
| vertexarray_body TOKEN_TriMesh Int int_vector
{
  $$ = $1;
  for (int i=0;i<$4->size();i+=3) {
    vec3i tri((*$4)[i],(*$4)[i+1],(*$4)[i+2]);
    $1->triangle.push_back(tri);
  }
  delete $4;
}
;


vec3f_vector
: /* eol */ { $$ = new std::vector<vec3fa>; }
| vec3f_vector Float Float Float { $$ = $1; $$->push_back(vec3f($2,$3,$4)); }
;


int_vector
: /* eol */ { $$ = new std::vector<int>; }
| int_vector Int { $$ = $1; $$->push_back($2); }
;


stri:
TOKEN_STri
TOKEN_V0 Float Float Float
TOKEN_V1 Float Float Float
TOKEN_V2 Float Float Float
TOKEN_N0 Float Float Float
TOKEN_N1 Float Float Float
TOKEN_N2 Float Float Float
texture
{
  vec3f v0($3,$4,$5);
  vec3f v1($7,$8,$9);
  vec3f v2($11,$12,$13);
  vec3f n0($15,$16,$17);
  vec3f n1($19,$20,$21);
  vec3f n2($23,$24,$25);
  tachyon::VertexArray *sva = ospray::tachyon::parserModel->getSTriVA(1);
  int textureID = ospray::tachyon::parserModel->addTexture($26);
  delete $26;

  sva->triangle.push_back(vec3i(sva->coord.size())+vec3i(0,1,2));
  sva->coord.push_back(v0);
  sva->coord.push_back(v1);
  sva->coord.push_back(v2);
  sva->normal.push_back(n0);
  sva->normal.push_back(n1);
  sva->normal.push_back(n2);
  sva->perTriTextureID.push_back(textureID);
}
;

sphere
: TOKEN_Sphere TOKEN_Center Float Float Float TOKEN_Rad Float
texture
{
  ospray::tachyon::Sphere s;
  s.center.x = $3;
  s.center.y = $4;
  s.center.z = $5;
  s.rad = $7;
  s.textureID = ospray::tachyon::parserModel->addTexture($8);
  delete $8;
  ospray::tachyon::parserModel->addSphere(s);
}
;


fcylinder
: TOKEN_FCylinder
  TOKEN_Base Float Float Float
  TOKEN_Apex Float Float Float
  TOKEN_Rad Float
texture
{
  ospray::tachyon::Cylinder s;
  s.base.x = $3;
  s.base.y = $4;
  s.base.z = $5;
  s.apex.x = $7;
  s.apex.y = $8;
  s.apex.z = $9;
  s.rad    = $11;

  ospray::tachyon::Texture *texture = $12;
  // PING;
  // PRINT(texture->phong.color);
  s.textureID = ospray::tachyon::parserModel->addTexture(texture);
  delete texture;
  ospray::tachyon::parserModel->addCylinder(s);
}
;

texture
: TOKEN_Texture texture_body { $$ = $2; }
;

texture_body
: /* eol */ { $$ = new ospray::tachyon::Texture; }
| texture_body TOKEN_Ambient  Float { $$ = $1; $$->ambient  = $3; }
| texture_body TOKEN_Diffuse  Float { $$ = $1; $$->diffuse  = $3; }
| texture_body TOKEN_Specular Float { $$ = $1; $$->specular = $3; }
| texture_body TOKEN_Opacity  Float { $$ = $1; $$->opacity  = $3; }
| texture_body TOKEN_Phong TOKEN_Plastic Float TOKEN_Phong_size Float TOKEN_Color vec3f
{
  $$ = $1;
  $$->phong.plastic = $4;
  $$->phong.size = $6;
  $$->color = *$8; delete $8;
}
| texture_body TOKEN_TexFunc Int { $$ = $1; $$->texFunc = $3; }
| texture_body TOKEN_TransMode TOKEN_R3D { $$ = $1; }
;


background
: TOKEN_Background Float Float Float
{ ospray::tachyon::parserModel->backgroundColor = vec3f($2,$3,$4); }
;

background_gradient
: TOKEN_Background_Gradient Identifier background_gradient_body
;

background_gradient_body
: /* eol */
| background_gradient_body TOKEN_UpDir Float Float Float
| background_gradient_body TOKEN_TopVal Float
| background_gradient_body TOKEN_BottomVal Float
| background_gradient_body TOKEN_TopColor Float Float Float
| background_gradient_body TOKEN_BottomColor Float Float Float
;


fog
: TOKEN_Fog TOKEN_Exp2 TOKEN_Start Float TOKEN_End Float TOKEN_Density Float TOKEN_Color Float Float Float
;


camera
: TOKEN_Camera camera_body TOKEN_End_Camera
;

camera_body
: /* empty */
| camera_body TOKEN_Projection TOKEN_Orthographic
| camera_body TOKEN_Zoom Float
| camera_body TOKEN_Aspectratio Float
| camera_body TOKEN_Antialiasing Int
| camera_body TOKEN_Raydepth Int
| camera_body TOKEN_Center  vec3f
{ ospray::tachyon::parserModel->getCamera(true)->center  = *$3; delete $3; }
| camera_body TOKEN_Viewdir vec3f
{ ospray::tachyon::parserModel->getCamera(true)->viewDir = *$3; delete $3; }
| camera_body TOKEN_Updir   vec3f
{ ospray::tachyon::parserModel->getCamera(true)->upDir   = *$3; delete $3; }
;

directional_light
: TOKEN_Directional_Light directional_light_body
{ ospray::tachyon::parserModel->addDirLight(*$2); delete $2; }
;

directional_light_body
: /* eol */ { $$ = new ospray::tachyon::DirLight; }
| directional_light_body TOKEN_Direction vec3f
{ $$ = $1; $$->direction = *$3; delete $3; }
| directional_light_body TOKEN_Color vec3f
{ $$ = $1; $$->color = *$3; delete $3; }
;

vec3f
: Float Float Float
{ $$ = new vec3f($1,$2,$3); }
;

shader_mode
: TOKEN_SHADER_MODE Identifier shader_mode_body TOKEN_END_SHADER_MODE
;

shader_mode_body
: TOKEN_TRANS_VMD shader_mode_body
| TOKEN_FOG_VMD shader_mode_body
| TOKEN_Ambient_Occlusion ambient_occlusion_body
| /* nothing */
;

ambient_occlusion_body
: /* eol */
| ambient_occlusion_body TOKEN_Ambient_Color Float Float Float
| ambient_occlusion_body TOKEN_Rescale_Direct Float
| ambient_occlusion_body TOKEN_Samples Int
;

resolution
: TOKEN_RESOLUTION Int Int
{ printf("#osp:tachyon: ignoring resolution %i %i\n",$2,$3); }
;

Float
: TOKEN_FLOAT_CONSTANT { $$ = yylval.floatVal; }
| Int                  { $$ = $1; }
;

Int: TOKEN_INT_CONSTANT { $$ = yylval.intVal; }
;

Identifier: TOKEN_IDENTIFIER { $$ = yylval.identifierVal; }
;

%%
#include <stdio.h>

void yyerror(const char *s)
{
  std::cout << "=======================================================" << std::endl;
  PRINT(yytext);
  Error(Loc::current, s);
}

