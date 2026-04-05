// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
// SPDX-FileCopyrightText: Copyright (C) 2006 Jens Granseuer <jensgr@gmx.net>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __MVE_H__
#define __MVE_H__

#define MVE_PALETTE_COUNT 256

#define MVE_DEFAULT_AUDIO_STREAM 0x01

/* MVE chunk types */
#define MVE_CHUNK_INIT_AUDIO 0x0000
#define MVE_CHUNK_AUDIO_ONLY 0x0001
#define MVE_CHUNK_INIT_VIDEO 0x0002
#define MVE_CHUNK_VIDEO      0x0003
#define MVE_CHUNK_SHUTDOWN   0x0004
#define MVE_CHUNK_END        0x0005

/* MVE segment opcodes */
#define MVE_OC_END_OF_STREAM      0x00
#define MVE_OC_END_OF_CHUNK       0x01
#define MVE_OC_CREATE_TIMER       0x02
#define MVE_OC_AUDIO_BUFFERS      0x03
#define MVE_OC_PLAY_AUDIO         0x04
#define MVE_OC_VIDEO_BUFFERS      0x05
#define MVE_OC_PLAY_VIDEO         0x07
#define MVE_OC_AUDIO_DATA         0x08
#define MVE_OC_AUDIO_SILENCE      0x09
#define MVE_OC_VIDEO_MODE         0x0A
#define MVE_OC_PALETTE            0x0C
#define MVE_OC_PALETTE_COMPRESSED 0x0D
#define MVE_OC_CODE_MAP           0x0F
#define MVE_OC_VIDEO_DATA         0x11

/* audio flags */
#define MVE_AUDIO_STEREO     0x0001
#define MVE_AUDIO_16BIT      0x0002
#define MVE_AUDIO_COMPRESSED 0x0004

/* video flags */
#define MVE_VIDEO_DELTA_FRAME 0x0001

#endif /* __MVE_H__ */
