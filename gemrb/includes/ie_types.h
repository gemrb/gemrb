/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/includes/ie_types.h,v 1.3 2004/08/01 08:49:08 guidoj Exp $
 *
 */

// ie_types.h : defines data types used to load IE structures

#ifndef IE_TYPES_H
#define IE_TYPES_H

#ifdef WIN32
typedef unsigned char ieByte;
typedef unsigned short ieWord;
typedef unsigned long int ieDword;

typedef unsigned char ie_uint8;
typedef unsigned short ie_uint16;
typedef unsigned long int ie_uint32;
typedef unsigned long long int ie_uint64;

typedef signed char ie_int8;
typedef signed short ie_int16;
typedef signed long int ie_int32;
typedef signed long long int ie_int64;

#else
#include "../../config.h"
typedef unsigned char ieByte;
typedef unsigned char ie_uint8;
typedef signed char ie_int8;
typedef unsigned short ieWord;
typedef unsigned short ie_uint16;
typedef signed short ie_int16;
#if (SIZEOF_INT == 4)
typedef unsigned int ieDword;
typedef unsigned int ie_uint32;
typedef signed int ie_int32;
#elif (SIZE_LONG_INT == 4)
typedef unsigned long int ieDword;
typedef unsigned long int ie_uint32;
typedef signed long int ie_int32;
#endif
#if (SIZEOF_LONG_INT == 8)
typedef unsigned long int ieQword;
typedef unsigned long int ie_uint64;
typedef signed long int ie_int64;
#elif (SIZE_LONG_LONG_INT == 8)
typedef unsigned long long int ieQword;
typedef unsigned long long int ie_uint64;
typedef signed long long int ie_int64;
#endif
#endif

typedef ie_uint32 ieStrRef; 
typedef char ieResRef[8];

#endif  //! IE_TYPES_H

