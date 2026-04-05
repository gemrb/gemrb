// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
// SPDX-FileCopyrightText: Copyright (C) 2006 Jens Granseuer <jensgr@gmx.net>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef __GST_MVE_DEMUX_H__
#define __GST_MVE_DEMUX_H__

#include "globals.h"

#include "Logging/Logging.h"

using namespace GemRB;

#define G_UNLIKELY(x)           (x)
#define GST_WARNING(msg)        Log(WARNING, "MVEPlayer", msg)
#define GST_ERROR(msg, p1)      Log(ERROR, "MVEPlayer", msg, p1)
#define GST_ERROR2(msg, p1, p2) Log(ERROR, "MVEPlayer", msg, p1, p2)

/* Define GET function for unaligned memory */
#define _GST_GET(__data, __idx, __size, __shift) \
	(((guint##__size)(((guint8*) (__data))[__idx])) << __shift)

/**
 * GST_READ_UINT16_LE:
 * @data: memory location
 *
 * Read a 16 bit unsigned integer value in little endian format from the memory buffer.
 */
#define GST_READ_UINT16_LE(data) (_GST_GET(data, 1, 16, 8) | \
				  _GST_GET(data, 0, 16, 0))

/**
 * GST_READ_UINT32_LE:
 * @data: memory location
 *
 * Read a 32 bit unsigned integer value in little endian format from the memory buffer.
 */
#define GST_READ_UINT32_LE(data) (_GST_GET(data, 3, 32, 24) | \
				  _GST_GET(data, 2, 32, 16) | \
				  _GST_GET(data, 1, 32, 8) | \
				  _GST_GET(data, 0, 32, 0))

typedef struct _GstMveDemuxStream GstMveDemuxStream;
using gint = int;
using gboolean = gint;
using guint8 = ieByte;
using guint16 = ieWord;
using guint32 = ieDword;

struct _GstMveDemuxStream {
	/* video properties */
	guint16 width;
	guint16 height;
	guint8* code_map;
	guint16* back_buf1;
	guint16* back_buf2;
	guint32 max_block_offset;
};

int ipvideo_decode_frame8(const GstMveDemuxStream*, const unsigned char*, short unsigned int);
int ipvideo_decode_frame16(const GstMveDemuxStream*, const unsigned char*, short unsigned int);
void ipaudio_uncompress(short int*, short unsigned int, const unsigned char*, unsigned char);

#endif /* __GST_MVE_DEMUX_H__ */
