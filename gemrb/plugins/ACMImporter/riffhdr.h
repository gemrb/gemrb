#ifndef _ACM_LAB_RIFF_HEADER_H
#define _ACM_LAB_RIFF_HEADER_H

#include <stdio.h>

typedef struct {
	char riff_sig[4];
	unsigned long  total_len_m8;
	char wave_sig[8];
	unsigned long formatex_len;
	unsigned short wFormatTag, nChannels;
	unsigned long nSamplesPerSec, nAvgBytesPerSec;
	unsigned short nBlockAlign, wBitsPerSample;
	char data_sig[4];
	unsigned long raw_data_len;
} RIFF_HEADER;

typedef struct {
	char wavc_sig[4];
	char wavc_ver[4];
  long uncompressed;
  long compressed;
  long headersize;
  short channels;
  short bits;
  short samplespersec;
  short unknown;
} WAVC_HEADER;

void write_riff_header (void* memory, long samples, int channels, int samplerate);
void write_wavc_header (FILE *foutp, long samples, int channels, int compressed, int samplerate);

#endif //_ACM_LAB_RIFF_HEADER_H
