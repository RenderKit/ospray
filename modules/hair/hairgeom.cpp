
#undef NDEBUG

#include "hairgeom.h"

namespace ospray {
  namespace hair {

    int maxHairsToParse = 1<<30;
    const int NUM_SUBDIVS_FOR_LEAFTEST = 3;

    box3f vertexBounds3f_outer(const Vertex &v)
    {
      return box3f(v.pos-vec3f(v.radius),v.pos+vec3f(v.radius));
    }
    box4f vertexBounds_outer(const Vertex &v)
    {
      vec4f rad(v.radius,v.radius,v.radius,0.f);
      return box4f((vec4f)v-rad,(vec4f)v+rad);
    }




    box3f vertexBounds3f_inner(const Vertex &v)
    {
      return box3f(v.pos,v.pos);
    }
    box4f vertexBounds_inner(const Vertex &v)
    {
      return box4f((vec4f)v,(vec4f)v);
    }

    inline box3f lineBounds_inner(const Vertex &v0, const Vertex &v1)
    {
      return box3f(min(v0.pos,v1.pos),max(v0.pos,v1.pos));
    }

    bool overlaps_inner(const Segment &seg, const box3f &bounds, int depth)
    {
      if (embree::disjoint(bounds,lineBounds_inner(seg.v0,seg.v1)) && 
          embree::disjoint(bounds,lineBounds_inner(seg.v1,seg.v2)) &&
          embree::disjoint(bounds,lineBounds_inner(seg.v2,seg.v3)))
        return false;
      
      if (depth >= NUM_SUBDIVS_FOR_LEAFTEST) return true;

      Segment s0,s1;
      seg.subdivide(s0,s1);
      return overlaps_inner(s0,bounds,depth+1) || overlaps_inner(s1,bounds,depth+1);
    }


    inline box3f lineBounds_outer(const Vertex &v0, const Vertex &v1)
    {
      box3f b = embree::empty;
      b.extend(vertexBounds3f_outer(v0));
      b.extend(vertexBounds3f_outer(v1));
      return b;
    }

    bool overlaps_outer(const Segment &seg, const box3f &bounds, int depth)
    {
      if (embree::disjoint(bounds,lineBounds_outer(seg.v0,seg.v1)) && 
          embree::disjoint(bounds,lineBounds_outer(seg.v1,seg.v2)) &&
          embree::disjoint(bounds,lineBounds_outer(seg.v2,seg.v3)))
        return false;
      
      if (depth >= NUM_SUBDIVS_FOR_LEAFTEST) return true;

      Segment s0,s1;
      seg.subdivide(s0,s1);
      return overlaps_outer(s0,bounds,depth+1) || overlaps_outer(s1,bounds,depth+1);
    }

    bool computeOverlap_inner(const Vertex &v0,
                        const Vertex &v1,
                        const box4f &bounds, 
                        box4f &overlap)
    {
      box4f lineBounds = embree::empty;
      lineBounds.extend(vertexBounds_inner(v0));
      lineBounds.extend(vertexBounds_inner(v1));
      if (embree::disjoint(bounds,lineBounds))
        return false;
      overlap = embree::merge(overlap,embree::intersect(bounds,lineBounds));
      return true;
    }

    int computeOverlap_inner(const Segment &seg, 
                             const box4f &bounds, 
                             box4f &overlap,
                             int depth)
    {
      box4f segBounds = seg.bounds_inner();
      if (embree::disjoint(bounds,segBounds))
        return 0;
      if (depth < NUM_SUBDIVS_FOR_LEAFTEST) {
        Segment s0,s1;
        seg.subdivide(s0,s1);
        return
          computeOverlap_inner(s0,bounds,overlap,depth+1) +
          computeOverlap_inner(s1,bounds,overlap,depth+1);
      } else {
        return
          computeOverlap_inner(seg.v0,seg.v1,bounds,overlap) +
          computeOverlap_inner(seg.v1,seg.v2,bounds,overlap) +
          computeOverlap_inner(seg.v2,seg.v3,bounds,overlap);
      }
    }




    bool computeOverlap_outer(const Vertex &v0,
                        const Vertex &v1,
                        const box4f &bounds, 
                        box4f &overlap)
    {
      box4f lineBounds = embree::empty;
      lineBounds.extend(vertexBounds_outer(v0));
      lineBounds.extend(vertexBounds_outer(v1));
      if (embree::disjoint(bounds,lineBounds))
        return false;
      overlap = embree::merge(overlap,embree::intersect(bounds,lineBounds));
      return true;
    }

    int computeOverlap_outer(const Segment &seg, 
                             const box4f &bounds, 
                             box4f &overlap,
                             int depth)
    {
      box4f segBounds = seg.bounds_outer();
      if (embree::disjoint(bounds,segBounds))
        return 0;
      if (depth < NUM_SUBDIVS_FOR_LEAFTEST) {
        Segment s0,s1;
        seg.subdivide(s0,s1);
        return
          computeOverlap_outer(s0,bounds,overlap,depth+1) +
          computeOverlap_outer(s1,bounds,overlap,depth+1);
      } else {
        return
          computeOverlap_outer(seg.v0,seg.v1,bounds,overlap) +
          computeOverlap_outer(seg.v1,seg.v2,bounds,overlap) +
          computeOverlap_outer(seg.v2,seg.v3,bounds,overlap);
      }
    }

    box4f Segment::bounds_inner() const 
    {
      box4f b(v0,v0);
      b.extend(v1);
      b.extend(v2);
      b.extend(v3);
      return b;
    }
    box3f Segment::bounds3f_inner() const 
    {
      box3f b(v0.pos,v0.pos);
      b.extend(v1.pos);
      b.extend(v2.pos);
      b.extend(v3.pos);
      return b;
    }
    box4f Segment::bounds_outer() const 
    {
      box4f b = embree::empty;
      b.extend(vertexBounds_outer(v0));
      b.extend(vertexBounds_outer(v1));
      b.extend(vertexBounds_outer(v2));
      b.extend(vertexBounds_outer(v3));
      return b;
    }
    box3f Segment::bounds3f_outer() const 
    {
      box3f b = embree::empty;
      b.extend(vertexBounds3f_outer(v0));
      b.extend(vertexBounds3f_outer(v1));
      b.extend(vertexBounds3f_outer(v2));
      b.extend(vertexBounds3f_outer(v3));
      return b;
    }

    Segment HairGeom::getSegment(int cID, int sID) const
    {
      return getSegment(curve[cID].firstVertex+3*sID);
    }
    Segment HairGeom::getSegment(long startVertex) const
    {
      Segment s;
      s.v0 = vertex[startVertex+0];
      s.v1 = vertex[startVertex+1];
      s.v2 = vertex[startVertex+2];
      s.v3 = vertex[startVertex+3];
      return s;
    }


    int HairGeom::numSegments() const 
    {
      int sum = 0;
      for (int i=0;i<curve.size();i++) sum += curve[i].numSegments;
      return sum;
    }


    void HairGeom::importTXT(const std::string &inFileName)
    {
      FILE *file = fopen(inFileName.c_str(),"r");
      Assert(file);

      const int LINESZ=10000;
      char line[LINESZ];
      char name[LINESZ];
      char *s = fgets(line,LINESZ,file); // header
      assert(s);
      int totalPoints = 0;
      int totalSegments = 0;
      bounds = embree::empty;
      while (fgets(line,LINESZ,file) && !feof(file) && curve.size() < maxHairsToParse) {
        // new curve
        int numPoints = 0;
        int rc = sscanf(line,"Curve: %s 4 Tracks %i",name,&numPoints);
        Assert2(rc == 2, "could not parse num points");
        // printf("[%i]",numPoints); fflush(0);
        // printf("[%s:%i]",name,numPoints); fflush(0);
        s = fgets(line,LINESZ,file); // curve header (track info)
        assert(s);
        // Curve *curve_i = new Curve(name); this->curve.push_back(curve_i);
        Curve curve;
        curve.firstVertex = vertex.size();
        for (int i=0;i<numPoints;i++) {
          s = fgets(line,LINESZ,file); // header
          Assert(s);
          int id;
          Vertex p;
          if (i==0) {
            rc = sscanf(line,"%i: %s %f %f %f %f",&id,name,&p.pos.x,&p.pos.y,&p.pos.z,&p.radius);
            Assert(rc == 6);
          } else {
            rc = sscanf(line,"%i: %f %f %f %f",&id,&p.pos.x,&p.pos.y,&p.pos.z,&p.radius);
            Assert(rc == 5);
          }
          vertex.push_back(p);
          bounds.extend(p);
        }
        curve.numSegments = (vertex.size()-curve.firstVertex) / 3;
        this->curve.push_back(curve);
        totalSegments += curve.numSegments;
      }
      cout << endl;
      cout << "done parsing" << endl;
      cout << " total num points " << vertex.size() << endl;
      cout << " total num curves " << curve.size() << endl;
      cout << " total num segments " << totalSegments << endl;
      fclose(file);
    }

    struct HBHeader {
      float version;
      int   numVertices;
      int   numCurves;
      int   numSegments;
      box4f bounds;
    };

    void HairGeom::save(const std::string &outFileName)
    {
      FILE *file = fopen(outFileName.c_str(),"wb");
      Assert(file);
      
      HBHeader header;
      header.version = 1.01f;
      header.numVertices = vertex.size();
      header.numCurves   = curve.size();
      header.numSegments = numSegments();
      header.bounds = bounds;
      PRINT(bounds);
      fwrite(&header,sizeof(header),1,file);
      fwrite(&vertex[0],sizeof(vertex[0]),vertex.size(),file);
      fwrite(&curve[0],sizeof(curve[0]),curve.size(),file);
      fclose(file);
    }

    void HairGeom::load(const std::string &outFileName)
    {
      FILE *file = fopen(outFileName.c_str(),"rb");
      Assert(file);
      
      HBHeader header;
      fread(&header,sizeof(header),1,file);
      vertex.resize(header.numVertices);
      fread(&vertex[0],sizeof(vertex[0]),vertex.size(),file);
      curve.resize(header.numCurves);
      fread(&curve[0],sizeof(curve[0]),curve.size(),file);
      fclose(file);
      bounds = header.bounds;
      
      PRINT(header.numVertices);
      PRINT(header.numVertices);
      PRINT(header.numSegments);
      PRINT(header.bounds);
    }

    void HairGeom::convertOOC(const std::string &inFileName, const std::string &outFileName)
    {
      std::stringstream cmd;
      cmd << "/usr/bin/bunzip2 -c " << inFileName;
      PRINT(cmd.str());
      FILE *in = popen(cmd.str().c_str(),"r");
      // FILE *in = fopen(inFileName.c_str(),"r");
      Assert(in);

      FILE *out = fopen(outFileName.c_str(),"wb");
      Assert(out);

      HBHeader header;
      bzero(&header,sizeof(header));
      fwrite(&header,sizeof(header),1,out);

      const int LINESZ=10000;
      char line[LINESZ];
      char name[LINESZ];
      char *s = fgets(line,LINESZ,in); // header
      assert(s);
      int totalPoints = 0;
      int totalSegments = 0;
      bounds = embree::empty;
#if 1
      int id;
      Vertex p;
      std::vector<Vertex> curveVtx;
      while (fgets(line,LINESZ,in) && !feof(in)) {
        if (line[0] == 'C') /* Curve : ...*/ {
          fgets(line,LINESZ,in); // Tracks ...
          fgets(line,LINESZ,in); // first line
          if (!curveVtx.empty()) {
            Curve curve;
            curve.firstVertex = totalPoints; //vertex.size();
            curve.numSegments = curveVtx.size() / 3;
            this->curve.push_back(curve);
            totalSegments += curve.numSegments;
            for (int i=0;i<curveVtx.size();i++) 
              fwrite(&curveVtx[i],sizeof(curveVtx[i]),1,out);
            totalPoints += curveVtx.size();
            curveVtx.clear();
          } 
          int rc = sscanf(line,"%i: %s %f %f %f %f",&id,name,&p.pos.x,&p.pos.y,&p.pos.z,&p.radius);
          if (rc != 6) PRINT(line);
          Assert(rc == 6);
          curveVtx.push_back(p);
          bounds.extend(p);
        } else {
          int rc = sscanf(line,"%i: %f %f %f %f",&id,&p.pos.x,&p.pos.y,&p.pos.z,&p.radius);
          Assert(rc == 5);
          curveVtx.push_back(p);
          bounds.extend(p);
        }
      }
      Curve curve;
      curve.firstVertex = totalPoints; //vertex.size();
      curve.numSegments = curveVtx.size() / 3;// (totalPoints-curve.firstVertex) / 3;
      this->curve.push_back(curve);
      totalSegments += curve.numSegments;
      for (int i=0;i<curveVtx.size();i++) 
        fwrite(&curveVtx[i],sizeof(curveVtx[i]),1,out);
      totalPoints += curveVtx.size();
      curveVtx.clear();
#else
      while (fgets(line,LINESZ,in) && !feof(in) && curve.size() < maxHairsToParse) {
        // new curve
        int numPoints = 0;
        int rc = sscanf(line,"Curve: %s 4 Tracks %i",name,&numPoints);
        if (rc != 2) {
          cout << "error in parsing: " << endl;
          PRINT(line);
        }
        Assert2(rc == 2, "could not parse num points");
        // printf("[%i]",numPoints); fflush(0);
        // printf("[%s:%i]",name,numPoints); fflush(0);
        s = fgets(line,LINESZ,in); // curve header (track info)
        assert(s);
        // Curve *curve_i = new Curve(name); this->curve.push_back(curve_i);
        Curve curve;
        curve.firstVertex = totalPoints; //vertex.size();
        for (int i=0;i<numPoints;i++) {
          s = fgets(line,LINESZ,in); // header
          Assert(s);
          int id;
          Vertex p;
          if (i==0) {
            rc = sscanf(line,"%i: %s %f %f %f %f",&id,name,&p.pos.x,&p.pos.y,&p.pos.z,&p.radius);
            Assert(rc == 6);
          } else {
            rc = sscanf(line,"%i: %f %f %f %f",&id,&p.pos.x,&p.pos.y,&p.pos.z,&p.radius);
            Assert(rc == 5);
          }
          totalPoints++;
          fwrite(&p,sizeof(p),1,out);
          // vertex.push_back(p);
          bounds.extend(p);
        }
        curve.numSegments = (totalPoints-curve.firstVertex) / 3;
        this->curve.push_back(curve);
        totalSegments += curve.numSegments;
      }
#endif
      cout << endl;
      cout << "done parsing" << endl;
      cout << " total num points " << totalPoints << endl;
      cout << " total num curves " << this->curve.size() << endl;
      cout << " total num segments " << totalSegments << endl;
      fclose(in);

      fwrite(&this->curve[0],sizeof(this->curve[0]),this->curve.size(),out);
      header.version = 1.01f;
      header.numVertices = totalPoints;//vertex.size();
      header.numCurves   = this->curve.size();
      header.numSegments = numSegments();
      header.bounds = bounds;
      PRINT(bounds);

      fseek(out,0,SEEK_SET);
      fwrite(&header,sizeof(header),1,out);

      fclose(out);
    }


    /*! transform all vertices such that bouds are [(0):(1)] */
    void HairGeom::transformToOrigin()
    {
      // vec4f shift = bounds.lower;
      // vec4f scale = rcp(bounds.upper - bounds.lower);

      // for (int i=0;i<vertex.size();i++)
      //   vertex[i] = ((vec4f)vertex[i]-shift)*scale;

      // bounds.lower = (bounds.lower-shift)*scale;
      // bounds.upper = (bounds.upper-shift)*scale;
      vec4f shift = bounds.lower;
      shift.w = 0.f;
      // vec4f scale = rcp(bounds.upper - bounds.lower);

      for (int i=0;i<vertex.size();i++)
        vertex[i] = ((vec4f)vertex[i]-shift);

      bounds.lower = (bounds.lower-shift);
      bounds.upper = (bounds.upper-shift);
    }

    void Segment::subdivide(Segment &l, Segment &r, const float f) const
    {
      const vec4f v00 = v0;
      const vec4f v01 = v1;
      const vec4f v02 = v2;
      const vec4f v03 = v3;

      const vec4f v10 = (1.f-f)*v00 + f*(v01);
      const vec4f v11 = (1.f-f)*v01 + f*(v02);
      const vec4f v12 = (1.f-f)*v02 + f*(v03);

      const vec4f v20 = (1.f-f)*v10 + f*(v11);
      const vec4f v21 = (1.f-f)*v11 + f*(v12);

      const vec4f v30 = (1.f-f)*v20 + f*(v21);
      
      l.v0 = v00;
      l.v1 = v10;
      l.v2 = v20;
      l.v3 = v30;

      r.v0 = v30;
      r.v1 = v21;
      r.v2 = v12;
      r.v3 = v03;
    }

    // OSP_REGISTER_RENDERER(RVRenderer,rv);

    extern "C" void ospray_init_module_hair() 
    {
      printf("#ospray: 'hair' module loaded...\n");
    }

  }
}
