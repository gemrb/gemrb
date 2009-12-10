/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2009 The GemRB Project
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
 * $Id: BIKPlay.cpp 6168 2009-05-28 22:28:33Z mattinm $
 *
 */

/*
 * code derived from Bink Audio decoder
 * Copyright (c) 2007-2009 Peter Ross (pross@xvid.org)
 * Copyright (c) 2009 Daniel Verkamp (daniel@drv.nu)
 *
 */


#include <assert.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <cstdio>
#include "../Core/Video.h"
#include "../Core/Audio.h"
#include "../Core/Variables.h"
#include "BIKPlay.h"
#include "../../includes/ie_types.h"
#include "rational.h"

static Video *video = NULL;
static unsigned char g_palette[768];
static int g_truecolor;
static ieDword *cbAtFrame = NULL;
static ieDword *strRef = NULL;

static const int ff_wma_critical_freqs[25] = {
	100,   200,  300, 400,   510,  630,  770,    920,
	1080, 1270, 1480, 1720, 2000, 2320, 2700,   3150,
	3700, 4400, 5300, 6400, 7700, 9500, 12000, 15500,
	24500,
};

BIKPlay::BIKPlay(void)
{
	str = NULL;
	autoFree = false;
	video = core->GetVideoDriver();
	inbuff = NULL;
	maxRow = 0;
	rowCount = 0;
	frameCount = 0;
}

BIKPlay::~BIKPlay(void)
{
	if (str && autoFree) {
		delete( str );
	}
	av_freep((void **) &inbuff);
}

void BIKPlay::av_set_pts_info(AVRational &time_base, unsigned int pts_num, unsigned int pts_den)
{
	//pts_wrap_bits, if needed, is always 64
	if(av_reduce(time_base.num, time_base.den, pts_num, pts_den, INT_MAX)) {
	//bla bla, something didn't work
	}
	
	if(!time_base.num || !time_base.den)
	time_base.num = time_base.den = 0;
}

int BIKPlay::ReadHeader(DataStream *str)
{
	str->Seek(0,GEM_STREAM_START);
	str->Read( header.signature, BIK_SIGNATURE_LEN );
	str->ReadDword(&header.filesize);
	header.filesize += 8;
	str->ReadDword(&header.framecount);
	
	if (header.framecount > 1000000) {
	return -1;
	}
	
	str->ReadDword(&header.maxframesize);
	if (header.maxframesize > header.filesize) {
	return -1;
	}
	
	str->Seek(4,GEM_CURRENT_POS);
	
	str->ReadDword(&header.width);
	str->ReadDword(&header.height);
	
	ieDword fps_num, fps_den;

	str->ReadDword(&fps_num);
	str->ReadDword(&fps_den);
	
	if (fps_num == 0 || fps_den == 0) {
	//        av_log(s, AV_LOG_ERROR, "invalid header: invalid fps (%d/%d)\n", fps_num, fps_den);
	return -1;
	}
	//also sets pts_wrap_bits to 64
	av_set_pts_info(v_timebase, fps_den, fps_num);
	
	str->Seek(4,GEM_CURRENT_POS);
	str->ReadDword(&header.tracks);
	
	//we handle only single tracks, is this a problem with multi language iwd2?
	if (header.tracks > 1) {
	return -1;
	}
	
	if (header.tracks) {
	str->Seek(4 * header.tracks,GEM_CURRENT_POS);
	//make sure we use one track, if more needed, rewrite this part
	assert(header.tracks==1);
	
	str->ReadWord(&header.samplerate);
	//also sets pts_wrap_bits to 64
	//av_set_pts_info(s_timebase, 1, header.samplerate);  //unused, we simply use header.samplerate
	str->ReadWord(&header.audioflag);
	
	str->Seek(4 * header.tracks,GEM_CURRENT_POS);
	}
	
	/* build frame index table */
	ieDword pos, next_pos;
	int keyframe;

	str->ReadDword(&pos);
	keyframe = pos & 1;
	pos &= ~1;

	frames.reserve(header.framecount);
	for (unsigned int i = 0; i < header.framecount; i++) {
	if (i == header.framecount - 1) {
		next_pos = header.filesize;
	} else {
		str->ReadDword(&next_pos);
	}
	if (next_pos <= pos) {
		//            av_log(s, AV_LOG_ERROR, "invalid frame index table\n");
		return -1;
	}
	//offset, size, keyframe
	binkframe frame;

	//the order of instructions is important here!
	frame.pos=pos;
	frame.keyframe=keyframe;
	pos = next_pos&~1;    
	keyframe = next_pos&1;
	frame.size=pos-frame.pos;
	//sanity hack, we might as well just go belly up and refuse playing
	if (frame.size>header.maxframesize) {
		frame.size = header.maxframesize;
	}

	frames.push_back(frame);

	}
	inbuff = (ieByte *) av_malloc(header.maxframesize);
	if (!inbuff) {
	return -2;
	}
	
	str->Seek(4, GEM_CURRENT_POS);
	return 0;
}

bool BIKPlay::Open(DataStream* stream, bool autoFree)
{
	validVideo = false;
	if (stream == NULL) {
		return false;
	}
	if (str && this->autoFree) {
		delete( str );
	}
	str = stream;
	this->autoFree = autoFree;
	
	str->Read( &header.signature, BIK_SIGNATURE_LEN );
	if (memcmp( header.signature, BIK_SIGNATURE_DATA, 4 ) == 0) {
	validVideo = ReadHeader(stream)==0;
	return validVideo;
	}
	return false;
}

void BIKPlay::CallBackAtFrames(ieDword cnt, ieDword *arg, ieDword *arg2 )
{
	maxRow = cnt;
	frameCount = 0;
	rowCount = 0;
	cbAtFrame = arg;
	strRef = arg2;
}

int BIKPlay::Play()
{
	if (!validVideo) {
		return 0;
	}
	//Start Movie Playback
	frameCount = 0;
	int ret = doPlay( );

	EndAudio();
	av_freep((void **) &inbuff);
	return ret;
}

//this code could be in the movieplayer parent class
void get_current_time(long &sec, long &usec) {
#ifdef _WIN32
	DWORD time;
	time = GetTickCount();

	sec = time / 1000;
	usec = (time % 1000) * 1000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);

	sec = tv.tv_sec;
	usec = tv.tv_usec;
#endif
} 

void BIKPlay::timer_start()
{
	get_current_time(timer_last_sec, timer_last_usec);
}

void BIKPlay::timer_wait()
{
	long sec, usec;
	get_current_time(sec, usec);

	while (sec > timer_last_sec) {
		usec += 1000000;
		timer_last_sec++;
	}

	//quick hack, we should rather use the rational time base as ffmpeg
	frame_wait = v_timebase.num*1000000/v_timebase.den;
	while (usec - timer_last_usec > (long)frame_wait) {
		usec -= frame_wait;
		video_frameskip++;
	}

	long to_sleep = frame_wait - (usec - timer_last_usec);
#ifdef _WIN32
	Sleep(to_sleep / 1000);
#else
	usleep(to_sleep);
#endif

	timer_start();
}

bool BIKPlay::next_frame()
{
	if (timer_last_sec) timer_wait();
	if(frameCount>=header.framecount) {
	return false;
	}
	binkframe frame = frames[frameCount++];
	frame.size = fileRead( frame.pos, inbuff, frame.size);
	ieDword audframesize = *(ieDword *) inbuff;
	DecodeAudioFrame(inbuff+4, audframesize);
	//DecodeVideoFrame(inbuff+audframesize+4, frame.size-audframesize-4);
	if (!timer_last_sec) timer_start();
	return true;
}

int BIKPlay::doPlay()
{
	int done = 0;

	//bink is always truecolor
	g_truecolor = 1;
	//palette is not really needed
	memset( g_palette, 0, 768 );

	frame_wait = 0;
	timer_last_sec = 0;
	video_frameskip = 0;

	if (sound_init( core->GetAudioDrv()->CanPlay())) {
	//sound couldn't be initialized
	return 1;
	}

	video->InitMovieScreen(outputwidth,outputheight);
	//meeh, signed/unsigned comparison sucks
	if (outputwidth<(signed) header.width || outputheight<(signed) header.height) {
	//movie dimensions are higher than available screen
	return 2;
	}
	//video_init(w,h);
	
	while (!done && next_frame()) {
		done = video->PollMovieEvents();
	}

	return 0;
}

unsigned int BIKPlay::fileRead(unsigned int pos, void* buf, unsigned int count)
{
	unsigned numread;

	str->Seek(pos, GEM_STREAM_START);
	numread = str->Read( buf, count );
	return ( numread == count );
}

void BIKPlay::showFrame(unsigned char* buf, unsigned int bufw,
	unsigned int bufh, unsigned int sx, unsigned int sy, unsigned int w,
	unsigned int h, unsigned int dstx, unsigned int dsty)
{
	ieDword titleref = 0;

	if (cbAtFrame && strRef) {
		frameCount ++;
		if ((rowCount<maxRow) && (frameCount >= cbAtFrame[rowCount]) ) {
			rowCount++;
		}
		//draw subtitle here
		if (rowCount) {
			titleref = strRef[rowCount-1];
		}
	}
	video->showFrame(buf,bufw,bufh,sx,sy,w,h,dstx,dsty, g_truecolor, g_palette, titleref);
}

int BIKPlay::setAudioStream()
{
	ieDword volume;
	core->GetDictionary()->Lookup( "Volume Movie", volume) ;
	int source = core->GetAudioDrv()->SetupNewStream(0, 0, 0, volume, false, false);
	return source;
}

void BIKPlay::freeAudioStream(int stream)
{
	if (stream > -1)
		core->GetAudioDrv()->ReleaseStream(stream, true);
}

void BIKPlay::queueBuffer(int stream, unsigned short bits, int channels, short* memory, int size, int samplerate)
{
	if (stream > -1)
		core->GetAudioDrv()->QueueBuffer(stream, bits, channels, memory, size, samplerate);
}


/**
 * @file libavcodec/binkaudio.c
 *
 * Technical details here:
 *  http://wiki.multimedia.cx/index.php?title=Bink_Audio
 */

int BIKPlay::sound_init(bool need_init)
{
	int sample_rate = header.samplerate;
	int sample_rate_half;
	unsigned int i;
	int frame_len_bits;
	int ret;
	
	if(need_init) {
	s_stream = setAudioStream();
	} else {
	s_stream = -1;
	return 0;
	}

	if(s_stream<0) {
	return 0;
	}
	
	if(header.audioflag&BINK_AUD_STEREO) {
	header.channels=2;
	}
	
	/* determine frame length */
	if (sample_rate < 22050) {
	frame_len_bits = 9;
	} else if (sample_rate < 44100) {
	frame_len_bits = 10;
	} else {
	frame_len_bits = 11;
	}
	//audio frame length
	s_frame_len = 1 << frame_len_bits;
	
	if (header.channels > MAX_CHANNELS) {
	//av_log(s->avctx, AV_LOG_ERROR, "too many channels: %d\n", s->channels);
	return -1;
	}
	
	if (header.audioflag&BINK_AUD_USEDCT) {
	s_channels = header.channels;
	} else {
	// audio is already interleaved for the RDFT format variant
	sample_rate *= header.channels;
	s_frame_len *= header.channels;
	s_channels = 1;
	if (header.channels == 2)
		frame_len_bits++;
	}
	
	s_overlap_len   = s_frame_len / 16;
	s_block_size    = (s_frame_len - s_overlap_len) * s_channels;
	sample_rate_half = (sample_rate + 1) / 2;
	s_root          = (float) (2.0 / sqrt(s_frame_len));
	
	/* calculate number of bands */
	for (s_num_bands = 1; s_num_bands < 25; s_num_bands++) {
	if (sample_rate_half <= ff_wma_critical_freqs[s_num_bands - 1]) {
		break;
	}
	}
	
	s_bands = (unsigned int *) av_malloc((s_num_bands + 1) * sizeof(*s_bands));
	if (!s_bands) {
	return -2;
	}
	
	/* populate bands data */
	s_bands[0] = 1;
	for (i = 1; i < s_num_bands; i++)
	s_bands[i] = ff_wma_critical_freqs[i - 1] * (s_frame_len / 2) / sample_rate_half;
	s_bands[s_num_bands] = s_frame_len / 2;
	
	s_first = 1;
	
	for (i = 0; i < s_channels; i++)
	s_coeffs_ptr[i] = s_coeffs + i * s_frame_len;
	
	if (header.audioflag&BINK_AUD_USEDCT)
	ret = ff_dct_init(&s_trans.dct, frame_len_bits, 0);
	else
	ret = ff_rdft_init(&s_trans.rdft, frame_len_bits, IRIDFT);
	
	return ret;
}

int BIKPlay::EndAudio()
{
	freeAudioStream(s_stream);
	av_freep((void **) &s_bands);
	if (header.audioflag&BINK_AUD_USEDCT)
	ff_dct_end(&s_trans.dct);
	else
	ff_rdft_end(&s_trans.rdft);
	return 0;
} 

static const uint8_t rle_length_tab[16] = {
	2, 3, 4, 5, 6, 8, 9, 10, 11, 12, 13, 14, 15, 16, 32, 64
};

const uint8_t ff_log2_tab[256]={
		0,0,1,1,2,2,2,2,3,3,3,3,3,3,3,3,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
		5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,
		7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7
};
 
static inline int float_to_int16_one(const float *src){
	register int tmp = *(const int32_t*)src;
	if(tmp & 0xf0000){
		tmp = (0x43c0ffff - tmp)>>31;
		// is this faster on some gcc/cpu combinations?
//      if(tmp > 0x43c0ffff) tmp = 0xFFFF;
//      else                 tmp = 0;
	}
	return tmp - 0x8000;
} 

static void ff_float_to_int16_interleave_c(int16_t *dst, const float **src, long len, int channels){
	int i;
	if(channels==2){
		for(i=0; i<len; i++){
			dst[2*i]   = float_to_int16_one(src[0]+i);
			dst[2*i+1] = float_to_int16_one(src[1]+i);
		}
		return;
	}
	//one channel
	for(i=0; i<len; i++)
		dst[i] = float_to_int16_one(src[0]+i);
}

/**
 * Decode Bink Audio block
 * @param[out] out Output buffer (must contain s->block_size elements)
 */
void BIKPlay::DecodeBlock(short *out)
{
	unsigned int ch, i, j, k;
	float q, quant[25];
	int width, coeff;

	if (header.audioflag&BINK_AUD_USEDCT) {
		s_gb.skip_bits(2);
	}

	for (ch = 0; ch < s_channels; ch++) {
		FFTSample *coeffs = s_coeffs_ptr[ch];
		q = 0.0;
		coeffs[0] = s_gb.get_float();
		coeffs[1] = s_gb.get_float();

		for (i = 0; i < s_num_bands; i++) {
			int value = s_gb.get_bits(8);
			quant[i] = (float) pow(10.0, FFMIN(value, 95) * 0.066399999);
		}

		// find band (k)
		for (k = 0; s_bands[k] * 2 < 2; k++) {
			q = quant[k];
		}

		// parse coefficients
		i = 2;
		while (i < s_frame_len) {
			if (s_gb.get_bits(1)) {
				j = i + rle_length_tab[s_gb.get_bits(4)] * 8;
			} else {
				j = i + 8;
			}

			if (j > s_frame_len)
				j = s_frame_len;

			width = s_gb.get_bits(4);
			if (width == 0) {
				memset(coeffs + i, 0, (j - i) * sizeof(*coeffs));
				i = j;
				while (s_bands[k] * 2 < i)
					q = quant[k++];
			} else {
				while (i < j) {
					if (s_bands[k] * 2 == i)
						q = quant[k++];
					coeff = s_gb.get_bits(width);
					if (coeff) {
						if (s_gb.get_bits(1))
							coeffs[i] = -q * coeff;
						else
							coeffs[i] =  q * coeff;
					} else {
						coeffs[i] = 0.0;
					}
					i++;
				}
			}
		}

		if (header.audioflag&BINK_AUD_USEDCT)
			ff_dct_calc (&s_trans.dct,  coeffs);
		else
			ff_rdft_calc(&s_trans.rdft, coeffs);

		for (i = 0; i < s_frame_len; i++)
			coeffs[i] *= s_root;
	}

#if 0
    if(s_first)
    {
        unsigned int count = (unsigned int) s_frame_len;
        printf("Outbuffer floats: ");
        for(i=0;i<count;i++) {
                printf("%f ", s_coeffs_ptr[0][i]);
        }
        printf("\n");
    }
#endif 

	ff_float_to_int16_interleave_c(out, (const float **)s_coeffs_ptr, s_frame_len, s_channels);

	if (!s_first) {
		unsigned int count = s_overlap_len * s_channels;
		int shift = av_log2(count);
		for (i = 0; i < count; i++) {
			out[i] = (s_previous[i] * (count - i) + out[i] * i) >> shift;
		}
	}
#if 0
        else {
        unsigned int count = s_overlap_len * s_channels;
        printf("Outbuffer: ");
        for(i=0;i<count;i++) {
                printf("%d ", out[i]);
        }
        printf("\n");
	}
#endif

	memcpy(s_previous, out + s_block_size,
		   s_overlap_len * s_channels * sizeof(*out));

	s_first = 0;
}

//audio samples
int BIKPlay::DecodeAudioFrame(void *data, int data_size)
{
	int bits = data_size*8;
	s_gb.init_get_bits((uint8_t *) data, bits);

	unsigned int reported_size = s_gb.get_bits_long(32);
	ieWordSigned *samples = (ieWordSigned *) calloc(reported_size+s_block_size,1);
	if (!samples) {
		return -1;
	}

	ieWordSigned *outbuf = samples;
	ieWordSigned *samples_end  = samples+reported_size/sizeof(ieWordSigned);

	//s_block_size is in sample units
	while (s_gb.get_bits_count() < bits && outbuf + s_block_size <= samples_end) {
		DecodeBlock(outbuf);
		outbuf += s_block_size;
		s_gb.get_bits_align32();
	}

	unsigned int ret = (unsigned int) ((uint8_t*)outbuf - (uint8_t*)samples);

	//sample format is signed 16 bit integers
	queueBuffer(s_stream, 16, s_channels, samples, ret, header.samplerate);

	free(samples);
	if(ret<reported_size) {
		//abort();
		return ret;
	}
	return (unsigned int) reported_size;
}

//video frame
void BIKPlay::segment_video_play()
{
	if (video_frameskip) {
		video_frameskip--;
		video_skippedframes++;
	} else {
		unsigned int dest_x = (outputwidth - header.width) >> 1;
		unsigned int dest_y = (outputheight - header.height) >> 1;
		//TODO: video buffer
		showFrame((ieByte *)0, header.width, header.height, 0, 0, header.width, header.height, dest_x, dest_y);
	}
}
