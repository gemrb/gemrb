#ifndef VIDEOMODES_H
#define VIDEOMODES_H

#include "../../includes/win32def.h"
#include "VideoMode.h"
#include <vector>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT VideoModes
{
private:
	std::vector<VideoMode> modes;
public:
	VideoModes(void);
	~VideoModes(void);
	int AddVideoMode(int w, int h, int bpp, bool fs, bool checkUnique=true);
	int FindVideoMode(VideoMode &vm);
	void RemoveEntry(unsigned long n);
	void Empty(void);
	VideoMode operator[](unsigned long n);
	int Count(void);
};

#endif
