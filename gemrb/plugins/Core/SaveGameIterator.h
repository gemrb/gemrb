#ifndef SAVEGAMEITERATOR_H
#define SAVEGAMEITERATOR_H

#include <time.h>
#include <sys/stat.h>
#include "FileStream.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT SaveGame {
public:
	SaveGame(char * path, char * name, char * prefix, int pCount)
	{
		strncpy(Prefix, prefix, sizeof(Prefix) );
		strncpy(Path, path, sizeof(Path) );
		strncpy(Name, name, sizeof(Name) );
		PortraitCount = pCount;
		char nPath[_MAX_PATH];
		struct stat my_stat;
		sprintf(nPath, "%s%s%s.bmp", Path, SPathDelimiter, Prefix);
		stat(nPath, &my_stat);
		strftime(Date, _MAX_PATH, "%c",localtime(&my_stat.st_mtime));
	};
	~SaveGame() {};
	int GetPortraitCount() { return PortraitCount; };
	const char *GetName() { return Name; };
	const char *GetPrefix() { return Prefix; };
	const char *GetPath() { return Path; };
	const char *GetDate() { return Date; };

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
	char Prefix[10];
	char Name[_MAX_PATH];
	char Date[_MAX_PATH];
	int  PortraitCount;
};

class GEM_EXPORT SaveGameIterator
{
public:
	SaveGameIterator(void);
	~SaveGameIterator(void);
	int GetSaveGameCount();
	SaveGame * GetSaveGame(int index);
};

#endif
