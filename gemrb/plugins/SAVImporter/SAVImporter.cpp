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

#include "SAVImporter.h"

#include "Compressor.h"
#include "Interface.h"
#include "Logging/Logging.h"
#include "PluginMgr.h"

using namespace GemRB;

int SAVImporter::DecompressSaveGame(DataStream *compressed, SaveGameAREExtractor& areExtractor)
{
	char Signature[8];
	compressed->Read( Signature, 8 );
	if (strncmp(Signature, "SAV V1.0", 8) != 0) {
		return GEM_ERROR;
	}
	strpos_t All = compressed->Remains();
	strpos_t Current;
	size_t percent;
	size_t last_percent = 20;
	if (!All) return GEM_ERROR;

	tick_t startTime = GetMilliseconds();
	do {
		ieDword fnlen, complen, declen;
		compressed->ReadDword(fnlen);
		if (!fnlen) {
			Log(ERROR, "SAVImporter", "Corrupt Save Detected");
			return GEM_ERROR;
		}
		std::string fname(fnlen, '\0');
		compressed->Read(&fname[0], fnlen);

		// fnlen includes a terminating zero
		fname.resize(fnlen - 1);
		StringToLower(fname);

		auto position = compressed->GetPos();
		compressed->ReadDword(declen);
		compressed->ReadDword(complen);

		strpos_t pos = fname.find(".are");
		if (pos != std::string::npos && pos == fname.length() - 4) {
			areExtractor.registerLocation(fname.substr(0, pos), position);
			compressed->Seek(complen, GEM_CURRENT_POS);
		} else {
			Log(MESSAGE, "SAVImporter", "Decompressing {}", fname);
			DataStream* cached = CacheCompressedStream(compressed, fname, complen, true);

			if (!cached)
				return GEM_ERROR;
			delete cached;
		}

		Current = compressed->Remains();
		//starting at 20% going up to 70%
		percent = (20 + (All - Current) * 50 / All);
		if (percent - last_percent > 5) {
			core->LoadProgress(static_cast<int>(percent));
			last_percent = percent;
		}
	}
	while(Current);

	tick_t endTime = GetMilliseconds();
	Log(MESSAGE, "Core", "{} ms (extracting the SAV)", endTime - startTime);
	return GEM_OK;
}

//this one can create .sav files only
int SAVImporter::CreateArchive(DataStream *compressed)
{
	if (!compressed) {
		return GEM_ERROR;
	}

	char Signature[9];
	strlcpy(Signature, "SAV V1.0", 9);
	compressed->Write(Signature, 8);

	return GEM_OK;
}

int SAVImporter::AddToSaveGame(DataStream *str, DataStream *uncompressed)
{
	size_t fnlen = uncompressed->filename.length() + 1;
	strpos_t declen = uncompressed->Size();
	str->WriteScalar<size_t, ieDword>(fnlen);
	str->Write(uncompressed->filename.begin(), fnlen);
	str->WriteScalar<strpos_t, ieDword>(declen);
	//baaah, we dump output right in the stream, we get the compressed length
	//only after the compressed data was written
	ieDword complen = 0xcdcdcdcd; //placeholder
	strpos_t Pos = str->GetPos(); //storing the stream position
	str->WriteDword(complen);

	PluginHolder<Compressor> comp = MakePluginHolder<Compressor>(PLUGIN_COMPRESSION_ZLIB);
	comp->Compress( str, uncompressed );

	//writing compressed length (calculated)
	strpos_t Pos2 = str->GetPos();
	complen = ieDword(Pos2 - Pos - sizeof(ieDword)); //calculating the compressed stream size
	str->Seek(Pos, GEM_STREAM_START); //going back to the placeholder
	str->WriteDword(complen);       //updating size
	str->Seek(Pos2, GEM_STREAM_START);//resuming work
	return GEM_OK;
}

int SAVImporter::AddToSaveGameCompressed(DataStream *str, DataStream *compressed) {
	using BufferT = std::array<uint8_t, 4096>;
	BufferT buffer{};

	BufferT::size_type remaining = compressed->Size();
	while (remaining > 0) {
		auto copySize = std::min(buffer.size(), remaining);
		compressed->Read(buffer.data(), copySize);
		str->Write(buffer.data(), copySize);
		remaining -= copySize;
	}

	return GEM_OK;
}

#include "plugindef.h"

GEMRB_PLUGIN(0xCDF132C, "SAV File Importer")
PLUGIN_CLASS(IE_SAV_CLASS_ID, SAVImporter)
END_PLUGIN()
