#ifndef MVEPLAY_H
#define MVEPLAY_H

#include "../../includes/globals.h"
#include "../Core/MoviePlayer.h"

#define LOADED_FROM_FILE	0
#define LOADED_FROM_STREAM	1
#define LOADED_FROM_MEMORY	2

class MVEPlay :	public MoviePlayer
{
private:
	int loadMethod;
	bool autoFreeBuffer;
	
	FILE * stream;
	
	Byte * ptr;
	int ptrlen;

	char Header[20];

public:
	MVEPlay(void);
	~MVEPlay(void);
	int LoadFromFile(char* filename);
	int LoadFromStream(FILE* stream, bool autoFree = false);
	int LoadFromMemory(Byte* buffer, int length, bool autoFree = false);
	int UnloadMovie(void);
	int Play();
};

#endif
