// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "KEYImporter.h"

#include "Interface.h"
#include "ResourceDesc.h"

#include "Logging/Logging.h"
#include "Streams/FileStream.h"
#include "System/FileFilters.h"

using namespace GemRB;

static path_t AddCBF(path_t file)
{
	size_t pos = file.find_last_of('.');
	if (pos != path_t::npos) {
		file.replace(pos, 4, ".cbf");
	} else {
		file += ".cbf";
	}
	return file;
}

static bool PathExists(BIFEntry* entry, const path_t& path)
{
	entry->path = PathJoin(path, entry->name);
	if (FileExists(entry->path)) {
		return true;
	}
	entry->path = PathJoin(path, AddCBF(entry->name));
	if (FileExists(entry->path)) {
		return true;
	}

	return false;
}

static bool PathExists(BIFEntry* entry, const std::vector<std::string>& pathlist)
{
	for (const auto& path : pathlist) {
		if (PathExists(entry, path)) {
			return true;
		}
	}

	return false;
}

static void FindBIF(BIFEntry* entry)
{
	entry->cd = 0;
	entry->found = PathExists(entry, core->config.GamePath);
	if (entry->found) {
		return;
	}
	// also check the data/Data path for gog
	path_t path = PathJoin(core->config.GamePath, core->config.GameDataPath);
	entry->found = PathExists(entry, path);
	if (entry->found) {
		return;
	}

	for (int i = 0; i < MAX_CD; i++) {
		if (PathExists(entry, core->config.CD[i])) {
			entry->found = true;
			entry->cd = i;
			return;
		}
	}

	Log(ERROR, "KEYImporter", "Cannot find {}...", entry->name);
}

bool KEYImporter::Open(const path_t& resfile, std::string desc)
{
	description = std::move(desc);
	if (!core->IsAvailable(IE_BIF_CLASS_ID)) {
		Log(ERROR, "KEYImporter", "An Archive Plug-in is not Available");
		return false;
	}

	// NOTE: Interface::Init has already resolved resfile.
	Log(MESSAGE, "KEYImporter", "Opening {}...", resfile);
	FileStream* f = FileStream::OpenFile(resfile);
	if (!f) {
		// Check for backslashes (false escape characters)
		// this check probably belongs elsewhere (e.g. ResolveFilePath)
		if (resfile.find("\\ ") != path_t::npos) {
			Log(MESSAGE, "KEYImporter", "Escaped space(s) detected in path!. Do not escape spaces in your GamePath!");
		}
		Log(ERROR, "KEYImporter", "Cannot open Chitin.key");
		Log(ERROR, "KeyImporter", "This means you set the GamePath config variable incorrectly.");
		Log(ERROR, "KeyImporter", "It must point to the directory that holds a readable Chitin.key");
		return false;
	}
	Log(MESSAGE, "KEYImporter", "Checking file type...");
	char Signature[8];
	f->Read(Signature, 8);
	if (strncmp(Signature, "KEY V1  ", 8) != 0) {
		Log(ERROR, "KEYImporter", "File has an Invalid Signature.");
		delete f;
		return false;
	}
	Log(MESSAGE, "KEYImporter", "Reading Resources...");
	ieDword BifCount, ResCount, BifOffset, ResOffset;
	f->ReadDword(BifCount);
	f->ReadDword(ResCount);
	f->ReadDword(BifOffset);
	f->ReadDword(ResOffset);
	Log(MESSAGE, "KEYImporter", "BIF Files Count: {} (Starting at {} Bytes)",
	    BifCount, BifOffset);
	Log(MESSAGE, "KEYImporter", "RES Count: {} (Starting at {} Bytes)",
	    ResCount, ResOffset);
	f->Seek(BifOffset, GEM_STREAM_START);

	ieDword BifLen, ASCIIZOffset;
	ieWord ASCIIZLen;
	for (unsigned int i = 0; i < BifCount; i++) {
		BIFEntry be;
		f->Seek(BifOffset + (12 * i), GEM_STREAM_START);
		f->ReadDword(BifLen);
		f->ReadDword(ASCIIZOffset);
		f->ReadWord(ASCIIZLen);
		f->ReadWord(be.BIFLocator);
		be.name.resize(ASCIIZLen);
		f->Seek(ASCIIZOffset, GEM_STREAM_START);
		f->Read(&be.name[0], ASCIIZLen);

		// IWD ToTL: \\data\\zcHMar.bif
		if (be.name[0] == '\\') {
			be.name.erase(0, 1);
			ASCIIZLen -= 1;
		}

		for (int p = 0; p < ASCIIZLen; p++) {
			//some MAC versions use : as delimiter
			if (be.name[p] == '\\' || be.name[p] == ':')
				be.name[p] = PathDelimiter;
		}
		FindBIF(&be);
		biffiles.push_back(std::move(be));
	}
	f->Seek(ResOffset, GEM_STREAM_START);

	MapKey key;
	ieDword ResLocator;
	ieWord type;

	for (unsigned int i = 0; i < ResCount; i++) {
		f->ReadResRef(key.ref);
		f->ReadWord(type);
		f->ReadDword(ResLocator);
		key.type = type;

		// seems to be always the last entry?
		if (!key.ref.IsEmpty())
			resources.emplace(key, ResLocator);
	}

	Log(MESSAGE, "KEYImporter", "Resources Loaded...");
	delete f;
	return true;
}

// If we find BIFs with the same name in the given path, use
// them instead. This allows language-aware overrides.
void KEYImporter::MergeBifsFromPath(const path_t& path)
{
	DirectoryIterator dirIt { path };
	dirIt.SetFilterPredicate(std::make_shared<ExtFilter>("bif"));

	do {
		BIFEntry bifEntry;
		const path_t& name = dirIt.GetName();
		bifEntry.found = true;
		bifEntry.name = name;
		bifEntry.path = PathJoin(path, name);

		auto checkName = fmt::format("data/{}", name);
		auto lookup =
			std::find_if(biffiles.begin(), biffiles.end(), [&checkName](const BIFEntry& bif) {
				// names in biffiles have a trailing NUL
				auto bifName = bif.name;
				bifName.resize(checkName.size());
				return bifName == checkName;
			});

		if (lookup != biffiles.end()) {
			*lookup = bifEntry;
		}
	} while (++dirIt);
}

bool KEYImporter::HasResource(StringView resname, SClass_ID type)
{
	return resources.find({ ResRef(resname), type }) != resources.cend();
}

bool KEYImporter::HasResource(StringView resname, const ResourceDesc& type)
{
	return HasResource(resname, type.GetKeyType());
}

DataStream* KEYImporter::GetStream(const ResRef& resname, ieWord type)
{
	if (type == 0)
		return nullptr;

	const auto lookup = resources.find({ resname, type });
	if (lookup == resources.cend())
		return 0;

	auto ResLocator = lookup->second;
	unsigned int bifnum = (ResLocator & 0xFFF00000) >> 20;

	// supports BIFF-less, KEY'd games (demo)
	if (bifnum >= biffiles.size()) {
		return nullptr;
	}

	if (!biffiles[bifnum].found) {
		Log(ERROR, "KEYImporter", "Cannot find {}... Resource unavailable.",
		    biffiles[bifnum].name);
		return nullptr;
	}

	PluginHolder<IndexedArchive> ai = MakePluginHolder<IndexedArchive>(IE_BIF_CLASS_ID);
	if (ai->OpenArchive(biffiles[bifnum].path) == GEM_ERROR) {
		Log(ERROR, "KEYImporter", "Cannot open archive {}", biffiles[bifnum].path);
		return nullptr;
	}

	DataStream* ret = ai->GetStream(ResLocator, type);
	if (ret) {
		ret->filename.Format("{}.{}", resname, TypeExt(type));
		StringToLower(ret->filename);
		return ret;
	}

	return nullptr;
}

DataStream* KEYImporter::GetResource(StringView resname, SClass_ID type)
{
	//the word masking is a hack for synonyms, currently used for bcs==bs
	return GetStream(ResRef(resname), type & 0xFFFF);
}

DataStream* KEYImporter::GetResource(StringView resname, const ResourceDesc& type)
{
	return GetStream(ResRef(resname), type.GetKeyType());
}

#include "plugindef.h"

GEMRB_PLUGIN(0x1DFDEF80, "KEY File Importer")
PLUGIN_CLASS(PLUGIN_RESOURCE_KEY, KEYImporter)
END_PLUGIN()
