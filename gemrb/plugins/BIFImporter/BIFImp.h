#ifndef BIFIMP_H
#define BIFIMP_H

#include "../../includes/globals.h"
#include "../Core/ArchiveImporter.h"
#include "../Core/CachedFileStream.h"

typedef struct FileEntry {
	unsigned long	resLocator;
	unsigned long	dataOffset;
	unsigned long	fileSize;
	unsigned short	type;
	unsigned short  u1; //Unknown Field
} FileEntry;

typedef struct TileEntry {
	unsigned long	resLocator;
	unsigned long	dataOffset;
	unsigned long	tilesCount;
	unsigned long	fileSize;
	unsigned short	type;
	unsigned short	u1; //Unknown Field
} TileEntry;

class BIFImp : public ArchiveImporter
{
private:
	char path[_MAX_PATH];
	std::vector<FileEntry> fentries;
	std::vector<TileEntry> tentries;
	CachedFileStream* stream;
public:
	BIFImp(void);
	~BIFImp(void);
	int OpenArchive(char* filename, bool cacheCheck = true);
	DataStream* GetStream(unsigned long Resource, unsigned long Type);
private:
	void ReadBIF(void);
};

#endif
