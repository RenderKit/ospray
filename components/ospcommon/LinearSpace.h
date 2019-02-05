// ======================================================================== //
// Copyright 2009-2019 Intel Corporation                                    //
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

#include "vec.h"
#include "Quaternion.h"

namespace ospcommon {

  ////////////////////////////////////////////////////////////////////////////////
  /// 2D Linear Transform (2x2 Matrix)
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> struct LinearSpace2
  {
    using Vector = T;
    using Scalar = typename T::scalar_t;
    
    /*! default matrix constructor */
    inline LinearSpace2           ( ) = default;
    inline LinearSpace2           ( const LinearSpace2& other ) { vx = other.vx; vy = other.vy; }
    inline LinearSpace2& operator=( const LinearSpace2& other ) { vx = other.vx; vy = other.vy; return *this; }

    template<typename L1> inline LinearSpace2( const LinearSpace2<L1>& s ) : vx(s.vx), vy(s.vy) {}

    /*! matrix construction from column vectors */
    inline LinearSpace2(const Vector& vx, const Vector& vy)
      : vx(vx), vy(vy) {}

    /*! matrix construction from row mayor data */
    inline LinearSpace2(const Scalar& m00, const Scalar& m01, 
                               const Scalar& m10, const Scalar& m11)
      : vx(m00,m10), vy(m01,m11) {}

    /*! compute the determinant of the matrix */
    inline const Scalar det() const { return vx.x*vy.y - vx.y*vy.x; }

    /*! compute adjoint matrix */
    inline const LinearSpace2 adjoint() const { return LinearSpace2(vy.y,-vy.x,-vx.y,vx.x); }

    /*! compute inverse matrix */
    inline const LinearSpace2 inverse() const { return adjoint()/det(); }

    /*! compute transposed matrix */
    inline const LinearSpace2 transposed() const { return LinearSpace2(vx.x,vx.y,vy.x,vy.y); }

    /*! returns first row of matrix */
    inline const Vector row0() const { return Vector(vx.x,vy.x); }

    /*! returns second row of matrix */
    inline const Vector row1() const { return Vector(vx.y,vy.y); }

    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////

    inline LinearSpace2( ZeroTy ) : vx(zero), vy(zero) {}
    inline LinearSpace2( OneTy ) : vx(one, zero), vy(zero, one) {}

    /*! return matrix for scaling */
    static inline LinearSpace2 scale(const Vector& s) {
      return LinearSpace2(s.x,   0,
                          0  , s.y);
    }

    /*! return matrix for rotation */
    static inline LinearSpace2 rotate(const Scalar& r) {
      Scalar s = sin(r), c = cos(r);
      return LinearSpace2(c, -s,
                          s,  c);
    }

    /*! return closest orthogonal matrix (i.e. a general rotation including reflection) */
    LinearSpace2 orthogonal() const {
      LinearSpace2 m = *this;

      // mirrored?
      Scalar mirror(one);
      if (m.det() < Scalar(zero)) {
        m.vx = -m.vx;
        mirror = -mirror;
      }

      // rotation
      for (int i = 0; i < 99; i++) {
        const LinearSpace2 m_next = 0.5 * (m + m.transposed().inverse());
        const LinearSpace2 d = m_next - m;
        m = m_next;
        // norm^2 of difference small enough?
        if (max(dot(d.vx, d.vx), dot(d.vy, d.vy)) < 1e-8)
          break;
      }

      // rotation * mirror_x
      return LinearSpace2(mirror*m.vx, m.vy);
    }

  public:

    /*! the column vectors of the matrix */
    Vector vx,vy;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Unary Operators
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> inline LinearSpace2<T> operator -( const LinearSpace2<T>& a ) { return LinearSpace2<T>(-a.vx,-a.vy); }
  template<typename T> inline LinearSpace2<T> operator +( const LinearSpace2<T>& a ) { return LinearSpace2<T>(+a.vx,+a.vy); }
  template<typename T> inline LinearSpace2<T> rcp       ( const LinearSpace2<T>& a ) { return a.inverse(); }

  ////////////////////////////////////////////////////////////////////////////////
  // Binary Operators
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> inline LinearSpace2<T> operator +( const LinearSpace2<T>& a, const LinearSpace2<T>& b ) { return LinearSpace2<T>(a.vx+b.vx,a.vy+b.vy); }
  template<typename T> inline LinearSpace2<T> operator -( const LinearSpace2<T>& a, const LinearSpace2<T>& b ) { return LinearSpace2<T>(a.vx-b.vx,a.vy-b.vy); }

  template<typename T> inline LinearSpace2<T> operator*(const typename T::Scalar & a, const LinearSpace2<T>& b) { return LinearSpace2<T>(a*b.vx, a*b.vy); }
  template<typename T> inline T               operator*(const LinearSpace2<T>& a, const T              & b) { return b.x*a.vx + b.y*a.vy; }
  template<typename T> inline LinearSpace2<T> operator*(const LinearSpace2<T>& a, const LinearSpace2<T>& b) { return LinearSpace2<T>(a*b.vx, a*b.vy); }

  template<typename T> inline LinearSpace2<T> operator/(const LinearSpace2<T>& a, const typename T::Scalar & b) { return LinearSpace2<T>(a.vx/b, a.vy/b); }
  template<typename T> inline LinearSpace2<T> operator/(const LinearSpace2<T>& a, const LinearSpace2<T>& b) { return a * rcp(b); }

  template<typename T> inline LinearSpace2<T>& operator *=( LinearSpace2<T>& a, const LinearSpace2<T>& b ) { return a = a * b; }
  template<typename T> inline LinearSpace2<T>& operator /=( LinearSpace2<T>& a, const LinearSpace2<T>& b ) { return a = a / b; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Comparison Operators
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> inline bool operator ==( const LinearSpace2<T>& a, const LinearSpace2<T>& b ) { return a.vx == b.vx && a.vy == b.vy; }
  template<typename T> inline bool operator !=( const LinearSpace2<T>& a, const LinearSpace2<T>& b ) { return a.vx != b.vx || a.vy != b.vy; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Output Operators
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> static std::ostream& operator<<(std::ostream& cout, const LinearSpace2<T>& m) {
    return cout << "{ vx = " << m.vx << ", vy = " << m.vy << "}";
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// 3D Linear Transform (3x3 Matrix)
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> struct LinearSpace3
  {
    using Vector = T;
    using Scalar = typename T::scalar_t;

    /*! default matrix constructor */
    inline LinearSpace3           ( ) = default;
    inline LinearSpace3           ( const LinearSpace3& other ) { vx = other.vx; vy = other.vy; vz = other.vz; }
    inline LinearSpace3& operator=( const LinearSpace3& other ) { vx = other.vx; vy = other.vy; vz = other.vz; return *this; }

    template<typename L1> inline LinearSpace3( const LinearSpace3<L1>& s ) : vx(s.vx), vy(s.vy), vz(s.vz) {}

    /*! matrix construction from column vectors */
    inline LinearSpace3(const Vector& vx, const Vector& vy, const Vector& vz)
      : vx(vx), vy(vy), vz(vz) {}

    /*! construction from quaternion */
    inline LinearSpace3( const QuaternionT<Scalar>& q )
      : vx((q.r*q.r + q.i*q.i - q.j*q.j - q.k*q.k), 2.0f*(q.i*q.j + q.r*q.k), 2.0f*(q.i*q.k - q.r*q.j))
      , vy(2.0f*(q.i*q.j - q.r*q.k), (q.r*q.r - q.i*q.i + q.j*q.j - q.k*q.k), 2.0f*(q.j*q.k + q.r*q.i))
      , vz(2.0f*(q.i*q.k + q.r*q.j), 2.0f*(q.j*q.k - q.r*q.i), (q.r*q.r - q.i*q.i - q.j*q.j + q.k*q.k)) {}

    /*! matrix construction from row mayor data */
    inline LinearSpace3(const Scalar& m00, const Scalar& m01, const Scalar& m02,
                               const Scalar& m10, const Scalar& m11, const Scalar& m12,
                               const Scalar& m20, const Scalar& m21, const Scalar& m22)
      : vx(m00,m10,m20), vy(m01,m11,m21), vz(m02,m12,m22) {}

    /*! compute the determinant of the matrix */
    inline const Scalar det() const { return dot(vx,cross(vy,vz)); }

    /*! compute adjoint matrix */
    inline const LinearSpace3 adjoint() const { return LinearSpace3(cross(vy,vz),cross(vz,vx),cross(vx,vy)).transposed(); }

    /*! compute inverse matrix */
    inline const LinearSpace3 inverse() const { return adjoint()/det(); }

    /*! compute transposed matrix */
    inline const LinearSpace3 transposed() const { return LinearSpace3(vx.x,vx.y,vx.z,vy.x,vy.y,vy.z,vz.x,vz.y,vz.z); }

    /*! returns first row of matrix */
    inline const Vector row0() const { return Vector(vx.x,vy.x,vz.x); }

    /*! returns second row of matrix */
    inline const Vector row1() const { return Vector(vx.y,vy.y,vz.y); }

    /*! returns third row of matrix */
    inline const Vector row2() const { return Vector(vx.z,vy.z,vz.z); }

    ////////////////////////////////////////////////////////////////////////////////
    /// Constants
    ////////////////////////////////////////////////////////////////////////////////

    inline LinearSpace3( ZeroTy ) : vx(zero), vy(zero), vz(zero) {}
    inline LinearSpace3( OneTy ) : vx(one, zero, zero), vy(zero, one, zero), vz(zero, zero, one) {}

    /*! return matrix for scaling */
    static inline LinearSpace3 scale(const Vector& s) {
      return LinearSpace3(s.x,   0,   0,
                          0  , s.y,   0,
                          0  ,   0, s.z);
    }

    /*! return matrix for rotation around arbitrary axis */
    static inline LinearSpace3 rotate(const Vector& _u, const Scalar& r) {
      Vector u = normalize(_u);
      Scalar s = sin(r), c = cos(r);
      return LinearSpace3(u.x*u.x+(1-u.x*u.x)*c,  u.x*u.y*(1-c)-u.z*s,    u.x*u.z*(1-c)+u.y*s,
                          u.x*u.y*(1-c)+u.z*s,    u.y*u.y+(1-u.y*u.y)*c,  u.y*u.z*(1-c)-u.x*s,
                          u.x*u.z*(1-c)-u.y*s,    u.y*u.z*(1-c)+u.x*s,    u.z*u.z+(1-u.z*u.z)*c);
    }

  public:

    /*! the column vectors of the matrix */
    Vector vx,vy,vz;
  };

  ////////////////////////////////////////////////////////////////////////////////
  // Unary Operators
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> inline LinearSpace3<T> operator -( const LinearSpace3<T>& a ) { return LinearSpace3<T>(-a.vx,-a.vy,-a.vz); }
  template<typename T> inline LinearSpace3<T> operator +( const LinearSpace3<T>& a ) { return LinearSpace3<T>(+a.vx,+a.vy,+a.vz); }
  template<typename T> inline LinearSpace3<T> rcp       ( const LinearSpace3<T>& a ) { return a.inverse(); }

  /* constructs a coordinate frame form a normalized normal */
  template<typename T> inline LinearSpace3<T> frame(const T& N) 
  {
    const T dx0 = cross(T(one,zero,zero),N);
    const T dx1 = cross(T(zero,one,zero),N);
    const T dx = normalize(select(dot(dx0,dx0) > dot(dx1,dx1),dx0,dx1));
    const T dy = normalize(cross(N,dx));
    return LinearSpace3<T>(dx,dy,N);
  }

  /* constructs a coordinate frame from a normal and approximate x-direction */
  template<typename T> inline LinearSpace3<T> frame(const T& N, const T& dxi)
  {
    if (abs(dot(dxi,N)) > 0.99f) return frame(N); // fallback in case N and dxi are very parallel
    const T dx = normalize(cross(dxi,N));
    const T dy = normalize(cross(N,dx));
    return LinearSpace3<T>(dx,dy,N);
  }
  
  /* clamps linear space to range -1 to +1 */
  template<typename T> inline LinearSpace3<T> clamp(const LinearSpace3<T>& space) {
    return LinearSpace3<T>(clamp(space.vx,T(-1.0f),T(1.0f)),
                           clamp(space.vy,T(-1.0f),T(1.0f)),
                           clamp(space.vz,T(-1.0f),T(1.0f)));
  }

  ////////////////////////////////////////////////////////////////////////////////
  // Binary Operators
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> inline LinearSpace3<T> operator +( const LinearSpace3<T>& a, const LinearSpace3<T>& b ) { return LinearSpace3<T>(a.vx+b.vx,a.vy+b.vy,a.vz+b.vz); }
  template<typename T> inline LinearSpace3<T> operator -( const LinearSpace3<T>& a, const LinearSpace3<T>& b ) { return LinearSpace3<T>(a.vx-b.vx,a.vy-b.vy,a.vz-b.vz); }

  template<typename T> inline LinearSpace3<T> operator*(const typename T::Scalar & a, const LinearSpace3<T>& b) { return LinearSpace3<T>(a*b.vx, a*b.vy, a*b.vz); }
  template<typename T> inline T               operator*(const LinearSpace3<T>& a, const T              & b) { return b.x*a.vx + b.y*a.vy + b.z*a.vz; }
  template<typename T> inline LinearSpace3<T> operator*(const LinearSpace3<T>& a, const LinearSpace3<T>& b) { return LinearSpace3<T>(a*b.vx, a*b.vy, a*b.vz); }

  template<typename T> inline LinearSpace3<T> operator/(const LinearSpace3<T>& a, const typename T::Scalar & b) { return LinearSpace3<T>(a.vx/b, a.vy/b, a.vz/b); }
  template<typename T> inline LinearSpace3<T> operator/(const LinearSpace3<T>& a, const LinearSpace3<T>& b) { return a * rcp(b); }

  template<typename T> inline LinearSpace3<T>& operator *=( LinearSpace3<T>& a, const LinearSpace3<T>& b ) { return a = a * b; }
  template<typename T> inline LinearSpace3<T>& operator /=( LinearSpace3<T>& a, const LinearSpace3<T>& b ) { return a = a / b; }

  template<typename T> inline T xfmPoint (const LinearSpace3<T>& s, const T& a) { return madd(T(a.x),s.vx,madd(T(a.y),s.vy,T(a.z*s.vz))); }
  template<typename T> inline T xfmVector(const LinearSpace3<T>& s, const T& a) { return madd(T(a.x),s.vx,madd(T(a.y),s.vy,T(a.z*s.vz))); }
  template<typename T> inline T xfmNormal(const LinearSpace3<T>& s, const T& a) { return xfmVector(s.inverse().transposed(),a); }

  ////////////////////////////////////////////////////////////////////////////////
  /// Comparison Operators
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> inline bool operator ==( const LinearSpace3<T>& a, const LinearSpace3<T>& b ) { return a.vx == b.vx && a.vy == b.vy && a.vz == b.vz; }
  template<typename T> inline bool operator !=( const LinearSpace3<T>& a, const LinearSpace3<T>& b ) { return a.vx != b.vx || a.vy != b.vy || a.vz != b.vz; }

  ////////////////////////////////////////////////////////////////////////////////
  /// Output Operators
  ////////////////////////////////////////////////////////////////////////////////

  template<typename T> inline std::ostream& operator<<(std::ostream& cout, const LinearSpace3<T>& m) {
    return cout << "{ vx = " << m.vx << ", vy = " << m.vy << ", vz = " << m.vz << "}";
  }

  /*! Shortcuts for common linear spaces. */
  using LinearSpace2f  = LinearSpace2<vec2f> ;
  using LinearSpace3f  = LinearSpace3<vec3f> ;
  using LinearSpace3fa = LinearSpace3<vec3fa>;

  using linear2f = LinearSpace2f;
  using linear3f = LinearSpace3f;
} // ::ospcommon
