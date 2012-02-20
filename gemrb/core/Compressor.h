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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#ifndef COMPRESSOR_H
#define COMPRESSOR_H

#include "Plugin.h"
#include "System/DataStream.h"

namespace GemRB {

class GEM_EXPORT Compressor : public Plugin {
public:
	Compressor(void);
	virtual ~Compressor(void);
	/** decompresses a datastream (memory or file) to a FILE * stream */
	virtual int Decompress(DataStream* dest, DataStream* source, unsigned int size_guess = 0) const = 0;
	/** compresses a datastream (memory or file) to another DataStream */
	virtual int Compress(DataStream *dest, DataStream* source) const = 0;
};

}

#endif
