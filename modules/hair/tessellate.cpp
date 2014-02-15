// do assertions in this file - it's not performance critical
#undef NDEBUG

#include "common/ospcommon.h"

namespace ospray {
  namespace hair {
    using std::cout;
    using std::endl;

    uint maxHairs = -1;
    uint numStrips = 6;
    uint numSubdiv = 1;

    ulong numGenerated = 0;

    struct Hair {
      struct Point {
        vec3f pos;
        float radius;

        inline operator vec4f() const { return (vec4f&)pos; }
        Point() {};
        Point(const vec4f &a) { (vec4f&)pos = a; }
        inline Point &operator=(const vec4f &a) { (vec4f&)pos = a; return *this; }
      };
      struct Curve {
        std::string name;
        std::vector<Point> point;
        // std::vector<Point> tangent; // derivative, used for cubic bezier splines

        Curve(std::string name) : name(name) {};

        // void computeTangents()
        // {
        //   tangent.resize(point.size());
        //   for (int i=1;i<point.size()-1;i++)
        //     tangent[i] = 0.3f * ((vec4f)point[i+1]-(vec4f)point[i-1]);

        //   int N = point.size()-1;
        //   tangent[0] = 0.3f * (((vec4f)point[1]-2.f*(vec4f)tangent[1])-(vec4f)point[0]);
        //   tangent[N] = 0.3f * ((vec4f)point[N]-((vec4f)point[N-1]+2.f*(vec4f)tangent[N-1]));
        // }
      };
      std::vector<Curve*> curve;


      void parse(std::string fileName);
    };

    void emitSegment(const Hair::Point &a, const Hair::Point &b, FILE *file)
    {
      vec3f l = b.pos - a.pos;
      if (length(l) < 1e-6f) return;
      embree::LinearSpace3<vec3f> frame = embree::frame(l);

#if 1
      for (int i=0;i<numStrips;i++) {
        const float f = (i/float(numStrips))*2.f*M_PI;
        const float u = cosf(f);
        const float v = sinf(f);
        vec3f va
          = a.pos 
          + a.radius * u * frame.vx 
          + a.radius * v * frame.vy;
        fprintf(file,"v %f %f %f\n",va.x,va.y,va.z);
        vec3f vb
          = b.pos 
          + b.radius * u * frame.vx 
          + b.radius * v * frame.vy;
        fprintf(file,"v %f %f %f\n",vb.x,vb.y,vb.z);
      }
      for (int i=0;i<numStrips;i++) {
        int total = 2*numStrips;
        int a0 = (2*i)  %total;
        int b0 = (2*i+1)%total;
        int a1 = (2*i+2)%total;
        int b1 = (2*i+3)%total;
        fprintf(file,"f %i %i %i %i\n",-total+a0,-total+b0,-total+b1,-total+a1);
        numGenerated+=2;
      }
#else
      vec3f begin_x0 = a.pos - a.radius * frame.vx;
      vec3f begin_x1 = a.pos + a.radius * frame.vx;
      
      vec3f begin_y0 = a.pos - a.radius * frame.vy;
      vec3f begin_y1 = a.pos + a.radius * frame.vy;
      
      vec3f end_x0 = b.pos - b.radius * frame.vx;
      vec3f end_x1 = b.pos + b.radius * frame.vx;
      
      vec3f end_y0 = b.pos - b.radius * frame.vy;
      vec3f end_y1 = b.pos + b.radius * frame.vy;
      
#if 1
      fprintf(file,"v %f %f %f\n",begin_x0.x,begin_x0.y,begin_x0.z);
      fprintf(file,"v %f %f %f\n",begin_x1.x,begin_x1.y,begin_x1.z);
      fprintf(file,"v %f %f %f\n",end_x0.x,end_x0.y,end_x0.z);
      fprintf(file,"v %f %f %f\n",end_x1.x,end_x1.y,end_x1.z);

      fprintf(file,"v %f %f %f\n",begin_y0.x,begin_y0.y,begin_y0.z);
      fprintf(file,"v %f %f %f\n",begin_y1.x,begin_y1.y,begin_y1.z);
      fprintf(file,"v %f %f %f\n",end_y0.x,end_y0.y,end_y0.z);
      fprintf(file,"v %f %f %f\n",end_y1.x,end_y1.y,end_y1.z);

      fprintf(file,"f -8 -4 -2 -6\n");
      fprintf(file,"f -4 -7 -5 -2\n");

      fprintf(file,"f -7 -3 -1 -5\n");
      fprintf(file,"f -3 -8 -6 -1\n");
#else
      fprintf(file,"v %f %f %f\n",begin_x0.x,begin_x0.y,begin_x0.z);
      fprintf(file,"v %f %f %f\n",begin_x1.x,begin_x1.y,begin_x1.z);
      fprintf(file,"v %f %f %f\n",end_x0.x,end_x0.y,end_x0.z);
      fprintf(file,"v %f %f %f\n",end_x1.x,end_x1.y,end_x1.z);
      fprintf(file,"f -4 -3 -1 -2\n");
      
      fprintf(file,"v %f %f %f\n",begin_y0.x,begin_y0.y,begin_y0.z);
      fprintf(file,"v %f %f %f\n",begin_y1.x,begin_y1.y,begin_y1.z);
      fprintf(file,"v %f %f %f\n",end_y0.x,end_y0.y,end_y0.z);
      fprintf(file,"v %f %f %f\n",end_y1.x,end_y1.y,end_y1.z);
      fprintf(file,"f -4 -3 -1 -2\n");
#endif
#endif
    }

    void emitSegment(const Hair::Point &v00,
                     const Hair::Point &v01,
                     const Hair::Point &v02,
                     const Hair::Point &v03,
                     int subdiv, FILE *file)
    {
      if (subdiv > 0) {
        Hair::Point v10 = 0.5f*((vec4f)v00+(vec4f)v01);
        Hair::Point v11 = 0.5f*((vec4f)v01+(vec4f)v02);
        Hair::Point v12 = 0.5f*((vec4f)v02+(vec4f)v03);

        Hair::Point v20 = 0.5f*((vec4f)v10+(vec4f)v11);
        Hair::Point v21 = 0.5f*((vec4f)v11+(vec4f)v12);

        Hair::Point v30 = 0.5f*((vec4f)v20+(vec4f)v21);
        emitSegment(v00,v10,v20,v30,subdiv-1,file);
        emitSegment(v30,v21,v12,v03,subdiv-1,file);
      } else {
        emitSegment(v00,v01,file);
        emitSegment(v01,v02,file);
        emitSegment(v02,v03,file);
      }
    }
    // void tessellate(Hair::Curve*curve,int i, int j, FILE *file)
    // {
    //   PING;
    //   PRINT(i);
    //   Hair::Point v00 = (vec4f)curve->point[i];
    //   Hair::Point v01 = (vec4f)curve->point[i]+(vec4f)curve->tangent[i];
    //   Hair::Point v02 = (vec4f)curve->point[j]-(vec4f)curve->tangent[j];
    //   Hair::Point v03 = (vec4f)curve->point[j];
    //   emitSegment(v00,v01,v02,v03,numSubdiv,file);
    // }
    void tessellate(Hair::Curve &curve, FILE *file)
    {
#if 1
      for (int i=0;(i+3) < curve.point.size();i+=3) {
        emitSegment(curve.point[i],
                    curve.point[i+1],
                    curve.point[i+2],
                    curve.point[i+3],
                    numSubdiv,file);
      }
#else
      for (int i=1;i<curve.point.size();i++) {
        tessellate(&curve,i-1,i,file); //curve[i-1],curve[i],file);
        // PRINT(l);
        // PRINT(frame.vx);
        // PRINT(frame.vy);
        // PRINT(frame.vz);
      }
#endif
    }

    void tessellate(Hair &hair, std::string outFileName)
    {
      FILE *file = fopen(outFileName.c_str(),"w");
      Assert(file);
      for (int i=0;i<hair.curve.size();i++) {
        tessellate(*hair.curve[i],file);
      }
      fclose(file);
    }

    void Hair::parse(std::string fileName)
    {
      FILE *file = fopen(fileName.c_str(),"r");
      const int LINESZ=10000;
      char line[LINESZ];
      char name[LINESZ];
      char *s = fgets(line,LINESZ,file); // header
      assert(s);
      int totalPoints = 0;
      int totalSegments = 0;
      while (fgets(line,LINESZ,file) && !feof(file) && curve.size() < maxHairs) {
        // new curve
        int numPoints = 0;
        int rc = sscanf(line,"Curve: %s 4 Tracks %i",name,&numPoints);
        Assert2(rc == 2, "could not parse num points");
        // printf("[%i]",numPoints); fflush(0);
        // printf("[%s:%i]",name,numPoints); fflush(0);
        s = fgets(line,LINESZ,file); // curve header (track info)
        assert(s);
        Curve *curve_i = new Curve(name); this->curve.push_back(curve_i);

        for (int i=0;i<numPoints;i++) {
          Point p;
          s = fgets(line,LINESZ,file); // header
          Assert(s);
          int id;
          if (i==0) {
            rc = sscanf(line,"%i: %s %f %f %f %f",&id,name,&p.pos.x,&p.pos.y,&p.pos.z,&p.radius);
            Assert(rc == 6);
          } else {
            rc = sscanf(line,"%i: %f %f %f %f",&id,&p.pos.x,&p.pos.y,&p.pos.z,&p.radius);
            Assert(rc == 5);
          }
          // cout << p.pos << " " << p.radius << endl;
          curve_i->point.push_back(p);
          totalPoints++;
        }
        totalSegments += curve_i->point.size()-1;
      }
      cout << endl;
      cout << "done parsing" << endl;
      cout << " num curves " << this->curve.size() << endl;
      cout << " total num points " << totalPoints << endl;
      cout << " total num segments " << totalSegments << endl;
      fclose(file);
    }
    void usage()
    {
      cout << "usage: ./tesselate <infile.txt> -o <outfile.obj> [-nsub <nsub>] [-nstrips <nstrips>] [--max-hairs <max-hairs>]" << endl;
      cout << " max-hairs: maximum number of hari curves we'll be parsing" << endl;
      cout << " nsub : number of bezier subdivisions to be applies to each hair segment" << endl;
      cout << "        (careful: num polys is exponential in this number: 1 or 2 is usually good" << endl;
      cout << " nstrips: number of 'strips' we'll be subdivigin each cylinder into" << endl;
      exit(0);
    }

    void main(int ac, char **av)
    {
      char *inFileName = NULL;
      char *outFileName = NULL;
      
      for (int i=1;i<ac;i++) {
        std::string arg = av[i];
        if (arg[0] == '-') {
          if (arg == "-o")
            outFileName = av[++i];
          else if (arg == "-h" || arg == "--help")
            usage();
          else if (arg == "-nsub" || arg == "--nsub")
            numSubdiv = atoi(av[++i]);
          else if (arg == "-nstrips" || arg == "--nstrips")
            numStrips = atoi(av[++i]);
          else if (arg == "--max-hairs" || arg == "--max")
            maxHairs = atoi(av[++i]);
          else 
            usage();
        } else {
          if (inFileName) usage();
          inFileName = av[i];
        }
      }
      if (!inFileName) usage();
      if (!outFileName) usage();
      Hair hair;
      hair.parse(inFileName);
      tessellate(hair,outFileName);
      cout << "total num triangles emitted: " << numGenerated*1e-6f << "M" << endl;
    }
  }
}

int main(int ac, char **av)
{
  ospray::hair::main(ac,av);
  return 0;
}
