#pragma once

#include "common/ospcommon.h"

namespace ospray {
  namespace hair {
    using std::cout;
    using std::endl;

    extern int maxHairsToParse;

    struct Vertex {
      vec3f pos;
      float radius;
      
      inline operator vec4f() const { return (vec4f&)pos; }
      Vertex() {};
      Vertex(const vec4f &a) { (vec4f&)pos = a; }
      inline Vertex &operator=(const vec4f &a) { (vec4f&)pos = a; return *this; }
    };
    
    /*! a *segment* is one bezier segment (four control points) of one hair curve */
    struct Segment {
      Vertex v0, v1, v2, v3;

      void subdivide(Segment &l, Segment &r, const float f = 0.5f) const;
      box4f bounds() const { return bounds_outer(); };
      box3f bounds3f() const { return bounds3f_outer(); };
      box4f bounds_inner() const;
      box3f bounds3f_inner() const;
      box4f bounds_outer() const;
      box3f bounds3f_outer() const;
    };

    //! tests for overlap, return true if overlaps, false if not
    bool overlaps_outer(const Segment &seg, const box3f &bounds, int depth = 0);
    /*! compute overlap between given segment and given spatial region
      'bounds', and return (conservative) intersection area in
      'overlap'. if no overlap occurs, returns 0 and leaves 'overlap'
      unchanged; else returns value > 0 and sets overlap to area
      consesrvatively bounding the overlap region. upon calling
      overlap MUST be set to empty! */
    int computeOverlap_outer(const Segment &seg, 
                       const box4f &bounds, 
                       box4f &overlap,
                             int depth=0);
    inline int computeOverlap(const Segment &seg, 
                              const box4f &bounds, 
                              box4f &overlap,
                              int depth=0)
    { return computeOverlap_outer(seg,bounds,overlap,depth); }

    int computeOverlap(const Segment &seg, 
                       const box3f &bounds, 
                       box3f &overlap,
                       int depth=0);

    //! tests for overlap, return true if overlaps, false if not
    inline bool overlaps(const Segment &seg, const box3f &bounds, int depth = 0)
    { return overlaps_outer(seg,bounds,depth); }

    //! tests for overlap, return true if overlaps, false if not
    bool overlaps_inner(const Segment &seg, const box3f &bounds, int depth = 0);
    /*! compute overlap between given segment and given spatial region
      'bounds', and return (conservative) intersection area in
      'overlap'. if no overlap occurs, returns 0 and leaves 'overlap'
      unchanged; else returns value > 0 and sets overlap to area
      consesrvatively bounding the overlap region. upon calling
      overlap MUST be set to empty! */
    int computeOverlap_inner(const Segment &seg, 
                       const box4f &bounds, 
                       box4f &overlap,
                       int depth=0);

    /*! a *fragment* of a hair is a cylindrical piece of a Segment
      after that segment has been approximated through N such
      fragment. each fragment is essentially a cylinder or
      truncated cone. most often we will refer to fragments only
      implicitly (i.e., as "the i'th fragment of the j'th hair of
      the 'k' th curve) */
    struct Fragment {
      Vertex v0,v1;
    };

    struct HairGeom {
      /*! list of *all* vertices of all curves */
      std::vector<Vertex> vertex;
      
      /*! a curve is essentially a complete hair consisting of multiple segments */
      struct Curve {
        uint32 firstVertex; /*! the first vertex in the vertex list */
        uint32 numSegments;
      };
      std::vector<Curve> curve;
      
      box4f  bounds;

      //! return num segments in scene, total
      int numSegments() const;
      //! return num segments of specified curve
      int numSegments(int cID) const;

      /*! transform all vertices such that bouds are [(0):(1)] */
      void transformToOrigin();

      //! return segment starting at given vertex (ie, NOT curve ID, but START VERTEX!)
      Segment getSegment(long startVertex) const;
      //! return given segment of given curve. careful: those are ID's, not start vertices
      Segment getSegment(int curveID, int segID) const;

      void convertOOC(const std::string &inFileName, const std::string &outFileName);
      void importTXT(const std::string &fileName);
      void load(const std::string &fileName);
      void save(const std::string &fileName);
    };
  }
}
