#ifndef AVUTIL_COMMON_H
#define AVUTIL_COMMON_H

#include "../../includes/win32def.h"
#include "../../includes/globals.h"

#include <ctype.h>
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
 
#define av_const
#define av_cold
#define av_flatten
#define attribute_deprecated
#define av_unused
#define av_uninit(x) x
#define av_always_inline inline 

#define uint8_t unsigned char
#define uint16_t unsigned short
#define uint32_t unsigned int
#define uint64_t __int64

#define int8_t signed char
#define int16_t signed short
#define int32_t signed int
#define int64_t signed __int64

//rounded division & shift
#define RSHIFT(a,b) ((a) > 0 ? ((a) + ((1<<(b))>>1))>>(b) : ((a) + ((1<<(b))>>1)-1)>>(b))
/* assume b>0 */
#define ROUNDED_DIV(a,b) (((a)>0 ? (a) + ((b)>>1) : (a) - ((b)>>1))/(b))
#define FFABS(a) ((a) >= 0 ? (a) : (-(a)))
#define FFSIGN(a) ((a) > 0 ? 1 : -1)

#define FFMAX(a,b) ((a) > (b) ? (a) : (b))
#define FFMAX3(a,b,c) FFMAX(FFMAX(a,b),c)
#define FFMIN(a,b) ((a) > (b) ? (b) : (a))
#define FFMIN3(a,b,c) FFMIN(FFMIN(a,b),c)

#define FFSWAP(type,a,b) do{type SWAP_tmp= b; b= a; a= SWAP_tmp;}while(0)
#define FF_ARRAY_ELEMS(a) (sizeof(a) / sizeof((a)[0]))
#define FFALIGN(x, a) (((x)+(a)-1)&~((a)-1))

#ifndef M_E
#define M_E            2.7182818284590452354   /* e */
#endif
#ifndef M_LN2
#define M_LN2          0.69314718055994530942  /* log_e 2 */
#endif
#ifndef M_LN10
#define M_LN10         2.30258509299404568402  /* log_e 10 */
#endif
#ifndef M_PI
#define M_PI           3.14159265358979323846  /* pi */
#endif
#ifndef M_SQRT1_2
#define M_SQRT1_2      0.70710678118654752440  /* 1/sqrt(2) */
#endif
#ifndef NAN
#define NAN            (0.0/0.0)
#endif
#ifndef INFINITY
#define INFINITY       (1.0/0.0)
#endif

/* misc math functions */


extern const uint8_t ff_log2_tab[256];

static inline av_const int av_log2(unsigned int v)
{
    int n = 0;
    if (v & 0xffff0000) {
        v >>= 16;
        n += 16;
    }
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    n += ff_log2_tab[v];

    return n;
}

static inline av_const int av_log2_16bit(unsigned int v)
{
    int n = 0;
    if (v & 0xff00) {
        v >>= 8;
        n += 8;
    }
    //n += ff_log2_tab[v];

    return n;
}


/**
 * Clips a signed integer value into the amin-amax range.
 * @param a value to clip
 * @param amin minimum value of the clip range
 * @param amax maximum value of the clip range
 * @return clipped value
 */

static inline av_const int av_clip(int a, int amin, int amax)
{
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}

/**
 * Clips a signed integer value into the 0-255 range.
 * @param a value to clip
 * @return clipped value
 */

inline unsigned char av_clip_uint8(int a)
{
    if (a&(~255)) return (-a)>>31;
    else          return a;
}

/**
 * Clips a signed integer value into the 0-65535 range.
 * @param a value to clip
 * @return clipped value
 */
/*
static inline av_const uint16_t av_clip_uint16(int a)
{
    if (a&(~65535)) return (-a)>>31;
    else            return a;
}
*/
/**
 * Clips a signed integer value into the -32768,32767 range.
 * @param a value to clip
 * @return clipped value
 */
/*
static inline av_const int16_t av_clip_int16(int a)
{
    if ((a+32768) & ~65535) return (a>>31) ^ 32767;
    else                    return a;
}
*/
/**
 * Clips a float value into the amin-amax range.
 * @param a value to clip
 * @param amin minimum value of the clip range
 * @param amax maximum value of the clip range
 * @return clipped value
 */
/*
static inline av_const float av_clipf(float a, float amin, float amax)
{
    if      (a < amin) return amin;
    else if (a > amax) return amax;
    else               return a;
}
*/
/** Computes ceil(log2(x)).
 * @param x value used to compute ceil(log2(x))
 * @return computed ceiling of log2(x)
 */

/*
static inline av_const int av_ceil_log2(int x)
{
    return av_log2((x - 1) << 1);
}
*/

int64_t av_const av_gcd(int64_t a, int64_t b);

#define BIK_SIGNATURE_LEN 4
#define BIK_SIGNATURE_DATA "BIKi"

#define MAX_CHANNELS 2
#define BINK_BLOCK_MAX_SIZE (MAX_CHANNELS << 11)

enum BinkAudFlags {
    BINK_AUD_16BITS = 0x4000, ///< prefer 16-bit output
    BINK_AUD_STEREO = 0x2000,
    BINK_AUD_USEDCT = 0x1000
};

void *av_malloc(unsigned int size);
void av_freep(void **ptr);

//#ifdef HAVE_AV_CONFIG_H
//#    include "internal.h"
//#endif /* HAVE_AV_CONFIG_H */

#endif /* AVUTIL_COMMON_H */
