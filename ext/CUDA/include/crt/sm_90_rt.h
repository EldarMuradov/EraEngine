/*
 * Copyright 2022-2023 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to NVIDIA intellectual property rights under U.S. and
 * international Copyright laws.
 *
 * These Licensed Deliverables contained herein is PROPRIETARY and
 * CONFIDENTIAL to NVIDIA and is being provided under the terms and
 * conditions of a form of NVIDIA software license agreement by and
 * between NVIDIA and Licensee ("License Agreement") or electronically
 * accepted by Licensee.  Notwithstanding any terms or conditions to
 * the contrary in the License Agreement, reproduction or disclosure
 * of the Licensed Deliverables to any third party without the express
 * written consent of NVIDIA is prohibited.
 *
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, NVIDIA MAKES NO REPRESENTATION ABOUT THE
 * SUITABILITY OF THESE LICENSED DELIVERABLES FOR ANY PURPOSE.  IT IS
 * PROVIDED "AS IS" WITHOUT EXPRESS OR IMPLIED WARRANTY OF ANY KIND.
 * NVIDIA DISCLAIMS ALL WARRANTIES WITH REGARD TO THESE LICENSED
 * DELIVERABLES, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY,
 * NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE.
 * NOTWITHSTANDING ANY TERMS OR CONDITIONS TO THE CONTRARY IN THE
 * LICENSE AGREEMENT, IN NO EVENT SHALL NVIDIA BE LIABLE FOR ANY
 * SPECIAL, INDIRECT, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, OR ANY
 * DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS
 * ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THESE LICENSED DELIVERABLES.
 *
 * U.S. Government End Users.  These Licensed Deliverables are a
 * "commercial item" as that term is defined at 48 C.F.R. 2.101 (OCT
 * 1995), consisting of "commercial computer software" and "commercial
 * computer software documentation" as such terms are used in 48
 * C.F.R. 12.212 (SEPT 1995) and is provided to the U.S. Government
 * only as a commercial end item.  Consistent with 48 C.F.R.12.212 and
 * 48 C.F.R. 227.7202-1 through 227.7202-4 (JUNE 1995), all
 * U.S. Government End Users acquire the Licensed Deliverables with
 * only those rights set forth herein.
 *
 * Any use of the Licensed Deliverables in individual and commercial
 * software must include, in the user documentation and internal
 * comments to the code, the above Disclaimer and U.S. Government End
 * Users Notice.
 */

#if !defined(__CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__)
#if defined(_MSC_VER)
#pragma message("crt/sm_90_rt.h is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead.")
#else
#warning "crt/sm_90_rt.h is an internal header file and must not be used directly.  Please use cuda_runtime_api.h or cuda_runtime.h instead."
#endif
#define __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#define __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_SM_90_RT_H__
#endif

#if !defined(__SM_90_RT_H__)
#define __SM_90_RT_H__

#if defined(__CUDACC_RTC__)
#define __SM_90_RT_DECL__ __host__ __device__
#else /* !__CUDACC_RTC__ */
#define __SM_90_RT_DECL__ static __device__ __inline__
#endif /* __CUDACC_RTC__ */

#if defined(__cplusplus) && defined(__CUDACC__)

#if !defined(__CUDA_ARCH__) || __CUDA_ARCH__ >= 900

/*******************************************************************************
*                                                                              *
*                                                                              *
*                                                                              *
*******************************************************************************/

#include "builtin_types.h"
#include "device_types.h"
#include "host_defines.h"

#ifndef __CUDA_ARCH__
#define __DEF_IF_HOST { }
#else  /* !__CUDA_ARCH__ */
#define __DEF_IF_HOST ;
#endif /* __CUDA_ARCH__ */

//NOTE: For NVRTC, these declarations have been moved into the compiler (to reduce compile time)
#define EXCLUDE_FROM_RTC

__SM_90_RT_DECL__ unsigned __isCtaShared(const void *ptr) __DEF_IF_HOST
__SM_90_RT_DECL__ unsigned __isClusterShared(const void *ptr) __DEF_IF_HOST
__SM_90_RT_DECL__ void *__cluster_map_shared_rank(const void *ptr, unsigned target_block_rank)  __DEF_IF_HOST
__SM_90_RT_DECL__ unsigned __cluster_query_shared_rank(const void *ptr) __DEF_IF_HOST
__SM_90_RT_DECL__ uint2 __cluster_map_shared_multicast(const void *ptr, unsigned cluster_cta_mask) __DEF_IF_HOST
__SM_90_RT_DECL__ unsigned __clusterDimIsSpecified() __DEF_IF_HOST
__SM_90_RT_DECL__ dim3 __clusterDim() __DEF_IF_HOST
__SM_90_RT_DECL__ dim3 __clusterRelativeBlockIdx() __DEF_IF_HOST
__SM_90_RT_DECL__ dim3 __clusterGridDimInClusters() __DEF_IF_HOST
__SM_90_RT_DECL__ dim3 __clusterIdx() __DEF_IF_HOST
__SM_90_RT_DECL__ unsigned __clusterRelativeBlockRank() __DEF_IF_HOST
__SM_90_RT_DECL__ unsigned __clusterSizeInBlocks() __DEF_IF_HOST
__SM_90_RT_DECL__ void __cluster_barrier_arrive() __DEF_IF_HOST
__SM_90_RT_DECL__ void __cluster_barrier_arrive_relaxed() __DEF_IF_HOST
__SM_90_RT_DECL__ void __cluster_barrier_wait() __DEF_IF_HOST
__SM_90_RT_DECL__ void __threadfence_cluster() __DEF_IF_HOST

__SM_90_RT_DECL__ float2 atomicAdd(float2 *__address, float2 val) __DEF_IF_HOST
__SM_90_RT_DECL__ float2 atomicAdd_block(float2 *__address, float2 val) __DEF_IF_HOST
__SM_90_RT_DECL__ float2 atomicAdd_system(float2 *__address, float2 val) __DEF_IF_HOST
__SM_90_RT_DECL__ float4 atomicAdd(float4 *__address, float4 val) __DEF_IF_HOST
__SM_90_RT_DECL__ float4 atomicAdd_block(float4 *__address, float4 val) __DEF_IF_HOST
__SM_90_RT_DECL__ float4 atomicAdd_system(float4 *__address, float4 val) __DEF_IF_HOST

#undef EXCLUDE_FROM_RTC

//Note: below atomic functions are templates, so cannot be represented in NVRTC
//builtins representation, so they have to be parsed on every NVRTC compilation.
//(notice 'EXCLUDE_FROM_RTC' ends above)


#ifndef __NV_DISABLE_128_ATOMICS
// lgen definitions for 128b atomics
extern "C" {
  __device__ __device_builtin__ void __u128AtomicCAS(void *, void *, void *, void *);
  __device__ __device_builtin__ void __u128AtomicCAS_block(void *, void *, void *, void *);
  __device__ __device_builtin__ void __u128AtomicExch(void *, void *, void *);
  __device__ __device_builtin__ void __u128AtomicExch_block(void *, void *, void *);
}

// macro to get address of object, to workaround situations where the type overloads the "&" operator
#define __NV_ATOMIC_ADDRESSOF(__val) \
        (void *)(&(const_cast<char &>(reinterpret_cast<const volatile char &>(__val))))

// enable_if
template<bool __b, typename _T>
struct __nv_atomic_enable_if { };

template<typename _T>
struct __nv_atomic_enable_if<true, _T> { typedef _T __type; };

// alignof
#if defined(__CUDACC_RTC__)
#define __NV_ATOMIC_ALIGNOF __alignof__
#else
#define __NV_ATOMIC_ALIGNOF __alignof
#endif

// trivially copyable
template <typename _T>
struct __nv_atomic_triv_cp_helper {
#if defined(__GNUC__)
#if  (__GNUC__ < 4) || (__GNUC__ == 4 && __GNUC_MINOR__ < 3)
  static const bool __val = true;
#elif (__GNUC__ < 5)
  static const bool __val = __has_trivial_copy(_T);
#else
  static const bool __val = __is_trivially_copyable(_T);
#endif
#else
  static const bool __val = __is_trivially_copyable(_T);
#endif
};
#define __NV_ATOMIC_TRIVIALLY_COPYABLE(_T) \
        __nv_atomic_triv_cp_helper<_T>::__val

// return type
#if __cplusplus >= 202002L // C++20 or greater
#define __NV_ATOMIC_RET_TYPE(_T) _T
#else
#define __NV_ATOMIC_RET_TYPE(_T) typename \
  __nv_atomic_enable_if<sizeof(_T) == 16 && \
  __NV_ATOMIC_ALIGNOF(_T) >= 16 && \
  __NV_ATOMIC_TRIVIALLY_COPYABLE(_T), _T>::__type
#endif

// requires
#if __cplusplus >= 202002L // C++20 or greater
#define __NV_ATOMIC_REQUIRES(_T) \
  requires(sizeof(_T) == 16 && \
  __NV_ATOMIC_ALIGNOF(_T) >= 16 && \
  __NV_ATOMIC_TRIVIALLY_COPYABLE(_T))
#else
#define __NV_ATOMIC_REQUIRES(_T)
#endif

// temp value and return value
#if __cplusplus >= 201103L || defined(_MSC_VER) // C++11 or greater, or MSC
#define __NV_ATOMIC_TEMP(_T) union _U \
  {_T __ret; __device__ __inline__ _U() {}}; _U __u
#define __NV_ATOMIC_RET(_T) __u.__ret
#else
#define __NV_ATOMIC_TEMP(_T) _T __ret
#define __NV_ATOMIC_RET(_T) __ret
#endif

// templated 128-bit atomics
template <typename _T>
__SM_90_RT_DECL__ __NV_ATOMIC_RET_TYPE(_T)
atomicCAS(_T *__address, _T __compare, _T __val) __NV_ATOMIC_REQUIRES(_T) {
  __NV_ATOMIC_TEMP(_T);
  __u128AtomicCAS((void *)(__address),
                  __NV_ATOMIC_ADDRESSOF(__compare),
                  __NV_ATOMIC_ADDRESSOF(__val),
                  __NV_ATOMIC_ADDRESSOF(__NV_ATOMIC_RET(_T)));
  return __NV_ATOMIC_RET(_T);
}

template <typename _T>
__SM_90_RT_DECL__ __NV_ATOMIC_RET_TYPE(_T)
atomicCAS_block(_T *__address, _T __compare, _T __val) __NV_ATOMIC_REQUIRES(_T) {
  __NV_ATOMIC_TEMP(_T);
  __u128AtomicCAS_block((void *)(__address),
                  __NV_ATOMIC_ADDRESSOF(__compare),
                  __NV_ATOMIC_ADDRESSOF(__val),
                  __NV_ATOMIC_ADDRESSOF(__NV_ATOMIC_RET(_T)));
  return __NV_ATOMIC_RET(_T);
}

template <typename _T>
__SM_90_RT_DECL__ __NV_ATOMIC_RET_TYPE(_T)
atomicExch(_T *__address, _T __val) __NV_ATOMIC_REQUIRES(_T) {
  __NV_ATOMIC_TEMP(_T);
  __u128AtomicExch((void *)(__address),
                  __NV_ATOMIC_ADDRESSOF(__val),
                  __NV_ATOMIC_ADDRESSOF(__NV_ATOMIC_RET(_T)));
  return __NV_ATOMIC_RET(_T);
}

template <typename _T>
__SM_90_RT_DECL__ __NV_ATOMIC_RET_TYPE(_T)
atomicExch_block(_T *__address, _T __val) __NV_ATOMIC_REQUIRES(_T) {
  __NV_ATOMIC_TEMP(_T);
  __u128AtomicExch_block((void *)(__address),
                  __NV_ATOMIC_ADDRESSOF(__val),
                  __NV_ATOMIC_ADDRESSOF(__NV_ATOMIC_RET(_T)));
  return __NV_ATOMIC_RET(_T);
}
#endif /* !__NV_DISABLE_128_ATOMICS */

#endif /* !__CUDA_ARCH__ || __CUDA_ARCH__ >= 900 */

#endif /* __cplusplus && __CUDACC__ */

#undef __DEF_IF_HOST
#undef __SM_90_RT_DECL__

#if !defined(__CUDACC_RTC__) && defined(__CUDA_ARCH__)
#include "sm_90_rt.hpp"
#endif /* !__CUDACC_RTC__ && defined(__CUDA_ARCH__) */

#endif /* !__SM_90_RT_H__ */

#if defined(__UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_SM_90_RT_H__)
#undef __CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS__
#undef __UNDEF_CUDA_INCLUDE_COMPILER_INTERNAL_HEADERS_SM_90_RT_H__
#endif

