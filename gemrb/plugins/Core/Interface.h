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
#include "WindowMgr.h"
#include "ScriptEngine.h"
#include "Button.h"
#include "Console.h"
#include "SoundMgr.h"
#include "Variables.h"

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
	WindowMgr * windowmgr;
	ScriptEngine * guiscript;
	SoundMgr * soundmgr;
	/** Windows Array */
	std::vector<Window*> windows;
	/** Free Windows Array Slots */
	std::vector<bool> freeslots;
	Variables * vars;
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
	/** Returns the Window Manager */
	WindowMgr * GetWindowMgr();
	/** Get GUI Script Manager */
	ScriptEngine * GetGUIScriptEngine();
	/** Loads a Window in the Window Manager */
	int LoadWindow(unsigned short WindowID);
	/** Get a Control on a Window */
	int GetControl(unsigned short WindowIndex, unsigned short ControlID);
	/** Set the Text of a Control */
	int SetText(unsigned short WindowIndex, unsigned short ControlIndex, const char * string);
	/** Set a Window Visible Flag */
	int SetVisible(unsigned short WindowIndex, bool visible);
	/** Show a Window in Modal Mode */
	int ShowModal(unsigned short WindowIndex);
	/** Set an Event of a Control */
	int SetEvent(unsigned short WindowIndex, unsigned short ControlIndex, unsigned long EventID, char * funcName);
	/** Set the Status of a Control in a Window */
	int SetControlStatus(unsigned short WindowIndex, unsigned short ControlIndex, unsigned long Status);
	/** Get a Window from the Loaded Window List */
	Window * GetWindow(unsigned short WindowIndex);
	/** Removes a Loaded Window */
	int DelWindow(unsigned short WindowIndex);
	/** Popup the Console */
	void PopupConsole();
	/** Draws the Console */
	void DrawConsole();
	/** Get the Sound Manager */
	SoundMgr * GetSoundMgr();
	/** Get the Variables Dictionary */
	Variables * GetDictionary();
private:
	bool LoadConfig(void);
public:
	char GemRBPath[_MAX_PATH], CachePath[_MAX_PATH], GUIScriptsPath[_MAX_PATH], GamePath[_MAX_PATH], CD1[_MAX_PATH], CD2[_MAX_PATH], CD3[_MAX_PATH], CD4[_MAX_PATH], CD5[_MAX_PATH];
	int Width, Height, Bpp;
	bool FullScreen;
	/** Draws the Visible windows in the Windows Array */
	void DrawWindows(void);
  /** Sends a termination signal to the Video Driver */
  bool Quit(void);
	/** Next Script Name */
	char NextScript[64];
	/** Need to Load a new Script */
	bool ChangeScript;
	/** Console is on Screen */
	bool ConsolePopped;
	/** The Console Object */
	Console * console;
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
