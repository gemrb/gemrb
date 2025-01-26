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

#ifndef BIKPLAYER_H
#define BIKPLAYER_H

#include "globals.h"

#include "MoviePlayer.h"

#include "Audio/AudioBackend.h"

// FIXME: This has to be included last, since it defines int*_t, which causes
// mingw g++ 4.5.0 to choke.
#include "GetBitContext.h"
#include "common.h"
#include "dsputil.h"
#include "rational.h"

#include <vector>

namespace GemRB {

#define BIK_SIGNATURE_LEN  4
#define BIK_SIGNATURE_DATA "BIKi"

#define MAX_CHANNELS        2
#define BINK_BLOCK_MAX_SIZE (MAX_CHANNELS << 11)

#if defined(__arm__)
	#define SET_INT_TYPE uint8_t
	#define SET_INT_VALUE(ptr, value) \
		*(ptr) = (value) && 0xff; \
		(ptr)++; \
		*(ptr) = ((value) >> 8) && 0xff; \
		(ptr)++
#else
	#define SET_INT_TYPE int16_t
	#define SET_INT_VALUE(ptr, value) \
		*(ptr)++ = (value)
#endif

#if defined(__arm__)
	#define GET_INT_VALUE(value, ptr) \
		(value) = *(ptr); \
		(ptr)++; \
		(value) |= (*(ptr)) << 8; \
		(ptr)++
#else
	#define GET_INT_VALUE(value, ptr) \
		(value) = *reinterpret_cast<int16_t*>(ptr); \
		(ptr) += 2
#endif

enum BinkAudFlags {
	BINK_AUD_16BITS = 0x4000, ///< prefer 16-bit output
	BINK_AUD_STEREO = 0x2000,
	BINK_AUD_USEDCT = 0x1000
};

/**
 * IDs for different data types used in Bink video codec
 */
enum Sources {
	BINK_SRC_BLOCK_TYPES = 0, ///< 8x8 block types
	BINK_SRC_SUB_BLOCK_TYPES, ///< 16x16 block types (a subset of 8x8 block types)
	BINK_SRC_COLORS, ///< pixel values used for different block types
	BINK_SRC_PATTERN, ///< 8-bit values for 2-colour pattern fill
	BINK_SRC_X_OFF, ///< X components of motion value
	BINK_SRC_Y_OFF, ///< Y scomponents of motion value
	BINK_SRC_INTRA_DC, ///< DC values for intrablocks with DCT
	BINK_SRC_INTER_DC, ///< DC values for intrablocks with DCT
	BINK_SRC_RUN, ///< run lengths for special fill block

	BINK_NB_SRC
};

/**
 * Bink video block types
 */
enum BlockTypes {
	SKIP_BLOCK = 0, ///< skipped block
	SCALED_BLOCK, ///< block has size 16x16
	MOTION_BLOCK, ///< block is copied from previous frame with some offset
	RUN_BLOCK, ///< block is composed from runs of colours with custom scan order
	RESIDUE_BLOCK, ///< motion block with some difference added
	INTRA_BLOCK, ///< intra DCT block
	FILL_BLOCK, ///< block is filled with single colour
	INTER_BLOCK, ///< motion block with DCT applied to the difference
	PATTERN_BLOCK, ///< block is filled with two colours following custom pattern
	RAW_BLOCK ///< uncoded 8x8 block
};

typedef struct AVFrame {
	/**
	* pointer to the picture planes.
	* This might be different from the first allocated byte
	* - encoding: 
	* - decoding: 
	*/
	uint8_t* data[3] {};
	int linesize[3] {};

	AVFrame() = default;

	AVFrame(int width, int height)
	{
		get_buffer(width, height);
	}

	~AVFrame()
	{
		release_buffer();
	}

	void ff_fill_linesize(int width)
	{
		int w2 = (width + (1 << 1) - 1) >> 1;
		linesize[0] = width;
		linesize[1] = w2;
		linesize[2] = w2;
	}

	void release_buffer() const
	{
		for (auto& i : data) {
			av_free(i);
		}
	}

	void get_buffer(int width, int height)
	{
		release_buffer();

		ff_fill_linesize(width);
		for (int plane = 0; plane < 3; plane++) {
			data[plane] = (uint8_t*) av_malloc(linesize[plane] * height);
		}
	}
} AVFrame;

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
	ieDword tracks; //BinkAudFlags
	//optional if tracks == 1 (original had multiple tracks)
	ieWord unknown2;
	ieWord channels;
	ieWord samplerate;
	ieWord audioflag;
	ieDword unknown4;
} binkheader;

typedef struct {
	int keyframe;
	strpos_t pos;
	strpos_t size;
} binkframe;

typedef struct Bundle {
	int len; ///< length of number of entries to decode (in bits)
	Tree tree; ///< Huffman tree-related data
	uint8_t* data; ///< buffer for decoded symbols
	uint8_t* data_end; ///< buffer end
	uint8_t* cur_dec; ///< pointer to the not yet decoded part of the buffer
	uint8_t* cur_ptr; ///< pointer to the data that is not read from buffer yet
} Bundle;

class BIKPlayer : public MoviePlayer {
private:
	bool validVideo = false;
	binkheader header {};
	std::vector<binkframe> frames;
	ieByte* inbuff = nullptr;

	//audio context (consider packing it in a struct)
	unsigned int s_frame_len = 0;
	unsigned int s_channels = 0;
	int s_overlap_len = 0;
	int s_block_size = 0;
	unsigned int* s_bands = nullptr;
	float s_root = 0;
	unsigned int s_num_bands = 0;
	int s_first = 0;
	bool s_audio = false;
	Holder<SoundStreamSourceHandle> audioStream;
	AudioBufferFormat audioStreamFormat;

#pragma pack(push, 16)
	FFTSample s_coeffs[BINK_BLOCK_MAX_SIZE];
	short s_previous[BINK_BLOCK_MAX_SIZE / 16]; ///< coeffs from previous audio block
#pragma pack(pop)

	float* s_coeffs_ptr[MAX_CHANNELS] {}; ///< pointers to the coeffs arrays for float_to_int16_interleave

	union {
		RDFTContext rdft;
		DCTContext dct;
	} s_trans;
	GetBitContext s_gb;

	//video context (consider packing it in a struct)
	AVRational v_timebase {};
	bool video_rendered_frame = false;

	//bink specific
	ScanTable c_scantable {};
	Bundle c_bundle[BINK_NB_SRC] {}; ///< bundles for decoding all data types
	Tree c_col_high[16] {}; ///< trees for decoding high nibble in "colours" data type
	int c_col_lastval = 0; ///< value of last decoded high nibble in "colours" data type

	//huffman trees for video decoding
	VLC bink_trees[16];
	int16_t table[16 * 128][2] {};
	GetBitContext v_gb;

	AVFrame c_frames[2];
	AVFrame* c_pic = nullptr;
	AVFrame* c_last = nullptr;

private:
	void segment_video_play();
	strret_t fileRead(strpos_t pos, void* buf, strpos_t count);

	void FreeAudioStream() const;
	void QueueBuffer(unsigned short bits, int channels, short* memory, int size, int samplerate);
	int sound_init();
	void ff_init_scantable(ScanTable* st, const uint8_t* src_scantable) const;
	int video_init();
	void av_set_pts_info(AVRational& time_base, unsigned int pts_num, unsigned int pts_den) const;
	int ReadHeader();
	void DecodeBlock(short* out);
	int DecodeAudioFrame(void* data, int data_size);
	inline int get_value(int bundle);
	int read_dct_coeffs(DCTELEM block[64], const uint8_t* scan, bool is_intra);
	int read_residue(DCTELEM block[64], int masks_count);
	int read_runs(Bundle* b);
	int read_motion_values(Bundle* b);
	int read_block_types(Bundle* b);
	int read_patterns(Bundle* b);
	int read_colors(Bundle* b);
	int read_dcs(Bundle* b, int start_bits, int has_sign);
	int get_vlc2(int16_t (*table)[2], int bits, int max_depth);
	void read_bundle(int bundle_num);
	void init_lengths(int width, int bw);
	int DecodeVideoFrame(void* data, int data_size, VideoBuffer& buf);
	int EndAudio();
	int EndVideo();

protected:
	bool DecodeFrame(VideoBuffer&) override;

public:
	BIKPlayer() noexcept;
	BIKPlayer(const BIKPlayer&) = delete;
	~BIKPlayer() override;
	BIKPlayer& operator=(const BIKPlayer&) = delete;
	bool Import(DataStream* stream) override;

	void Stop();
};

}

#endif
