/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/Core/Interface.h,v 1.102 2004/08/03 17:36:16 avenger_teambg Exp $
 *
 */

#ifndef INTERFACE_H
#define INTERFACE_H

#include "Compressor.h"
#include "InterfaceDesc.h"
#include "PluginMgr.h"
#include "Video.h"
#include "ResourceMgr.h"
#include "../../includes/SClassID.h"
#include "StringMgr.h"
#include "ActorBlock.h"
#include "Factory.h"
#include "ImageMgr.h"
#include "ActorMgr.h"
#include "Font.h"
#include "EventMgr.h"
#include "WindowMgr.h"
#include "ScriptEngine.h"
#include "Button.h"
#include "Label.h"
#include "Slider.h"
#include "TextEdit.h"
#include "Console.h"
#include "SoundMgr.h"
#include "SaveGameIterator.h"
#include "Variables.h"
#include "MusicMgr.h"
#include "TableMgr.h"
#include "SymbolMgr.h"
#include "MoviePlayer.h"
#include "DataFileMgr.h"
//#include "PathFinder.h"
#include "GameScript.h"
#include "Game.h"
#include "GameControl.h"
#include "GlobalTimer.h"
#include "SaveGameMgr.h"

typedef struct Table {
	TableMgr * tm;
	char ResRef[8];
	bool free;
} Table;

typedef struct Symbol {
	SymbolMgr * sm;
	char ResRef[8];
	bool free;
} Symbol;

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
public:
	StringMgr *strings;
	GlobalTimer * timer;
private:
	Factory * factory;
	ImageMgr * pal256;
	ImageMgr * pal32;
	ImageMgr * pal16;
	std::vector<Font*> fonts;
	EventMgr * evntmgr;
	WindowMgr * windowmgr;
	char WindowPack[10];
	ScriptEngine * guiscript;
	SoundMgr * soundmgr;
	SaveGameIterator *sgiterator;
	/** Windows Array */
	std::vector<Window*> windows;
	std::vector<int> topwin;
	Variables * vars;
	Variables * tokens;
	MusicMgr * music;
	std::vector<Table> tables;
	std::vector<Symbol> symbols;
	DataFileMgr * INIparty;
	DataFileMgr * INIbeasts;
	DataFileMgr * INIquests;
	Game * game;
	int GameFeatures;
	char ButtonFont[9];
	char CursorBam[9];
	char TooltipFont[9];
	char Palette16[9];
	char Palette32[9];
	char Palette256[9];
	unsigned int* slotmatrix; //itemtype vs slottype
	unsigned int* slottypes;  //slottype vs slot mapping
	int ItemTypes;
	int SlotTypes; //this is the same as the inventory size
	int tooltip_x;
	int tooltip_y;
	char* tooltip_text;
public:
	char GlobalScript[9];
	char GlobalMap[9];
	Animation **Cursors;
	int CursorCount;
public:
	Interface(int iargc, char **iargv);
	~Interface(void);
	int Init(void);
	//TODO: Core Methods in Interface Class
	int SetFeature(int value, int position);
	int HasFeature(int position);
	bool IsAvailable(SClass_ID filetype);
	void * GetInterface(SClass_ID filetype);
	const char * TypeExt(SClass_ID type);
	Video * GetVideoDriver();
	ResourceMgr * GetResourceMgr();
	char * GetString(ieStrRef strref, unsigned long options=0);
	void FreeInterface(void * ptr);
	Factory * GetFactory(void);
	/** No descriptions */
	Color * GetPalette(int index, int colors);
	/** Returns a preloaded Font */
	Font * GetFont(char * ResRef);
	Font * GetFont(unsigned int index);
	/** Returns the button font */
	Font * GetButtonFont();
	/** Returns the Event Manager */
	EventMgr * GetEventMgr();
	/** Returns the Window Manager */
	WindowMgr * GetWindowMgr();
	/** Get GUI Script Manager */
	ScriptEngine * GetGUIScriptEngine();
	/** Returns a PC index, by loading a creature */
	int LoadCreature(char *ResRef, int InParty);
	/** Removes a creature by slot */
//	int UnloadCreature(unsigned int Slot);
	/** Removes a creature by pointer */
//	int UnloadCreature(Actor *actor);
	/** Returns the actor count */
//	int GetActorCount() { return actors.size(); };
	/** Add the actor to the list of actors and returns its index */
//	int AddActor(Actor* actor);
	/** Returns the actor pointer for Slot */
//	Actor *GetActor(unsigned int Slot);
	/** Enters the premade actors (chargen PC's) into the game */
//	void EnterActors(const char *StartArea);
	/** Returns the starting party size (chargen PC's) count */
//	int GetPartySize();
	/** Returns actor index for partyslot PartySlotCount */
//	int FindPlayer(int PartySlotCount);
	/** Sets a stat for the creature in actor index Slot */
	int SetCreatureStat(unsigned int Slot, unsigned int StatID, int StatValue, int Mod);
	/** returns the stat of a creature (mod:1-modified, 0-base) */
	int GetCreatureStat(unsigned int Slot, unsigned int StatID, int Mod);
	/** Loads a WindowPack (CHUI file) in the Window Manager */
	bool LoadWindowPack(const char *name);
	/** Loads a Window in the Window Manager */
	int LoadWindow(unsigned short WindowID);
	/** Creates a Window in the Window Manager */
#ifdef WIN32
#ifdef CreateWindow
#undef CreateWindow
#endif
#endif
	int CreateWindow(unsigned short WindowID, int XPos, int YPos, unsigned int Width, unsigned int Height, char* Background);
	/** Sets a Window on the Top */
	void SetOnTop(int Index)
	{
		std::vector<int>::iterator t;
		for(t = topwin.begin(); t != topwin.end(); ++t) {
			if((*t) == Index) {
				topwin.erase(t);
				break;
			}
		}
		if(topwin.size() != 0)
			topwin.insert(topwin.begin(), Index);
		else
			topwin.push_back(Index);
	}
	/** Add a window to the Window List */
	void AddWindow(Window * win)
	{
		int slot = -1;
		for(unsigned int i = 0; i < windows.size(); i++) {
			if(windows[i]==NULL) {
				slot = i;
				break;
			}
		}
		if(slot == -1) {
			windows.push_back(win);
			slot=(int)windows.size()-1;
		}
		else
			windows[slot] = win;
		win->Invalidate();
	}
	/** Get a Control on a Window */
	int GetControl(unsigned short WindowIndex, unsigned long ControlID);
	/** Set the Text of a Control */
	int SetText(unsigned short WindowIndex, unsigned short ControlIndex, const char * string);
	/** Set the Tooltip text of a Control */
	int SetTooltip(unsigned short WindowIndex, unsigned short ControlIndex, const char * string);
	/** sets tooltip to be displayed */
	void DisplayTooltip(int x, int y, char* text);
	/** Actually draws tooltip on the screen. Called from SDLVideoDriver */
	void DrawTooltip();
	/** displays a string in the textarea */
	void DisplayConstantString(int stridx, unsigned int color);
	/** Set a Window Visible Flag */
	int SetVisible(unsigned short WindowIndex, int visible);
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
	/** Redraws all window */
	void RedrawAll();
	/** Popup the Console */
	void PopupConsole();
	/** Draws the Console */
	void DrawConsole();
	/** Get the Sound Manager */
	SoundMgr * GetSoundMgr();
	/** Get the SaveGameIterator */
	SaveGameIterator * GetSaveGameIterator();
	/** Get the Variables Dictionary */
	Variables * GetDictionary();
	/** Get the Token Dictionary */
	Variables * GetTokenDictionary();
	/** Get the Music Manager */
	MusicMgr * GetMusicMgr();
	/** Loads a 2DA Table, returns -1 on error or the Table Index on success */
	int LoadTable(const char * ResRef);
	/** Gets the index of a loaded table, returns -1 on error */
	int GetIndex(const char * ResRef);
	/** Gets a Loaded Table by its index, returns NULL on error */
	TableMgr * GetTable(unsigned int index);
	/** Frees a Loaded Table, returns false on error, true on success */
	bool DelTable(unsigned int index);
	/** Loads an IDS Table, returns -1 on error or the Symbol Table Index on success */
	int LoadSymbol(const char * ResRef);
	/** Gets the index of a loaded Symbol Table, returns -1 on error */
	int GetSymbolIndex(const char * ResRef);
	/** Gets a Loaded Symbol Table by its index, returns NULL on error */
	SymbolMgr * GetSymbol(unsigned int index);
	/** Frees a Loaded Symbol Table, returns false on error, true on success */
	bool DelSymbol(unsigned int index);
	/** Plays a Movie */
	int PlayMovie(char * ResRef);
	/** Generates traditional random number xdy+z */
	int Roll(int dice, int size, int add);
	/** Loads a Game Compiled Script */
	int LoadScript(const char * ResRef);
	/** Sets a Variable in the Game Script Engine */
	void SetGameVariable(const char * VarName, const char * Context, int value);
	/** Enables/Disables the CutScene Mode */
	void SetCutSceneMode(bool active);
	/** returns true if in cutscene mode */
	bool InCutSceneMode();
	/** Updates the Game Script Engine State */
	void GSUpdate()
	{
		timer->Update();
	}
	/** Get the Party INI Interpreter */
	DataFileMgr * GetPartyINI()
	{
		return INIparty;
	}
	DataFileMgr * GetBeastsINI()
	{
		return INIbeasts;
	}
	DataFileMgr * GetQuestsINI()
	{
		return INIquests;
	}
	/** Gets the Game class */
	Game * GetGame()
	{
		return game;
	}
	GameControl *GetGameControl();

	void QuitGame();
	void LoadGame(int index);
	/*reads the filenames of the sounds folder into a list */
	int GetCharSounds(TextArea *ta);
	/*returns true if an itemtype is acceptable for a slottype */
	int CanUseItemType(int itype, int slottype);
	/*removes all files from directory*/
	void DelTree(const char *path, bool onlysaved);
	/*returns true if the file should be saved */
	bool SavedExtension(const char *filename);
private:
	bool LoadConfig(void);
	bool LoadConfig(const char *filename);
	bool LoadGemRBINI();
	bool LoadINI(const char * filename);
	bool InitItemTypes();
	bool ReadStrrefs();

public:
	char GameData[9];
	char GameOverride[9];
	char GameNameResRef[9];
	char UserDir[_MAX_PATH];
	int argc;
	char **argv;	  
	char GameName[_MAX_PATH];
	char GameType[_MAX_PATH];
	char GemRBPath[_MAX_PATH];
	char PluginsPath[_MAX_PATH];
	char CachePath[_MAX_PATH];
	char GUIScriptsPath[_MAX_PATH];
	char GamePath[_MAX_PATH];
	char SavePath[_MAX_PATH];
	char INIConfig[_MAX_PATH];
	char CD1[_MAX_PATH];
	char CD2[_MAX_PATH];
	char CD3[_MAX_PATH];
	char CD4[_MAX_PATH];
	char CD5[_MAX_PATH];
	char CD6[_MAX_PATH];
	int Width, Height, Bpp, ForceStereo;
	unsigned int TooltipDelay;
	bool FullScreen, CaseSensitive, GameOnCD, SkipIntroVideos, DrawFPS;
	/** Draws the Visible windows in the Windows Array */
	void DrawWindows(void);
	/** Sends a termination signal to the Video Driver */
	bool Quit(void);
	/** CheatKey support */
	void EnableCheatKeys(int Flag)
	{
        	CheatFlag=(Flag > 0);
	}

	bool CheatEnabled()
	{
        	return CheatFlag;
	}

	/** Next Script Name */
	char NextScript[64];
	/** Need to Load a new Script */
	bool ChangeScript;
	/** Console is on Screen */
	bool ConsolePopped;
	/** Cheats enabled? */
	bool CheatFlag;
	/** The Console Object */
	Console * console;

#ifdef _DEBUG
	int FileStreamPtrCount;
	int CachedFileStreamPtrCount;
#endif
};

#ifndef GEM_BUILD_DLL
#ifdef WIN32
__declspec(dllimport) Interface * core;
__declspec(dllimport) HANDLE hConsole;
#else
extern Interface * core;
#endif
#endif

#endif
