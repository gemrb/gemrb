//#include "stdafx.h"
// Writes the RIFF header into a stream

#include <stdio.h>
#include <string.h>
#include "riffhdr.h"

RIFF_HEADER riff = {
	{'R', 'I', 'F', 'F'},
	0,
	{'W', 'A', 'V', 'E', 'f', 'm', 't', ' '},
	16, 1, 2,
	22050, 22050*4,
	4, 16,
	{'d', 'a', 't', 'a'}
};

void write_riff_header (void *memory, long samples, int channels, int samplerate) {
	riff.raw_data_len = samples * sizeof(short);
	riff.total_len_m8 = riff.raw_data_len + sizeof(RIFF_HEADER) - 8;
  riff.nChannels=(unsigned short) channels;  
  riff.nSamplesPerSec=samplerate;
  riff.nAvgBytesPerSec=channels*sizeof(short)*samplerate;
	memcpy(memory, &riff, sizeof(RIFF_HEADER) );
}

WAVC_HEADER wavc={
  {'W','A','V','C'},
  {'V','1','.','0'},
  0,0,28,
  2,16,22050,0x9ffdu
};

void write_wavc_header (FILE *fpoi, long samples, int channels, int compressed, int samplerate) {
  wavc.uncompressed=samples*sizeof(short);
  wavc.compressed=compressed;
	wavc.channels=(short) channels;
  wavc.samplespersec=(short) samplerate;
  rewind(fpoi);
	fwrite(&wavc, 1, sizeof(WAVC_HEADER) ,fpoi );
}
