#ifndef MOVIEPLAYER_H
#define MOVIEPLAYER_H

#include "../../includes/globals.h"
#include "Plugin.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT MoviePlayer : public Plugin
{
public:
	MoviePlayer(void);
	virtual ~MoviePlayer(void);
	virtual int LoadFromFile(char* filename) = 0;
	virtual int LoadFromStream(FILE * stream, bool autoFree = false) = 0;
	virtual int LoadFromMemory(Byte * buffer, int length, bool autoFree = false) = 0;
	virtual int UnloadMovie(void) = 0;
	virtual int Play() = 0;
};

#endif
