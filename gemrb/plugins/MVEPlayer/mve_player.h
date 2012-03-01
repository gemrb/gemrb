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

#ifndef MVE_PLAYER_H
#define MVE_PLAYER_H

struct _GstMveDemuxStream;

namespace GemRB {

class MVEPlayer {
protected:
	class MVEPlay *host;
	char *buffer;
	unsigned int buffersize;
	unsigned int chunk_size;
	unsigned int chunk_offset;

	unsigned int outputwidth;
	unsigned int outputheight;

	long timer_last_sec;
	long timer_last_usec;
	unsigned int frame_wait;

	_GstMveDemuxStream *video_data;
	unsigned short video_width;
	unsigned short video_height;
	unsigned short *video_back_buf;
	bool truecolour;
	bool video_rendered_frame;
	unsigned int video_frameskip;
	unsigned int video_skippedframes;

	bool audio_compressed;
	int audio_num_channels;
	unsigned short audio_sample_rate;
	unsigned short audio_sample_size;
	short *audio_buffer;
	int audio_stream;

	bool playsound;
	bool done;

	bool request_data(unsigned int len);

	bool verify_header();
	bool process_chunk();
	bool process_segment(unsigned short len,
			unsigned char type, unsigned char version);

	void segment_create_timer();
	void timer_start();
	void timer_wait();

	void segment_video_init(unsigned char version);
	void segment_video_mode();
	void segment_video_palette();
	void segment_video_compressedpalette();
	void segment_video_codemap(unsigned short size);
	void segment_video_data(unsigned short size);
	void segment_video_play();

	void segment_audio_init(unsigned char version);
	void segment_audio_data(bool silent);

public:
	MVEPlayer(class MVEPlay *file);
	~MVEPlayer();

	void sound_init(bool playsound);
	void video_init(unsigned int width, unsigned int height);

	bool start_playback();
	bool next_frame();
	
	bool is_truecolour() { return truecolour; }
};

}

#endif
