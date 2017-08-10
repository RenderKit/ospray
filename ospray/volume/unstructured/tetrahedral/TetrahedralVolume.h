// ======================================================================== //
// Copyright 2009-2017 Intel Corporation                                    //
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

#pragma once

// ospray
#include "../../Volume.h"

#include "../../../common/OSPCommon.h"

// Geometry intersection acceleration structure (BVH).
#include <embree2/rtcore.h>
#include <embree2/rtcore_ray.h>

#include "../MinMaxBVH2.h"

void embree_error_handler( const RTCError code, const char* str ) {
    printf( "Embree: " );
    switch( code ) {
    case RTC_UNKNOWN_ERROR: printf( "RTC_UNKNOWN_ERROR" ); break;
    case RTC_INVALID_ARGUMENT: printf( "RTC_INVALID_ARGUMENT" ); break;
    case RTC_INVALID_OPERATION: printf( "RTC_INVALID_OPERATION" ); break;
    case RTC_OUT_OF_MEMORY: printf( "RTC_OUT_OF_MEMORY" ); break;
    case RTC_UNSUPPORTED_CPU: printf( "RTC_UNSUPPORTED_CPU" ); break;
    case RTC_CANCELLED: printf( "RTC_CANCELLED" ); break;
    default: printf( "invalid error code" ); break;
    }
    if( str ) {
        printf( " (" );
        while( *str ) putchar( *str++ );
        printf( ")\n" );
    }
    //exit( 1 );
}

namespace ospray
{
class TetrahedralVolume : public Volume
{
public:

    class Point {
    public:
        float x,y,z;

        Point () :
            x(0), y(0), z(0)
        {}

        Point (float _x, float _y, float _z) :
            x(_x), y(_y), z(_z)
        {}

        float & operator[] (int index)
        {
            if (index == 0) {
                return x;
            }

            if (index == 1) {
                return y;
            }

            if (index == 2) {
                return z;
            }

            throw std::runtime_error("Point operator[] index out of range.");
        }
    };

    static void Cross (const Point & A, const Point & B, Point & C)
    {
        // A cross B
        C.x = A.y * B.z - A.z * B.y;
        C.y = A.z * B.x - A.x * B.z;
        C.z = A.x * B.y - A.y * B.x;
    }

    static void Sub (const Point & A, const Point & B, Point & C)
    {
        // A - B
        C.x = A.x - B.x;
        C.y = A.y - B.y;
        C.z = A.z - B.z;
    }

    static float Dot (const Point & A, const Point & B)
    {
        return (A.x*B.x + A.y*B.y + A.z*B.z);
    }

    static void Normalize (Point & A)
    {
        float length = sqrtf(Dot(A,A));
        A.x /= length;
        A.y /= length;
        A.z /= length;
    }

    class Int4 {
    public:
        int i,j,k,w;

        Int4 () :
            i(0), j(0), k(0), w(0)
        {}

        Int4 (int _i, int _j, int _k, int _w) :
            i(_i), j(_j), k(_k), w(_w)
        {}

        int & operator[] (int index)
        {
            if (index == 0) {
                return i;
            }

            if (index == 1) {
                return j;
            }

            if (index == 2) {
                return k;
            }

            if (index == 3) {
                return w;
            }

            throw std::runtime_error("Int4 operator[] index out of range.");
        }
    };

    class BBox {
    public:
        float min_x, min_y, min_z, max_x, max_y, max_z;

        BBox () :
            min_x(0), min_y(0), min_z(0), max_x(0), max_y(0), max_z(0)
        {}

        BBox (float _min_x, float _min_y, float _min_z, float _max_x, float _max_y, float _max_z) :
            min_x(_min_x),
            min_y(_min_y),
            min_z(_min_z),
            max_x(_max_x),
            max_y(_max_y),
            max_z(_max_z)
        {}

        bool Contains(const Point & p) {
            if (p.x < min_x) return false;
            if (p.x > max_x) return false;

            if (p.y < min_y) return false;
            if (p.y > max_y) return false;

            if (p.z < min_z) return false;
            if (p.z > max_z) return false;

            return true;
        }
    };

    class TetFaceNormal {
    public:
        Point normals[4];
        Point corners[4];
    };

    std::vector<BBox> tetBBoxes;

    int nVertices;
    std::vector<Point> vertices;

    int nTetrahedra;
    std::vector<Int4> tetrahedra;

    std::vector<float> field;// Attribute value at each vertex.

    box3f bbox;

    std::vector<TetFaceNormal> faceNormals;

    // BVH
    RTCDevice embree_device;
    RTCScene embree_scene;
    int geomID;

    MinMaxBVH bvh;

    bool finished;

    TetrahedralVolume() :
        finished(false)
    {
        embree_device = rtcNewDevice( NULL );
        rtcDeviceSetErrorFunction( embree_device, embree_error_handler );
        //embree_scene = rtcDeviceNewScene(embree_device, RTC_SCENE_STATIC | RTC_SCENE_INCOHERENT | RTC_SCENE_HIGH_QUALITY | RTC_SCENE_ROBUST, RTC_INTERSECT1);

        embree_scene = rtcDeviceNewScene( embree_device, RTC_SCENE_STATIC | RTC_SCENE_INCOHERENT, RTC_INTERSECT1 );
    }

    void addTetBBox (int id) {
        Int4 & t = tetrahedra[id];
        BBox & tet_bbox = tetBBoxes[id];
        float & x0 = tet_bbox.min_x,
                & y0 = tet_bbox.min_y,
                & z0 = tet_bbox.min_z,
                & x1 = tet_bbox.max_x,
                & y1 = tet_bbox.max_y,
                & z1 = tet_bbox.max_z;
        for (int i = 0; i < 4; i++) {
            const Point & p = vertices[t[i]];
            if (i == 0) {
                x0 = x1 = p.x;
                y0 = y1 = p.y;
                z0 = z1 = p.z;
            } else {
                if(p.x < x0) x0 = p.x;
                if(p.x > x1) x1 = p.x;

                if(p.y < y0) y0 = p.y;
                if(p.y > y1) y1 = p.y;

                if(p.z < z0) z0 = p.z;
                if(p.z > z1) z1 = p.z;
            }
        }
    }

    void addTetrahedralMeshToScene(RTCScene scene);

    virtual ~TetrahedralVolume() = default;

    //! A string description of this class.
    virtual std::string toString() const override;

    //! Allocate storage and populate the volume, called through the OSPRay API.
    virtual void commit() override;

    //! Copy voxels into the volume at the given index
    /*! \returns 0 on error, any non-zero value indicates success */
    virtual int setRegion(const void *source_pointer,
                          const vec3i &target_index,
                          const vec3i &source_count) override;

    //! Compute samples at the given world coordinates.
    virtual void computeSamples(float **results,
                                const vec3f *worldCoordinates,
                                const size_t &count) override;

    float sample(float world_x, float world_y, float world_z);

    void getTetBBox(size_t id, box4f & bbox);

protected:

    //! Create the equivalent ISPC volume container.
    virtual void createEquivalentISPC();

    //! Complete volume initialization (only on first commit).
    virtual void finish() override;
};

} // ::ospray

extern "C" float callSample(ospray::TetrahedralVolume *volume, float world_x, float world_y, float world_z) {
    return volume->sample(world_x,world_y,world_z);
}

void tetBoundsFunc(void *userData, size_t id, RTCBounds& bounds_o);
void tetIntersectFunc(void *userData, RTCRay & ray, size_t id);
void tetOccludedFunc(void *userData, RTCRay & ray, size_t id);

void tetBoundsFunc3(void *userData, void* geomUserPtr, size_t id, size_t timeStep, RTCBounds& bounds_o);


