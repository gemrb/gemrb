/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "OGGReader.h"

static size_t ovfd_read(void *ptr, size_t size, size_t nmemb, void *datasource)
{
	DataStream *vb = (DataStream *) datasource;
	int bytesToRead = size * nmemb;

	int remains = vb->Remains();
	if(remains<=0) {
		/* no more reading, we're at the end */
		return 0;
	}
	if(bytesToRead > remains ) {
		bytesToRead = remains;
	}
	vb->Read(ptr, bytesToRead);
	return bytesToRead;
}

static int ovfd_seek(void *datasource, ogg_int64_t offset, int whence) {
	DataStream *vb = (DataStream *) datasource;
	switch(whence) {
		case SEEK_SET:
			if(vb->Seek(offset, GEM_STREAM_START)<0)
				return -1;
			break;
		case SEEK_CUR:
			if(vb->Seek(offset, GEM_CURRENT_POS)<0)
				return -1;
			break;
		case SEEK_END:
			if(vb->Seek(vb->Size()+offset, GEM_STREAM_START)<0)
				return -1;
			break;
		default:
			return -1;
	}
	return vb->GetPos();
}

static int ovfd_close(void * /*datasource*/) {
	return 0;
}

static long ovfd_tell(void *datasource) {
	DataStream *vb = (DataStream *) datasource;
	return (long) vb->GetPos();
}

bool OGGReader::Open(DataStream* stream)
{
	str = stream;
	Close();

	char Signature[4];
	stream->Read( Signature, 4 );
	stream->Seek( 0, GEM_STREAM_START );
	if(strnicmp(Signature, "oggs", 4) != 0)
		return false;

	vorbis_info *info;
	int res;
	ov_callbacks cbstruct = {
		ovfd_read, ovfd_seek, ovfd_close, ovfd_tell
	};

	res=ov_open_callbacks(str, &OggStream, NULL, 0, cbstruct);
	if(res<0) {
		Log(ERROR, "Sound", "Couldn't initialize vorbis!");
		return false;
	}
	info = ov_info(&OggStream, -1);
	channels = info->channels;
	samplerate = info->rate;
	samples_left = ( samples = ov_pcm_total(&OggStream, -1) );
	return true;
}

int OGGReader::read_samples(short* buffer, int count)
{
	int whatisthis;

	if(samples_left<count) {
		count=samples_left;
	}
	int samples_got=0;
	int samples_need=count;
	while(samples_need) {
		int rd=ov_read(&OggStream, (char *)buffer, samples_need<<1, str->IsEndianSwitch(), 2, 1, &whatisthis);
		if(rd==OV_HOLE) {
			continue;
		}
		if(rd<=0) {
			break;
		}
		rd>>=1;
		buffer+=rd;
		samples_got+=rd;
		samples_need-=rd;
	}
	samples_left-=samples_got;

	return samples_got;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x18C310C3, "OGG File Importer")
PLUGIN_RESOURCE(OGGReader, "ogg")
END_PLUGIN()
