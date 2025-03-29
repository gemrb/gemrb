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

#include "WAVReader.h"

#include <vector>

using namespace GemRB;

// WAVEFORMATEX structure (from MS SDK)
struct cWAVEFORMATEX {
	unsigned short wFormatTag; /* format type */
	unsigned short nChannels; /* number of channels (i.e. mono, stereo...) */
	unsigned int nSamplesPerSec; /* sample rate */
	unsigned int nAvgBytesPerSec; /* for buffer estimation */
	unsigned short nBlockAlign; /* block size of data */
	unsigned short wBitsPerSample; /* number of bits per sample of mono data */
	unsigned short cbSize; /* the count in bytes of the size of */
	/* extra information (after cbSize) */
};

struct RIFF_CHUNK {
	unsigned int fourcc;
	unsigned int length;
};

const unsigned char RIFF_4cc[] = {
	'R', 'I', 'F', 'F'
};
const unsigned char WAVE_4cc[] = {
	'W', 'A', 'V', 'E'
};
const unsigned char fmt_4cc[] = {
	'f', 'm', 't', ' '
};
const unsigned char fact_4cc[] = {
	'f', 'a', 'c', 't'
};
const unsigned char data_4cc[] = {
	'd', 'a', 't', 'a'
};

bool RawPCMReader::Import(DataStream* str)
{
	samples = str->Size();
	str->Seek(0, GEM_STREAM_START);
	if (is16bit) {
		samples >>= 1; // each sample has 16 bit
	}
	samplesLeft = samples;
	return true;
}

size_t RawPCMReader::read_samples(short* buffer, size_t count)
{
	if (count > samplesLeft) {
		count = samplesLeft;
	}
	size_t res = 0;
	if (count) {
		res = str->Read(buffer, count * (is16bit ? 2 : 1));
	}
	if (!is16bit) {
		char* alt_buff = (char*) buffer;
		strret_t i = res;
		while (i--) {
			alt_buff[(i << 1) + 1] = (char) (alt_buff[i] - 0x80);
			alt_buff[i << 1] = 0;
		}
	}
	if (is16bit) {
		res >>= 1;
	}
	samplesLeft -= res;
	return res;
}

static constexpr size_t CHANNEL_SPLIT_BUFFER_SIZE = 4096;

size_t RawPCMReader::ReadSamplesIntoChannels(char* channel1, char* channel2, size_t numSamples)
{
	std::vector<char> buffer;
	buffer.resize(CHANNEL_SPLIT_BUFFER_SIZE);

	uint8_t bytesPerChannel = is16bit ? 2 : 1;
	uint8_t bytesPerSample = 2 * bytesPerChannel;
	auto samplesRead = str->Read(buffer.data(), CHANNEL_SPLIT_BUFFER_SIZE) / bytesPerSample;
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

		totalSamples += samplesRead;
		samplesRead = str->Read(buffer.data(), CHANNEL_SPLIT_BUFFER_SIZE) / bytesPerSample;
	} while (samplesRead > 0 && totalSamples <= numSamples);

	return totalSamples;
}

bool WavPCMReader::Import(DataStream* stream)
{
	if (!RawPCMReader::Import(stream))
		return false;

	char Signature[4];
	stream->Read(Signature, 4);
	stream->Seek(0, GEM_STREAM_START);
	if (strnicmp(Signature, "RIFF", 4) != 0)
		return false;

	cWAVEFORMATEX fmt;
	RIFF_CHUNK r_hdr, fmt_hdr, data_hdr;
	unsigned int wave;
	memset(&fmt, 0, sizeof(fmt));

	//str->Read( &r_hdr, sizeof( r_hdr ) );
	//don't swap this
	str->Read(&r_hdr.fourcc, 4);
	str->ReadDword(r_hdr.length);
	//don't swap this
	str->Read(&wave, 4);
	if (memcmp(&r_hdr.fourcc, RIFF_4cc, 4) != 0 ||
	    memcmp(&wave, WAVE_4cc, 4) != 0) {
		return false;
	}

	//str->Read( &fmt_hdr, sizeof( fmt_hdr ) );
	//don't swap this
	str->Read(&fmt_hdr.fourcc, 4);
	str->ReadDword(fmt_hdr.length);
	if (memcmp(&fmt_hdr.fourcc, fmt_4cc, 4) != 0 ||
	    fmt_hdr.length > sizeof(cWAVEFORMATEX)) {
		return false;
	}
	memset(&fmt, 0, sizeof(fmt));
	str->Read(&fmt, fmt_hdr.length);

	if (fmt.wFormatTag != 1) {
		return false;
	}
	is16bit = (fmt.wBitsPerSample == 16);

	//str->Read( &data_hdr, sizeof( data_hdr ) );
	//don't swap this
	str->Read(&data_hdr.fourcc, 4);
	str->ReadDword(data_hdr.length);

	if (memcmp(&data_hdr.fourcc, fact_4cc, 4) == 0) {
		str->Seek(data_hdr.length, GEM_CURRENT_POS);
		//str->Read( &data_hdr, sizeof( data_hdr ) );
		str->ReadDword(data_hdr.fourcc);
		str->ReadDword(data_hdr.length);
	}
	if (memcmp(&data_hdr.fourcc, data_4cc, 4) != 0) {
		return false;
	}

	samples = data_hdr.length;
	if (is16bit) {
		samples >>= 1;
	}
	samplesLeft = samples;
	channels = fmt.nChannels;
	sampleRate = fmt.nSamplesPerSec;
	return true;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x11BB1288, "WAV File Importer")
PLUGIN_IE_RESOURCE(WavPCMReader, "wav", (ieWord) IE_WAV_CLASS_ID)
END_PLUGIN()
