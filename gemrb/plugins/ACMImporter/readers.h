#ifndef _ACM_LAB_SOUND_READER_H
#define _ACM_LAB_SOUND_READER_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "unpacker.h"
#include "decoder.h"
#include "general.h"

#define INIT_NO_ERROR_MSG 0
#define INIT_NEED_ERROR_MSG 1

// Abstract Sound Reader class
class CSoundReader {
protected:
	long samples; // total count of sound samples
  long channels;
  long samplerate;
	long samples_left; // count of unread samples
	int is16bit; // 1 - if 16 bit file, 0 - otherwise
	FILE* file; // file handle

public:
	CSoundReader (int fhandle)
    : samples (0), channels(0), samples_left (0), is16bit (1), file (NULL) 
  { file=fdopen(fhandle,"rb");
  };

	virtual ~CSoundReader() {
		if (file) fclose (file);
	};

  long get_channels()
  {
    return channels;
  }
  long get_samplerate() { return samplerate; }
	virtual int init_reader () = 0; // initializes the sound reader

	long get_length() { return samples; }; // returns the total samples count
	long get_samples_left() { return samples_left; };
	int get_bits() { return (is16bit)?16:8; };

	virtual const char* get_file_type() = 0;

	virtual long read_samples (short* buffer, long count) = 0; // returns actual count of read samples
	virtual short read_one_sample(); // returns next sound sample
};

// RAW file reader
class CRawPCMReader: public CSoundReader {
public:
	CRawPCMReader (int fhandle, int bits, int len)
		: CSoundReader (fhandle) {
			is16bit = (bits == 16);
      samples=len;
		};

	virtual int init_reader ();
	virtual long read_samples (short* buffer, long count);
	virtual const char* get_file_type() { return (is16bit? "RAW16": "RAW8"); };
};

// WAV files
class CWavPCMReader: public CRawPCMReader {
public:
	CWavPCMReader (int fhandle,long len)
		: CRawPCMReader (fhandle, 16, len) {};
	virtual int init_reader ();
	virtual const char* get_file_type() { return "WAV"; };
};

// IP's ACM files
class CACMReader: public CSoundReader {
private:
	int levels, subblocks;
	int block_size;
	long *block, *values;
	long samples_ready;
	CValueUnpacker* unpacker; // ACM-stream unpacker
	CSubbandDecoder* decoder; // IP's subband decoder

	int make_new_samples();
public:
	CACMReader (int fhandle)
		: CSoundReader (fhandle),
		block (NULL), values (NULL),
		samples_ready (0),
		unpacker (NULL), decoder (NULL) {};
	virtual ~CACMReader() {
		if (block) delete block;
		if (unpacker) delete unpacker;
		if (decoder) delete decoder;
	};

	virtual int init_reader ();
	virtual const char* get_file_type() { return "ACM"; };
	virtual long read_samples (short* buffer, long count);

	int get_levels() { return levels; };
	int get_subblocks() { return subblocks; }
};


// WAVEFORMATEX structure (from MS SDK)
typedef struct
{
	unsigned short wFormatTag;         /* format type */
	unsigned short nChannels;          /* number of channels (i.e. mono, stereo...) */
	unsigned long  nSamplesPerSec;     /* sample rate */
	unsigned long  nAvgBytesPerSec;    /* for buffer estimation */
	unsigned short nBlockAlign;        /* block size of data */
	unsigned short wBitsPerSample;     /* number of bits per sample of mono data */
	unsigned short cbSize;             /* the count in bytes of the size of */
					   /* extra information (after cbSize) */
} cWAVEFORMATEX;

typedef struct {
	unsigned long fourcc;
	unsigned long length;
} RIFF_CHUNK;

const unsigned char RIFF_4cc[] = { 'R', 'I', 'F', 'F' };
const unsigned char WAVE_4cc[] = { 'W', 'A', 'V', 'E' };
const unsigned char fmt_4cc[]  = { 'f', 'm', 't', ' ' };
const unsigned char fact_4cc[] = { 'f', 'a', 'c', 't' };
const unsigned char data_4cc[] = { 'd', 'a', 't', 'a' };


// File open routine.
CSoundReader* CreateSoundReader (int fhandle, int open_mode, long samples);

// Open modes:
#define SND_READER_AUTO 0
#define SND_READER_RAW8 1
#define SND_READER_RAW16 2
#define SND_READER_WAV 3
#define SND_READER_ACM 4

#endif
