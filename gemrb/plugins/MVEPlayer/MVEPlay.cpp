#include "MVEPlay.h"

char Header[] = {"Interplay MVE File\x1A\0"};

MVEPlay::MVEPlay(void)
{
}

MVEPlay::~MVEPlay(void)
{
}

int MVEPlay::LoadFromFile(char* filename)
{
	loadMethod = LOADED_FROM_FILE;
	stream = fopen(filename, "rb");
	if(stream == NULL)
		return GEM_ERROR;
	//TODO: Implement MVE File Loading
	return GEM_OK;
}

int MVEPlay::LoadFromStream(FILE* stream, bool autoFree)
{
	loadMethod = LOADED_FROM_STREAM;
	autoFreeBuffer = autoFree;
	this->stream = stream;
	//TODO: Implement MVE File Loading
	return GEM_OK;
}

int MVEPlay::LoadFromMemory(Byte* buffer, int length, bool autoFree)
{
	loadMethod = LOADED_FROM_MEMORY;
	autoFreeBuffer = autoFree;
	ptr = buffer;
	ptrlen = length;
	//TODO: Implement MVE File Loading
	return GEM_OK;
}

int MVEPlay::UnloadMovie(void)
{
	switch(loadMethod) {
		case LOADED_FROM_FILE:
			{
				fclose(stream);
			}
		break;

		case LOADED_FROM_STREAM:
			{
				if(autoFreeBuffer)
					fclose(stream);
			}
		break;

		case LOADED_FROM_MEMORY:
			{
				if(autoFreeBuffer)
					free(ptr);
			}
		break;

		default:
			return GEM_OK;
	}
	return GEM_OK;
}

int MVEPlay::Play()
{
	return 0;
}
