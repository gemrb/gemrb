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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/ACMImporter/readers.h,v 1.8 2004/08/10 19:11:29 guidoj Exp $
 *
 */

#ifndef _ACM_LAB_SOUND_READER_H
#define _ACM_LAB_SOUND_READER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unpacker.h"
#include "decoder.h"
#include "general.h"
#include "../Core/DataStream.h"

#define INIT_NO_ERROR_MSG 0
#define INIT_NEED_ERROR_MSG 1

// Abstract Sound Reader class
class CSoundReader {
protected:
	int samples; // total count of sound samples
	int channels;
	int samplerate;
	int samples_left; // count of unread samples
	int is16bit; // 1 - if 16 bit file, 0 - otherwise
	//FILE* file; // file handle
	DataStream* stream;
	bool autoFree;

public:
	CSoundReader(DataStream* stream, bool autoFree = true)//int fhandle)

		: samples( 0 ), channels( 0 ), samples_left( 0 ), is16bit( 1 )
	{
		//file=fdopen(fhandle,"rb");
		this->stream = stream;
		this->autoFree = autoFree;
	};

	virtual ~CSoundReader()
	{
		//if (file) fclose (file);
		if (stream && autoFree) {
			delete( stream );
		}
	};

	int get_channels()
	{
		return channels;
	}
	int get_samplerate()
	{
		return samplerate;
	}
	virtual int init_reader() = 0; // initializes the sound reader

	int get_length()
	{
		return samples;
	}; // returns the total samples count
	int get_samples_left()
	{
		return samples_left;
	};
	int get_bits()
	{
		return ( is16bit ) ? 16 : 8;
	};

	virtual const char* get_file_type() = 0;

	virtual int read_samples(short* buffer, int count) = 0; // returns actual count of read samples
//	virtual short read_one_sample(); // returns next sound sample
};

// RAW file reader
class CRawPCMReader : public CSoundReader {
public:
	CRawPCMReader(DataStream* stream, int bits, int len, bool autoFree = true)//int fhandle, int bits, int len)

		: CSoundReader( stream, autoFree )
	{
		is16bit = ( bits == 16 );
		samples = len;
	};

	virtual int init_reader();
	virtual int read_samples(short* buffer, int count);
	virtual const char* get_file_type()
	{
		return ( is16bit ? "RAW16" : "RAW8" );
	};
};

// WAV files
class CWavPCMReader : public CRawPCMReader {
public:
	CWavPCMReader(DataStream* stream, int len, bool autoFree = true) 
		: CRawPCMReader( stream, 16, len, autoFree )
	{
	};
	virtual int init_reader();
	virtual const char* get_file_type()
	{
		return "WAV";
	};
};

// IP's ACM files
class CACMReader : public CSoundReader {
private:
	int levels, subblocks;
	int block_size;
	int* block, * values;
	int samples_ready;
	CValueUnpacker* unpacker; // ACM-stream unpacker
	CSubbandDecoder* decoder; // IP's subband decoder

	int make_new_samples();
public:
	CACMReader(DataStream* stream, bool autoFree = true)//int fhandle)

		: CSoundReader( stream, autoFree ), block( NULL ), values( NULL ),
		samples_ready( 0 ), unpacker( NULL ), decoder( NULL )
	{
	};
	virtual ~CACMReader()
	{
		if (block) {
			free(block);
		}
		if (unpacker) {
			delete unpacker;
		}
		if (decoder) {
			delete decoder;
		}
	};

	virtual int init_reader();
	virtual const char* get_file_type()
	{
		return "ACM";
	};
	virtual int read_samples(short* buffer, int count);

	int get_levels()
	{
		return levels;
	};
	int get_subblocks()
	{
		return subblocks;
	}
};


// WAVEFORMATEX structure (from MS SDK)
typedef struct {
	unsigned short wFormatTag;  	   /* format type */
	unsigned short nChannels;   	   /* number of channels (i.e. mono, stereo...) */
	unsigned int nSamplesPerSec;     /* sample rate */
	unsigned int nAvgBytesPerSec;    /* for buffer estimation */
	unsigned short nBlockAlign; 	   /* block size of data */
	unsigned short wBitsPerSample;     /* number of bits per sample of mono data */
	unsigned short cbSize;  		   /* the count in bytes of the size of */
	/* extra information (after cbSize) */
} cWAVEFORMATEX;

typedef struct {
	unsigned int fourcc;
	unsigned int length;
} RIFF_CHUNK;

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


// File open routine.
CSoundReader* CreateSoundReader(DataStream* stream, int open_mode,
	int samples, bool autoFree = true);

// Open modes:
#define SND_READER_AUTO 0
#define SND_READER_RAW8 1
#define SND_READER_RAW16 2
#define SND_READER_WAV 3
#define SND_READER_ACM 4

#endif
