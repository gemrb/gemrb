// SPDX-FileCopyrightText: 2011 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __GemRB__Freetype__
#define __GemRB__Freetype__

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H

/**
 *  This macro turns the version numbers into a numeric value:
 *  \verbatim
 (1,2,3) -> (1203)
 \endverbatim
 *
 *  This assumes that there will never be more than 100 patchlevels.
 */
#define FREETYPE_VERSIONNUM(X, Y, Z) \
	((X) * 1000 + (Y) * 100 + (Z))

/**
 *  This is the version number macro for the current Freetype version.
 */
#define FREETYPE_COMPILEDVERSION \
	FREETYPE_VERSIONNUM(FREETYPE_MAJOR, FREETYPE_MINOR, FREETYPE_PATCH)

/**
 *  This macro will evaluate to true if compiled with Freetype at least X.Y.Z.
 */
#define FREETYPE_VERSION_ATLEAST(X, Y, Z) \
	(FREETYPE_COMPILEDVERSION >= FREETYPE_VERSIONNUM(X, Y, Z))

/* Handy routines for converting from fixed point */
#define FT_FLOOR(X) ((X & -64) / 64)
#define FT_CEIL(X)  (((X + 63) & -64) / 64)

namespace GemRB {
void LogFTError(FT_Error errCode);
}

#endif /* defined(__GemRB__Freetype__) */
