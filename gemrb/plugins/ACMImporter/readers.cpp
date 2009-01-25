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
 * $Id$
 *
 */

//#include "stdafx.h"
// Classes for sound files.
// Supported formats: PCM-RAW, PCM-WAV (both 8 and 16 bits),
//   Ogg Vorbis and Interplay's ACM.

#include <stdio.h>
#include "readers.h"
#include "general.h"

#ifdef HAS_VORBIS_SUPPORT
static size_t ovfd_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	DataStream *vb = (DataStream *) datasource;
	int bytesToRead = size * nmemb;

	int remains = vb->Remains();
	if(remains<=0) {
		/* no more reading, we're at the end */
		return 0;
	}
	if(bytesToRead > remains ) {
		bytesToRead = remains;
	}
	vb->Read(ptr, bytesToRead);
	return bytesToRead;
}

static int ovfd_seek(void *datasource, int64_t offset, int whence) {
	DataStream *vb = (DataStream *) datasource;
	switch(whence) {
		case SEEK_SET:
			if(vb->Seek(offset, GEM_STREAM_START)<0)
				return -1;
			break;
		case SEEK_CUR:
			if(vb->Seek(offset, GEM_CURRENT_POS)<0)
				return -1;
			break;
		case SEEK_END:
			if(vb->Seek(vb->Size()+offset, GEM_STREAM_START)<0)
				return -1;
			break;
		default:
			return -1;
	}
	return vb->GetPos();
}

static int ovfd_close(void * /*datasource*/) {
	return 0;
}

static long ovfd_tell(void *datasource) {
	DataStream *vb = (DataStream *) datasource;
	return (long) vb->GetPos();
}

int COGGReader::init_reader()
{
	vorbis_info *info;
	int res;
	ov_callbacks cbstruct = {
		ovfd_read, ovfd_seek, ovfd_close, ovfd_tell
	};

	res=ov_open_callbacks(stream, &OggStream, NULL, 0, cbstruct);
	if(res<0) {
		printMessage("Sound","Couldn't initialize vorbis!\n", LIGHT_RED);
		return 0;
	}
	info = ov_info(&OggStream, -1);
	channels = info->channels;
	samplerate = info->rate;
	samples_left = ( samples = ov_pcm_total(&OggStream, -1) );
	return 1;
}

int COGGReader::read_samples(short* buffer, int count)
{
	int whatisthis;

	if(samples_left<count) {
		count=samples_left;
	}
	int samples_got=0;
	int samples_need=count;
	while(samples_need) {
		int rd=ov_read(&OggStream, (char *)buffer, samples_need<<1, 0, 2, 1, &whatisthis);
		if(rd==OV_HOLE) {
			continue;
		}
		if(rd<=0) {
			break;
		}
		rd>>=1;
		buffer+=rd;
		samples_got+=rd;
		samples_need-=rd;
	}
	samples_left-=samples_got;
	return samples_got;
}

#endif

int CACMReader::init_reader()
{
	ACM_Header hdr;
	char tmp[4];

	stream->Read( tmp, sizeof( tmp ) );
	if (!memcmp( tmp, "WAVC", 4 )) {
		stream->Seek( 24, GEM_CURRENT_POS );
	} else {
		stream->Seek( -4, GEM_CURRENT_POS );
	}
	//stream->Read( &hdr, sizeof( ACM_Header ) );
	//maybe this'll work on a PPC

	stream->ReadDword( &hdr.signature );
	stream->ReadDword( &hdr.samples );
	stream->ReadWord( &hdr.channels );
	stream->ReadWord( &hdr.rate );
	ieWord tmpword;
	stream->ReadWord( &tmpword );
	//subblocks = (int) (tmpword&0xfff);
	//levels = (int) (tmpword>>12);
	subblocks = (int) (tmpword>>4);
	levels = (int) (tmpword&15);

	if (hdr.signature != IP_ACM_SIG) {
		return 0;
	}
	samples_left = ( samples = hdr.samples );
	channels = hdr.channels;
	samplerate = hdr.rate;
	//levels = hdr.levels;
	//subblocks = hdr.subblocks;

	block_size = ( 1 << levels ) * subblocks;
	//using malloc for simple arrays (supposed to be faster)
	block = (int *) malloc(sizeof(int)*block_size);
	if (!block) {
		return 0;
	}
	unpacker = new CValueUnpacker( levels, subblocks, stream ); 
	if (!unpacker || !unpacker->init_unpacker()) {
		return 0;
	}
	decoder = new CSubbandDecoder( levels ); 
	if (!decoder || !decoder->init_decoder()) {
		return 0;
	}
	return 1;
}
int CACMReader::make_new_samples()
{
	if (!unpacker->get_one_block( block )) {
		return 0;
	}

	decoder->decode_data( block, subblocks );
	values = block;
	samples_ready = ( block_size > samples_left ) ? samples_left : block_size;
	samples_left -= samples_ready;

	return 1;
}

int CACMReader::read_samples(short* buffer, int count)
{
	int res = 0;
	while (res < count) {
		if (samples_ready == 0) {
			if (samples_left == 0)
				break;
			if (!make_new_samples())
				break;
		}
		*buffer = ( short ) ( ( *values ) >> levels );
		values++;
		buffer++;
		res += 1;
		samples_ready--;
	}
	return res;
}

CSoundReader* CreateSoundReader(DataStream* stream, int type, int samples,
	bool autoFree)
{
	CSoundReader* res = NULL;

	switch (type) {
#ifdef HAS_VORBIS_SUPPORT
		case SND_READER_OGG:
			res = new COGGReader( stream, autoFree );
			break;
#endif
		case SND_READER_ACM:
			res = new CACMReader( stream, autoFree );
			break;
		case SND_READER_WAV:
			res = new CWavPCMReader( stream, samples, autoFree );
			break;
		default:
			if (autoFree) delete stream;
			break;
	}
	if (res) {
		if (!res->init_reader()) {
			delete res;
			res = NULL;
		}
	}
	return res;
}

int CRawPCMReader::init_reader()
{
	if (samples < 0) {
		samples = stream->Size();
		stream->Seek( 0, GEM_STREAM_START );
		if (is16bit) {
			samples >>= 1; // each sample has 16 bit
		}
	}
	samples_left = samples;
	return 1;
}

inline void fix_endian(ieDword &dest)
{
	unsigned char tmp;
	tmp=((unsigned char *) &dest)[0];
	((unsigned char *) &dest)[0]=((unsigned char *) &dest)[3];
	((unsigned char *) &dest)[3]=tmp;
	tmp=((unsigned char *) &dest)[1];
	((unsigned char *) &dest)[1]=((unsigned char *) &dest)[2];
	((unsigned char *) &dest)[2]=tmp;

}

inline void fix_endian(ieWord &dest)
{
	unsigned char tmp;
	tmp=((unsigned char *) &dest)[0];
	((unsigned char *) &dest)[0]=((unsigned char *) &dest)[1];
	((unsigned char *) &dest)[1]=tmp;
}


int CRawPCMReader::read_samples(short* buffer, int count)
{
	if (count > samples_left) {
		count = samples_left;
	}
	int res = 0;
	if (count) {
		res = stream->Read( buffer, count * ( ( is16bit ? 2 : 1 ) ) );
	}
	if (!is16bit) {
		char* alt_buff = ( char* ) buffer;
		int i = res;
		while(i--) {
			alt_buff[( i << 1 ) + 1] = ( char ) ( alt_buff[i] - 0x80 );
			alt_buff[i << 1] = 0;
		}
	}
	if(is16bit) {
		res >>= 1;
		if (stream->IsEndianSwitch()) {
			for (size_t i = 0; i < (size_t)count; i++) {
				fix_endian(((ieWord *)buffer)[i]);
			}
		}
	}
	samples_left -= res;
	return res;
}

int CWavPCMReader::init_reader()
{
	int res = CRawPCMReader::init_reader(); if (!res) {
												return res;
	}

	cWAVEFORMATEX fmt;
	RIFF_CHUNK r_hdr, fmt_hdr, data_hdr;
	unsigned int wave;
	memset( &fmt, 0, sizeof( fmt ) );

	//stream->Read( &r_hdr, sizeof( r_hdr ) );
	//don't swap this
	stream->Read(&r_hdr.fourcc, 4);
	stream->ReadDword(&r_hdr.length);
	//don't swap this
	stream->Read( &wave, 4 );
	if (r_hdr.fourcc != *( unsigned int * ) RIFF_4cc ||
		wave != *( unsigned int * ) WAVE_4cc) {
		return 0;
	}

	//stream->Read( &fmt_hdr, sizeof( fmt_hdr ) );
	//don't swap this
	stream->Read(&fmt_hdr.fourcc,4);
	stream->ReadDword(&fmt_hdr.length);
	if (fmt_hdr.fourcc != *( unsigned int * ) fmt_4cc ||
		fmt_hdr.length > sizeof( cWAVEFORMATEX )) {
		return 0;
	}
	memset(&fmt,0,sizeof(fmt) );
	stream->Read( &fmt, fmt_hdr.length );
	//hmm, we should swap fmt bytes if we are on a mac
	//but we don't know exactly how much of the structure we'll read
	//so we have to swap the bytes after reading them
	if (stream->IsEndianSwitch()) {
		fix_endian(fmt.wFormatTag);
		fix_endian(fmt.nChannels);
		fix_endian(fmt.nSamplesPerSec);
		fix_endian(fmt.wBitsPerSample);
		//we don't use these fields, so who cares
		//fix_endian(fmt.nAvgBytesPerSec);
		//fix_endian(fmt.nBlockAlign);
		//fix_endian(fmt.cbSize);
	}
	if (fmt.wFormatTag != 1) {
		return 0;
	}
	is16bit = ( fmt.wBitsPerSample == 16 );

	//stream->Read( &data_hdr, sizeof( data_hdr ) );
	//don't swap this
	stream->Read(&data_hdr.fourcc,4);
	stream->ReadDword(&data_hdr.length);

	if (data_hdr.fourcc == *( unsigned int * ) fact_4cc) {
		stream->Seek( data_hdr.length, GEM_CURRENT_POS );
		//stream->Read( &data_hdr, sizeof( data_hdr ) );
		stream->ReadDword(&data_hdr.fourcc);
		stream->ReadDword(&data_hdr.length);
	}
	if (data_hdr.fourcc != *( unsigned int * ) data_4cc) {
		return 0;
	}

	samples = data_hdr.length;
	if (is16bit) {
		samples >>= 1;
	}
	samples_left = samples;
	channels = fmt.nChannels;
	samplerate = fmt.nSamplesPerSec;
	return 1;
}

