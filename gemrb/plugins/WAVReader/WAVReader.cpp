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
 */

#include "WAVReader.h"

#include <utility>

using namespace GemRB;

// WAVEFORMATEX structure (from MS SDK)
struct cWAVEFORMATEX {
	unsigned short wFormatTag;  	   /* format type */
	unsigned short nChannels;   	   /* number of channels (i.e. mono, stereo...) */
	unsigned int nSamplesPerSec;     /* sample rate */
	unsigned int nAvgBytesPerSec;    /* for buffer estimation */
	unsigned short nBlockAlign; 	   /* block size of data */
	unsigned short wBitsPerSample;     /* number of bits per sample of mono data */
	unsigned short cbSize;  		   /* the count in bytes of the size of */
	/* extra information (after cbSize) */
};

struct RIFF_CHUNK {
	unsigned int fourcc;
	unsigned int length;
};

const unsigned char RIFF_4cc[] = {
	'R', 'I', 'F', 'F'
};
const unsigned char WAVE_4cc[] = {
	'W', 'A', 'V', 'E'
};
const unsigned char fmt_4cc[] = {
	'f', 'm', 't', ' '
};
const unsigned char fact_4cc[] = {
	'f', 'a', 'c', 't'
};
const unsigned char data_4cc[] = {
	'd', 'a', 't', 'a'
};

bool RawPCMReader::Open(DataStream* stream)
{
	str = stream;

	samples = str->Size();
	str->Seek( 0, GEM_STREAM_START );
	if (is16bit) {
		samples >>= 1; // each sample has 16 bit
	}
	samples_left = samples;
	return 1;
}

inline void fix_endian(ieDword &dest)
{
	std::swap(((unsigned char *) &dest)[0],((unsigned char *) &dest)[3]);
	std::swap(((unsigned char *) &dest)[1],((unsigned char *) &dest)[2]);
}

inline void fix_endian(ieWord &dest)
{
	std::swap(((unsigned char *) &dest)[0],((unsigned char *) &dest)[1]);
}

int RawPCMReader::read_samples(short* buffer, int count)
{
	if (count > samples_left) {
		count = samples_left;
	}
	int res = 0;
	if (count) {
		res = str->Read( buffer, count * ( ( is16bit ? 2 : 1 ) ) );
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
		if (str->IsEndianSwitch()) {
			for (size_t i = 0; i < (size_t)count; i++) {
				fix_endian(((ieWord *)buffer)[i]);
			}
		}
	}
	samples_left -= res;
	return res;
}

bool WavPCMReader::Open(DataStream* stream)
{
	if (!RawPCMReader::Open(stream))
		return false;

	char Signature[4];
	stream->Read( Signature, 4 );
	stream->Seek( 0, GEM_STREAM_START );
	if(strnicmp(Signature, "RIFF", 4) != 0)
		return false;

	cWAVEFORMATEX fmt;
	RIFF_CHUNK r_hdr, fmt_hdr, data_hdr;
	unsigned int wave;
	memset( &fmt, 0, sizeof( fmt ) );

	//str->Read( &r_hdr, sizeof( r_hdr ) );
	//don't swap this
	str->Read(&r_hdr.fourcc, 4);
	str->ReadDword(&r_hdr.length);
	//don't swap this
	str->Read( &wave, 4 );
	if (memcmp(&r_hdr.fourcc, RIFF_4cc, 4) != 0 ||
		memcmp(&wave, WAVE_4cc, 4) != 0) {
		return false;
	}

	//str->Read( &fmt_hdr, sizeof( fmt_hdr ) );
	//don't swap this
	str->Read(&fmt_hdr.fourcc,4);
	str->ReadDword(&fmt_hdr.length);
	if (memcmp(&fmt_hdr.fourcc, fmt_4cc, 4) != 0 ||
		fmt_hdr.length > sizeof( cWAVEFORMATEX )) {
		return false;
	}
	memset(&fmt,0,sizeof(fmt) );
	str->Read( &fmt, fmt_hdr.length );
	//hmm, we should swap fmt bytes if we are on a mac
	//but we don't know exactly how much of the structure we'll read
	//so we have to swap the bytes after reading them
	if (str->IsEndianSwitch()) {
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
		return false;
	}
	is16bit = ( fmt.wBitsPerSample == 16 );

	//str->Read( &data_hdr, sizeof( data_hdr ) );
	//don't swap this
	str->Read(&data_hdr.fourcc,4);
	str->ReadDword(&data_hdr.length);

	if (memcmp(&data_hdr.fourcc, fact_4cc, 4) == 0) {
		str->Seek( data_hdr.length, GEM_CURRENT_POS );
		//str->Read( &data_hdr, sizeof( data_hdr ) );
		str->ReadDword(&data_hdr.fourcc);
		str->ReadDword(&data_hdr.length);
	}
	if (memcmp(&data_hdr.fourcc, data_4cc, 4) != 0) {
		return false;
	}

	samples = data_hdr.length;
	if (is16bit) {
		samples >>= 1;
	}
	samples_left = samples;
	channels = fmt.nChannels;
	samplerate = fmt.nSamplesPerSec;
	return true;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x11BB1288, "WAV File Importer")
PLUGIN_IE_RESOURCE(WavPCMReader, "wav", (ieWord)IE_WAV_CLASS_ID)
END_PLUGIN()
