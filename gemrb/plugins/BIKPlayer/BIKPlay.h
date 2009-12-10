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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $Id: BIKPlay.h 6012 2009-05-20 19:10:20Z fuzzie $
 *
 */

#ifndef BIKPLAY_H
#define BIKPLAY_H

#include "../../includes/win32def.h"
#include "../../includes/globals.h"
#include "../Core/MoviePlayer.h"
#include "../Core/Interface.h"
#include "common.h"
#include "rational.h"
#include "GetBitContext.h"
#include "dsputil.h"

typedef struct {
	char signature[BIK_SIGNATURE_LEN];
	ieDword filesize;
	ieDword framecount;
	ieDword maxframesize;
	//ieDword framecount2; //unused by player
	ieDword width; 
	ieDword height;
	ieDword fps;
	ieDword divider;
	ieDword videoflag;
	ieDword tracks;  //BinkAudFlags
	//optional if tracks == 1 (original had multiple tracks)
	ieWord unknown2;
	ieWord channels;
	ieWord samplerate;
	ieWord audioflag;
	ieDword unknown4;
} binkheader;

typedef struct {
	int keyframe;
	ieDword pos;
	ieDword size;
} binkframe;

class BIKPlay : public MoviePlayer {

private:
	DataStream* str;
	bool autoFree;
	bool validVideo;
	binkheader header;
	std::vector<binkframe> frames;
	ieByte *inbuff;

	//subtitle and frame counting
	ieDword maxRow;
	ieDword rowCount;
	ieDword frameCount;
	
	//audio context (consider packing it in a struct)
	unsigned int s_frame_len;
	unsigned int s_channels;
	int s_overlap_len;
	int s_block_size;
	unsigned int *s_bands;
	float s_root;
	unsigned int s_num_bands;
	int s_first;
	bool s_audio;
	int s_stream;  //openal stream handle

#pragma pack(push,16)
	FFTSample s_coeffs[BINK_BLOCK_MAX_SIZE];
	short s_previous[BINK_BLOCK_MAX_SIZE / 16];  ///< coeffs from previous audio block
#pragma pack(pop)

	float *s_coeffs_ptr[MAX_CHANNELS]; ///< pointers to the coeffs arrays for float_to_int16_interleave

	union {
	      RDFTContext rdft;
	      DCTContext dct;
	} s_trans;
	GetBitContext s_gb;

	//video context (consider packing it in a struct)
	AVRational v_timebase;
	long timer_last_sec;
	long timer_last_usec;
	unsigned int frame_wait;
 	bool video_rendered_frame;
	unsigned int video_frameskip;
	bool done;
	int outputwidth, outputheight;
	unsigned int video_skippedframes;
 
private:
	void timer_start();
	void timer_wait();
	void segment_video_play();
	bool next_frame();
	int doPlay();
	unsigned int fileRead(unsigned int pos, void* buf, unsigned int count);
	void showFrame(unsigned char* buf, unsigned int bufw,
		unsigned int bufh, unsigned int sx, unsigned int sy,
		unsigned int w, unsigned int h, unsigned int dstx,
		unsigned int dsty);
	//void setPalette(unsigned char* p, unsigned start, unsigned count);
	int pollEvents();
	int setAudioStream();
	void freeAudioStream(int stream);
	void queueBuffer(int stream, unsigned short bits,
	              int channels, short* memory,
	              int size, int samplerate);
	int sound_init(bool need_init);
	void av_set_pts_info(AVRational &time_base, unsigned int pts_num, unsigned int pts_den);
	int ReadHeader(DataStream *str);
	void DecodeBlock(short *out);
	int DecodeAudioFrame(void *data, int data_size);
	int DecodeVideoFrame(void *data, int data_size);
	int EndAudio();
	int PlayBik(DataStream *stream);
public:
	BIKPlay(void);
	~BIKPlay(void);
	bool Open(DataStream* stream, bool autoFree = true);
	void CallBackAtFrames(ieDword cnt, ieDword *arg, ieDword *arg2);
	int Play();	

public:
	void release(void)
	{
		delete this;
	}
};

#endif
