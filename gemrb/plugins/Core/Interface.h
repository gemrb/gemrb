#ifndef INTERFACE_H
#define INTERFACE_H

#include "Compressor.h"
#include "InterfaceDesc.h"
#include "PluginMgr.h"
#include "Video.h"
#include "ResourceMgr.h"
#include "../../includes/SClassID.h"
#include "StringMgr.h"
#include "Actor.h"
#include "HCAnimationSeq.h"
#include "Factory.h"
#include "ImageMgr.h"
#include "Font.h"
#include "EventMgr.h"

#ifdef WIN32

#include <windows.h>

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Interface : public InterfaceDesc
{
private:
	PluginMgr * plugin;
	Video * video;
	ResourceMgr * key;
	StringMgr *strings;
	HCAnimationSeq * hcanims;
	Factory * factory;
	ImageMgr * pal256;
	ImageMgr * pal16;
	std::vector<Font*> fonts;
	EventMgr * evntmgr;
public:
	Interface(void);
	~Interface(void);
	int Init(void);
	//TODO: Core Methods in Interface Class
	bool IsAvailable(SClass_ID filetype);
	void * GetInterface(SClass_ID filetype);
	char * TypeExt(SClass_ID type);
	Video * GetVideoDriver();
	ResourceMgr * GetResourceMgr();
	char * GetString(unsigned long strref);
	void GetHCAnim(Actor * act);
	void FreeInterface(void * ptr);
	Factory * GetFactory(void);
  /** No descriptions */
  Color * GetPalette(int index, int colors);
  /** Returns a preloaded Font */
  Font * GetFont(char * ResRef);
  /** Returns the Event Manager */
  EventMgr * GetEventMgr();
private:
	bool LoadConfig(void);
public:
	char GemRBPath[_MAX_PATH], CachePath[_MAX_PATH], GamePath[_MAX_PATH], CD1[_MAX_PATH], CD2[_MAX_PATH], CD3[_MAX_PATH], CD4[_MAX_PATH], CD5[_MAX_PATH];
	int Width, Height, Bpp;
	bool FullScreen;
};
#ifndef GEM_BUILD_DLL
#ifdef WIN32
__declspec(dllimport) Interface * core;
__declspec(dllimport) HANDLE hConsole;
#else
extern Interface *core;
#endif
#endif

#endif
