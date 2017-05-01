/*
//@HEADER
// ************************************************************************
//
//               KokkosKernels 0.9: Linear Algebra and Graph Kernels
//                 Copyright 2017 Sandia Corporation
//
// Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
// the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// 1. Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright
// notice, this list of conditions and the following disclaimer in the
// documentation and/or other materials provided with the distribution.
//
// 3. Neither the name of the Corporation nor the names of the
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY SANDIA CORPORATION "AS IS" AND ANY
// EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
// PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL SANDIA CORPORATION OR THE
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Questions? Contact Siva Rajamanickam (srajama@sandia.gov)
//
// ************************************************************************
//@HEADER
*/

#ifndef _KOKKOSKERNELS_BITUTILS_HPP
#define _KOKKOSKERNELS_BITUTILS_HPP
#include "Kokkos_Core.hpp"

namespace KokkosKernels{

  namespace Experimental{

    namespace Util{


KOKKOS_INLINE_FUNCTION
int set_bit_count(unsigned int x){
  return __builtin_popcount(x);
}

KOKKOS_INLINE_FUNCTION
int set_bit_count(unsigned long x){
  return __builtin_popcountl(x);
}

KOKKOS_INLINE_FUNCTION
int set_bit_count(unsigned long long x){
  return __builtin_popcountl(x);
}

KOKKOS_INLINE_FUNCTION
int set_bit_count( int x){
  return __builtin_popcount(x);
}

KOKKOS_INLINE_FUNCTION
int set_bit_count( long x){
  return __builtin_popcountl(x);
}

KOKKOS_INLINE_FUNCTION
int set_bit_count( long long x){
  return __builtin_popcountl(x);
}



KOKKOS_INLINE_FUNCTION
int least_set_bit(unsigned int x){
  return __builtin_ffs (x);
}

KOKKOS_INLINE_FUNCTION
int least_set_bit(unsigned long x){
  return __builtin_ffsl(x);
}

KOKKOS_INLINE_FUNCTION
int least_set_bit(unsigned long long x){
  return __builtin_ffsll(x);
}

KOKKOS_INLINE_FUNCTION
int least_set_bit( int x){
  return __builtin_ffs(x);
}

KOKKOS_INLINE_FUNCTION
int least_set_bit( long x){
  return __builtin_ffsl(x);
}

KOKKOS_INLINE_FUNCTION
int least_set_bit( long long x){
  return __builtin_popcountl(x);
}


    }
  }
}
#endif
