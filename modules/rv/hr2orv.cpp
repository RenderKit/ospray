#include <common/ospcommon.h>
#include <string>
#include <sstream>

using namespace std;
using namespace rivl;

struct Layer {
  //const char *name;
  string name;
  float z0, z1;
  float r,g,b,a;


  long count; // just for stats...
};

struct BVHNode {
  vec3f min;
  unsigned int offset;
  vec3f max;
  unsigned int count;
};

vector<Layer> layers;

#define inf 1e-20f

int maxPrims = 1<<30;

const char *layerDesc = "set layerName2bottomZ(p+) -40 \
set layerName2topZ(pl) 919 \
set layerName2bottomZ(pl) 1320 \
set layerName2topZ(p+) -1 \
set layerName2bottomZ(n+) -40 \
set layerName2topZ(n+) -1 \
set layerName2bottomZ(p) 0 \
set layerName2topZ(p) 39 \
set layerName2bottomZ(tcn) 0 \
set layerName2topZ(tcn) 39 \
set layerName2bottomZ(gcn) 0 \
set layerName2topZ(gcn) 39 \
set layerName2bottomZ(m0) 440 \
set layerName2topZ(m0) 479 \
set layerName2bottomZ(m1) 880 \
set layerName2topZ(m1) 919 \
set layerName2bottomZ(m2) 1320 \
set layerName2topZ(m2) 1359 \
set layerName2bottomZ(m3) 1760 \
set layerName2topZ(m3) 1799 \
set layerName2bottomZ(m4) 2200 \
set layerName2topZ(m4) 2239 \
set layerName2bottomZ(m5) 2640 \
set layerName2topZ(m5) 2679 \
set layerName2bottomZ(m6) 3080 \
set layerName2topZ(m6) 3119 \
set layerName2bottomZ(m7) 3780 \
set layerName2topZ(m7) 3879 \
set layerName2bottomZ(m8) 4480 \
set layerName2topZ(m8) 4579 \
set layerName2bottomZ(m9) 9080 \
set layerName2topZ(m9) 9679 \
set layerName2bottomZ(m10) 13680 \
set layerName2topZ(m10) 14279 \
set layerName2bottomZ(m11) 18280 \
set layerName2topZ(m11) 21279 \
set layerName2bottomZ(tm1) 41280 \
set layerName2topZ(tm1) 44279 \
set layerName2bottomZ(mp1) 64280 \
set layerName2topZ(mp1) 67279 \
set layerName2bottomZ(mp2) 87280 \
set layerName2topZ(mp2) 90279 \
 \
set layerName2bottomZ(vcn) 40 \
set layerName2topZ(vcn) 439 \
set layerName2bottomZ(vg) 40 \
set layerName2topZ(vg) 439 \
set layerName2bottomZ(vt) 40 \
set layerName2topZ(vt) 439 \
set layerName2bottomZ(v0) 480 \
set layerName2topZ(v0) 879 \
set layerName2bottomZ(v1) 920 \
set layerName2topZ(v1) 1319 \
set layerName2bottomZ(v2) 1360 \
set layerName2topZ(v2) 1759 \
set layerName2bottomZ(v3) 1800 \
set layerName2topZ(v3) 2199 \
set layerName2bottomZ(v4) 2240 \
set layerName2topZ(v4) 2639 \
set layerName2bottomZ(v5) 2680 \
set layerName2topZ(v5) 3079 \
set layerName2bottomZ(v6) 3120 \
set layerName2topZ(v6) 3779 \
set layerName2bottomZ(v7) 3880 \
set layerName2topZ(v7) 4479 \
set layerName2bottomZ(v8) 4580 \
set layerName2topZ(v8) 9079 \
set layerName2bottomZ(v9) 9680 \
set layerName2topZ(v9) 13679 \
set layerName2bottomZ(v10) 14280 \
set layerName2topZ(v10) 18279 \
set layerName2bottomZ(v11) 21280 \
set layerName2topZ(v11) 41279 \
set layerName2bottomZ(tv1) 44280 \
set layerName2topZ(tv1) 64279 \
set layerName2bottomZ(vp12) 67280 \
set layerName2topZ(vp12) 87279"; 


#define TOK "\t ()\n\r"

void parseLayerDesc()
{
  char *tok = strtok(strdup(layerDesc),TOK);
  while (tok) {
    //set
    tok = strtok(NULL,TOK);
    //layername...topZ

    // PRINT(tok);
    char *name0 = strtok(NULL,TOK);
    // PRINT(name0);
    char *z0 = strtok(NULL,TOK);
    
    //set
    tok = strtok(NULL,TOK);
    tok = strtok(NULL,TOK);
    //layername
    char *name1 = strtok(NULL,TOK);
    char *z1 = strtok(NULL,TOK);


    // PRINT(z0);
    // PRINT(z1);
    Layer l;
    l.name = strdup(name0);
    l.z0 = atof(z0);
    l.z1 = atof(z1);

    l.r = (((layers.size()+123124)*11*13) % 256) / 255.f;
    l.g = (((layers.size()+123124)*17*13) % 256) / 255.f;
    l.b = (((layers.size()+123124)*11*17) % 256) / 255.f;
    l.a = .5f;
    l.count = 0;
    layers.push_back(l);

    tok = strtok(NULL,TOK);
  }
  
}

long numBoxesWritten = 0;

bool pushBox(//FILE *obj, 
             FILE *box, const box3fa &b)
{
  static long lastPing = 0;

  fwrite(&b,sizeof(b),1,box);

  long thisPing = (++numBoxesWritten)/100000;
  while (thisPing != lastPing) {
    ++lastPing; cout << "[" << lastPing << "00k]" << flush;
  }


  if (numBoxesWritten >= maxPrims) {
    cout << "written " << numBoxesWritten << " boxes, breaking" << endl;
    return false;
  }


  return true;
}

bool copyBinary(FILE *bin, string fileName, unsigned long &where, int &count,long sz)
{
  where = ftell(bin);
  count = 0;
  FILE *in = fopen(fileName.c_str(),"rb");
  if (!in) return false;
  
  char buf[sz];
  while (fread(buf,sz,1,in) && !feof(in)) {
    fwrite(buf,sz,1,bin); ++count;
  }
  fclose(in);
  unlink(fileName.c_str());
  return true;
}

bool copyAttrib(FILE *bin, string baseName, string attrName, unsigned long &where, 
                int &count, float &amin, float &amax)
{
  // return copyBinary(bin,baseName+"."+attrName+".attr",where,count,sizeof(float));
  where = ftell(bin);
  count = 0;
  string fileName = baseName+"."+attrName+".attr";
  FILE *in = fopen(fileName.c_str(),"rb");
  if (!in) return false;
  
  float f;
  while (fread(&f,sizeof(f),1,in) && !feof(in)) {
    if (count == 0) { amin = amax = f; }
    else { amin = min(amin,f), amax=max(amax,f); }
    fwrite(&f,sizeof(f),1,bin); ++count;
  }
  fclose(in);
  unlink(fileName.c_str());
  return true;
}

void makeBGF(string baseName, const vector<string> &attrib)
{
  string xmlName = baseName+".xml";
  string binName = baseName+".xml.bin";
  FILE *bin = fopen(binName.c_str(),"wb");
  FILE *xml = fopen(xmlName.c_str(),"w");

  fprintf(xml,"<?xml version=\"1.0\"?>\n");
  fprintf(xml,"<BGFscene>\n");
  {
    for (int i=0;i<layers.size();i++) {
      Layer *l = &layers[i];
      // fprintf(mat,"newmtl mat_%s\n\tkd %f %f %f\n\td %f\n",
      //         l->name,l->r,l->g,l->b,l->a);
      fprintf(xml,"<Material name=\"%s\" type=\"OBJMaterial\"  id=\"%i\">\n",
              l->name.c_str(),i);
      fprintf(xml," <param name=\"d\" type=\"float\">%f</param>\n",l->a);
      fprintf(xml," <param name=\"kd\" type=\"float3\">%f %f %f</param>\n",
              l->r,l->g,l->b);
      fprintf(xml,"</Material>\n");
    }
    fprintf(xml,"<HaroonMesh name=\"%s\" id=\"1\">\n",baseName.c_str());
    fprintf(xml,"  <materiallist>");
    for (int i=0;i<layers.size();i++)
      fprintf(xml," %i",i);
    fprintf(xml,"  </materiallist>\n");

    int num = -1;
    unsigned long ofs = 0;
    // copyBinary(bin,baseName+".box",ofs,num,sizeof(box3fa));
    // fprintf(xml," <position num=\"%li\" ofs=\"%li\"/>\n",num,ofs);
    copyBinary(bin,baseName+".bvh",ofs,num,2*sizeof(BVHNode));
    fprintf(xml," <position num=\"%li\" ofs=\"%li\"/>\n",num,ofs);
    for (int i=0;i<attrib.size();i++) {
      float minAttrib=0,maxAttrib=0;
      if (copyAttrib(bin,baseName,attrib[i],ofs,num,minAttrib,maxAttrib)) {
        fprintf(xml," <attrib name=\"%s\" num=\"%li\" ofs=\"%li\""
                " min=\"%f\" max=\"%f\"/>\n",
                attrib[i].c_str(),num,ofs,minAttrib,maxAttrib);
      }
    }
    
    {
      // fprintf(xml," <layers num=\"%li\">\n",layers.size());
      for (int i=0;i<layers.size();i++) {
        Layer *l = &layers[i];
        fprintf(xml,"  <layer name=\"%s\" red=\"%f\" "
                "green=\"%f\" blue=\"%f\" count=\"%li\"/>\n",
                l->name.c_str(),l->r,l->g,l->b,l->count);
      }
      // fprintf(xml," </layers>\n");
    }
    fprintf(xml,"</HaroonMesh>\n");
  }
  fprintf(xml,"</BGFscene>\n");
  fclose(bin);
  fclose(xml);
}

void buildRec(box3fa *box, BVHNode *bvh,
              int nodeID, int &nextFree,
              const box3fa &centBounds,int begin, int end)
{
  // if (nodeID < 10) {
  //   cout << "-------------------------------------------------------" << endl;
  //   PRINT(nodeID);
  //   PRINT(begin);
  //   PRINT(end);
  // }
  if (end-begin == 1) {
    bvh[nodeID].min = box[begin].min;
    bvh[nodeID].max = box[begin].max;

    int layerID = (int&)box[begin].max.w;
    if (layerID < 0 || layerID >= layers.size()) {
      PRINT(nodeID);
      PRINT(begin);
      PRINT(end);
      PRINT(layers.size());
      PRINT(layerID);
      PRINT((int&)box[begin].min.w);
      PRINT((int&)box[begin].max.w);
      FATAL("invalid layer id in primitive");
    }
    bvh[nodeID].offset = (int&)box[begin].min.w;
    bvh[nodeID].count  = 1+layerID;
    layers[layerID].count++;
  } else { 
    int mid;
    box3fa lCentBounds;
    box3fa rCentBounds;
    if (centBounds.min == centBounds.max) {
      mid = (begin+end)/2;
      lCentBounds = rCentBounds = centBounds;
    } else {
      int dim = centBounds.widestDimension();
      float split = centBounds.center()[dim];

      lCentBounds.clear();
      rCentBounds.clear();

      long l = begin;
      long r = end-1;
      while (1) {
        while (l <= r) {
          vec3fa cent = box[l].center();
          if (cent[dim] < split) {
            lCentBounds.extend(cent);
            l++;
          } else {
            rCentBounds.extend(cent);
            break;
          }
        }
        while (l <= r) {
          vec3fa cent = box[r].center();
          if (cent[dim] >= split) {
            rCentBounds.extend(cent);
            --r;
          } else {
            lCentBounds.extend(cent);
            break;
          }
        }
        if (l >= r) { 
          break;
        }

        swap(box[l].min.x,box[r].min.x);
        swap(box[l].min.y,box[r].min.y);
        swap(box[l].min.z,box[r].min.z);
        swap(box[l].min.w,box[r].min.w);

        swap(box[l].max.x,box[r].max.x);
        swap(box[l].max.y,box[r].max.y);
        swap(box[l].max.z,box[r].max.z);
        swap(box[l].max.w,box[r].max.w);

        // swap(box[l],box[r]);
        ++l;
        --r;
      }
      mid = l;

      if (mid == begin || mid == end) {
        PRINT(begin);
        PRINT(end);
        PRINT(mid);
        PRINT(centBounds);
        FATAL("invalid split!");
        mid = (begin+end)/2;
        lCentBounds = rCentBounds = centBounds;
      }
    }
    int child = nextFree;
    nextFree+=2;

    bvh[nodeID].count = 0;
    bvh[nodeID].offset = child;

    buildRec(box,bvh,child+0,nextFree,lCentBounds,begin,mid);
    buildRec(box,bvh,child+1,nextFree,rCentBounds,mid,end);
    bvh[nodeID].min = min(bvh[child+0].min,
                          bvh[child+1].min);
    bvh[nodeID].max = max(bvh[child+0].max,
                          bvh[child+1].max);
  }
}

void *mmapFile(const string &fileName, long fileSize)
{
  cout << "mapping memfile " << fileName << endl;
  int fd = ::open(fileName.c_str(),O_LARGEFILE|O_RDWR);
  if (fd == -1)
    perror("could not open file");
  void *mem = mmap(NULL,fileSize,PROT_READ|PROT_WRITE,MAP_SHARED,fd,0);
  if (!mem)
    FATAL("could not mmap! " << fileName);
  return mem;
}

void buildBVH(const string boxName, const string bvhName, 
              const box3fa &centBounds, long numPrims)
{
  cout << "preparing BVH build" << endl;
  FILE *bvhFile = fopen(bvhName.c_str(),"wb");
  assert(bvhFile);
  BVHNode dummyNode;
  for (int i=0;i<2*numPrims;i++)
    fwrite(&dummyNode,sizeof(dummyNode),1,bvhFile);
  fclose(bvhFile);
  
  box3fa *box   = (box3fa*)mmapFile(boxName,numPrims*sizeof(box3fa));
  BVHNode *bvh = (BVHNode*)mmapFile(bvhName,numPrims*2*sizeof(BVHNode));
  // int  *primID = new int[numPrims];
  // for (int i=0;i<numPrims;i++)
  //   primID[i] = i;
  
  int nextNode = 2;
  cout << "building BVH..." << endl;
  buildRec(box,bvh,0,nextNode,centBounds,0,numPrims);
  cout << "BVH build done!" << endl;

  munmap(box,numPrims*sizeof(box3fa));
  munmap(bvh,numPrims*2*sizeof(BVHNode));
}

int main(int ac, char **av)
{
  if (ac == 1) 
    FATAL("usage: hr2rivl [--max maxPrimsToWrite] input.hr output // no extension on output");

  string baseName = "/tmp/haroon";
  string inName = "";
  for (int i=1;i<ac;i++) {
    string arg = av[i];
    if (arg == "--max") {
      maxPrims = atoi(av[++i]);
      continue;
    } else if (arg == "--maxPrimsToWrite") {
      maxPrims = atoi(av[++i]);
      continue;
    } else if (arg[0] == '-') {
      FATAL("unknown param "<<arg);
    } else if (inName == "") {
      inName = arg;
    } else {
      baseName = arg;
    }
  }

  parseLayerDesc();
  // string objName = string(baseName)+".obj";
  // string matName = string(baseName)+".mat";

  box3fa allBounds; allBounds.clear();
  box3fa allCentBounds; allCentBounds.clear();
  // FILE *obj = fopen(objName.c_str(),"w");
  string boxName = string(baseName)+".box";
  string bvhName = string(baseName)+".bvh";
  FILE *box = fopen(boxName.c_str(),"wb");
  assert(box);
  FILE *in = fopen(inName.c_str(),"r");
  assert(in);
  // fprintf(obj,"mtllib %s\n",matName.c_str());
#define LNSZ 10000
  char line[LNSZ];
  Layer *l = NULL;
  long layerID = -1;
  vector<string> attributes;
  vector<FILE *> attribFile;
  while (fgets(line,LNSZ,in) && !feof(in)) {
    // if (line[0] == '&') continue;
    // if (line[0] == '!') continue;
    // if (line[0] == 0) continue;
    // if (line[0] == '\n') continue;

    char *tok = strtok(line,TOK); // attrib
    if (!tok) continue;

    if (!strcmp(tok,"&ATTRIBUTES")) {
      cout << "parsing attributes" << endl;
      if (!attributes.empty())
        FATAL("&ATTRIBUTES specified twice!?");
      while (tok) {
        // PRINT(tok);
        attributes.push_back(tok);
        std::stringstream afName;
        afName << string(baseName) << "." << tok << ".attr";
        FILE *af = NULL;
        if (tok[0] != '&'
            && strcmp(tok,"llx")
            && strcmp(tok,"lly")
            && strcmp(tok,"urx")
            && strcmp(tok,"ury")
            && strcmp(tok,"layer")
            ) {
          af = fopen(afName.str().c_str(),"wb");
          assert(af);
        }
        // cout << "FILE no " << attribFile.size() << " is " << (int*)af << endl;
        attribFile.push_back(af);
        tok = strtok(NULL,TOK);
      }
      cout << "found " << attributes.size() << " attributes" << endl;
      continue;
    }
    if (tok[0] == '&') continue;
    if (tok[0] == '!') continue;
    
    if (attributes.empty()) 
      FATAL("no attributes specified!");

    string layerName = "";
    float llx = -inf,lly = -inf,urx = -inf,ury = -inf;
    int attrID = 0;
    vector<string>::const_iterator it = attributes.begin();
    while (tok) {
      if (it == attributes.end())
        FATAL("more elements than attributes!");
      if (*it == "llx")
        llx = atof(tok);
      else if (*it == "lly")
        lly = atof(tok);
      else if (*it == "urx")
        urx = atof(tok);
      else if (*it == "ury")
        ury = atof(tok);
      else if (*it == "layer")
        layerName = tok;
      else {
        // PRINT(attrID);
        FILE *af = attribFile[attrID];
        // PRINT(af);
        if (af) {
          // PRINT(attribFile.size());
          // PRINT(af);
          assert(af);
          float f = atof(tok);
          // PRINT(ftell(af));
          // PRINT(f);
          fwrite(&f,sizeof(float),1,af);
        }
      }
      
      tok = strtok(NULL,TOK);
      ++it;
      ++attrID;
    }


    if (llx == -inf)
      FATAL("llx not specified");
    if (lly == -inf)
      FATAL("lly not specified");
    if (urx == -inf)
      FATAL("urx not specified");
    if (ury == -inf)
      FATAL("ury not specified");
    if (layerName == "")
      FATAL("layer name not specified");
// // &ATTRIBUTES Idc Irms currentDir IdcUpNE IdcDnSW current_type layer llx lly urx ury resistance 
//     tok = strtok(NULL,TOK); // Idc
//     tok = strtok(NULL,TOK); // Irms
//     tok = strtok(NULL,TOK); // currentDir
//     tok = strtok(NULL,TOK); // IdcUpNE
//     tok = strtok(NULL,TOK); // IdcUpSW
//     tok = strtok(NULL,TOK); // current_type
//     char *layerName = strtok(NULL,TOK); // layer
    // for (char *c = (char*)layerName.c_str(); *c; ++c)
    //   *c = tolower(*c);
    // float llx = atof(strtok(NULL,TOK)); // llx
    // float lly = atof(strtok(NULL,TOK)); // lly
    // float urx = atof(strtok(NULL,TOK)); // urx
    // float ury = atof(strtok(NULL,TOK)); // ury
    // tok = strtok(NULL,TOK); // resistance


//     char attrib[100];
//     float Idc, Irms;
//     char currentDir[100], currentType[100];
//     char layerName[10];
    box3fa bounds;
    bounds.min.x = llx;
    bounds.min.y = lly;
    bounds.max.x = urx;
    bounds.max.y = ury;

    if (!l || strcasecmp(l->name.c_str(),layerName.c_str())) {
      l = &*layers.begin();//&layers[0]; 
      layerID = 0;
      while (l!=&*layers.end()// l->name
             && strcasecmp(l->name.c_str(),layerName.c_str())) {
        l++;
        layerID++;
      }
      if (l == &*layers.end()) {
        // FATAL("layer "+string(layerName)+" not found");
        cout << "layer "+string(layerName)+" not found" << endl;
        l = &layers[0];
        layerID = 0;
      }
      // cout << "[" << layerName << "]" << flush;
      // cout << "parsing layer " << layerName << endl;
      assert(l->name != "");
      // fprintf(obj,"usemtl mat_%s\n",l->name);
    }

    bounds.min.z = l->z0;
    bounds.max.z = l->z1;
    (int&)bounds.min.w = numBoxesWritten;
    (int&)bounds.max.w = layerID;
    allBounds.extend(bounds);
    allCentBounds.extend(bounds.center());
    if (!pushBox(box,bounds))
      break;
  }
  for (int i=0;i<attribFile.size();i++)
    if (attribFile[i]) fclose(attribFile[i]);

  PRINT(allBounds);
  fclose(box);


  buildBVH(boxName,bvhName,allCentBounds,numBoxesWritten);
  

  makeBGF(baseName,attributes);

  unlink(bvhName.c_str());
  unlink(boxName.c_str());

  // cout << "Stats: prims per layer" << endl;
  // for (int i=0;i<layers.size();i++) {
  //   cout << "layer " << layers[i].name << " : " << layers[i].count << endl;
  // }
}
