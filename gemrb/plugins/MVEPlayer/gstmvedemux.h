/*
 * GStreamer demultiplexer plugin for Interplay MVE movie files
 *
 * Copyright (C) 2006 Jens Granseuer <jensgr@gmx.net>
 * 
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifndef __GST_MVE_DEMUX_H__
#define __GST_MVE_DEMUX_H__

#include "win32def.h"
#include "globals.h"

using namespace GemRB;

#define G_UNLIKELY(x) (x)
#define GST_WARNING print
#define GST_ERROR print

/* Define GET function for unaligned memory */
#define _GST_GET(__data, __idx, __size, __shift) \
	(((guint##__size) (((guint8 *) (__data))[__idx])) << __shift)

/**
 * GST_READ_UINT16_LE:
 * @data: memory location
 *
 * Read a 16 bit unsigned integer value in little endian format from the memory buffer.
 */
#define GST_READ_UINT16_LE(data)        (_GST_GET (data, 1, 16,  8) | \
					 _GST_GET (data, 0, 16,  0))

/**
 * GST_READ_UINT32_LE:
 * @data: memory location
 *
 * Read a 32 bit unsigned integer value in little endian format from the memory buffer.
 */
#define GST_READ_UINT32_LE(data)        (_GST_GET (data, 3, 32, 24) | \
					 _GST_GET (data, 2, 32, 16) | \
					 _GST_GET (data, 1, 32,  8) | \
					 _GST_GET (data, 0, 32,  0))

/*#include <gst/gst.h>
#include <gst/base/gstadapter.h>

G_BEGIN_DECLS

#define GST_TYPE_MVE_DEMUX \
  (gst_mve_demux_get_type())
#define GST_MVE_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_MVE_DEMUX,GstMveDemux))
#define GST_MVE_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_MVE_DEMUX,GstMveDemuxClass))
#define GST_IS_MVE_DEMUX(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_MVE_DEMUX))
#define GST_IS_MVE_DEMUX_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_MVE_DEMUX))*/

typedef int gint;
typedef gint gboolean;
typedef ieByte guint8;
typedef ieWord guint16;
typedef ieDword guint32;

/*typedef struct _GstMveDemux       GstMveDemux;
typedef struct _GstMveDemuxClass  GstMveDemuxClass;*/
typedef struct _GstMveDemuxStream GstMveDemuxStream;

/*struct _GstMveDemux
{
  GstElement element;

  GstPad *sinkpad;

  GstMveDemuxStream *video_stream;
  GstMveDemuxStream *audio_stream;

  gint state;*/

  /* time per frame (1/framerate) */
/*  GstClockTime frame_duration;*/

  /* push based variables */
/*  guint16 needed_bytes;
  GstAdapter *adapter;*/
  
  /* size of current chunk */
/*  guint32 chunk_size;*/
  /* offset in current chunk */
/*  guint32 chunk_offset;
};*/

/*struct _GstMveDemuxClass 
{
  GstElementClass parent_class;
};*/

struct _GstMveDemuxStream {
  /* shared properties */
  /*GstCaps *caps;
  GstPad *pad;
  GstClockTime last_ts;*/
  /*gint64 offset;*/

  /* video properties */
  guint16 width;
  guint16 height;
  /*guint8 bpp;*/   /* bytes per pixel */
  guint8 *code_map;
  /*gboolean code_map_avail;*/
  guint16 *back_buf1;
  guint16 *back_buf2;
  guint32 max_block_offset;
  /*GstBuffer *palette;
  GstBuffer *buffer;*/

  /* audio properties */
  /*guint16 sample_rate;
  guint16 n_channels;
  guint16 sample_size;
  gboolean compression;*/
};

int ipvideo_decode_frame8(const GstMveDemuxStream*, const unsigned char*, short unsigned int);
int ipvideo_decode_frame16(const GstMveDemuxStream*, const unsigned char*, short unsigned int);
void ipaudio_uncompress(short int*, short unsigned int, const unsigned char*, unsigned char);

/*GType gst_mve_demux_get_type (void);

G_END_DECLS*/

#endif /* __GST_MVE_DEMUX_H__ */
