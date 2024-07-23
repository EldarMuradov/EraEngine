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

#define CUSPARSE_G77              1
#define CUSPARSE_INTEL_FORTRAN    2
#define CUSPARSE_G95              3

/* Default to g77 on Linux, and Intel Fortran on Win32 */
#if defined(_WIN32)
#define CUSPARSE_FORTRAN_COMPILER CUSPARSE_INTEL_FORTRAN
#elif defined(__linux) || defined(__QNX__)
#define CUSPARSE_FORTRAN_COMPILER CUSPARSE_G95
#elif defined(__APPLE__)
#define CUSPARSE_FORTRAN_COMPILER CUSPARSE_G95
#define RETURN_COMPLEX   1
#else
#error unsupported platform
#endif

#if (CUSPARSE_FORTRAN_COMPILER==CUSPARSE_G77) || (CUSPARSE_FORTRAN_COMPILER==CUSPARSE_G95)
/* NOTE: Must use -fno-second-underscore when building Fortran source with g77
 *       g77 invocation may not use -fno-f2c, which forces different return
 *       type conventions than the one used below
 */
#define CUDA_MALLOC                 cuda_malloc_
#define CUDA_FREE                   cuda_free_
#define CUDA_MEMCPY_FORT2C_INT      cuda_memcpy_fort2c_int_
#define CUDA_MEMCPY_FORT2C_REAL     cuda_memcpy_fort2c_real_
#define CUDA_MEMCPY_C2FORT_INT      cuda_memcpy_c2fort_int_
#define CUDA_MEMCPY_C2FORT_REAL     cuda_memcpy_c2fort_real_
#define CUDA_MEMSET                 cuda_memset_
#define GET_SHIFTED_ADDRESS         get_shifted_address_

#define CUSPARSE_CREATE             cusparse_create_
#define CUSPARSE_DESTROY            cusparse_destroy_
#define CUSPARSE_GET_VERSION        cusparse_get_version_
#define CUSPARSE_SET_STREAM         cusparse_set_stream_
#define CUSPARSE_SET_POINTER_MODE   cusparse_set_pointer_mode_
#define CUSPARSE_GET_POINTER_MODE   cusparse_get_pointer_mode_
#define CUSPARSE_CREATE_MAT_DESCR   cusparse_create_mat_descr_
#define CUSPARSE_DESTROY_MAT_DESCR  cusparse_destroy_mat_descr_
#define CUSPARSE_SET_MAT_TYPE       cusparse_set_mat_type_
#define CUSPARSE_GET_MAT_TYPE       cusparse_get_mat_type_
#define CUSPARSE_SET_MAT_FILL_MODE  cusparse_set_mat_fill_mode_
#define CUSPARSE_GET_MAT_FILL_MODE  cusparse_get_mat_fill_mode_
#define CUSPARSE_SET_MAT_DIAG_TYPE  cusparse_set_mat_diag_type_
#define CUSPARSE_GET_MAT_DIAG_TYPE  cusparse_get_mat_diag_type_
#define CUSPARSE_SET_MAT_INDEX_BASE cusparse_set_mat_index_base_
#define CUSPARSE_GET_MAT_INDEX_BASE cusparse_get_mat_index_base_
#define CUSPARSE_CREATE_SOLVE_ANALYSIS_INFO cusparse_create_solve_analysis_info_
#define CUSPARSE_DESTROY_SOLVE_ANALYSIS_INFO cusparse_destroy_solve_analysis_info_
//#define CUSPARSE_SAXPYI             cusparse_saxpyi_
//#define CUSPARSE_DAXPYI             cusparse_daxpyi_
//#define CUSPARSE_CAXPYI             cusparse_caxpyi_
//#define CUSPARSE_ZAXPYI             cusparse_zaxpyi_
//#define CUSPARSE_SGTHR              cusparse_sgthr_
//#define CUSPARSE_DGTHR              cusparse_dgthr_
//#define CUSPARSE_CGTHR              cusparse_cgthr_
//#define CUSPARSE_ZGTHR              cusparse_zgthr_
//#define CUSPARSE_SGTHRZ             cusparse_sgthrz_
//#define CUSPARSE_DGTHRZ             cusparse_dgthrz_
//#define CUSPARSE_CGTHRZ             cusparse_cgthrz_
//#define CUSPARSE_ZGTHRZ             cusparse_zgthrz_
//#define CUSPARSE_SSCTR              cusparse_ssctr_
//#define CUSPARSE_DSCTR              cusparse_dsctr_
//#define CUSPARSE_CSCTR              cusparse_csctr_
//#define CUSPARSE_ZSCTR              cusparse_zsctr_
//#define CUSPARSE_SROTI              cusparse_sroti_
//#define CUSPARSE_DROTI              cusparse_droti_
//#define CUSPARSE_SCSRMV             cusparse_scsrmv_
//#define CUSPARSE_DCSRMV             cusparse_dcsrmv_
//#define CUSPARSE_CCSRMV             cusparse_ccsrmv_
//#define CUSPARSE_ZCSRMV             cusparse_zcsrmv_
//#define CUSPARSE_SCSRSV_ANALYSIS    cusparse_scsrsv_analysis_
//#define CUSPARSE_DCSRSV_ANALYSIS    cusparse_dcsrsv_analysis_
//#define CUSPARSE_CCSRSV_ANALYSIS    cusparse_ccsrsv_analysis_
//#define CUSPARSE_ZCSRSV_ANALYSIS    cusparse_zcsrsv_analysis_
//#define CUSPARSE_SCSRSV_SOLVE       cusparse_scsrsv_solve_
//#define CUSPARSE_DCSRSV_SOLVE       cusparse_dcsrsv_solve_
//#define CUSPARSE_CCSRSV_SOLVE       cusparse_ccsrsv_solve_
//#define CUSPARSE_ZCSRSV_SOLVE       cusparse_zcsrsv_solve_
//#define CUSPARSE_SCSRMM             cusparse_scsrmm_
//#define CUSPARSE_DCSRMM             cusparse_dcsrmm_
//#define CUSPARSE_CCSRMM             cusparse_ccsrmm_
//#define CUSPARSE_ZCSRMM             cusparse_zcsrmm_
//#define CUSPARSE_SCSRSM_ANALYSIS    cusparse_scsrsm_analysis_
//#define CUSPARSE_DCSRSM_ANALYSIS    cusparse_dcsrsm_analysis_
//#define CUSPARSE_CCSRSM_ANALYSIS    cusparse_ccsrsm_analysis_
//#define CUSPARSE_ZCSRSM_ANALYSIS    cusparse_zcsrsm_analysis_
//#define CUSPARSE_SCSRSM_SOLVE       cusparse_scsrsm_solve_
//#define CUSPARSE_DCSRSM_SOLVE       cusparse_dcsrsm_solve_
//#define CUSPARSE_CCSRSM_SOLVE       cusparse_ccsrsm_solve_
//#define CUSPARSE_ZCSRSM_SOLVE       cusparse_zcsrsm_solve_
#define CUSPARSE_SGTSV              cusparse_sgtsv_
#define CUSPARSE_DGTSV              cusparse_dgtsv_
#define CUSPARSE_CGTSV              cusparse_cgtsv_
#define CUSPARSE_ZGTSV              cusparse_zgtsv_
#define CUSPARSE_SGTSV_STRIDEDBATCH cusparse_sgtsv_stridedbatch_
#define CUSPARSE_DGTSV_STRIDEDBATCH cusparse_dgtsv_stridedbatch_
#define CUSPARSE_CGTSV_STRIDEDBATCH cusparse_cgtsv_stridedbatch_
#define CUSPARSE_ZGTSV_STRIDEDBATCH cusparse_zgtsv_stridedbatch_
#define CUSPARSE_SNNZ               cusparse_snnz_
#define CUSPARSE_DNNZ               cusparse_dnnz_
#define CUSPARSE_CNNZ               cusparse_cnnz_
#define CUSPARSE_ZNNZ               cusparse_znnz_
#define CUSPARSE_SDENSE2CSR         cusparse_sdense2csr_
#define CUSPARSE_DDENSE2CSR         cusparse_ddense2csr_
#define CUSPARSE_CDENSE2CSR         cusparse_cdense2csr_
#define CUSPARSE_ZDENSE2CSR         cusparse_zdense2csr_
#define CUSPARSE_SCSR2DENSE         cusparse_scsr2dense_
#define CUSPARSE_DCSR2DENSE         cusparse_dcsr2dense_
#define CUSPARSE_CCSR2DENSE         cusparse_ccsr2dense_
#define CUSPARSE_ZCSR2DENSE         cusparse_zcsr2dense_
#define CUSPARSE_SDENSE2CSC         cusparse_sdense2csc_
#define CUSPARSE_DDENSE2CSC         cusparse_ddense2csc_
#define CUSPARSE_CDENSE2CSC         cusparse_cdense2csc_
#define CUSPARSE_ZDENSE2CSC         cusparse_zdense2csc_
#define CUSPARSE_SCSC2DENSE         cusparse_scsc2dense_
#define CUSPARSE_DCSC2DENSE         cusparse_dcsc2dense_
#define CUSPARSE_CCSC2DENSE         cusparse_ccsc2dense_
#define CUSPARSE_ZCSC2DENSE         cusparse_zcsc2dense_
#define CUSPARSE_XCOO2CSR           cusparse_xcoo2csr_
#define CUSPARSE_XCSR2COO           cusparse_xcsr2coo_
//#define CUSPARSE_SCSR2CSC           cusparse_scsr2csc_
//#define CUSPARSE_DCSR2CSC           cusparse_dcsr2csc_
//#define CUSPARSE_CCSR2CSC           cusparse_ccsr2csc_
//#define CUSPARSE_ZCSR2CSC           cusparse_zcsr2csc_

#elif CUSPARSE_FORTRAN_COMPILER==CUSPARSE_INTEL_FORTRAN

#define CUDA_MALLOC                 CUDA_MALLOC
#define CUDA_FREE                   CUDA_FREE
#define CUDA_MEMCPY_FORT2C_INT      CUDA_MEMCPY_FORT2C_INT
#define CUDA_MEMCPY_FORT2C_REAL     CUDA_MEMCPY_FORT2C_REAL
#define CUDA_MEMCPY_C2FORT_INT      CUDA_MEMCPY_C2FORT_INT
#define CUDA_MEMCPY_C2FORT_REAL     CUDA_MEMCPY_C2FORT_REAL
#define CUDA_MEMSET                 CUDA_MEMSET
#define GET_SHIFTED_ADDRESS         GET_SHIFTED_ADDRESS

#define CUSPARSE_CREATE             CUSPARSE_CREATE
#define CUSPARSE_DESTROY            CUSPARSE_DESTROY
#define CUSPARSE_GET_VERSION        CUSPARSE_GET_VERSION
#define CUSPARSE_SET_STREAM         CUSPARSE_SET_STREAM
#define CUSPARSE_SET_POINTER_MODE   CUSPARSE_SET_POINTER_MODE
#define CUSPARSE_GET_POINTER_MODE   CUSPARSE_GET_POINTER_MODE
#define CUSPARSE_CREATE_MAT_DESCR   CUSPARSE_CREATE_MAT_DESCR
#define CUSPARSE_DESTROY_MAT_DESCR  CUSPARSE_DESTROY_MAT_DESCR
#define CUSPARSE_SET_MAT_TYPE       CUSPARSE_SET_MAT_TYPE
#define CUSPARSE_GET_MAT_TYPE       CUSPARSE_GET_MAT_TYPE
#define CUSPARSE_SET_MAT_FILL_MODE  CUSPARSE_SET_MAT_FILL_MODE
#define CUSPARSE_GET_MAT_FILL_MODE  CUSPARSE_GET_MAT_FILL_MODE
#define CUSPARSE_SET_MAT_DIAG_TYPE  CUSPARSE_SET_MAT_DIAG_TYPE
#define CUSPARSE_GET_MAT_DIAG_TYPE  CUSPARSE_GET_MAT_DIAG_TYPE
#define CUSPARSE_SET_MAT_INDEX_BASE CUSPARSE_SET_MAT_INDEX_BASE
#define CUSPARSE_GET_MAT_INDEX_BASE CUSPARSE_GET_MAT_INDEX_BASE
#define CUSPARSE_CREATE_SOLVE_ANALYSIS_INFO CUSPARSE_CREATE_SOLVE_ANALYSIS_INFO
#define CUSPARSE_DESTROY_SOLVE_ANALYSIS_INFO CUSPARSE_DESTROY_SOLVE_ANALYSIS_INFO
//#define CUSPARSE_SAXPYI             CUSPARSE_SAXPYI
//#define CUSPARSE_DAXPYI             CUSPARSE_DAXPYI
//#define CUSPARSE_CAXPYI             CUSPARSE_CAXPYI
//#define CUSPARSE_ZAXPYI             CUSPARSE_ZAXPYI
//#define CUSPARSE_SDOTI              CUSPARSE_SDOTI
//#define CUSPARSE_DDOTI              CUSPARSE_DDOTI
//#define CUSPARSE_CDOTI              CUSPARSE_CDOTI
//#define CUSPARSE_ZDOTI              CUSPARSE_ZDOTI
//#define CUSPARSE_CDOTCI             CUSPARSE_CDOTCI
//#define CUSPARSE_ZDOTCI             CUSPARSE_ZDOTCI
//#define CUSPARSE_SGTHR              CUSPARSE_SGTHR
//#define CUSPARSE_DGTHR              CUSPARSE_DGTHR
//#define CUSPARSE_CGTHR              CUSPARSE_CGTHR
//#define CUSPARSE_ZGTHR              CUSPARSE_ZGTHR
//#define CUSPARSE_SGTHRZ             CUSPARSE_SGTHRZ
//#define CUSPARSE_DGTHRZ             CUSPARSE_DGTHRZ
//#define CUSPARSE_CGTHRZ             CUSPARSE_CGTHRZ
//#define CUSPARSE_ZGTHRZ             CUSPARSE_ZGTHRZ
//#define CUSPARSE_SSCTR              CUSPARSE_SSCTR
//#define CUSPARSE_DSCTR              CUSPARSE_DSCTR
//#define CUSPARSE_CSCTR              CUSPARSE_CSCTR
//#define CUSPARSE_ZSCTR              CUSPARSE_ZSCTR
//#define CUSPARSE_SROTI              CUSPARSE_SROTI
//#define CUSPARSE_DROTI              CUSPARSE_DROTI
//#define CUSPARSE_SCSRMV             CUSPARSE_SCSRMV
//#define CUSPARSE_DCSRMV             CUSPARSE_DCSRMV
//#define CUSPARSE_CCSRMV             CUSPARSE_CCSRMV
//#define CUSPARSE_ZCSRMV             CUSPARSE_ZCSRMV
//#define CUSPARSE_SCSRSV_ANALYSIS    CUSPARSE_SCSRSV_ANALYSIS
//#define CUSPARSE_DCSRSV_ANALYSIS    CUSPARSE_DCSRSV_ANALYSIS
//#define CUSPARSE_CCSRSV_ANALYSIS    CUSPARSE_CCSRSV_ANALYSIS
//#define CUSPARSE_ZCSRSV_ANALYSIS    CUSPARSE_ZCSRSV_ANALYSIS
//#define CUSPARSE_SCSRSV_SOLVE       CUSPARSE_SCSRSV_SOLVE
//#define CUSPARSE_DCSRSV_SOLVE       CUSPARSE_DCSRSV_SOLVE
//#define CUSPARSE_CCSRSV_SOLVE       CUSPARSE_CCSRSV_SOLVE
//#define CUSPARSE_ZCSRSV_SOLVE       CUSPARSE_ZCSRSV_SOLVE
//#define CUSPARSE_SCSRMM             CUSPARSE_SCSRMM
//#define CUSPARSE_DCSRMM             CUSPARSE_DCSRMM
//#define CUSPARSE_CCSRMM             CUSPARSE_CCSRMM
//#define CUSPARSE_ZCSRMM             CUSPARSE_ZCSRMM
//#define CUSPARSE_SCSRSM_ANALYSIS    CUSPARSE_SCSRSM_ANALYSIS
//#define CUSPARSE_DCSRSM_ANALYSIS    CUSPARSE_DCSRSM_ANALYSIS
//#define CUSPARSE_CCSRSM_ANALYSIS    CUSPARSE_CCSRSM_ANALYSIS
//#define CUSPARSE_ZCSRSM_ANALYSIS    CUSPARSE_ZCSRSM_ANALYSIS
//#define CUSPARSE_SCSRSM_SOLVE       CUSPARSE_SCSRSM_SOLVE
//#define CUSPARSE_DCSRSM_SOLVE       CUSPARSE_DCSRSM_SOLVE
//#define CUSPARSE_CCSRSM_SOLVE       CUSPARSE_CCSRSM_SOLVE
//#define CUSPARSE_ZCSRSM_SOLVE       CUSPARSE_ZCSRSM_SOLVE
#define CUSPARSE_SGTSV              CUSPARSE_SGTSV
#define CUSPARSE_DGTSV              CUSPARSE_DGTSV
#define CUSPARSE_CGTSV              CUSPARSE_CGTSV
#define CUSPARSE_ZGTSV              CUSPARSE_ZGTSV
#define CUSPARSE_SGTSV_STRIDEDBATCH CUSPARSE_SGTSV_STRIDEDBATCH
#define CUSPARSE_DGTSV_STRIDEDBATCH CUSPARSE_DGTSV_STRIDEDBATCH
#define CUSPARSE_CGTSV_STRIDEDBATCH CUSPARSE_CGTSV_STRIDEDBATCH
#define CUSPARSE_ZGTSV_STRIDEDBATCH CUSPARSE_ZGTSV_STRIDEDBATCH
#define CUSPARSE_SNNZ               CUSPARSE_SNNZ
#define CUSPARSE_DNNZ               CUSPARSE_DNNZ
#define CUSPARSE_CNNZ               CUSPARSE_CNNZ
#define CUSPARSE_ZNNZ               CUSPARSE_ZNNZ
#define CUSPARSE_SDENSE2CSR         CUSPARSE_SDENSE2CSR
#define CUSPARSE_DDENSE2CSR         CUSPARSE_DDENSE2CSR
#define CUSPARSE_CDENSE2CSR         CUSPARSE_CDENSE2CSR
#define CUSPARSE_ZDENSE2CSR         CUSPARSE_ZDENSE2CSR
#define CUSPARSE_SCSR2DENSE         CUSPARSE_SCSR2DENSE
#define CUSPARSE_DCSR2DENSE         CUSPARSE_DCSR2DENSE
#define CUSPARSE_CCSR2DENSE         CUSPARSE_CCSR2DENSE
#define CUSPARSE_ZCSR2DENSE         CUSPARSE_ZCSR2DENSE
#define CUSPARSE_SDENSE2CSC         CUSPARSE_SDENSE2CSC
#define CUSPARSE_DDENSE2CSC         CUSPARSE_DDENSE2CSC
#define CUSPARSE_CDENSE2CSC         CUSPARSE_CDENSE2CSC
#define CUSPARSE_ZDENSE2CSC         CUSPARSE_ZDENSE2CSC
#define CUSPARSE_SCSC2DENSE         CUSPARSE_SCSC2DENSE
#define CUSPARSE_DCSC2DENSE         CUSPARSE_DCSC2DENSE
#define CUSPARSE_CCSC2DENSE         CUSPARSE_CCSC2DENSE
#define CUSPARSE_ZCSC2DENSE         CUSPARSE_ZCSC2DENSE
#define CUSPARSE_XCOO2CSR           CUSPARSE_XCOO2CSR
#define CUSPARSE_XCSR2COO           CUSPARSE_XCSR2COO
//#define CUSPARSE_SCSR2CSC           CUSPARSE_SCSR2CSC
//#define CUSPARSE_DCSR2CSC           CUSPARSE_DCSR2CSC
//#define CUSPARSE_CCSR2CSC           CUSPARSE_CCSR2CSC
//#define CUSPARSE_ZCSR2CSC           CUSPARSE_ZCSR2CSC

#else
#error unsupported Fortran compiler
#endif
