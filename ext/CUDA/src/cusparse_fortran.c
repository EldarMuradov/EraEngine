/*
 * Copyright 1993-2020 NVIDIA Corporation.  All rights reserved.
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
 *
 * @author Federico Busato                                                  <br>
 *         Nvidia Corporation, Santa Clara                                  <br>
 *         fbusato@nvidia.com
 * @date June, 2018
 * @version 0.1     Fix: type qualifier is meaningless on cast type,
 *                       enumerated type mixed with another type
 */

/*
 * This file contains example Fortran bindings for the CUSPARSE library, These
 * bindings have been tested with Intel Fortran 9.0 on 32-bit and 64-bit
 * Windows, and with g77 3.4.5 on 32-bit and 64-bit Linux. They will likely
 * have to be adjusted for other Fortran compilers and platforms.
 */

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(__GNUC__)
#    include <stdint.h>
#endif                    /* __GNUC__ */
#include "cuda_runtime.h" /* CUDA public header file     */
#include "cusparse_fortran_common.h"
#include "cusparse.h" /* CUSPARSE public header file */
#include "cusparse_fortran.h"

/*---------------------------------------------------------------------------*/
/*------------------------- AUXILIARY FUNCTIONS -----------------------------*/
/*---------------------------------------------------------------------------*/
int CUDA_MALLOC(ptr_t* devPtr, int* size) {
    void* tdevPtr = 0;
    int   error   = (int) cudaMalloc(&tdevPtr, (size_t)(*size));
    *devPtr       = (ptr_t) tdevPtr;
    return error;
}
int CUDA_FREE(ptr_t* devPtr) {
    return (int) cudaFree((void*) (*devPtr));
}
/* WARNING:
   (i) notice that when passing dstPtr and srcPtr to cudaMemcpy,
   one of the parameters is dereferenced, while the other is not. This
   reflects the fact that for _FORT2C_ dstPtr was allocated by cudaMalloc
   in the wrapper, while srcPtr was allocated in the Fortran code; on the
   other hand, for _C2FORT_ dstPtr was allocated in the Fortran code, while
   srcPtr was allocated by cudaMalloc in the wrapper. Users should be
   careful and take great care of this distinction in their own code.
   (ii) there are two functions _INT and _REAL in order to avoid
   warnings from the Fortran compiler due to the fact that arguments of
   different type are passed to the same function.
*/
int CUDA_MEMCPY_FORT2C_INT(ptr_t*       dstPtr,
                           const ptr_t* srcPtr,
                           int*         count,
                           int*         kind) {
    return (int) cudaMemcpy((void*) (*dstPtr),
                            (void*) srcPtr,
                            (size_t)(*count),
                            (enum cudaMemcpyKind)(*kind));
}
int CUDA_MEMCPY_FORT2C_REAL(ptr_t*       dstPtr,
                            const ptr_t* srcPtr,
                            int*         count,
                            int*         kind) {
    return (int) cudaMemcpy((void*) (*dstPtr),
                            (void*) srcPtr,
                            (size_t)(*count),
                            (enum cudaMemcpyKind)(*kind));
}
int CUDA_MEMCPY_C2FORT_INT(ptr_t*       dstPtr,
                           const ptr_t* srcPtr,
                           int*         count,
                           int*         kind) {
    return (int) cudaMemcpy((void*) dstPtr,
                            (void*) (*srcPtr),
                            (size_t)(*count),
                            (enum cudaMemcpyKind)(*kind));
}
int CUDA_MEMCPY_C2FORT_REAL(ptr_t*       dstPtr,
                            const ptr_t* srcPtr,
                            int*         count,
                            int*         kind) {
    return (int) cudaMemcpy((void*) dstPtr,
                            (void*) (*srcPtr),
                            (size_t)(*count),
                            (enum cudaMemcpyKind)(*kind));
}
int CUDA_MEMSET(ptr_t* devPtr, int* value, int* count) {
    return (int) cudaMemset((void*) (*devPtr), *value, (size_t)(*count));
}

void GET_SHIFTED_ADDRESS(ptr_t* originPtr, int* count, ptr_t* resultPtr) {
    char* temp = (char*) (*originPtr);
    *resultPtr = (ptr_t)(temp + (*count));
}

/*---------------------------------------------------------------------------*/
/*------------------------- CUSPARSE FUNCTIONS ------------------------------*/
/*---------------------------------------------------------------------------*/
int CUSPARSE_CREATE(ptr_t* handle) {
    cusparseHandle_t thandle = 0;
    int              error   = (int) cusparseCreate(&thandle);
    *handle                  = (ptr_t) thandle;
    return error;
}
int CUSPARSE_DESTROY(ptr_t* handle) {
    return (int) cusparseDestroy((cusparseHandle_t)(*handle));
}
int CUSPARSE_GET_VERSION(ptr_t* handle, int* version) {
    return (int) cusparseGetVersion((cusparseHandle_t)(*handle), version);
}
int CUSPARSE_SET_STREAM(ptr_t* handle, ptr_t* streamId) {
    return (int) cusparseSetStream((cusparseHandle_t)(*handle),
                                   (cudaStream_t)(*streamId));
}

int CUSPARSE_GET_POINTER_MODE(ptr_t* handle, int* mode) {
    cusparsePointerMode_t tmode = (cusparsePointerMode_t) 0;
    int                   error =
        (int) cusparseGetPointerMode((cusparseHandle_t)(*handle), &tmode);
    *mode = (int) tmode;
    return error;
}

int CUSPARSE_SET_POINTER_MODE(ptr_t* handle, int* mode) {
    return (int) cusparseSetPointerMode((cusparseHandle_t)(*handle),
                                        (cusparsePointerMode_t)(*mode));
}

int CUSPARSE_CREATE_MAT_DESCR(ptr_t* descrA) {
    cusparseMatDescr_t tdescrA = (cusparseMatDescr_t) 0;
    int                error   = (int) cusparseCreateMatDescr(&tdescrA);
    *descrA                    = (ptr_t) tdescrA;
    return error;
}
int CUSPARSE_DESTROY_MAT_DESCR(ptr_t* descrA) {
    return (int) cusparseDestroyMatDescr((cusparseMatDescr_t)(*descrA));
}

int CUSPARSE_SET_MAT_TYPE(ptr_t* descrA, int* type) {
    return (int) cusparseSetMatType((cusparseMatDescr_t)(*descrA),
                                    (cusparseMatrixType_t)(*type));
}
int CUSPARSE_GET_MAT_TYPE(const ptr_t* descrA) {
    return (int) cusparseGetMatType((cusparseMatDescr_t)(*descrA));
}
int CUSPARSE_SET_MAT_FILL_MODE(ptr_t* descrA, int* fillMode) {
    return (int) cusparseSetMatFillMode((cusparseMatDescr_t)(*descrA),
                                        (cusparseFillMode_t)(*fillMode));
}
int CUSPARSE_GET_MAT_FILL_MODE(const ptr_t* descrA) {
    return (int) cusparseGetMatFillMode((cusparseMatDescr_t)(*descrA));
}
int CUSPARSE_SET_MAT_DIAG_TYPE(ptr_t* descrA, int* diagType) {
    return (int) cusparseSetMatDiagType((cusparseMatDescr_t)(*descrA),
                                        (cusparseDiagType_t)(*diagType));
}
int CUSPARSE_GET_MAT_DIAG_TYPE(const ptr_t* descrA) {
    return (int) cusparseGetMatDiagType((cusparseMatDescr_t)(*descrA));
}
int CUSPARSE_SET_MAT_INDEX_BASE(ptr_t* descrA, int* base) {
    return (int) cusparseSetMatIndexBase((cusparseMatDescr_t)(*descrA),
                                         (cusparseIndexBase_t)(*base));
}
int CUSPARSE_GET_MAT_INDEX_BASE(const ptr_t* descrA) {
    return (int) cusparseGetMatIndexBase((cusparseMatDescr_t)(*descrA));
}

//int CUSPARSE_CREATE_SOLVE_ANALYSIS_INFO(ptr_t* info) {
//    cusparseSolveAnalysisInfo_t tinfo = (cusparseSolveAnalysisInfo_t) 0;
//    int error = (int) cusparseCreateSolveAnalysisInfo(&tinfo);
//    *info     = (ptr_t) tinfo;
//    return error;
//}

//int CUSPARSE_DESTROY_SOLVE_ANALYSIS_INFO(ptr_t* info) {
//    return (int) cusparseDestroySolveAnalysisInfo(
//        (cusparseSolveAnalysisInfo_t)(*info));
//}


/*---------------------------------------------------------------------------*/
/*------------------------- SPARSE LEVEL 2 ----------------------------------*/
/*---------------------------------------------------------------------------*/
/*int CUSPARSE_SCSRMV(ptr_t*       handle,
                    int*         transA,
                    int*         m,
                    int*         n,
                    int*         nnz,
                    const float* alpha,
                    const ptr_t* descrA,
                    const ptr_t* csrValA,
                    const ptr_t* csrRowPtrA,
                    const ptr_t* csrColIndA,
                    const ptr_t* x,
                    const float* beta,
                    ptr_t*       y) {
    return (int) cusparseScsrmv((cusparseHandle_t)(*handle),
                                (cusparseOperation_t)(*transA),
                                *m,
                                *n,
                                *nnz,
                                alpha,
                                (cusparseMatDescr_t)(*descrA),
                                (const float*) (*csrValA),
                                (const int*) (*csrRowPtrA),
                                (const int*) (*csrColIndA),
                                (const float*) (*x),
                                beta,
                                (float*) (*y));
}

int CUSPARSE_DCSRMV(ptr_t*        handle,
                    int*          transA,
                    int*          m,
                    int*          n,
                    int*          nnz,
                    const double* alpha,
                    const ptr_t*  descrA,
                    const ptr_t*  csrValA,
                    const ptr_t*  csrRowPtrA,
                    const ptr_t*  csrColIndA,
                    const ptr_t*  x,
                    const double* beta,
                    ptr_t*        y) {
    return (int) cusparseDcsrmv((cusparseHandle_t)(*handle),
                                (cusparseOperation_t)(*transA),
                                *m,
                                *n,
                                *nnz,
                                alpha,
                                (cusparseMatDescr_t)(*descrA),
                                (const double*) (*csrValA),
                                (const int*) (*csrRowPtrA),
                                (const int*) (*csrColIndA),
                                (const double*) (*x),
                                beta,
                                (double*) (*y));
}

int CUSPARSE_CCSRMV(ptr_t*           handle,
                    int*             transA,
                    int*             m,
                    int*             n,
                    int*             nnz,
                    const cuComplex* alpha,
                    const ptr_t*     descrA,
                    const ptr_t*     csrValA,
                    const ptr_t*     csrRowPtrA,
                    const ptr_t*     csrColIndA,
                    const ptr_t*     x,
                    const cuComplex* beta,
                    ptr_t*           y) {
    return (int) cusparseCcsrmv((cusparseHandle_t)(*handle),
                                (cusparseOperation_t)(*transA),
                                *m,
                                *n,
                                *nnz,
                                alpha,
                                (cusparseMatDescr_t)(*descrA),
                                (const cuComplex*) (*csrValA),
                                (const int*) (*csrRowPtrA),
                                (const int*) (*csrColIndA),
                                (const cuComplex*) (*x),
                                beta,
                                (cuComplex*) (*y));
}

int CUSPARSE_ZCSRMV(ptr_t*                 handle,
                    int*                   transA,
                    int*                   m,
                    int*                   n,
                    int*                   nnz,
                    const cuDoubleComplex* alpha,
                    const ptr_t*           descrA,
                    const ptr_t*           csrValA,
                    const ptr_t*           csrRowPtrA,
                    const ptr_t*           csrColIndA,
                    const ptr_t*           x,
                    const cuDoubleComplex* beta,
                    ptr_t*                 y) {
    return (int) cusparseZcsrmv((cusparseHandle_t)(*handle),
                                (cusparseOperation_t)(*transA),
                                *m,
                                *n,
                                *nnz,
                                alpha,
                                (cusparseMatDescr_t)(*descrA),
                                (const cuDoubleComplex*) (*csrValA),
                                (const int*) (*csrRowPtrA),
                                (const int*) (*csrColIndA),
                                (const cuDoubleComplex*) (*x),
                                beta,
                                (cuDoubleComplex*) (*y));
}

int CUSPARSE_SCSRSV_ANALYSIS(ptr_t*       handle,
                             int*         transA,
                             int*         m,
                             int*         nnz,
                             const ptr_t* descrA,
                             const ptr_t* csrValA,
                             const ptr_t* csrRowPtrA,
                             const ptr_t* csrColIndA,
                             ptr_t*       info) {
    return (int) cusparseScsrsv_analysis((cusparseHandle_t)(*handle),
                                         (cusparseOperation_t)(*transA),
                                         *m,
                                         *nnz,
                                         (cusparseMatDescr_t)(*descrA),
                                         (const float*) (*csrValA),
                                         (const int*) (*csrRowPtrA),
                                         (const int*) (*csrColIndA),
                                         (cusparseSolveAnalysisInfo_t)(*info));
}

int CUSPARSE_DCSRSV_ANALYSIS(ptr_t*       handle,
                             int*         transA,
                             int*         m,
                             int*         nnz,
                             const ptr_t* descrA,
                             const ptr_t* csrValA,
                             const ptr_t* csrRowPtrA,
                             const ptr_t* csrColIndA,
                             ptr_t*       info) {
    return (int) cusparseDcsrsv_analysis((cusparseHandle_t)(*handle),
                                         (cusparseOperation_t)(*transA),
                                         *m,
                                         *nnz,
                                         (cusparseMatDescr_t)(*descrA),
                                         (const double*) (*csrValA),
                                         (const int*) (*csrRowPtrA),
                                         (const int*) (*csrColIndA),
                                         (cusparseSolveAnalysisInfo_t)(*info));
}

int CUSPARSE_CCSRSV_ANALYSIS(ptr_t*       handle,
                             int*         transA,
                             int*         m,
                             int*         nnz,
                             const ptr_t* descrA,
                             const ptr_t* csrValA,
                             const ptr_t* csrRowPtrA,
                             const ptr_t* csrColIndA,
                             ptr_t*       info) {
    return (int) cusparseCcsrsv_analysis((cusparseHandle_t)(*handle),
                                         (cusparseOperation_t)(*transA),
                                         *m,
                                         *nnz,
                                         (cusparseMatDescr_t)(*descrA),
                                         (const cuComplex*) (*csrValA),
                                         (const int*) (*csrRowPtrA),
                                         (const int*) (*csrColIndA),
                                         (cusparseSolveAnalysisInfo_t)(*info));
}

int CUSPARSE_ZCSRSV_ANALYSIS(ptr_t*       handle,
                             int*         transA,
                             int*         m,
                             int*         nnz,
                             const ptr_t* descrA,
                             const ptr_t* csrValA,
                             const ptr_t* csrRowPtrA,
                             const ptr_t* csrColIndA,
                             ptr_t*       info) {
    return (int) cusparseZcsrsv_analysis((cusparseHandle_t)(*handle),
                                         (cusparseOperation_t)(*transA),
                                         *m,
                                         *nnz,
                                         (cusparseMatDescr_t)(*descrA),
                                         (const cuDoubleComplex*) (*csrValA),
                                         (const int*) (*csrRowPtrA),
                                         (const int*) (*csrColIndA),
                                         (cusparseSolveAnalysisInfo_t)(*info));
}

int CUSPARSE_SCSRSV_SOLVE(ptr_t*       handle,
                          int*         transA,
                          int*         m,
                          const float* alpha,
                          const ptr_t* descrA,
                          const ptr_t* csrValA,
                          const ptr_t* csrRowPtrA,
                          const ptr_t* csrColIndA,
                          ptr_t*       info,
                          const ptr_t* x,
                          ptr_t*       y) {
    return (int) cusparseScsrsv_solve((cusparseHandle_t)(*handle),
                                      (cusparseOperation_t)(*transA),
                                      *m,
                                      alpha,
                                      (cusparseMatDescr_t)(*descrA),
                                      (const float*) (*csrValA),
                                      (const int*) (*csrRowPtrA),
                                      (const int*) (*csrColIndA),
                                      (cusparseSolveAnalysisInfo_t)(*info),
                                      (const float*) (*x),
                                      (float*) (*y));
}

int CUSPARSE_DCSRSV_SOLVE(ptr_t*        handle,
                          int*          transA,
                          int*          m,
                          const double* alpha,
                          const ptr_t*  descrA,
                          const ptr_t*  csrValA,
                          const ptr_t*  csrRowPtrA,
                          const ptr_t*  csrColIndA,
                          ptr_t*        info,
                          const ptr_t*  x,
                          ptr_t*        y) {
    return (int) cusparseDcsrsv_solve((cusparseHandle_t)(*handle),
                                      (cusparseOperation_t)(*transA),
                                      *m,
                                      alpha,
                                      (cusparseMatDescr_t)(*descrA),
                                      (const double*) (*csrValA),
                                      (const int*) (*csrRowPtrA),
                                      (const int*) (*csrColIndA),
                                      (cusparseSolveAnalysisInfo_t)(*info),
                                      (const double*) (*x),
                                      (double*) (*y));
}

int CUSPARSE_CCSRSV_SOLVE(ptr_t*           handle,
                          int*             transA,
                          int*             m,
                          const cuComplex* alpha,
                          const ptr_t*     descrA,
                          const ptr_t*     csrValA,
                          const ptr_t*     csrRowPtrA,
                          const ptr_t*     csrColIndA,
                          ptr_t*           info,
                          const ptr_t*     x,
                          ptr_t*           y) {
    return (int) cusparseCcsrsv_solve((cusparseHandle_t)(*handle),
                                      (cusparseOperation_t)(*transA),
                                      *m,
                                      alpha,
                                      (cusparseMatDescr_t)(*descrA),
                                      (const cuComplex*) (*csrValA),
                                      (const int*) (*csrRowPtrA),
                                      (const int*) (*csrColIndA),
                                      (cusparseSolveAnalysisInfo_t)(*info),
                                      (const cuComplex*) (*x),
                                      (cuComplex*) (*y));
}

int CUSPARSE_ZCSRSV_SOLVE(ptr_t*                 handle,
                          int*                   transA,
                          int*                   m,
                          const cuDoubleComplex* alpha,
                          const ptr_t*           descrA,
                          const ptr_t*           csrValA,
                          const ptr_t*           csrRowPtrA,
                          const ptr_t*           csrColIndA,
                          ptr_t*                 info,
                          const ptr_t*           x,
                          ptr_t*                 y) {
    return (int) cusparseZcsrsv_solve((cusparseHandle_t)(*handle),
                                      (cusparseOperation_t)(*transA),
                                      *m,
                                      alpha,
                                      (cusparseMatDescr_t)(*descrA),
                                      (const cuDoubleComplex*) (*csrValA),
                                      (const int*) (*csrRowPtrA),
                                      (const int*) (*csrColIndA),
                                      (cusparseSolveAnalysisInfo_t)(*info),
                                      (const cuDoubleComplex*) (*x),
                                      (cuDoubleComplex*) (*y));
}*/

/*---------------------------------------------------------------------------*/
/*------------------------- SPARSE LEVEL 3 ----------------------------------*/
/*---------------------------------------------------------------------------*/
/*
int CUSPARSE_SCSRMM(ptr_t*       handle,
                    int*         transA,
                    int*         m,
                    int*         n,
                    int*         k,
                    int*         nnz,
                    const float* alpha,
                    const ptr_t* descrA,
                    const ptr_t* csrValA,
                    const ptr_t* csrRowPtrA,
                    const ptr_t* csrColIndA,
                    const ptr_t* B,
                    int*         ldb,
                    const float* beta,
                    ptr_t*       C,
                    int*         ldc) {
    return (int) cusparseScsrmm((cusparseHandle_t)(*handle),
                                (cusparseOperation_t)(*transA),
                                *m,
                                *n,
                                *k,
                                *nnz,
                                alpha,
                                (cusparseMatDescr_t)(*descrA),
                                (const float*) (*csrValA),
                                (const int*) (*csrRowPtrA),
                                (const int*) (*csrColIndA),
                                (const float*) (*B),
                                *ldb,
                                beta,
                                (float*) (*C),
                                *ldc);
}

int CUSPARSE_DCSRMM(ptr_t*        handle,
                    int*          transA,
                    int*          m,
                    int*          n,
                    int*          k,
                    int*          nnz,
                    const double* alpha,
                    const ptr_t*  descrA,
                    const ptr_t*  csrValA,
                    const ptr_t*  csrRowPtrA,
                    const ptr_t*  csrColIndA,
                    const ptr_t*  B,
                    int*          ldb,
                    const double* beta,
                    ptr_t*        C,
                    int*          ldc) {
    return (int) cusparseDcsrmm((cusparseHandle_t)(*handle),
                                (cusparseOperation_t)(*transA),
                                *m,
                                *n,
                                *k,
                                *nnz,
                                alpha,
                                (cusparseMatDescr_t)(*descrA),
                                (const double*) (*csrValA),
                                (const int*) (*csrRowPtrA),
                                (const int*) (*csrColIndA),
                                (const double*) (*B),
                                *ldb,
                                beta,
                                (double*) (*C),
                                *ldc);
}

int CUSPARSE_CCSRMM(ptr_t*           handle,
                    int*             transA,
                    int*             m,
                    int*             n,
                    int*             k,
                    int*             nnz,
                    const cuComplex* alpha,
                    const ptr_t*     descrA,
                    const ptr_t*     csrValA,
                    const ptr_t*     csrRowPtrA,
                    const ptr_t*     csrColIndA,
                    const ptr_t*     B,
                    int*             ldb,
                    const cuComplex* beta,
                    ptr_t*           C,
                    int*             ldc) {
    return (int) cusparseCcsrmm((cusparseHandle_t)(*handle),
                                (cusparseOperation_t)(*transA),
                                *m,
                                *n,
                                *k,
                                *nnz,
                                alpha,
                                (cusparseMatDescr_t)(*descrA),
                                (const cuComplex*) (*csrValA),
                                (const int*) (*csrRowPtrA),
                                (const int*) (*csrColIndA),
                                (const cuComplex*) (*B),
                                *ldb,
                                beta,
                                (cuComplex*) (*C),
                                *ldc);
}

int CUSPARSE_ZCSRMM(ptr_t*                 handle,
                    int*                   transA,
                    int*                   m,
                    int*                   n,
                    int*                   k,
                    int*                   nnz,
                    const cuDoubleComplex* alpha,
                    const ptr_t*           descrA,
                    const ptr_t*           csrValA,
                    const ptr_t*           csrRowPtrA,
                    const ptr_t*           csrColIndA,
                    const ptr_t*           B,
                    int*                   ldb,
                    const cuDoubleComplex* beta,
                    ptr_t*                 C,
                    int*                   ldc) {
    return (int) cusparseZcsrmm((cusparseHandle_t)(*handle),
                                (cusparseOperation_t)(*transA),
                                *m,
                                *n,
                                *k,
                                *nnz,
                                alpha,
                                (cusparseMatDescr_t)(*descrA),
                                (const cuDoubleComplex*) (*csrValA),
                                (const int*) (*csrRowPtrA),
                                (const int*) (*csrColIndA),
                                (const cuDoubleComplex*) (*B),
                                *ldb,
                                beta,
                                (cuDoubleComplex*) (*C),
                                *ldc);
}*/
/*
int CUSPARSE_SCSRSM_ANALYSIS(ptr_t*       handle,
                             int*         transA,
                             int*         m,
                             int*         nnz,
                             const ptr_t* descrA,
                             const ptr_t* csrValA,
                             const ptr_t* csrRowPtrA,
                             const ptr_t* csrColIndA,
                             ptr_t*       info) {
    return (int) cusparseScsrsm_analysis((cusparseHandle_t)(*handle),
                                         (cusparseOperation_t)(*transA),
                                         *m,
                                         *nnz,
                                         (cusparseMatDescr_t)(*descrA),
                                         (const float*) (*csrValA),
                                         (const int*) (*csrRowPtrA),
                                         (const int*) (*csrColIndA),
                                         (cusparseSolveAnalysisInfo_t)(*info));
}

int CUSPARSE_DCSRSM_ANALYSIS(ptr_t*       handle,
                             int*         transA,
                             int*         m,
                             int*         nnz,
                             const ptr_t* descrA,
                             const ptr_t* csrValA,
                             const ptr_t* csrRowPtrA,
                             const ptr_t* csrColIndA,
                             ptr_t*       info) {
    return (int) cusparseDcsrsm_analysis((cusparseHandle_t)(*handle),
                                         (cusparseOperation_t)(*transA),
                                         *m,
                                         *nnz,
                                         (cusparseMatDescr_t)(*descrA),
                                         (const double*) (*csrValA),
                                         (const int*) (*csrRowPtrA),
                                         (const int*) (*csrColIndA),
                                         (cusparseSolveAnalysisInfo_t)(*info));
}

int CUSPARSE_CCSRSM_ANALYSIS(ptr_t*       handle,
                             int*         transA,
                             int*         m,
                             int*         nnz,
                             const ptr_t* descrA,
                             const ptr_t* csrValA,
                             const ptr_t* csrRowPtrA,
                             const ptr_t* csrColIndA,
                             ptr_t*       info) {
    return (int) cusparseCcsrsm_analysis((cusparseHandle_t)(*handle),
                                         (cusparseOperation_t)(*transA),
                                         *m,
                                         *nnz,
                                         (cusparseMatDescr_t)(*descrA),
                                         (const cuComplex*) (*csrValA),
                                         (const int*) (*csrRowPtrA),
                                         (const int*) (*csrColIndA),
                                         (cusparseSolveAnalysisInfo_t)(*info));
}

int CUSPARSE_ZCSRSM_ANALYSIS(ptr_t*       handle,
                             int*         transA,
                             int*         m,
                             int*         nnz,
                             const ptr_t* descrA,
                             const ptr_t* csrValA,
                             const ptr_t* csrRowPtrA,
                             const ptr_t* csrColIndA,
                             ptr_t*       info) {
    return (int) cusparseZcsrsm_analysis((cusparseHandle_t)(*handle),
                                         (cusparseOperation_t)(*transA),
                                         *m,
                                         *nnz,
                                         (cusparseMatDescr_t)(*descrA),
                                         (const cuDoubleComplex*) (*csrValA),
                                         (const int*) (*csrRowPtrA),
                                         (const int*) (*csrColIndA),
                                         (cusparseSolveAnalysisInfo_t)(*info));
}

int CUSPARSE_SCSRSM_SOLVE(ptr_t*       handle,
                          int*         transA,
                          int*         m,
                          int*         n,
                          const float* alpha,
                          const ptr_t* descrA,
                          const ptr_t* csrValA,
                          const ptr_t* csrRowPtrA,
                          const ptr_t* csrColIndA,
                          ptr_t*       info,
                          const ptr_t* x,
                          int*         ldx,
                          ptr_t*       y,
                          int*         ldy) {
    return (int) cusparseScsrsm_solve((cusparseHandle_t)(*handle),
                                      (cusparseOperation_t)(*transA),
                                      *m,
                                      *n,
                                      alpha,
                                      (cusparseMatDescr_t)(*descrA),
                                      (const float*) (*csrValA),
                                      (const int*) (*csrRowPtrA),
                                      (const int*) (*csrColIndA),
                                      (cusparseSolveAnalysisInfo_t)(*info),
                                      (const float*) (*x),
                                      *ldx,
                                      (float*) (*y),
                                      *ldy);
}

int CUSPARSE_DCSRSM_SOLVE(ptr_t*        handle,
                          int*          transA,
                          int*          m,
                          int*          n,
                          const double* alpha,
                          const ptr_t*  descrA,
                          const ptr_t*  csrValA,
                          const ptr_t*  csrRowPtrA,
                          const ptr_t*  csrColIndA,
                          ptr_t*        info,
                          const ptr_t*  x,
                          int*          ldx,
                          ptr_t*        y,
                          int*          ldy) {
    return (int) cusparseDcsrsm_solve((cusparseHandle_t)(*handle),
                                      (cusparseOperation_t)(*transA),
                                      *m,
                                      *n,
                                      alpha,
                                      (cusparseMatDescr_t)(*descrA),
                                      (const double*) (*csrValA),
                                      (const int*) (*csrRowPtrA),
                                      (const int*) (*csrColIndA),
                                      (cusparseSolveAnalysisInfo_t)(*info),
                                      (const double*) (*x),
                                      *ldx,
                                      (double*) (*y),
                                      *ldy);
}

int CUSPARSE_CCSRSM_SOLVE(ptr_t*           handle,
                          int*             transA,
                          int*             m,
                          int*             n,
                          const cuComplex* alpha,
                          const ptr_t*     descrA,
                          const ptr_t*     csrValA,
                          const ptr_t*     csrRowPtrA,
                          const ptr_t*     csrColIndA,
                          ptr_t*           info,
                          const ptr_t*     x,
                          int*             ldx,
                          ptr_t*           y,
                          int*             ldy) {
    return (int) cusparseCcsrsm_solve((cusparseHandle_t)(*handle),
                                      (cusparseOperation_t)(*transA),
                                      *m,
                                      *n,
                                      alpha,
                                      (cusparseMatDescr_t)(*descrA),
                                      (const cuComplex*) (*csrValA),
                                      (const int*) (*csrRowPtrA),
                                      (const int*) (*csrColIndA),
                                      (cusparseSolveAnalysisInfo_t)(*info),
                                      (const cuComplex*) (*x),
                                      *ldx,
                                      (cuComplex*) (*y),
                                      *ldy);
}

int CUSPARSE_ZCSRSM_SOLVE(ptr_t*                 handle,
                          int*                   transA,
                          int*                   m,
                          int*                   n,
                          const cuDoubleComplex* alpha,
                          const ptr_t*           descrA,
                          const ptr_t*           csrValA,
                          const ptr_t*           csrRowPtrA,
                          const ptr_t*           csrColIndA,
                          ptr_t*                 info,
                          const ptr_t*           x,
                          int*                   ldx,
                          ptr_t*                 y,
                          int*                   ldy) {
    return (int) cusparseZcsrsm_solve((cusparseHandle_t)(*handle),
                                      (cusparseOperation_t)(*transA),
                                      *m,
                                      *n,
                                      alpha,
                                      (cusparseMatDescr_t)(*descrA),
                                      (const cuDoubleComplex*) (*csrValA),
                                      (const int*) (*csrRowPtrA),
                                      (const int*) (*csrColIndA),
                                      (cusparseSolveAnalysisInfo_t)(*info),
                                      (const cuDoubleComplex*) (*x),
                                      *ldx,
                                      (cuDoubleComplex*) (*y),
                                      *ldy);
}

int CUSPARSE_SGTSV(ptr_t*       handle,
                   int*         m,
                   int*         n,
                   const ptr_t* dl,
                   const ptr_t* d,
                   const ptr_t* du,
                   ptr_t*       B,
                   int*         ldb) {
    return (int) cusparseSgtsv((cusparseHandle_t)(*handle),
                               *m,
                               *n,
                               (const float*) (*dl),
                               (const float*) (*d),
                               (const float*) (*du),
                               (float*) (*B),
                               *ldb);
}

int CUSPARSE_DGTSV(ptr_t*       handle,
                   int*         m,
                   int*         n,
                   const ptr_t* dl,
                   const ptr_t* d,
                   const ptr_t* du,
                   ptr_t*       B,
                   int*         ldb) {
    return (int) cusparseDgtsv((cusparseHandle_t)(*handle),
                               *m,
                               *n,
                               (const double*) (*dl),
                               (const double*) (*d),
                               (const double*) (*du),
                               (double*) (*B),
                               *ldb);
}

int CUSPARSE_CGTSV(ptr_t*       handle,
                   int*         m,
                   int*         n,
                   const ptr_t* dl,
                   const ptr_t* d,
                   const ptr_t* du,
                   ptr_t*       B,
                   int*         ldb) {
    return (int) cusparseCgtsv((cusparseHandle_t)(*handle),
                               *m,
                               *n,
                               (const cuComplex*) (*dl),
                               (const cuComplex*) (*d),
                               (const cuComplex*) (*du),
                               (cuComplex*) (*B),
                               *ldb);
}

int CUSPARSE_ZGTSV(ptr_t*       handle,
                   int*         m,
                   int*         n,
                   const ptr_t* dl,
                   const ptr_t* d,
                   const ptr_t* du,
                   ptr_t*       B,
                   int*         ldb) {
    return (int) cusparseZgtsv((cusparseHandle_t)(*handle),
                               *m,
                               *n,
                               (const cuDoubleComplex*) (*dl),
                               (const cuDoubleComplex*) (*d),
                               (const cuDoubleComplex*) (*du),
                               (cuDoubleComplex*) (*B),
                               *ldb);
}

int CUSPARSE_SGTSV_STRIDEDBATCH(ptr_t*       handle,
                                int*         m,
                                const ptr_t* dl,
                                const ptr_t* d,
                                const ptr_t* du,
                                ptr_t*       x,
                                int*         batchCount,
                                int*         batchStride) {
    return (int) cusparseSgtsvStridedBatch((cusparseHandle_t)(*handle),
                                           *m,
                                           (const float*) (*dl),
                                           (const float*) (*d),
                                           (const float*) (*du),
                                           (float*) (*x),
                                           *batchCount,
                                           *batchStride);
}

int CUSPARSE_DGTSV_STRIDEDBATCH(ptr_t*       handle,
                                int*         m,
                                const ptr_t* dl,
                                const ptr_t* d,
                                const ptr_t* du,
                                ptr_t*       x,
                                int*         batchCount,
                                int*         batchStride) {
    return (int) cusparseDgtsvStridedBatch((cusparseHandle_t)(*handle),
                                           *m,
                                           (const double*) (*dl),
                                           (const double*) (*d),
                                           (const double*) (*du),
                                           (double*) (*x),
                                           *batchCount,
                                           *batchStride);
}

int CUSPARSE_CGTSV_STRIDEDBATCH(ptr_t*       handle,
                                int*         m,
                                const ptr_t* dl,
                                const ptr_t* d,
                                const ptr_t* du,
                                ptr_t*       x,
                                int*         batchCount,
                                int*         batchStride) {
    return (int) cusparseCgtsvStridedBatch((cusparseHandle_t)(*handle),
                                           *m,
                                           (const cuComplex*) (*dl),
                                           (const cuComplex*) (*d),
                                           (const cuComplex*) (*du),
                                           (cuComplex*) (*x),
                                           *batchCount,
                                           *batchStride);
}

int CUSPARSE_ZGTSV_STRIDEDBATCH(ptr_t*       handle,
                                int*         m,
                                const ptr_t* dl,
                                const ptr_t* d,
                                const ptr_t* du,
                                ptr_t*       x,
                                int*         batchCount,
                                int*         batchStride) {
    return (int) cusparseZgtsvStridedBatch((cusparseHandle_t)(*handle),
                                           *m,
                                           (const cuDoubleComplex*) (*dl),
                                           (const cuDoubleComplex*) (*d),
                                           (const cuDoubleComplex*) (*du),
                                           (cuDoubleComplex*) (*x),
                                           *batchCount,
                                           *batchStride);
}
*/
/*---------------------------------------------------------------------------*/
/*------------------------- CONVERSIONS -------------------------------------*/
/*---------------------------------------------------------------------------*/
int CUSPARSE_SNNZ(ptr_t*       handle,
                  int*         dirA,
                  int*         m,
                  int*         n,
                  const ptr_t* descrA,
                  const ptr_t* A,
                  int*         lda,
                  ptr_t*       nnzPerRowCol,
                  int*         nnzTotal) {
    return (int) cusparseSnnz((cusparseHandle_t)(*handle),
                              (cusparseDirection_t)(*dirA),
                              *m,
                              *n,
                              (cusparseMatDescr_t)(*descrA),
                              (const float*) (*A),
                              *lda,
                              (int*) (*nnzPerRowCol),
                              nnzTotal);
}

int CUSPARSE_DNNZ(ptr_t*       handle,
                  int*         dirA,
                  int*         m,
                  int*         n,
                  const ptr_t* descrA,
                  const ptr_t* A,
                  int*         lda,
                  ptr_t*       nnzPerRowCol,
                  int*         nnzTotal) {
    return (int) cusparseDnnz((cusparseHandle_t)(*handle),
                              (cusparseDirection_t)(*dirA),
                              *m,
                              *n,
                              (cusparseMatDescr_t)(*descrA),
                              (const double*) (*A),
                              *lda,
                              (int*) (*nnzPerRowCol),
                              nnzTotal);
}

int CUSPARSE_CNNZ(ptr_t*       handle,
                  int*         dirA,
                  int*         m,
                  int*         n,
                  const ptr_t* descrA,
                  const ptr_t* A,
                  int*         lda,
                  ptr_t*       nnzPerRowCol,
                  int*         nnzTotal) {
    return (int) cusparseCnnz((cusparseHandle_t)(*handle),
                              (cusparseDirection_t)(*dirA),
                              *m,
                              *n,
                              (cusparseMatDescr_t)(*descrA),
                              (const cuComplex*) (*A),
                              *lda,
                              (int*) (*nnzPerRowCol),
                              nnzTotal);
}

int CUSPARSE_ZNNZ(ptr_t*       handle,
                  int*         dirA,
                  int*         m,
                  int*         n,
                  const ptr_t* descrA,
                  const ptr_t* A,
                  int*         lda,
                  ptr_t*       nnzPerRowCol,
                  int*         nnzTotal) {
    return (int) cusparseZnnz((cusparseHandle_t)(*handle),
                              (cusparseDirection_t)(*dirA),
                              *m,
                              *n,
                              (cusparseMatDescr_t)(*descrA),
                              (const cuDoubleComplex*) (*A),
                              *lda,
                              (int*) (*nnzPerRowCol),
                              nnzTotal);
}

int CUSPARSE_XCOO2CSR(ptr_t*       handle,
                      const ptr_t* cooRowInd,
                      int*         nnz,
                      int*         m,
                      ptr_t*       csrRowPtr,
                      int*         idxBase) {
    return (int) cusparseXcoo2csr((cusparseHandle_t)(*handle),
                                  (const int*) (*cooRowInd),
                                  *nnz,
                                  *m,
                                  (int*) (*csrRowPtr),
                                  (cusparseIndexBase_t)(*idxBase));
}

int CUSPARSE_XCSR2COO(ptr_t*       handle,
                      const ptr_t* csrRowPtr,
                      int*         nnz,
                      int*         m,
                      ptr_t*       cooRowInd,
                      int*         idxBase) {
    return (int) cusparseXcsr2coo((cusparseHandle_t)(*handle),
                                  (const int*) (*csrRowPtr),
                                  *nnz,
                                  *m,
                                  (int*) (*cooRowInd),
                                  (cusparseIndexBase_t)(*idxBase));
}
/*
int CUSPARSE_SCSR2CSC(ptr_t*       handle,
                      int*         m,
                      int*         n,
                      int*         nnz,
                      const ptr_t* csrVal,
                      const ptr_t* csrRowPtr,
                      const ptr_t* csrColInd,
                      ptr_t*       cscVal,
                      ptr_t*       cscRowInd,
                      ptr_t*       cscColPtr,
                      int*         copyValues,
                      int*         idxBase) {
    return (int) cusparseScsr2csc((cusparseHandle_t)(*handle),
                                  *m,
                                  *n,
                                  *nnz,
                                  (const float*) (*csrVal),
                                  (const int*) (*csrRowPtr),
                                  (const int*) (*csrColInd),
                                  (float*) (*cscVal),
                                  (int*) (*cscRowInd),
                                  (int*) (*cscColPtr),
                                  (cusparseAction_t)(*copyValues),
                                  (cusparseIndexBase_t)(*idxBase));
}

int CUSPARSE_DCSR2CSC(ptr_t*       handle,
                      int*         m,
                      int*         n,
                      int*         nnz,
                      const ptr_t* csrVal,
                      const ptr_t* csrRowPtr,
                      const ptr_t* csrColInd,
                      ptr_t*       cscVal,
                      ptr_t*       cscRowInd,
                      ptr_t*       cscColPtr,
                      int*         copyValues,
                      int*         idxBase) {
    return (int) cusparseDcsr2csc((cusparseHandle_t)(*handle),
                                  *m,
                                  *n,
                                  *nnz,
                                  (const double*) (*csrVal),
                                  (const int*) (*csrRowPtr),
                                  (const int*) (*csrColInd),
                                  (double*) (*cscVal),
                                  (int*) (*cscRowInd),
                                  (int*) (*cscColPtr),
                                  (cusparseAction_t)(*copyValues),
                                  (cusparseIndexBase_t)(*idxBase));
}

int CUSPARSE_CCSR2CSC(ptr_t*       handle,
                      int*         m,
                      int*         n,
                      int*         nnz,
                      const ptr_t* csrVal,
                      const ptr_t* csrRowPtr,
                      const ptr_t* csrColInd,
                      ptr_t*       cscVal,
                      ptr_t*       cscRowInd,
                      ptr_t*       cscColPtr,
                      int*         copyValues,
                      int*         idxBase) {
    return (int) cusparseCcsr2csc((cusparseHandle_t)(*handle),
                                  *m,
                                  *n,
                                  *nnz,
                                  (const cuComplex*) (*csrVal),
                                  (const int*) (*csrRowPtr),
                                  (const int*) (*csrColInd),
                                  (cuComplex*) (*cscVal),
                                  (int*) (*cscRowInd),
                                  (int*) (*cscColPtr),
                                  (cusparseAction_t)(*copyValues),
                                  (cusparseIndexBase_t)(*idxBase));
}

int CUSPARSE_ZCSR2CSC(ptr_t*       handle,
                      int*         m,
                      int*         n,
                      int*         nnz,
                      const ptr_t* csrVal,
                      const ptr_t* csrRowPtr,
                      const ptr_t* csrColInd,
                      ptr_t*       cscVal,
                      ptr_t*       cscRowInd,
                      ptr_t*       cscColPtr,
                      int*         copyValues,
                      int*         idxBase) {
    return (int) cusparseZcsr2csc((cusparseHandle_t)(*handle),
                                  *m,
                                  *n,
                                  *nnz,
                                  (const cuDoubleComplex*) (*csrVal),
                                  (const int*) (*csrRowPtr),
                                  (const int*) (*csrColInd),
                                  (cuDoubleComplex*) (*cscVal),
                                  (int*) (*cscRowInd),
                                  (int*) (*cscColPtr),
                                  (cusparseAction_t)(*copyValues),
                                  (cusparseIndexBase_t)(*idxBase));
}
*/
