/*
 * DSP utils
 * Copyright (c) 2000, 2001, 2002 Fabrice Bellard
 * Copyright (c) 2002-2004 Michael Niedermayer <michaelni@gmx.at>
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavcodec/dsputil.h
 * DSP utils.
 * note, many functions in here may use MMX which trashes the FPU state, it is
 * absolutely necessary to call emms_c() between dsp & float/double code
 */

#ifndef AVCODEC_DSPUTIL_H
#define AVCODEC_DSPUTIL_H

#include "common.h"

//#define DEBUG
/* dct code */
typedef short DCTELEM;
typedef int DWTELEM;
typedef short IDWTELEM;

void fdct_ifast (DCTELEM *data);
void fdct_ifast248 (DCTELEM *data);
void ff_jpeg_fdct_islow (DCTELEM *data);
void ff_fdct248_islow (DCTELEM *data);

void j_rev_dct (DCTELEM *data);
void j_rev_dct4 (DCTELEM *data);
void j_rev_dct2 (DCTELEM *data);
void j_rev_dct1 (DCTELEM *data);
void ff_wmv2_idct_c(DCTELEM *data);

void ff_fdct_mmx(DCTELEM *block);
void ff_fdct_mmx2(DCTELEM *block);
void ff_fdct_sse2(DCTELEM *block);

void ff_vector_fmul_add_add_c(float *dst, const float *src0, const float *src1,
                              const float *src2, int src3, int blocksize, int step);
void ff_vector_fmul_window_c(float *dst, const float *src0, const float *src1,
                             const float *win, float add_bias, int len);
void ff_float_to_int16_c(int16_t *dst, const float *src, long len);
//void ff_float_to_int16_interleave_c(int16_t *dst, const float **src, long len, int channels);

/* pixel operations */
#define MAX_NEG_CROP 1024

/* temporary */
extern uint32_t ff_squareTbl[512];
extern uint8_t ff_cropTbl[256 + 2 * MAX_NEG_CROP];

/* add and put pixel (decoding) */
// blocksizes for op_pixels_func are 8x4,8x8 16x8 16x16
//h for op_pixels_func is limited to {width/2, width} but never larger than 16 and never smaller then 4
typedef void (*op_pixels_func)(uint8_t *block/*align width (8 or 16)*/, const uint8_t *pixels/*align 1*/, int line_size, int h);
typedef void (*tpel_mc_func)(uint8_t *block/*align width (8 or 16)*/, const uint8_t *pixels/*align 1*/, int line_size, int w, int h);
typedef void (*qpel_mc_func)(uint8_t *dst/*align width (8 or 16)*/, uint8_t *src/*align 1*/, int stride);
typedef void (*h264_chroma_mc_func)(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int srcStride, int h, int x, int y);
typedef void (*h264_weight_func)(uint8_t *block, int stride, int log2_denom, int weight, int offset);
typedef void (*h264_biweight_func)(uint8_t *dst, uint8_t *src, int stride, int log2_denom, int weightd, int weights, int offset);

#define DEF_OLD_QPEL(name)\
void ff_put_        ## name (uint8_t *dst/*align width (8 or 16)*/, uint8_t *src/*align 1*/, int stride);\
void ff_put_no_rnd_ ## name (uint8_t *dst/*align width (8 or 16)*/, uint8_t *src/*align 1*/, int stride);\
void ff_avg_        ## name (uint8_t *dst/*align width (8 or 16)*/, uint8_t *src/*align 1*/, int stride);

DEF_OLD_QPEL(qpel16_mc11_old_c)
DEF_OLD_QPEL(qpel16_mc31_old_c)
DEF_OLD_QPEL(qpel16_mc12_old_c)
DEF_OLD_QPEL(qpel16_mc32_old_c)
DEF_OLD_QPEL(qpel16_mc13_old_c)
DEF_OLD_QPEL(qpel16_mc33_old_c)
DEF_OLD_QPEL(qpel8_mc11_old_c)
DEF_OLD_QPEL(qpel8_mc31_old_c)
DEF_OLD_QPEL(qpel8_mc12_old_c)
DEF_OLD_QPEL(qpel8_mc32_old_c)
DEF_OLD_QPEL(qpel8_mc13_old_c)
DEF_OLD_QPEL(qpel8_mc33_old_c)

#define CALL_2X_PIXELS(a, b, n)\
static void a(uint8_t *block, const uint8_t *pixels, int line_size, int h){\
    b(block  , pixels  , line_size, h);\
    b(block+n, pixels+n, line_size, h);\
}

/* motion estimation */
// h is limited to {width/2, width, 2*width} but never larger than 16 and never smaller then 2
// although currently h<4 is not used as functions with width <8 are neither used nor implemented
typedef int (*me_cmp_func)(void /*MpegEncContext*/ *s, uint8_t *blk1/*align width (8 or 16)*/, uint8_t *blk2/*align 1*/, int line_size, int h)/* __attribute__ ((const))*/;


// for snow slices
typedef struct slice_buffer_s slice_buffer;

/**
 * Scantable.
 */
typedef struct ScanTable{
    const uint8_t *scantable;
    uint8_t permutated[64];
    uint8_t raster_end[64];
#if ARCH_PPC
                /** Used by dct_quantize_altivec to find last-non-zero */
    DECLARE_ALIGNED(16, uint8_t, inverse[64]);
#endif
} ScanTable;

void ff_init_scantable(uint8_t *, ScanTable *st, const uint8_t *src_scantable);

void ff_emulated_edge_mc(uint8_t *buf, uint8_t *src, int linesize,
                         int block_w, int block_h,
                         int src_x, int src_y, int w, int h);

/**
 * DSPContext.
 */
typedef struct DSPContext {
    /* pixel ops : interface with DCT */
    void (*get_pixels)(DCTELEM *block/*align 16*/, const uint8_t *pixels/*align 8*/, int line_size);
    void (*diff_pixels)(DCTELEM *block/*align 16*/, const uint8_t *s1/*align 8*/, const uint8_t *s2/*align 8*/, int stride);
    void (*put_pixels_clamped)(const DCTELEM *block/*align 16*/, uint8_t *pixels/*align 8*/, int line_size);
    void (*put_signed_pixels_clamped)(const DCTELEM *block/*align 16*/, uint8_t *pixels/*align 8*/, int line_size);
    void (*add_pixels_clamped)(const DCTELEM *block/*align 16*/, uint8_t *pixels/*align 8*/, int line_size);
    void (*add_pixels8)(uint8_t *pixels, DCTELEM *block, int line_size);
    void (*add_pixels4)(uint8_t *pixels, DCTELEM *block, int line_size);
    int (*sum_abs_dctelem)(DCTELEM *block/*align 16*/);
    /**
     * translational global motion compensation.
     */
    void (*gmc1)(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int srcStride, int h, int x16, int y16, int rounder);
    /**
     * global motion compensation.
     */
    void (*gmc )(uint8_t *dst/*align 8*/, uint8_t *src/*align 1*/, int stride, int h, int ox, int oy,
                    int dxx, int dxy, int dyx, int dyy, int shift, int r, int width, int height);
    void (*clear_block)(DCTELEM *block/*align 16*/);
    void (*clear_blocks)(DCTELEM *blocks/*align 16*/);
    int (*pix_sum)(uint8_t * pix, int line_size);
    int (*pix_norm1)(uint8_t * pix, int line_size);
// 16x16 8x8 4x4 2x2 16x8 8x4 4x2 8x16 4x8 2x4

    me_cmp_func sad[6]; /* identical to pix_absAxA except additional void * */
    me_cmp_func sse[6];
    me_cmp_func hadamard8_diff[6];
    me_cmp_func dct_sad[6];
    me_cmp_func quant_psnr[6];
    me_cmp_func bit[6];
    me_cmp_func rd[6];
    me_cmp_func vsad[6];
    me_cmp_func vsse[6];
    me_cmp_func nsse[6];
    me_cmp_func w53[6];
    me_cmp_func w97[6];
    me_cmp_func dct_max[6];
    me_cmp_func dct264_sad[6];

    me_cmp_func me_pre_cmp[6];
    me_cmp_func me_cmp[6];
    me_cmp_func me_sub_cmp[6];
    me_cmp_func mb_cmp[6];
    me_cmp_func ildct_cmp[6]; //only width 16 used
    me_cmp_func frame_skip_cmp[6]; //only width 8 used

    int (*ssd_int8_vs_int16)(const int8_t *pix1, const int16_t *pix2,
                             int size);

    /**
     * Halfpel motion compensation with rounding (a+b+1)>>1.
     * this is an array[4][4] of motion compensation functions for 4
     * horizontal blocksizes (8,16) and the 4 halfpel positions<br>
     * *pixels_tab[ 0->16xH 1->8xH ][ xhalfpel + 2*yhalfpel ]
     * @param block destination where the result is stored
     * @param pixels source
     * @param line_size number of bytes in a horizontal line of block
     * @param h height
     */
    op_pixels_func put_pixels_tab[4][4];

    /**
     * Halfpel motion compensation with rounding (a+b+1)>>1.
     * This is an array[4][4] of motion compensation functions for 4
     * horizontal blocksizes (8,16) and the 4 halfpel positions<br>
     * *pixels_tab[ 0->16xH 1->8xH ][ xhalfpel + 2*yhalfpel ]
     * @param block destination into which the result is averaged (a+b+1)>>1
     * @param pixels source
     * @param line_size number of bytes in a horizontal line of block
     * @param h height
     */
    op_pixels_func avg_pixels_tab[4][4];

    /**
     * Halfpel motion compensation with no rounding (a+b)>>1.
     * this is an array[2][4] of motion compensation functions for 2
     * horizontal blocksizes (8,16) and the 4 halfpel positions<br>
     * *pixels_tab[ 0->16xH 1->8xH ][ xhalfpel + 2*yhalfpel ]
     * @param block destination where the result is stored
     * @param pixels source
     * @param line_size number of bytes in a horizontal line of block
     * @param h height
     */
    op_pixels_func put_no_rnd_pixels_tab[4][4];

    /**
     * Halfpel motion compensation with no rounding (a+b)>>1.
     * this is an array[2][4] of motion compensation functions for 2
     * horizontal blocksizes (8,16) and the 4 halfpel positions<br>
     * *pixels_tab[ 0->16xH 1->8xH ][ xhalfpel + 2*yhalfpel ]
     * @param block destination into which the result is averaged (a+b)>>1
     * @param pixels source
     * @param line_size number of bytes in a horizontal line of block
     * @param h height
     */
    op_pixels_func avg_no_rnd_pixels_tab[4][4];

    void (*put_no_rnd_pixels_l2[2])(uint8_t *block/*align width (8 or 16)*/, const uint8_t *a/*align 1*/, const uint8_t *b/*align 1*/, int line_size, int h);

    /**
     * Thirdpel motion compensation with rounding (a+b+1)>>1.
     * this is an array[12] of motion compensation functions for the 9 thirdpe
     * positions<br>
     * *pixels_tab[ xthirdpel + 4*ythirdpel ]
     * @param block destination where the result is stored
     * @param pixels source
     * @param line_size number of bytes in a horizontal line of block
     * @param h height
     */
    tpel_mc_func put_tpel_pixels_tab[11]; //FIXME individual func ptr per width?
    tpel_mc_func avg_tpel_pixels_tab[11]; //FIXME individual func ptr per width?

    qpel_mc_func put_qpel_pixels_tab[2][16];
    qpel_mc_func avg_qpel_pixels_tab[2][16];
    qpel_mc_func put_no_rnd_qpel_pixels_tab[2][16];
    qpel_mc_func avg_no_rnd_qpel_pixels_tab[2][16];
    qpel_mc_func put_mspel_pixels_tab[8];

    /* C version: convert floats from the range [384.0,386.0] to ints in [-32768,32767]
     * simd versions: convert floats from [-32768.0,32767.0] without rescaling and arrays are 16byte aligned */
    void (*float_to_int16)(int16_t *dst, const float *src, long len);
    void (*float_to_int16_interleave)(int16_t *dst, const float **src, long len, int channels);

    /* (I)DCT */
    void (*fdct)(DCTELEM *block/* align 16*/);
    void (*fdct248)(DCTELEM *block/* align 16*/);

    /* IDCT really*/
    void (*idct)(DCTELEM *block/* align 16*/);

    /**
     * block -> idct -> clip to unsigned 8 bit -> dest.
     * (-1392, 0, 0, ...) -> idct -> (-174, -174, ...) -> put -> (0, 0, ...)
     * @param line_size size in bytes of a horizontal line of dest
     */
    void (*idct_put)(uint8_t *dest/*align 8*/, int line_size, DCTELEM *block/*align 16*/);

    /**
     * block -> idct -> add dest -> clip to unsigned 8 bit -> dest.
     * @param line_size size in bytes of a horizontal line of dest
     */
    void (*idct_add)(uint8_t *dest/*align 8*/, int line_size, DCTELEM *block/*align 16*/);

    /**
     * idct input permutation.
     * several optimized IDCTs need a permutated input (relative to the normal order of the reference
     * IDCT)
     * this permutation must be performed before the idct_put/add, note, normally this can be merged
     * with the zigzag/alternate scan<br>
     * an example to avoid confusion:
     * - (->decode coeffs -> zigzag reorder -> dequant -> reference idct ->...)
     * - (x -> referece dct -> reference idct -> x)
     * - (x -> referece dct -> simple_mmx_perm = idct_permutation -> simple_idct_mmx -> x)
     * - (->decode coeffs -> zigzag reorder -> simple_mmx_perm -> dequant -> simple_idct_mmx ->...)
     */
    uint8_t idct_permutation[64];
    int idct_permutation_type;
#define FF_NO_IDCT_PERM 1
#define FF_LIBMPEG2_IDCT_PERM 2
#define FF_SIMPLE_IDCT_PERM 3
#define FF_TRANSPOSE_IDCT_PERM 4
#define FF_PARTTRANS_IDCT_PERM 5
#define FF_SSE2_IDCT_PERM 6

    int (*try_8x8basis)(int16_t rem[64], int16_t weight[64], int16_t basis[64], int scale);
    void (*add_8x8basis)(int16_t rem[64], int16_t basis[64], int scale);
#define BASIS_SHIFT 16
#define RECON_SHIFT 6

    void (*draw_edges)(uint8_t *buf, int wrap, int width, int height, int w);
#define EDGE_WIDTH 16
} DSPContext;

void dsputil_static_init(void);

int ff_check_alignment(void);

/**
 * permute block according to permuatation.
 * @param last last non zero element in scantable order
 */
void ff_block_permute(DCTELEM *block, uint8_t *permutation, const uint8_t *scantable, int last);

void ff_set_cmp(DSPContext* c, me_cmp_func *cmp, int type);

#define         BYTE_VEC32(c)   ((c)*0x01010101UL)

static inline uint32_t rnd_avg32(uint32_t a, uint32_t b)
{
    return (a | b) - (((a ^ b) & ~BYTE_VEC32(0x01)) >> 1);
}

static inline uint32_t no_rnd_avg32(uint32_t a, uint32_t b)
{
    return (a & b) + (((a ^ b) & ~BYTE_VEC32(0x01)) >> 1);
}
/*
static inline int get_penalty_factor(int lambda, int lambda2, int type){
    switch(type&0xFF){
    default:
    case FF_CMP_SAD:
        return lambda>>FF_LAMBDA_SHIFT;
    case FF_CMP_DCT:
        return (3*lambda)>>(FF_LAMBDA_SHIFT+1);
    case FF_CMP_W53:
        return (4*lambda)>>(FF_LAMBDA_SHIFT);
    case FF_CMP_W97:
        return (2*lambda)>>(FF_LAMBDA_SHIFT);
    case FF_CMP_SATD:
    case FF_CMP_DCT264:
        return (2*lambda)>>FF_LAMBDA_SHIFT;
    case FF_CMP_RD:
    case FF_CMP_PSNR:
    case FF_CMP_SSE:
    case FF_CMP_NSSE:
        return lambda2>>FF_LAMBDA_SHIFT;
    case FF_CMP_BIT:
        return 1;
    }
}
*/
/**
 * Empty mmx state.
 * this must be called between any dsp function and float/double code.
 * for example sin(); dsp->idct_put(); emms_c(); cos()
 */
#define emms_c()

/* should be defined by architectures supporting
   one or more MultiMedia extension */
int mm_support(void);
extern int mm_flags;

//void dsputil_init_alpha(DSPContext* c, AVCodecContext *avctx);
//void dsputil_init_arm(DSPContext* c, AVCodecContext *avctx);
//void dsputil_init_bfin(DSPContext* c, AVCodecContext *avctx);
//void dsputil_init_mlib(DSPContext* c, AVCodecContext *avctx);
//void dsputil_init_mmi(DSPContext* c, AVCodecContext *avctx);
//void dsputil_init_mmx(DSPContext* c, AVCodecContext *avctx);
//void dsputil_init_ppc(DSPContext* c, AVCodecContext *avctx);
//void dsputil_init_sh4(DSPContext* c, AVCodecContext *avctx);
//void dsputil_init_vis(DSPContext* c, AVCodecContext *avctx);

#define DECLARE_ALIGNED_16(t, v) DECLARE_ALIGNED(16, t, v)
#define DECLARE_ALIGNED_8(t, v)  DECLARE_ALIGNED(8, t, v)

#if HAVE_MMX

#undef emms_c

static inline void emms(void)
{
    __asm__ volatile ("emms;":::"memory");
}


#define emms_c() \
{\
    if (mm_flags & FF_MM_MMX)\
        emms();\
}

#elif ARCH_ARM

#if HAVE_NEON
#   define STRIDE_ALIGN 16
#endif

#elif ARCH_PPC

#define STRIDE_ALIGN 16

#elif HAVE_MMI

#define STRIDE_ALIGN 16

#else

#define mm_flags 0
#define mm_support() 0

#endif

#ifndef STRIDE_ALIGN
#   define STRIDE_ALIGN 8
#endif

/* PSNR */
/*
void get_psnr(uint8_t *orig_image[3], uint8_t *coded_image[3],
              int orig_linesize[3], int coded_linesize,
              AVCodecContext *avctx);
*/
/* FFT computation */

/* NOTE: soon integer code will be added, so you must use the
   FFTSample type */
typedef float FFTSample;

struct MDCTContext;

typedef struct FFTComplex {
    FFTSample re, im;
} FFTComplex;

typedef struct FFTContext {
    int nbits;
    int inverse;
    uint16_t *revtab;
    FFTComplex *exptab;
    FFTComplex *exptab1; /* only used by SSE code */
    FFTComplex *tmp_buf;
    void (*fft_permute)(struct FFTContext *s, FFTComplex *z);
    void (*fft_calc)(struct FFTContext *s, FFTComplex *z);
    void (*imdct_calc)(struct MDCTContext *s, FFTSample *output, const FFTSample *input);
    void (*imdct_half)(struct MDCTContext *s, FFTSample *output, const FFTSample *input);
    void (*mdct_calc)(struct MDCTContext *s, FFTSample *output, const FFTSample *input);
    int split_radix;
    int permutation;
#define FF_MDCT_PERM_NONE       0
#define FF_MDCT_PERM_INTERLEAVE 1
} FFTContext;

extern FFTSample* const ff_cos_tabs[17];

#if CONFIG_HARDCODED_TABLES
#define COSTABLE_CONST const
#define SINTABLE_CONST const
#else
#define COSTABLE_CONST
#define SINTABLE_CONST
#endif

#define COSTABLE(size) COSTABLE_CONST FFTSample ff_cos_##size[size/2]

#define SINTABLE(size) SINTABLE_CONST FFTSample ff_sin_##size[size/2]

extern COSTABLE(16);
extern COSTABLE(32);
extern COSTABLE(64);
extern COSTABLE(128);
extern COSTABLE(256);
extern COSTABLE(512);
extern COSTABLE(1024);
extern COSTABLE(2048);
extern COSTABLE(4096);
extern COSTABLE(8192);
extern COSTABLE(16384);
extern COSTABLE(32768);
extern COSTABLE(65536);

/**
 * Initializes the cosine table in ff_cos_tabs[index]
 * \param index index in ff_cos_tabs array of the table to initialize
 */
void ff_init_ff_cos_tabs(int index);

extern SINTABLE(16);
extern SINTABLE(32);
extern SINTABLE(64);
extern SINTABLE(128);
extern SINTABLE(256);
extern SINTABLE(512);
extern SINTABLE(1024);
extern SINTABLE(2048);
extern SINTABLE(4096);
extern SINTABLE(8192);
extern SINTABLE(16384);
extern SINTABLE(32768);
extern SINTABLE(65536);

/**
 * Sets up a complex FFT.
 * @param nbits           log2 of the length of the input array
 * @param inverse         if 0 perform the forward transform, if 1 perform the inverse
 */
int ff_fft_init(FFTContext *s, int nbits, int inverse);
void ff_fft_permute_c(FFTContext *s, FFTComplex *z);
void ff_fft_permute_sse(FFTContext *s, FFTComplex *z);
void ff_fft_permute_neon(FFTContext *s, FFTComplex *z);
void ff_fft_calc_c(FFTContext *s, FFTComplex *z);
void ff_fft_calc_sse(FFTContext *s, FFTComplex *z);
void ff_fft_calc_3dn(FFTContext *s, FFTComplex *z);
void ff_fft_calc_3dn2(FFTContext *s, FFTComplex *z);
void ff_fft_calc_altivec(FFTContext *s, FFTComplex *z);
void ff_fft_calc_neon(FFTContext *s, FFTComplex *z);

/**
 * Do the permutation needed BEFORE calling ff_fft_calc().
 */
static inline void ff_fft_permute(FFTContext *s, FFTComplex *z)
{
    s->fft_permute(s, z);
}
/**
 * Do a complex FFT with the parameters defined in ff_fft_init(). The
 * input data must be permuted before. No 1.0/sqrt(n) normalization is done.
 */
static inline void ff_fft_calc(FFTContext *s, FFTComplex *z)
{
    s->fft_calc(s, z);
}
void ff_fft_end(FFTContext *s);

/* MDCT computation */

typedef struct MDCTContext {
    int n;  /* size of MDCT (i.e. number of input data * 2) */
    int nbits; /* n = 2^nbits */
    /* pre/post rotation tables */
    FFTSample *tcos;
    FFTSample *tsin;
    FFTContext fft;
} MDCTContext;

static inline void ff_imdct_calc(MDCTContext *s, FFTSample *output, const FFTSample *input)
{
    s->fft.imdct_calc(s, output, input);
}
static inline void ff_imdct_half(MDCTContext *s, FFTSample *output, const FFTSample *input)
{
    s->fft.imdct_half(s, output, input);
}

static inline void ff_mdct_calc(MDCTContext *s, FFTSample *output,
                                const FFTSample *input)
{
    s->fft.mdct_calc(s, output, input);
}

/**
 * Generate a Kaiser-Bessel Derived Window.
 * @param   window  pointer to half window
 * @param   alpha   determines window shape
 * @param   n       size of half window
 */
void ff_kbd_window_init(float *window, float alpha, int n);

/**
 * Generate a sine window.
 * @param   window  pointer to half window
 * @param   n       size of half window
 */
void ff_sine_window_init(float *window, int n);
extern float ff_sine_128 [ 128];
extern float ff_sine_256 [ 256];
extern float ff_sine_512 [ 512];
extern float ff_sine_1024[1024];
extern float ff_sine_2048[2048];
extern float ff_sine_4096[4096];
extern float * const ff_sine_windows[6];

int ff_mdct_init(MDCTContext *s, int nbits, int inverse, double scale);
void ff_imdct_calc_c(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_half_c(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_mdct_calc_c(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_calc_3dn(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_half_3dn(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_calc_3dn2(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_half_3dn2(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_calc_sse(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_half_sse(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_calc_neon(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_imdct_half_neon(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_mdct_calc_neon(MDCTContext *s, FFTSample *output, const FFTSample *input);
void ff_mdct_end(MDCTContext *s);

/* Real Discrete Fourier Transform */

enum RDFTransformType {
    RDFT,
    IRDFT,
    RIDFT,
    IRIDFT
};

typedef struct {
    int nbits;
    int inverse;
    int sign_convention;

    /* pre/post rotation tables */
    FFTSample *tcos;
    FFTSample *tsin;
    FFTContext fft;
} RDFTContext;

/**
 * Sets up a real FFT.
 * @param nbits           log2 of the length of the input array
 * @param trans           the type of transform
 */
int ff_rdft_init(RDFTContext *s, int nbits, enum RDFTransformType trans);
void ff_rdft_calc(RDFTContext *s, FFTSample *data);
void ff_rdft_end(RDFTContext *s);

/* Discrete Cosine Transform */

typedef struct {
    int nbits;
    int inverse;
    FFTComplex *data;
    FFTContext fft;
} DCTContext;

/**
 * Sets up (Inverse)DCT.
 * @param nbits           log2 of the length of the input array
 * @param inverse         >0 forward transform, <0 inverse transform
 */
int ff_dct_init(DCTContext *s, int nbits, int inverse);
void ff_dct_calc(DCTContext *s, FFTSample *data);
void ff_dct_end(DCTContext *s);
 

#if 0
#define WRAPPER8_16(name8, name16)\
static int name16(void /*MpegEncContext*/ *s, uint8_t *dst, uint8_t *src, int stride, int h){\
    return name8(s, dst           , src           , stride, h)\
          +name8(s, dst+8         , src+8         , stride, h);\
}

#define WRAPPER8_16_SQ(name8, name16)\
static int name16(void /*MpegEncContext*/ *s, uint8_t *dst, uint8_t *src, int stride, int h){\
    int score=0;\
    score +=name8(s, dst           , src           , stride, 8);\
    score +=name8(s, dst+8         , src+8         , stride, 8);\
    if(h==16){\
        dst += 8*stride;\
        src += 8*stride;\
        score +=name8(s, dst           , src           , stride, 8);\
        score +=name8(s, dst+8         , src+8         , stride, 8);\
    }\
    return score;\
}


static inline void copy_block2(uint8_t *dst, const uint8_t *src, int dstStride, int srcStride, int h)
{
    int i;
    for(i=0; i<h; i++)
    {
        AV_WN16(dst   , AV_RN16(src   ));
        dst+=dstStride;
        src+=srcStride;
    }
}

static inline void copy_block4(uint8_t *dst, const uint8_t *src, int dstStride, int srcStride, int h)
{
    int i;
    for(i=0; i<h; i++)
    {
        AV_WN32(dst   , AV_RN32(src   ));
        dst+=dstStride;
        src+=srcStride;
    }
}

static inline void copy_block8(uint8_t *dst, const uint8_t *src, int dstStride, int srcStride, int h)
{
    int i;
    for(i=0; i<h; i++)
    {
        AV_WN32(dst   , AV_RN32(src   ));
        AV_WN32(dst+4 , AV_RN32(src+4 ));
        dst+=dstStride;
        src+=srcStride;
    }
}

static inline void copy_block9(uint8_t *dst, const uint8_t *src, int dstStride, int srcStride, int h)
{
    int i;
    for(i=0; i<h; i++)
    {
        AV_WN32(dst   , AV_RN32(src   ));
        AV_WN32(dst+4 , AV_RN32(src+4 ));
        dst[8]= src[8];
        dst+=dstStride;
        src+=srcStride;
    }
}

static inline void copy_block16(uint8_t *dst, const uint8_t *src, int dstStride, int srcStride, int h)
{
    int i;
    for(i=0; i<h; i++)
    {
        AV_WN32(dst   , AV_RN32(src   ));
        AV_WN32(dst+4 , AV_RN32(src+4 ));
        AV_WN32(dst+8 , AV_RN32(src+8 ));
        AV_WN32(dst+12, AV_RN32(src+12));
        dst+=dstStride;
        src+=srcStride;
    }
}

static inline void copy_block17(uint8_t *dst, const uint8_t *src, int dstStride, int srcStride, int h)
{
    int i;
    for(i=0; i<h; i++)
    {
        AV_WN32(dst   , AV_RN32(src   ));
        AV_WN32(dst+4 , AV_RN32(src+4 ));
        AV_WN32(dst+8 , AV_RN32(src+8 ));
        AV_WN32(dst+12, AV_RN32(src+12));
        dst[16]= src[16];
        dst+=dstStride;
        src+=srcStride;
    }
}

#endif 

#endif /* AVCODEC_DSPUTIL_H */
