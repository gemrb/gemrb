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
 *
 */

/*
 * The code/specifications used as reference for this code were by:
 *
 * Mike Melanson <melanson@pcisys.net>
 * Jens Granseuer <jensgr@gmx.net>
 */

#include "mve_player.h"
#include "MVEPlayer.h"
#include "gstmvedemux.h"
#include "mve.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

/* mvevideodec8.cpp */
extern int ipvideo_decode_frame8 (const GstMveDemuxStream * s,
	const unsigned char *data, unsigned short len);
/* mvevideodec16.cpp */
extern int ipvideo_decode_frame16 (const GstMveDemuxStream * s,
	const unsigned char *data, unsigned short len);
/* mveaudiodec.cpp */
extern void ipaudio_uncompress (short *buffer,
	unsigned short buf_len, const unsigned char *data, unsigned char channels);

namespace GemRB {

/*
 * constructor: doesn't really do anything
 */
MVEPlayer::MVEPlayer(class MVEPlay *file) {
	buffer = NULL;
	host = file;
	done = false;

	audio_buffer = NULL;
	frame_wait = 0;
	timer_last_sec = 0;

	video_data = NULL;
	video_back_buf = NULL;

	video_frameskip = 0;
	video_skippedframes = 0;

	audio_stream = -1;

	playsound = true;
}

MVEPlayer::~MVEPlayer() {
	if (buffer) free(buffer);
	if (audio_buffer) free(audio_buffer);

	if (video_data) {
		if (video_data->code_map) free(video_data->code_map);
		free(video_data);
	}
	if (video_back_buf) free(video_back_buf);

	if (audio_stream != -1) host->freeAudioStream(audio_stream);

	if (video_skippedframes)
		print("Warning: Had to drop %d video frame(s).", video_skippedframes);
}

/*
 * high-level movie playback
 */

void MVEPlayer::video_init(unsigned int w, unsigned int h) {
	outputwidth = w;
	outputheight = h;
}

void MVEPlayer::sound_init(bool play) {
	playsound = play;
}

bool MVEPlayer::start_playback() {
	if (!verify_header()) return false;

	/*
	 * The first two chunks contain audio and video initialisation, hopefully.
	 */
	if (!process_chunk() || !process_chunk()) {
		print("Error: Failed to read initial movie chunks.");
		return false;
	}

	/* TODO: verify we have the needed information */

	return true;
}

bool MVEPlayer::next_frame() {
	if (timer_last_sec) timer_wait();

	video_rendered_frame = false;
	while (!video_rendered_frame) {
		if (done) return false;
		if (!process_chunk()) return false;
	}

	if (!timer_last_sec) timer_start();

	return true;
}

/*
 * parsing/demuxing
 */

bool MVEPlayer::request_data(unsigned int len) {
	if (!buffer) {
		buffer = (char*)malloc(len);
		buffersize = len;
	} else {
		if (len > buffersize) {
			buffer = (char*)realloc(buffer, len);
			buffersize = len;
		}
	}
	if (!host->fileRead(buffer, len)) return false;
	return true;
}

bool MVEPlayer::verify_header() {
	if (!request_data(MVE_PREAMBLE_SIZE)) return false;
	if (memcmp(buffer, MVE_PREAMBLE, MVE_PREAMBLE_SIZE) != 0) {
		print("Error: MVE preamble didn't match");
		return false;
	}
	return true;
}

bool MVEPlayer::process_chunk() {
	if (!request_data(4)) return false;
	chunk_offset = 0;
	chunk_size = GST_READ_UINT16_LE(buffer);
	unsigned int chunk_type = GST_READ_UINT16_LE(buffer + 2);
	(void)chunk_type; /* we don't care */

	while (chunk_offset < chunk_size) {
		chunk_offset += 4;
		if (!request_data(4)) return false;
		
		unsigned int segment_size = GST_READ_UINT16_LE(buffer);
		unsigned char segment_type = buffer[2];
		unsigned char segment_version = buffer[3];
		
		chunk_offset += segment_size;
		if (!process_segment(segment_size, segment_type, segment_version)) return false;
	}

	if (chunk_offset != chunk_size) {
		print("Error: Decoded past the end of an MVE chunk");
		return false;
	}

	return true;
}

bool MVEPlayer::process_segment(unsigned short len, unsigned char type, unsigned char version) {
	if (!request_data(len)) return false;

	switch (type) {
		case MVE_OC_END_OF_CHUNK:
			/* do nothing */
			break;
		case MVE_OC_CREATE_TIMER:
			segment_create_timer();
			break;
		case MVE_OC_AUDIO_BUFFERS:
			segment_audio_init(version);
			break;
		case MVE_OC_VIDEO_BUFFERS:
			segment_video_init(version);
			break;
		case MVE_OC_AUDIO_DATA:
		case MVE_OC_AUDIO_SILENCE:
			segment_audio_data(type == MVE_OC_AUDIO_SILENCE);
			break;
		case MVE_OC_VIDEO_MODE:
			segment_video_mode();
			break;
		case MVE_OC_PALETTE:
			segment_video_palette();
			break;
		case MVE_OC_PALETTE_COMPRESSED:
			segment_video_compressedpalette();
			break;
		case MVE_OC_CODE_MAP:
			segment_video_codemap(len);
			break;
		case MVE_OC_VIDEO_DATA:
			segment_video_data(len);
			break;
		case MVE_OC_END_OF_STREAM:
			done = true;
			break;
		case MVE_OC_PLAY_AUDIO:
			/* we don't care */
			break;
		case MVE_OC_PLAY_VIDEO:
			segment_video_play();
			break;
		case 0x13: case 0x14: case 0x15:
			/* ignore these */
			break;
		default:
			Log(WARNING, "MVEPlayer", "Skipping unknown segment type 0x%02x", type);
	}

	return true;
}

/*
 * timer handling
 */

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

void MVEPlayer::timer_start() {
	get_current_time(timer_last_sec, timer_last_usec);
}

void MVEPlayer::timer_wait() {
	long sec, usec;
	get_current_time(sec, usec);

	while (sec > timer_last_sec) {
		usec += 1000000;
		timer_last_sec++;
	}

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

void MVEPlayer::segment_create_timer() {
	/* new frame every (timer_rate * timer_subdiv) microseconds */
	unsigned int timer_rate = GST_READ_UINT32_LE(buffer);
	unsigned short timer_subdiv = GST_READ_UINT16_LE(buffer + 4);

	frame_wait = timer_rate * timer_subdiv;
}

/*
 * video handling
 */

void MVEPlayer::segment_video_init(unsigned char version) {
	unsigned short width = GST_READ_UINT16_LE(buffer) << 3;
	unsigned short height = GST_READ_UINT16_LE(buffer + 2) << 3;
/* count is unused
	unsigned short count = 1;
	if (version > 0) count = GST_READ_UINT16_LE(buffer + 4);
*/
	unsigned short temp = 0;
	if (version > 1) temp = GST_READ_UINT16_LE(buffer + 6);
	truecolour = !!temp;

	// some files have multiple initialisations
	if (video_data) {
		if (video_data->code_map) free(video_data->code_map);
		free(video_data);
	}
	if (video_back_buf) free(video_back_buf);

	unsigned int size = width * height * (truecolour ? 2 : 1);
	video_back_buf = (guint16 *)malloc(size * 2);
	memset(video_back_buf, 0, size * 2);

	video_data = (GstMveDemuxStream *)malloc(sizeof(GstMveDemuxStream));
	video_data->code_map = NULL;
	video_data->width = width;
	video_data->height = height;
	video_data->back_buf1 = video_back_buf;
	video_data->back_buf2 = video_back_buf + size/2;
	video_data->max_block_offset = (height - 7) * width - 8;
}

void MVEPlayer::segment_video_mode() {
	video_width = GST_READ_UINT16_LE(buffer);
	video_height = GST_READ_UINT16_LE(buffer + 2);
	unsigned short flags = GST_READ_UINT16_LE(buffer + 4);
	(void)flags; /* unknown/unused */
}

void MVEPlayer::segment_video_palette() {
	unsigned short palette_start = GST_READ_UINT16_LE(buffer);
	unsigned short palette_count = GST_READ_UINT16_LE(buffer + 2);

	char *palette = buffer + 4;

	host->setPalette((unsigned char *)palette - (3 * palette_start), palette_start, palette_count);
}

//appears to be unused
void MVEPlayer::segment_video_compressedpalette() {
#if 0
	char *data = buffer;

	unsigned int i, j;
	for (i = 0; i < 32; ++i) {
		unsigned char mask = *data;
		data++;

		if (mask) {
			for (j = 0; j < 8; ++j) {
				unsigned char r, g, b;
				r = (*data) << 2;
				++data;
				g = (*data) << 2;
				++data;
				b = (*data) << 2;
				++data;
				/* TODO: set palette position (i * 8) + j */
			}
		}
	}
#endif
}

void MVEPlayer::segment_video_codemap(unsigned short size) {
	if (!video_data) return; /* return failure? */

	/* alas, a cornucopia of memory management! */
	if (video_data->code_map) free(video_data->code_map);
	video_data->code_map = (guint8*)malloc(size);
	memcpy(video_data->code_map, buffer, size);
}

void MVEPlayer::segment_video_data(unsigned short size) {
	/* check for valid code_map? */

	unsigned short cur_frame = GST_READ_UINT16_LE(buffer);
	unsigned short last_frame = GST_READ_UINT16_LE(buffer + 2);
	unsigned short x_offset = GST_READ_UINT16_LE(buffer + 4);
	unsigned short y_offset = GST_READ_UINT16_LE(buffer + 6);
	unsigned short x_size = GST_READ_UINT16_LE(buffer + 8);
	unsigned short y_size = GST_READ_UINT16_LE(buffer + 10);
	(void)cur_frame; (void)last_frame; (void)x_offset; (void)y_offset;
	(void)x_size; (void)y_size; /* unused? */
	unsigned short flags = GST_READ_UINT16_LE(buffer + 12);

	char *data = buffer + 14;

	if (flags & MVE_VIDEO_DELTA_FRAME) {
		guint16 *temp = video_data->back_buf1;
		video_data->back_buf1 = video_data->back_buf2;
		video_data->back_buf2 = temp;
	}

	/* might want to check result code.. */
	if (truecolour) ipvideo_decode_frame16(video_data, (const unsigned char *)data, size);
	else ipvideo_decode_frame8(video_data, (const unsigned char *)data, size);
}

void MVEPlayer::segment_video_play() {
	if (video_frameskip) {
		video_frameskip--;
		video_skippedframes++;
	} else {
		unsigned int dest_x = (outputwidth - video_data->width) >> 1;
		unsigned int dest_y = (outputheight - video_data->height) >> 1;
		host->showFrame( (guint8 *) video_data->back_buf1, video_data->width, video_data->height, 0, 0, video_data->width, video_data->height, dest_x, dest_y);
	}

	video_rendered_frame = true;
}

/*
 * audio handling
 */

void MVEPlayer::segment_audio_init(unsigned char version) {
	if (!playsound) return;

	audio_stream = host->setAudioStream();
	if (audio_stream == -1) {
		print("Error: MVE player couldn't open audio. Will play silently.");
		playsound = false;
		return;
	}

	/* first 2 bytes unknown! */
	audio_sample_rate = GST_READ_UINT16_LE(buffer + 4);
	/* the docs say min_buffer_len is 16-bit for version 0, all other code just assumes 32-bit.. */
	unsigned int min_buffer_len = GST_READ_UINT32_LE(buffer + 6);

	unsigned short flags = GST_READ_UINT16_LE(buffer + 2);
	/* bit 0: 0 = mono, 1 = stereo */
	audio_num_channels = (flags & MVE_AUDIO_STEREO) + 1;
	/* bit 1: 0 = 8 bit, 1 = 16 bit */
	audio_sample_size = (((flags & MVE_AUDIO_16BIT) >> 1) + 1) * 8;
	/* bit 2: 0 = uncompressed, 1 = compressed */
	audio_compressed = ((version > 0) && (flags & MVE_AUDIO_COMPRESSED));

	min_buffer_len *= audio_num_channels;
	if (audio_sample_size == 16) min_buffer_len *= 2;
	if (audio_buffer) free(audio_buffer);
	audio_buffer = (short *)malloc(min_buffer_len);

/*	print("Movie audio: Sample rate %d, %d channels, %d bit, requested buffer size 0x%02x, %s",
		audio_sample_rate, audio_num_channels, audio_sample_size, min_buffer_len, audio_compressed ? "compressed" : "uncompressed");*/
}

void MVEPlayer::segment_audio_data(bool silent) {
	if (!playsound) return;

	unsigned short seq_index = GST_READ_UINT16_LE(buffer);
	(void)seq_index; /* we don't care */
	unsigned short stream_mask = GST_READ_UINT16_LE(buffer + 2);
	unsigned short audio_size = GST_READ_UINT16_LE(buffer + 4);
	char *data = buffer + 6;

	if (stream_mask & MVE_DEFAULT_AUDIO_STREAM) {
		if (silent) {
			memset(audio_buffer, 0, audio_size);
		} else {
			/* should we check size of audio_buffer? */
			if (audio_compressed)
				ipaudio_uncompress(audio_buffer, audio_size, (const unsigned char *)data, audio_num_channels);
			else
				memcpy(audio_buffer, data, audio_size);
		}
		host->queueBuffer(audio_stream, audio_sample_size, audio_num_channels, audio_buffer, audio_size, audio_sample_rate);
	} else {
		/* alternative audio stream, which we don't care about */
	}
}

}
