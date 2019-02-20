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

#include "../platform.h"
#include <atomic> // c++11 ...

/*! iw: TODO: move to c++11-style refcoutned object instead of this
    homebrewed one */
namespace ospcommon {
  namespace memory {
#ifdef __X86_64__
  typedef long long atomic_init_t;
#else
  typedef int atomic_init_t;
#endif
  typedef std::atomic<atomic_init_t> atomic_t;

  static struct NullTy {
  } null MAYBE_UNUSED;

  class RefCount
  {
  public:
    inline RefCount(atomic_init_t val = 1) : refCounter(val) {}
    virtual ~RefCount() {}

    /*! dummy copy-constructor and assignment operator because if they
        do not exist icc throws some error about "deleted function"
        when auto-constructing those. they should NEVER get called,
        though */
    inline RefCount(const RefCount &)
    {
      throw std::runtime_error("should not copy-construct refence-counted "
                               "objects!");
    }

    /*! dummy copy-constructor and assignment operator because if they
        do not exist icc throws some error about "deleted function"
        when auto-constructing those. they should NEVER get called,
        though */
    inline RefCount &operator=(const RefCount &)
    {
      throw std::runtime_error("should not copy-construct refence-counted "
                               "objects!");
      return *this;
    }

    virtual void refInc() { refCounter++; }
    virtual void refDec() { if ((--refCounter) == 0) delete this; }

  private:
    atomic_t refCounter;
  };

  /////////////////////////////////////////////////////////////////////////////
  /// Reference to single object
  /////////////////////////////////////////////////////////////////////////////

  template<typename Type>
  class Ref
  {
  public:
    Type* ptr {nullptr};

    __forceinline Ref( void ) : ptr(nullptr) {}
    __forceinline Ref(NullTy) : ptr(nullptr) {}
    __forceinline Ref( const Ref& input ) : ptr(input.ptr)
    { if ( ptr ) ptr->refInc(); }

    __forceinline Ref( Type* const input ) : ptr(input) {
      if ( ptr )
        ptr->refInc();
    }

    __forceinline ~Ref( void ) {
      if (ptr) ptr->refDec();
    }

    __forceinline Ref& operator= ( const Ref& input )
    {
      if ( input.ptr ) input.ptr->refInc();
      if (ptr) ptr->refDec();
      ptr = input.ptr;
      return *this;
    }

    __forceinline Ref& operator= ( Type* input )
    {
      if ( input ) input->refInc();
      if (ptr) ptr->refDec();
      ptr = input;
      return *this;
    }

    __forceinline Ref& operator= ( NullTy ) {
      if (ptr) ptr->refDec();
      ptr = nullptr;
      return *this;
    }

    __forceinline operator bool( void ) const { return ptr != nullptr; }

    __forceinline const Type& operator  *( void ) const { return *ptr; }
    __forceinline       Type& operator  *( void )       { return *ptr; }
    __forceinline const Type* operator ->( void ) const { return  ptr; }
    __forceinline       Type* operator ->( void )       { return  ptr; }

    template<typename TypeOut>
    __forceinline       Ref<TypeOut> cast()
    { return Ref<TypeOut>(static_cast<TypeOut*>(ptr)); }
    template<typename TypeOut>
    __forceinline const Ref<TypeOut> cast() const
    { return Ref<TypeOut>(static_cast<TypeOut*>(ptr)); }

    template<typename TypeOut>
    __forceinline       Ref<TypeOut> dynamicCast()
    { return Ref<TypeOut>(dynamic_cast<TypeOut*>(ptr)); }
    template<typename TypeOut>
    __forceinline const Ref<TypeOut> dynamicCast() const
    { return Ref<TypeOut>(dynamic_cast<TypeOut*>(ptr)); }
  };

  template<typename Type>
  __forceinline bool operator<(const Ref<Type>& a, const Ref<Type>& b)
  {
    return a.ptr <  b.ptr ;
  }

  template<typename Type>
  __forceinline  bool operator==(const Ref<Type>& a, NullTy)
  {
    return a.ptr == nullptr;
  }

  template<typename Type>
  __forceinline  bool operator==(NullTy, const Ref<Type>& b)
  {
    return nullptr  == b.ptr;
  }

  template<typename Type>
  __forceinline  bool operator==(const Ref<Type>& a, const Ref<Type>& b)
  {
    return a.ptr == b.ptr;
  }

  template<typename Type>
  __forceinline  bool operator!=(const Ref<Type>& a, NullTy)
  {
    return a.ptr != nullptr;
  }

  template<typename Type>
  __forceinline  bool operator!=(NullTy, const Ref<Type>& b)
  {
    return nullptr  != b.ptr;
  }

  template<typename Type>
  __forceinline  bool operator!=(const Ref<Type>& a, const Ref<Type>& b)
  {
    return a.ptr != b.ptr;
  }

  } // ::ospcommon::memory
} // ::ospcommon
