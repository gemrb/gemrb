#ifndef SAVEGAMEITERATOR_H
#define SAVEGAMEITERATOR_H

#include "FileStream.h"

class SaveGame {
public:
	SaveGame(char * path, char * name, char * prefix, int pCount)
	{
		strncpy(Prefix, prefix, sizeof(Prefix) );
		strncpy(Path, path, sizeof(Path) );
		strncpy(Name, name, sizeof(Name) );
		PortraitCount = pCount;
	};
	~SaveGame() {};
	int GetPortraitCount() { return PortraitCount; };
	const char *GetName() { return Name; };
	const char *GetPrefix() { return Prefix; };
	const char *GetPath() { return Path; };

	DataStream * GetPortrait(int index)
	{
		if(index > PortraitCount)
			return NULL;
		char nPath[_MAX_PATH];
		sprintf(nPath, "%s%sPORTRT%d.bmp", Path, SPathDelimiter, index);
		FileStream * fs = new FileStream();
		fs->Open(nPath, true);
		return fs;
	};
	DataStream * GetScreen()
	{
		char nPath[_MAX_PATH];
		sprintf(nPath, "%s%s%s.bmp", Path, SPathDelimiter, Prefix);
		FileStream * fs = new FileStream();
		fs->Open(nPath, true);
		return fs;
	};
private:
	char Path[_MAX_PATH];
	char Prefix[_MAX_PATH];
	char Name[_MAX_PATH];
	int  PortraitCount;
};

class SaveGameIterator
{
public:
	SaveGameIterator(void);
	~SaveGameIterator(void);
	int GetSaveGameCount();
	SaveGame * GetSaveGame(int index);
};

#endif
