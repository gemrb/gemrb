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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/includes/ie_types.h,v 1.4 2004/08/02 21:26:52 guidoj Exp $
 *
 */

// ie_types.h : defines data types used to load IE structures

#ifndef IE_TYPES_H
#define IE_TYPES_H

#if HAVE_CONFIG
#include "../../config.h"
#endif

typedef unsigned char ieByte;
typedef unsigned short ieWord;

#if (SIZEOF_INT == 4)
typedef unsigned int ieDword;
#elif (SIZE_LONG_INT == 4)
typedef unsigned long int ieDword;
#else
typedef unsigned long int ieDword;
#endif

typedef ieDword ieStrRef; 
typedef char ieResRef[8];

#endif  //! IE_TYPES_H

