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
 */

typedef size_t ptr_t;

/*
 * Example of Fortran callable thin wrappers for a few CUDA functions.
 */
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */

int CUDA_MALLOC(ptr_t *devPtr,
                int *size);

int CUDA_FREE(ptr_t *devPtr);

int CUDA_MEMCPY_FORT2C_INT(ptr_t *devPtr,
                           const ptr_t *hostPtr,
                           int *count,
                           int *kind);

int CUDA_MEMCPY_FORT2C_REAL(ptr_t *devPtr,
                            const ptr_t *hostPtr,
                            int *count,
                            int *kind);

int CUDA_MEMCPY_C2FORT_INT(ptr_t *devPtr,
                           const ptr_t *hostPtr,
                           int *count,
                           int *kind);

int CUDA_MEMCPY_C2FORT_REAL(ptr_t *devPtr,
                            const ptr_t *hostPtr,
                            int *count,
                            int *kind);

int CUDA_MEMSET(ptr_t *devPtr,
                int *value,
                int *count);

void GET_SHIFTED_ADDRESS(ptr_t *originPtr,
                         int *count,
                         ptr_t *resultPtr);

#if defined(__cplusplus)
}
#endif /* __cplusplus */

/*
 * Example of Fortran callable thin wrappers for CUSPARSE.
 * Fortran application must allocate and deallocate
 * GPU memory, and copy data up and down.
 */
#if defined(__cplusplus)
extern "C" {
#endif /* __cplusplus */
int CUSPARSE_CREATE(ptr_t *handle);

int CUSPARSE_DESTROY(ptr_t *handle);

int CUSPARSE_GET_VERSION(ptr_t *handle,
                         int *version);

int CUSPARSE_SET_STREAM(ptr_t *handle,
                        ptr_t *streamId);

int CUSPARSE_GET_POINTER_MODE(ptr_t *handle,
                              int *mode);

int CUSPARSE_SET_POINTER_MODE(ptr_t *handle,
                              int *mode);

int CUSPARSE_CREATE_MAT_DESCR(ptr_t *descrA);

int CUSPARSE_DESTROY_MAT_DESCR(ptr_t *descrA);

int CUSPARSE_SET_MAT_TYPE(ptr_t *descrA,
                          int *type);

int CUSPARSE_GET_MAT_TYPE(const ptr_t *descrA);

int CUSPARSE_SET_MAT_FILL_MODE(ptr_t *descrA,
                               int *fillMode);

int CUSPARSE_GET_MAT_FILL_MODE(const ptr_t *descrA);

int CUSPARSE_SET_MAT_DIAG_TYPE(ptr_t *descrA,
                               int *diagType);

int CUSPARSE_GET_MAT_DIAG_TYPE(const ptr_t *descrA);

int CUSPARSE_SET_MAT_INDEX_BASE(ptr_t *descrA,
                                int *base);

int CUSPARSE_GET_MAT_INDEX_BASE(const ptr_t *descrA);

int CUSPARSE_CREATE_SOLVE_ANALYSIS_INFO(ptr_t *info);

int CUSPARSE_DESTROY_SOLVE_ANALYSIS_INFO(ptr_t *info);

//int CUSPARSE_SAXPYI(ptr_t *handle,
//                    int *nnz,
//                    const float *alpha,
//                    const ptr_t *xVal,
//                    const ptr_t *xInd,
//                    ptr_t *y,
//                    int *idxBase);
//
//int CUSPARSE_DAXPYI(ptr_t *handle,
//                    int *nnz,
//                    const double *alpha,
//                    const ptr_t *xVal,
//                    const ptr_t *xInd,
//                    ptr_t *y,
//                    int *idxBase);
//
//int CUSPARSE_CAXPYI(ptr_t *handle,
//                    int *nnz,
//                    const cuComplex *alpha,
//                    const ptr_t *xVal,
//                    const ptr_t *xInd,
//                    ptr_t *y,
//                    int *idxBase);
//
//int CUSPARSE_ZAXPYI(ptr_t *handle,
//                    int *nnz,
//                    const cuDoubleComplex *alpha,
//                    const ptr_t *xVal,
//                    const ptr_t *xInd,
//                    ptr_t *y,
//                    int *idxBase);
//
//int CUSPARSE_SDOTI(ptr_t *handle,
//                   int   *nnz,
//                   const ptr_t *xVal,
//                   const ptr_t *xInd,
//                   const ptr_t *y,
//                   float *result,
//                   int *idxBase);
//
//int CUSPARSE_DDOTI(ptr_t *handle,
//                   int   *nnz,
//                   const ptr_t *xVal,
//                   const ptr_t *xInd,
//                   const ptr_t *y,
//                   double *result,
//                   int *idxBase);
//
//int CUSPARSE_CDOTI(ptr_t *handle,
//                   int   *nnz,
//                   const ptr_t *xVal,
//                   const ptr_t *xInd,
//                   const ptr_t *y,
//                   cuComplex *result,
//                   int *idxBase);
//
//int CUSPARSE_ZDOTI(ptr_t *handle,
//                   int   *nnz,
//                   const ptr_t *xVal,
//                   const ptr_t *xInd,
//                   const ptr_t *y,
//                   cuDoubleComplex *result,
//                   int *idxBase);
//
//int CUSPARSE_CDOTCI(ptr_t *handle,
//                    int   *nnz,
//                    const ptr_t *xVal,
//                    const ptr_t *xInd,
//                    const ptr_t *y,
//                    cuComplex *result,
//                    int *idxBase);
//
//int CUSPARSE_ZDOTCI(ptr_t *handle,
//                    int   *nnz,
//                    const ptr_t *xVal,
//                    const ptr_t *xInd,
//                    const ptr_t *y,
//                    cuDoubleComplex *result,
//                    int *idxBase);
//
//int CUSPARSE_SGTHR(ptr_t *handle,
//                   int *nnz,
//                   const ptr_t *y,
//                   ptr_t *xVal,
//                   const ptr_t *xInd,
//                   int *idxBase);
//
//int CUSPARSE_DGTHR(ptr_t *handle,
//                   int *nnz,
//                   const ptr_t *y,
//                   ptr_t *xVal,
//                   const ptr_t *xInd,
//                   int *idxBase);
//
//int CUSPARSE_CGTHR(ptr_t *handle,
//                   int *nnz,
//                   const ptr_t *y,
//                   ptr_t *xVal,
//                   const ptr_t *xInd,
//                   int *idxBase);
//
//int CUSPARSE_ZGTHR(ptr_t *handle,
//                   int *nnz,
//                   const ptr_t *y,
//                   ptr_t *xVal,
//                   const ptr_t *xInd,
//                   int *idxBase);
//
//int CUSPARSE_SGTHRZ(ptr_t *handle,
//                    int *nnz,
//                    ptr_t *y,
//                    ptr_t *xVal,
//                    const ptr_t *xInd,
//                    int *idxBase);
//
//int CUSPARSE_DGTHRZ(ptr_t *handle,
//                    int *nnz,
//                    ptr_t *y,
//                    ptr_t *xVal,
//                    const ptr_t *xInd,
//                    int *idxBase);
//
//int CUSPARSE_CGTHRZ(ptr_t *handle,
//                    int *nnz,
//                    ptr_t *y,
//                    ptr_t *xVal,
//                    const ptr_t *xInd,
//                    int *idxBase);
//
//int CUSPARSE_ZGTHRZ(ptr_t *handle,
//                    int *nnz,
//                    ptr_t *y,
//                    ptr_t *xVal,
//                    const ptr_t *xInd,
//                    int *idxBase);
//
//int CUSPARSE_SSCTR(ptr_t *handle,
//                   int *nnz,
//                   const ptr_t *xVal,
//                   const ptr_t *xInd,
//                   ptr_t *y,
//                   int *idxBase);
//
//int CUSPARSE_DSCTR(ptr_t *handle,
//                   int *nnz,
//                   const ptr_t *xVal,
//                   const ptr_t *xInd,
//                   ptr_t *y,
//                   int *idxBase);
//
//int CUSPARSE_CSCTR(ptr_t *handle,
//                   int *nnz,
//                   const ptr_t *xVal,
//                   const ptr_t *xInd,
//                   ptr_t *y,
//                   int *idxBase);
//
//int CUSPARSE_ZSCTR(ptr_t *handle,
//                   int *nnz,
//                   const ptr_t *xVal,
//                   const ptr_t *xInd,
//                   ptr_t *y,
//                   int *idxBase);
//
//
//int CUSPARSE_SROTI(ptr_t *handle,
//                   int *nnz,
//                   ptr_t *xVal,
//                   const ptr_t *xInd,
//                   ptr_t *y,
//                   const float *c,
//                   const float *s,
//                   int *idxBase);
//
//int CUSPARSE_DROTI(ptr_t *handle,
//                   int *nnz,
//                   ptr_t *xVal,
//                   const ptr_t *xInd,
//                   ptr_t *y,
//                   const double *c,
//                   const double *s,
//                   int *idxBase);

//int CUSPARSE_SCSRMV(ptr_t *handle,
//                    int *transA,
//                    int *m,
//                    int *n,
//                    int *nnz,
//                    const float *alpha,
//                    const ptr_t *descrA,
//                    const ptr_t *csrValA,
//                    const ptr_t *csrRowPtrA,
//                    const ptr_t *csrColIndA,
//                    const ptr_t *x,
//                    const float *beta,
//                    ptr_t *y);

//int CUSPARSE_DCSRMV(ptr_t *handle,
//                    int *transA,
//                    int *m,
//                    int *n,
//                    int *nnz,
//                    const double *alpha,
//                    const ptr_t *descrA,
//                    const ptr_t *csrValA,
//                    const ptr_t *csrRowPtrA,
//                    const ptr_t *csrColIndA,
//                    const ptr_t *x,
//                    const double *beta,
//                    ptr_t *y);

//int CUSPARSE_CCSRMV(ptr_t *handle,
//                    int *transA,
//                    int *m,
//                    int *n,
//                    int *nnz,
//                    const cuComplex *alpha,
//                    const ptr_t *descrA,
//                    const ptr_t *csrValA,
//                    const ptr_t *csrRowPtrA,
//                    const ptr_t *csrColIndA,
//                    const ptr_t *x,
//                    const cuComplex *beta,
//                    ptr_t *y);

//int CUSPARSE_ZCSRMV(ptr_t *handle,
//                    int *transA,
//                    int *m,
//                    int *n,
//                    int *nnz,
//                    const cuDoubleComplex *alpha,
//                    const ptr_t *descrA,
//                    const ptr_t *csrValA,
//                    const ptr_t *csrRowPtrA,
//                    const ptr_t *csrColIndA,
//                    const ptr_t *x,
//                    const cuDoubleComplex *beta,
//                    ptr_t *y);

//int CUSPARSE_SCSRSV_ANALYSIS(ptr_t *handle,
//                             int *transA,
//                             int *m,
//                             int *nnz,
//                             const ptr_t *descrA,
//                             const ptr_t *csrValA,
//                             const ptr_t *csrRowPtrA,
//                             const ptr_t *csrColIndA,
//                             ptr_t *info);

//int CUSPARSE_DCSRSV_ANALYSIS(ptr_t *handle,
//                             int *transA,
//                             int *m,
//                             int *nnz,
//                             const ptr_t *descrA,
//                             const ptr_t *csrValA,
//                             const ptr_t *csrRowPtrA,
//                             const ptr_t *csrColIndA,
//                             ptr_t *info);

//int CUSPARSE_CCSRSV_ANALYSIS(ptr_t *handle,
//                             int *transA,
//                             int *m,
//                             int *nnz,
//                             const ptr_t *descrA,
//                             const ptr_t *csrValA,
//                             const ptr_t *csrRowPtrA,
//                             const ptr_t *csrColIndA,
//                             ptr_t *info);

//int CUSPARSE_ZCSRSV_ANALYSIS(ptr_t *handle,
//                             int *transA,
//                             int *m,
//                             int *nnz,
//                             const ptr_t *descrA,
//                             const ptr_t *csrValA,
//                             const ptr_t *csrRowPtrA,
//                             const ptr_t *csrColIndA,
//                             ptr_t *info);


//int CUSPARSE_SCSRSV_SOLVE(ptr_t *handle,
//                          int *transA,
//                          int *m,
//                          const float *alpha,
//                          const ptr_t *descrA,
//                          const ptr_t *csrValA,
//                          const ptr_t *csrRowPtrA,
//                          const ptr_t *csrColIndA,
//                          ptr_t *info,
//                          const ptr_t *x,
//                          ptr_t *y);

//int CUSPARSE_DCSRSV_SOLVE(ptr_t *handle,
//                          int *transA,
//                          int *m,
//                          const double *alpha,
//                          const ptr_t *descrA,
//                          const ptr_t *csrValA,
//                          const ptr_t *csrRowPtrA,
//                          const ptr_t *csrColIndA,
//                          ptr_t *info,
//                          const ptr_t *x,
//                          ptr_t *y);

//int CUSPARSE_CCSRSV_SOLVE(ptr_t *handle,
//                          int *transA,
//                          int *m,
//                          const cuComplex *alpha,
//                          const ptr_t *descrA,
//                          const ptr_t *csrValA,
//                          const ptr_t *csrRowPtrA,
//                          const ptr_t *csrColIndA,
//                          ptr_t *info,
//                          const ptr_t *x,
//                          ptr_t *y);

//int CUSPARSE_ZCSRSV_SOLVE(ptr_t *handle,
//                          int *transA,
//                          int *m,
//                          const cuDoubleComplex *alpha,
//                          const ptr_t *descrA,
//                          const ptr_t *csrValA,
//                          const ptr_t *csrRowPtrA,
//                          const ptr_t *csrColIndA,
//                          ptr_t *info,
//                          const ptr_t *x,
//                          ptr_t *y);

//int CUSPARSE_SCSRMM(ptr_t *handle,
//                    int *transA,
//                    int *m,
//                    int *n,
//                    int *k,
//                    int *nnz,
//                    const float *alpha,
//                    const ptr_t *descrA,
//                    const ptr_t *csrValA,
//                    const ptr_t *csrRowPtrA,
//                    const ptr_t *csrColIndA,
//                    const ptr_t *B,
//                    int *ldb,
//                    const float *beta,
//                    ptr_t *C,
//                    int *ldc);
//
//int CUSPARSE_DCSRMM(ptr_t *handle,
//                    int *transA,
//                    int *m,
//                    int *n,
//                    int *k,
//                    int *nnz,
//                    const double *alpha,
//                    const ptr_t *descrA,
//                    const ptr_t *csrValA,
//                    const ptr_t *csrRowPtrA,
//                    const ptr_t *csrColIndA,
//                    const ptr_t *B,
//                    int *ldb,
//                    const double *beta,
//                    ptr_t *C,
//                    int *ldc);
//
//int CUSPARSE_CCSRMM(ptr_t *handle,
//                    int *transA,
//                    int *m,
//                    int *n,
//                    int *k,
//                    int *nnz,
//                    const cuComplex *alpha,
//                    const ptr_t *descrA,
//                    const ptr_t *csrValA,
//                    const ptr_t *csrRowPtrA,
//                    const ptr_t *csrColIndA,
//                    const ptr_t *B,
//                    int *ldb,
//                    const cuComplex *beta,
//                    ptr_t *C,
//                    int *ldc);
//
//int CUSPARSE_ZCSRMM(ptr_t *handle,
//                    int *transA,
//                    int *m,
//                    int *n,
//                    int *k,
//                    int *nnz,
//                    const cuDoubleComplex *alpha,
//                    const ptr_t *descrA,
//                    const ptr_t *csrValA,
//                    const ptr_t *csrRowPtrA,
//                    const ptr_t *csrColIndA,
//                    const ptr_t *B,
//                    int *ldb,
//                    const cuDoubleComplex *beta,
//                    ptr_t *C,
//                    int *ldc);

int CUSPARSE_SCSRSM_ANALYSIS(ptr_t *handle,
                             int *transA,
                             int *m,
                             int *nnz,
                             const ptr_t *descrA,
                             const ptr_t *csrValA,
                             const ptr_t *csrRowPtrA,
                             const ptr_t *csrColIndA,
                             ptr_t *info);

int CUSPARSE_DCSRSM_ANALYSIS(ptr_t *handle,
                             int *transA,
                             int *m,
                             int *nnz,
                             const ptr_t *descrA,
                             const ptr_t *csrValA,
                             const ptr_t *csrRowPtrA,
                             const ptr_t *csrColIndA,
                             ptr_t *info);

int CUSPARSE_CCSRSM_ANALYSIS(ptr_t *handle,
                             int *transA,
                             int *m,
                             int *nnz,
                             const ptr_t *descrA,
                             const ptr_t *csrValA,
                             const ptr_t *csrRowPtrA,
                             const ptr_t *csrColIndA,
                             ptr_t *info);

int CUSPARSE_ZCSRSM_ANALYSIS(ptr_t *handle,
                             int *transA,
                             int *m,
                             int *nnz,
                             const ptr_t *descrA,
                             const ptr_t *csrValA,
                             const ptr_t *csrRowPtrA,
                             const ptr_t *csrColIndA,
                             ptr_t *info);

int CUSPARSE_SCSRSM_SOLVE(ptr_t *handle,
                          int *transA,
                          int *m,
                          int *n,
                          const float *alpha,
                          const ptr_t *descrA,
                          const ptr_t *csrValA,
                          const ptr_t *csrRowPtrA,
                          const ptr_t *csrColIndA,
                          ptr_t *info,
                          const ptr_t *x,
                          int *ldx,
                          ptr_t *y,
                          int *ldy);

int CUSPARSE_DCSRSM_SOLVE(ptr_t *handle,
                          int *transA,
                          int *m,
                          int *n,
                          const double *alpha,
                          const ptr_t *descrA,
                          const ptr_t *csrValA,
                          const ptr_t *csrRowPtrA,
                          const ptr_t *csrColIndA,
                          ptr_t *info,
                          const ptr_t *x,
                          int *ldx,
                          ptr_t *y,
                          int *ldy);

int CUSPARSE_CCSRSM_SOLVE(ptr_t *handle,
                          int *transA,
                          int *m,
                          int *n,
                          const cuComplex *alpha,
                          const ptr_t *descrA,
                          const ptr_t *csrValA,
                          const ptr_t *csrRowPtrA,
                          const ptr_t *csrColIndA,
                          ptr_t *info,
                          const ptr_t *x,
                          int *ldx,
                          ptr_t *y,
                          int *ldy);

int CUSPARSE_ZCSRSM_SOLVE(ptr_t *handle,
                          int *transA,
                          int *m,
                          int *n,
                          const cuDoubleComplex *alpha,
                          const ptr_t *descrA,
                          const ptr_t *csrValA,
                          const ptr_t *csrRowPtrA,
                          const ptr_t *csrColIndA,
                          ptr_t *info,
                          const ptr_t *x,
                          int *ldx,
                          ptr_t *y,
                          int *ldy);

int CUSPARSE_SGTSV(ptr_t *handle,
                   int *m,
                   int *n,
                   const ptr_t *dl,
                   const ptr_t *d,
                   const ptr_t *du,
                   ptr_t *B,
                   int *ldb);

int CUSPARSE_DGTSV(ptr_t *handle,
                   int *m,
                   int *n,
                   const ptr_t *dl,
                   const ptr_t *d,
                   const ptr_t *du,
                   ptr_t *B,
                   int *ldb);

int CUSPARSE_CGTSV(ptr_t *handle,
                   int *m,
                   int *n,
                   const ptr_t *dl,
                   const ptr_t *d,
                   const ptr_t *du,
                   ptr_t *B,
                   int *ldb);

int CUSPARSE_ZGTSV(ptr_t *handle,
                   int *m,
                   int *n,
                   const ptr_t *dl,
                   const ptr_t *d,
                   const ptr_t *du,
                   ptr_t *B,
                   int *ldb);

int CUSPARSE_SGTSV_STRIDEDBATCH(ptr_t *handle,
                                int *m,
                                const ptr_t *dl,
                                const ptr_t *d,
                                const ptr_t *du,
                                ptr_t *x,
                                int *batchCount,
                                int *batchStride);

int CUSPARSE_DGTSV_STRIDEDBATCH(ptr_t *handle,
                                int *m,
                                const ptr_t *dl,
                                const ptr_t *d,
                                const ptr_t *du,
                                ptr_t *x,
                                int *batchCount,
                                int *batchStride);

int CUSPARSE_CGTSV_STRIDEDBATCH(ptr_t *handle,
                                int *m,
                                const ptr_t *dl,
                                const ptr_t *d,
                                const ptr_t *du,
                                ptr_t *x,
                                int *batchCount,
                                int *batchStride);

int CUSPARSE_ZGTSV_STRIDEDBATCH(ptr_t *handle,
                                int *m,
                                const ptr_t *dl,
                                const ptr_t *d,
                                const ptr_t *du,
                                ptr_t *x,
                                int *batchCount,
                                int *batchStride);

int CUSPARSE_SNNZ(ptr_t *handle,
                  int *dirA,
                  int *m,
                  int *n,
                  const ptr_t *descrA,
                  const ptr_t *A,
                  int *lda,
                  ptr_t *nnzPerRowCol,
                  int *nnzTotal);

int CUSPARSE_DNNZ(ptr_t *handle,
                  int *dirA,
                  int *m,
                  int *n,
                  const ptr_t *descrA,
                  const ptr_t *A,
                  int *lda,
                  ptr_t *nnzPerRowCol,
                  int *nnzTotal);

int CUSPARSE_CNNZ(ptr_t *handle,
                  int *dirA,
                  int *m,
                  int *n,
                  const ptr_t *descrA,
                  const ptr_t *A,
                  int *lda,
                  ptr_t *nnzPerRowCol,
                  int *nnzTotal);

int CUSPARSE_ZNNZ(ptr_t *handle,
                  int *dirA,
                  int *m,
                  int *n,
                  const ptr_t *descrA,
                  const ptr_t *A,
                  int *lda,
                  ptr_t *nnzPerRowCol,
                  int *nnzTotal);


int CUSPARSE_SDENSE2CSR(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *A,
                        int *lda,
                        const ptr_t *nnzPerRow,
                        ptr_t *csrValA,
                        ptr_t *csrRowPtrA,
                        ptr_t *csrColIndA);

int CUSPARSE_DDENSE2CSR(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *A,
                        int *lda,
                        const ptr_t *nnzPerRow,
                        ptr_t *csrValA,
                        ptr_t *csrRowPtrA,
                        ptr_t *csrColIndA);

int CUSPARSE_CDENSE2CSR(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *A,
                        int *lda,
                        const ptr_t *nnzPerRow,
                        ptr_t *csrValA,
                        ptr_t *csrRowPtrA,
                        ptr_t *csrColIndA);

int CUSPARSE_ZDENSE2CSR(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *A,
                        int *lda,
                        const ptr_t *nnzPerRow,
                        ptr_t *csrValA,
                        ptr_t *csrRowPtrA,
                        ptr_t *csrColIndA);


int CUSPARSE_SCSR2DENSE(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *csrValA,
                        const ptr_t *csrRowPtrA,
                        const ptr_t *csrColIndA,
                        ptr_t *A,
                        int *lda);

int CUSPARSE_DCSR2DENSE(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *csrValA,
                        const ptr_t *csrRowPtrA,
                        const ptr_t *csrColIndA,
                        ptr_t *A,
                        int *lda);

int CUSPARSE_CCSR2DENSE(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *csrValA,
                        const ptr_t *csrRowPtrA,
                        const ptr_t *csrColIndA,
                        ptr_t *A,
                        int *lda);

int CUSPARSE_ZCSR2DENSE(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *csrValA,
                        const ptr_t *csrRowPtrA,
                        const ptr_t *csrColIndA,
                        ptr_t *A,
                        int *lda);

int CUSPARSE_SDENSE2CSC(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *A,
                        int *lda,
                        const ptr_t *nnzPerCol,
                        ptr_t *cscValA,
                        ptr_t *cscRowIndA,
                        ptr_t *cscColPtrA);

int CUSPARSE_DDENSE2CSC(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *A,
                        int *lda,
                        const ptr_t *nnzPerCol,
                        ptr_t *cscValA,
                        ptr_t *cscRowIndA,
                        ptr_t *cscColPtrA);

int CUSPARSE_CDENSE2CSC(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *A,
                        int *lda,
                        const ptr_t *nnzPerCol,
                        ptr_t *cscValA,
                        ptr_t *cscRowIndA,
                        ptr_t *cscColPtrA);

int CUSPARSE_ZDENSE2CSC(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *A,
                        int *lda,
                        const ptr_t *nnzPerCol,
                        ptr_t *cscValA,
                        ptr_t *cscRowIndA,
                        ptr_t *cscColPtrA);

int CUSPARSE_SCSC2DENSE(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *cscValA,
                        const ptr_t *cscRowIndA,
                        const ptr_t *cscColPtrA,
                        ptr_t *A,
                        int *lda);

int CUSPARSE_DCSC2DENSE(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *cscValA,
                        const ptr_t *cscRowIndA,
                        const ptr_t *cscColPtrA,
                        ptr_t *A,
                        int *lda);

int CUSPARSE_CCSC2DENSE(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *cscValA,
                        const ptr_t *cscRowIndA,
                        const ptr_t *cscColPtrA,
                        ptr_t *A,
                        int *lda);

int CUSPARSE_ZCSC2DENSE(ptr_t *handle,
                        int *m,
                        int *n,
                        const ptr_t *descrA,
                        const ptr_t *cscValA,
                        const ptr_t *cscRowIndA,
                        const ptr_t *cscColPtrA,
                        ptr_t *A,
                        int *lda);

int CUSPARSE_XCOO2CSR(ptr_t *handle,
                      const ptr_t *cooRowInd,
                      int *nnz,
                      int *m,
                      ptr_t *csrRowPtr,
                      int *idxBase);

int CUSPARSE_XCSR2COO(ptr_t *handle,
                      const ptr_t *csrRowPtr,
                      int *nnz,
                      int *m,
                      ptr_t *cooRowInd,
                      int *idxBase);
/*
int CUSPARSE_SCSR2CSC(ptr_t *handle,
                      int *m,
                      int *n,
                      int *nnz,
                      const ptr_t *csrVal,
                      const ptr_t *csrRowPtr,
                      const ptr_t *csrColInd,
                      ptr_t *cscVal,
                      ptr_t *cscRowInd,
                      ptr_t *cscColPtr,
                      int *copyValues,
                      int *idxBase);

int CUSPARSE_DCSR2CSC(ptr_t *handle,
                      int *m,
                      int *n,
                      int *nnz,
                      const ptr_t *csrVal,
                      const ptr_t *csrRowPtr,
                      const ptr_t *csrColInd,
                      ptr_t *cscVal,
                      ptr_t *cscRowInd,
                      ptr_t *cscColPtr,
                      int *copyValues,
                      int *idxBase);

int CUSPARSE_CCSR2CSC(ptr_t *handle,
                      int *m,
                      int *n,
                      int *nnz,
                      const ptr_t *csrVal,
                      const ptr_t *csrRowPtr,
                      const ptr_t *csrColInd,
                      ptr_t *cscVal,
                      ptr_t *cscRowInd,
                      ptr_t *cscColPtr,
                      int *copyValues,
                      int *idxBase);

int CUSPARSE_ZCSR2CSC(ptr_t *handle,
                      int *m,
                      int *n,
                      int *nnz,
                      const ptr_t *csrVal,
                      const ptr_t *csrRowPtr,
                      const ptr_t *csrColInd,
                      ptr_t *cscVal,
                      ptr_t *cscRowInd,
                      ptr_t *cscColPtr,
                      int *copyValues,
                      int *idxBase);
*/
#if defined(__cplusplus)
}
#endif /* __cplusplus */
