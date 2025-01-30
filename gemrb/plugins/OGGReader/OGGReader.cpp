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

#include "Logging/Logging.h"

using namespace GemRB;

static size_t ovfd_read(void* ptr, size_t size, size_t nmemb, void* datasource)
{
	DataStream* vb = (DataStream*) datasource;
	int bytesToRead = size * nmemb;

	int remains = vb->Remains();
	if (remains <= 0) {
		/* no more reading, we're at the end */
		return 0;
	}
	if (bytesToRead > remains) {
		bytesToRead = remains;
	}
	vb->Read(ptr, bytesToRead);
	return bytesToRead;
}

static int ovfd_seek(void* datasource, ogg_int64_t offset, int whence)
{
	DataStream* vb = (DataStream*) datasource;
	switch (whence) {
		case SEEK_SET:
			if (vb->Seek(offset, GEM_STREAM_START) < 0)
				return -1;
			break;
		case SEEK_CUR:
			if (vb->Seek(offset, GEM_CURRENT_POS) < 0)
				return -1;
			break;
		case SEEK_END:
			if (vb->Seek(vb->Size() + offset, GEM_STREAM_START) < 0)
				return -1;
			break;
		default:
			return -1;
	}
	return vb->GetPos();
}

static int ovfd_close(void* /*datasource*/)
{
	return 0;
}

static long ovfd_tell(void* datasource)
{
	const DataStream* vb = (const DataStream*) datasource;
	return (long) vb->GetPos();
}

bool OGGReader::Import(DataStream* stream)
{
	Close();

	char Signature[4];
	stream->Read(Signature, 4);
	stream->Seek(0, GEM_STREAM_START);
	if (strnicmp(Signature, "oggs", 4) != 0)
		return false;

	const vorbis_info* info;
	int res;
	ov_callbacks cbstruct = {
		ovfd_read, ovfd_seek, ovfd_close, ovfd_tell
	};

	res = ov_open_callbacks(stream, &OggStream, NULL, 0, cbstruct);
	if (res < 0) {
		Log(ERROR, "Sound", "Couldn't initialize vorbis!");
		return false;
	}
	info = ov_info(&OggStream, -1);
	channels = info->channels;
	sampleRate = info->rate;
	auto total = ov_pcm_total(&OggStream, -1);
	samplesLeft = 0;
	if (total != OV_EINVAL) {
		samplesLeft = static_cast<size_t>(total);
	}
	// align to how WAVReader counts samples (one is per channel)
	samples = samplesLeft * channels;
	return true;
}

size_t OGGReader::read_samples(short* buffer, size_t count)
{
	int whatisthis;

	if (samplesLeft < count) {
		count = samplesLeft;
	}
	size_t samples_got = 0;
	int samples_need = count;
	while (samples_need) {
		int rd = ov_read(&OggStream, (char*) buffer, samples_need << 1, false, 2, 1, &whatisthis);
		if (rd == OV_HOLE) {
			continue;
		}
		if (rd <= 0) {
			break;
		}
		rd >>= 1;
		buffer += rd;
		samples_got += rd;
		samples_need -= rd;
	}
	samplesLeft -= samples_got;

	return samples_got;
}

static constexpr size_t CHANNEL_SPLIT_BUFFER_SIZE = 4096;

size_t OGGReader::ReadSamplesIntoChannels(char* channel1, char* channel2, size_t numSamples)
{
	std::vector<char> buffer;
	buffer.resize(CHANNEL_SPLIT_BUFFER_SIZE);
	int streamPos = 0;
	numSamples /= channels;

	uint8_t bytesPerChannel = 2;
	uint8_t bytesPerSample = 2 * bytesPerChannel;
	auto samplesRead = ov_read(&OggStream, buffer.data(), CHANNEL_SPLIT_BUFFER_SIZE, false, 2, 1, &streamPos) / bytesPerSample;
	size_t totalSamples = 0;
	if (samplesRead > 0) {
		totalSamples = static_cast<size_t>(samplesRead);
	}

	size_t z = 0;
	do {
		for (decltype(samplesRead) i = 0; i < samplesRead; ++i) {
			auto bufferOffset = i * bytesPerSample;
			for (uint8_t j = 0; j < bytesPerChannel; ++j) {
				channel1[z + j] = buffer[bufferOffset + j];
				channel2[z + j] = buffer[bufferOffset + j + 2];
			}
			z += bytesPerChannel;
		}

		samplesRead = ov_read(&OggStream, buffer.data(), CHANNEL_SPLIT_BUFFER_SIZE, false, 2, 1, &streamPos) / bytesPerSample;
		if (samplesRead == OV_HOLE) {
			continue;
		}
		totalSamples += samplesRead;
	} while (samplesRead > 0 && totalSamples <= numSamples);

	return totalSamples;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x18C310C3, "OGG File Importer")
PLUGIN_RESOURCE(OGGReader, "ogg")
PLUGIN_IE_RESOURCE(OGGReader, "wav", (ieWord) IE_WAV_CLASS_ID) // ees suck, eg. GAM_04.wav is actually an ogg file
END_PLUGIN()
