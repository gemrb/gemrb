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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/readers.cpp,v 1.3 2004/01/02 15:56:47 balrog994 Exp $
 *
 */

//#include "stdafx.h"
// Classes for sound files.
// Supported formats: PCM-RAW, PCM-WAV (both 8 and 16 bits),
//   and Interplay's ACM.

#include <stdio.h>
#include "readers.h"
#include "general.h"

int CACMReader::init_reader () {
	ACM_Header hdr;
	char tmp[4];

	stream->Read(tmp, sizeof(tmp));
	if(!memcmp(tmp,"WAVC",4) ) {
		stream->Seek(24, GEM_CURRENT_POS);
	} else {
		stream->Seek(-4, GEM_CURRENT_POS);
	}
	stream->Read(&hdr, sizeof(ACM_Header));
	if (hdr.signature != IP_ACM_SIG) {
		return 0;
	}
	samples_left = (samples = hdr.samples);
	channels = hdr.channels;
	samplerate = hdr.rate;
	levels = hdr.levels;
	subblocks = hdr.subblocks;

	block_size = (1 << levels) * subblocks;
	block = new long [block_size]; 
	if (!block) 
		return 0;
	unpacker = new CValueUnpacker (levels, subblocks, stream); 
	if ( !unpacker || !unpacker->init_unpacker() ) 
		return 0;
	decoder = new CSubbandDecoder (levels); 
	if ( !decoder || !decoder->init_decoder() ) 
		return 0;
	return 1;
}
int CACMReader::make_new_samples() {
	if ( !unpacker->get_one_block (block) ) 
		return 0;

	long* test = block;
	for (int i=0; i<block_size; i++, test++)
		if (*(short*)test != *test) 
			printf ("! %ld ", *test);

	decoder->decode_data (block, subblocks);
	values = block;
	samples_ready = (block_size > samples_left) ? samples_left: block_size;
	samples_left -= samples_ready;

	return 1;
	}
long CACMReader::read_samples (short* buffer, long count) {
	long res = 0;
	while (res < count) {
		if (samples_ready == 0) {
			if (samples_left == 0) break;
			if (!make_new_samples()) break;
		}
		*buffer = (short) ((*values) >> levels);
		values++;
		buffer++;
		res += 1;
		samples_ready--;
	}
	for (int i=res; i<count; i++, buffer++) 
		*buffer = 0;
	return res;
}

CSoundReader* CreateSoundReader (DataStream *stream, int type, long samples, bool autoFree)
{
	CSoundReader* res = NULL;
	
	switch(type)
	{
	case SND_READER_ACM:
		res = new CACMReader (stream, autoFree);
		break;
	case SND_READER_WAV:
		res = new CWavPCMReader (stream, samples, autoFree);
		break;
	}
	if (res)
	{
		if ( !res->init_reader() )
		{
				delete res;
				res = NULL;
			}
	}
	return res;
}

short CSoundReader::read_one_sample() {
	short res;
	if ( !read_samples (&res, 1) )
		res = 0; // no more samples left => return 0;
	return res;
}

int CRawPCMReader::init_reader () {
	if(samples<0)
	{
		samples = stream->Size();
		stream->Seek(0, GEM_STREAM_START);
		//fseek (file, 0, SEEK_END); samples = ftell (file); rewind (file);
		if (is16bit) 
			samples >>= 1; // each sample has 16 bit
	}
	samples_left = samples;
	return 1;
}

long CRawPCMReader::read_samples (short* buffer, long count) {
	long bkup_count = count;
	if (count > samples_left) count = samples_left;
	long res = 0, i;
	if (count)
		res = stream->Read(buffer, count*((is16bit?2:1)));
		//res = fread (buffer, (is16bit)?2:1, count, file);
	if (!is16bit) {
		char* alt_buff = (char*) buffer;
		for (i=res-1; i>=0; i--) {
			alt_buff [(i << 1) + 1] = (char) (alt_buff [i] - 0x80);
			alt_buff [i << 1] = 0;
		}
	}
	buffer += res;
	for (i=res; i<bkup_count; i++, buffer++) *buffer = 0;
	samples_left -= res;
	return res;
}

int CWavPCMReader::init_reader () {
	int res = CRawPCMReader::init_reader (); if (!res) return res;

	cWAVEFORMATEX fmt;
	RIFF_CHUNK r_hdr, fmt_hdr, data_hdr;
	unsigned long wave;
	memset (&fmt, 0, sizeof(fmt));
	stream->Read(&r_hdr, sizeof(r_hdr));
	stream->Read(&wave, 4);
	//fread (&r_hdr, 1, sizeof(r_hdr), file); fread (&wave, 1, 4, file);
	if (r_hdr.fourcc != *(unsigned long*)RIFF_4cc || wave != *(unsigned long*)WAVE_4cc) {
		return 0;
	}
	
	stream->Read(&fmt_hdr, sizeof(fmt_hdr));
	//fread (&fmt_hdr, 1, sizeof(fmt_hdr), file);
	if (fmt_hdr.fourcc != *(unsigned long*)fmt_4cc || fmt_hdr.length > sizeof(cWAVEFORMATEX)) {
		return 0;
	}
	stream->Read(&fmt, fmt_hdr.length);
	//fread (&fmt, 1, fmt_hdr.length, file);
	if (fmt.wFormatTag != 1) {
		return 0;
	}
	is16bit = (fmt.wBitsPerSample == 16);

	stream->Read(&data_hdr, sizeof(data_hdr));
	//fread (&data_hdr, 1, sizeof(data_hdr), file);
	if (data_hdr.fourcc == *(unsigned long*)fact_4cc) {
		stream->Seek(data_hdr.length, GEM_CURRENT_POS);
		stream->Read(&data_hdr, sizeof(data_hdr));
		//fseek (file, data_hdr.length, SEEK_CUR);
		//fread (&data_hdr, 1, sizeof(data_hdr), file);
	}
	if (data_hdr.fourcc != *(unsigned long*)data_4cc) {
		return 0;
	}

	samples = data_hdr.length;
	if (is16bit) samples >>= 1;
	samples_left = samples;
	channels=fmt.nChannels;
	samplerate=fmt.nSamplesPerSec;
	return 1;
}

