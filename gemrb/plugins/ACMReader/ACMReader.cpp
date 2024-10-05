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

#include "ACMReader.h"

#include "general.h"

using namespace GemRB;

bool ACMReader::Import(DataStream* str)
{
	Close();

	ACM_Header hdr;

	char Signature[4];
	ieDword SignatureDword;
	str->Read( Signature, 4 );
	str->Seek( 0, GEM_STREAM_START );
	str->ReadDword(SignatureDword);
	if (!memcmp( Signature, "WAVC", 4 )) {
		str->Seek( 28, GEM_STREAM_START );
	} else if (SignatureDword == IP_ACM_SIG) {
		str->Seek( 0, GEM_STREAM_START );
	} else {
		return false;
	}

	str->ReadDword(hdr.signature);
	str->ReadDword(hdr.samples);
	str->ReadWord(hdr.channels);
	str->ReadWord(hdr.rate);
	ieWord tmpword;
	str->ReadWord(tmpword);
	subblocks = tmpword >> 4;
	levels = tmpword & 15;

	if (hdr.signature != IP_ACM_SIG) {
		return false;
	}
	samples_left = ( samples = hdr.samples );
	channels = hdr.channels;
	samplerate = hdr.rate;
	//levels = hdr.levels;
	//subblocks = hdr.subblocks;

	block_size = ( 1 << levels ) * subblocks;
	//using malloc for simple arrays (supposed to be faster)
	block = (int *) malloc(sizeof(int)*block_size);
	if (!block) {
		return false;
	}
	unpacker = new CValueUnpacker( levels, subblocks, str );
	if (!unpacker || !unpacker->init_unpacker()) {
		return false;
	}
	decoder = new CSubbandDecoder( levels );
	if (!decoder || !decoder->init_decoder()) {
		return false;
	}
	return true;
}
int ACMReader::make_new_samples()
{
	if (!unpacker->get_one_block( block )) {
		return 0;
	}

	decoder->decode_data( block, subblocks );
	values = block;
	samples_ready = ( block_size > samples_left ) ? samples_left : block_size;
	samples_left -= samples_ready;

	return 1;
}

int ACMReader::read_samples(short* buffer, int count)
{
	int res = 0;
	while (res < count) {
		if (samples_ready == 0) {
			if (samples_left == 0)
				break;
			if (!make_new_samples())
				break;
		}
		*buffer = ( short ) ( ( *values ) >> levels );
		values++;
		buffer++;
		res += 1;
		samples_ready--;
	}
	return res;
}

static constexpr size_t CHANNEL_SPLIT_SAMPLE_SIZE = 2048;

int ACMReader::ReadSamplesIntoChannels(char *channel1, char *channel2, int numSamples) {
	std::vector<char> buffer;
	// sample: 2 channels à 2 bytes ...
	buffer.resize(CHANNEL_SPLIT_SAMPLE_SIZE * 4);

	// ... but read_samples needs twice the read for the same number of samples
	auto samplesRead = read_samples(reinterpret_cast<short*>(buffer.data()), CHANNEL_SPLIT_SAMPLE_SIZE * 2) / 2;
	auto totalSamples = samplesRead;

	size_t z = 0;
	do {
		for (decltype(samplesRead) i = 0; i < samplesRead; ++i) {
			auto bufferOffset = i * 4;
			channel1[z]   = buffer[bufferOffset];
			channel1[z+1] = buffer[bufferOffset + 1];
			channel2[z]   = buffer[bufferOffset + 2];
			channel2[z+1] = buffer[bufferOffset + 3];
			z += 2;
		}

		samplesRead = read_samples(reinterpret_cast<short*>(buffer.data()), CHANNEL_SPLIT_SAMPLE_SIZE * 2) / 2;
		totalSamples += samplesRead;
	} while (samplesRead > 0 && totalSamples <= numSamples);

	return totalSamples;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x10373EE, "ACM File Importer")
PLUGIN_IE_RESOURCE(ACMReader, "acm", (ieWord)IE_ACM_CLASS_ID)
PLUGIN_IE_RESOURCE(ACMReader, "wav", (ieWord)IE_WAV_CLASS_ID)
END_PLUGIN()
