#ifndef SOUNDMGR_H
#define SOUNDMGR_H

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

class GEM_EXPORT SoundMgr : public Plugin
{
public:
	SoundMgr(void);
	virtual ~SoundMgr(void);
	virtual bool Init(void) = 0;
	virtual unsigned long Play(const char * ResRef) = 0;
	virtual unsigned long LoadFile(const char * filename) = 0;
	virtual bool Stop(unsigned long index) = 0;
	virtual bool Play(unsigned long index) = 0;
	virtual void ResetMusics() = 0;
};

#endif
