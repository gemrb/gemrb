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
 * $Id$
 *
 */

//#include "stdafx.h"
// Writes the RIFF header into a stream

#include <stdio.h>
#include <string.h>
#include "riffhdr.h"

RIFF_HEADER riff = {
	{'R', 'I', 'F', 'F'}, 0, {'W', 'A', 'V', 'E', 'f', 'm', 't', ' '}, 16, 1,
	2, 22050, 22050 * 4, 4, 16, {'d', 'a', 't', 'a'}, 0
};

void write_riff_header(void* memory, int samples, int channels,
	int samplerate)
{
	riff.raw_data_len = samples * sizeof( short );
	riff.total_len_m8 = riff.raw_data_len + sizeof( RIFF_HEADER ) - 8;
	riff.nChannels = ( unsigned short ) channels;  
	riff.nSamplesPerSec = samplerate;
	riff.nAvgBytesPerSec = channels * sizeof( short ) * samplerate;
	memcpy( memory, &riff, sizeof( RIFF_HEADER ) );
}

WAVC_HEADER wavc = {
	{'W','A','V','C'}, {'V','1','.','0'}, 0, 0, 28, 2, 16, 22050, 0x9ffdu
};

void write_wavc_header(FILE* fpoi, int samples, int channels, int compressed,
	int samplerate)
{
	wavc.uncompressed = samples * sizeof( short );
	wavc.compressed = compressed;
	wavc.channels = ( short ) channels;
	wavc.samplespersec = ( short ) samplerate;
	rewind( fpoi );
	fwrite( &wavc, 1, sizeof( WAVC_HEADER ), fpoi );
}
