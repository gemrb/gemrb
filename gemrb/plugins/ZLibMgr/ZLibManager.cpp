/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ZLibMgr/ZLibManager.cpp,v 1.6 2004/02/24 22:20:35 balrog994 Exp $
 *
 */

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "ZLibManager.h"

#include <zlib.h>


ZLibManager::ZLibManager(void)
{
}

ZLibManager::~ZLibManager(void)
{
}


#define INPUTSIZE  4096
#define OUTPUTSIZE 4096

// ZLib Decompression Routine
int ZLibManager::Decompress(FILE* dest, DataStream* source)
{
	unsigned char bufferin[INPUTSIZE], bufferout[OUTPUTSIZE];
	z_stream stream;
	int result;

	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	result = inflateInit( &stream );
	if (result != Z_OK) {
		return GEM_ERROR;
	}

	stream.avail_in = 0;
	while (1) {
		stream.next_out = bufferout;
		stream.avail_out = OUTPUTSIZE;
		if (stream.avail_in == 0) {
			stream.next_in = bufferin;
			stream.avail_in = source->Read( bufferin, INPUTSIZE );
			if (stream.avail_in < 0) {
				return GEM_ERROR;
			}
		}
		result = inflate( &stream, Z_NO_FLUSH );
		if (( result != Z_OK ) && ( result != Z_STREAM_END )) {
			return GEM_ERROR;
		}
		if (fwrite( bufferout, 1, OUTPUTSIZE - stream.avail_out, dest ) <
			OUTPUTSIZE -
			stream.avail_out) {
			return GEM_ERROR;
		}
		if (result == Z_STREAM_END) {
			if (stream.avail_in > 0)
				source->Seek( -stream.avail_in, GEM_CURRENT_POS );
			result = inflateEnd( &stream );
			if (result != Z_OK)
				return GEM_ERROR;
			return GEM_OK;
		}
	}
}
