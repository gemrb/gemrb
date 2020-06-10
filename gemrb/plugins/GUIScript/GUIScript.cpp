/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "GUIScript.h"

#include "PythonHelpers.h"

#include "Audio.h"
#include "CharAnimations.h"
#include "ControlAnimation.h"
#include "DataFileMgr.h"
#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "EffectQueue.h"
#include "Game.h"
#include "GameData.h"
#include "ImageFactory.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "Item.h"
#include "Map.h"
#include "MusicMgr.h"
#include "Palette.h"
#include "PalettedImageMgr.h"
#include "ResourceDesc.h"
#include "RNG.h"
#include "SaveGameIterator.h"
#include "Spell.h"
#include "TableMgr.h"
#include "TileMap.h"
#include "Video.h"
#include "WorldMap.h"
#include "GameScript/GSUtils.h" //checkvariable
#include "GUI/Button.h"
#include "GUI/EventMgr.h"
#include "GUI/GameControl.h"
#include "GUI/Label.h"
#include "GUI/MapControl.h"
#include "GUI/TextArea.h"
#include "GUI/TextEdit.h"
#include "GUI/Window.h"
#include "GUI/WorldMapControl.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "System/FileStream.h"
#include "System/Logger/MessageWindowLogger.h"
#include "System/VFS.h"

#include <algorithm>
#include <cstdio>

// MIPSPro fix for IRIX
GEM_EXPORT size_t strlcpy(char *, const char *, size_t);

using namespace GemRB;

GUIScript *GemRB::gs = NULL;

//this stuff is missing from Python 2.2
#ifndef PyDoc_VAR
#define PyDoc_VAR(name) static char name[]
#endif

#ifndef PyDoc_STR
# ifdef WITH_DOC_STRINGS
# define PyDoc_STR(str) str
# else
# define PyDoc_STR(str) ""
# endif
#endif

#ifndef PyDoc_STRVAR
#define PyDoc_STRVAR(name,str) PyDoc_VAR(name) = PyDoc_STR(str)
#endif

// a shorthand for declaring methods in method table
#define METHOD(name, args) {#name, GemRB_ ## name, args, GemRB_ ## name ## __doc}

static int SpecialItemsCount = -1;
static int StoreSpellsCount = -1;
static int UsedItemsCount = -1;

//Check removal/equip/swap of item based on item name and actor's scriptname
#define CRI_REMOVE 0
#define CRI_EQUIP  1
#define CRI_SWAP   2
#define CRI_REMOVEFORSWAP 3

//bit used in SetCreatureStat to access some fields
#define EXTRASETTINGS 0x1000

//maximum distance for passing items between two characters
#define MAX_DISTANCE 500

struct UsedItemType {
	ieResRef itemname;
	ieVariable username; //death variable
	ieStrRef value;
	int flags;
};

typedef char EventNameType[17];
#define IS_DROP	0
#define IS_GET	1
#define IS_SWINGOFFSET 2 // offset to the swing sound columns

#define UNINIT_IEDWORD 0xcccccccc

static SpellDescType *SpecialItems = NULL;

static SpellDescType *StoreSpells = NULL;
static ItemExtHeader *ItemArray = NULL;
static SpellExtHeader *SpellArray = NULL;
static UsedItemType *UsedItems = NULL;

static int ReputationIncrease[20]={(int) UNINIT_IEDWORD};
static int ReputationDonation[20]={(int) UNINIT_IEDWORD};
//4 action button indices are packed on a single ieDword, there are 32 actions max.
//there are additional fake action buttons
static ieDword GUIAction[MAX_ACT_COUNT]={UNINIT_IEDWORD};
static ieDword GUITooltip[MAX_ACT_COUNT]={UNINIT_IEDWORD};
static ieResRef GUIResRef[MAX_ACT_COUNT];
static EventNameType GUIEvent[MAX_ACT_COUNT];
static bool QuitOnError = false;

// Natural screen size of currently loaded winpack
static int CHUWidth = 0;
static int CHUHeight = 0;

static Store *rhstore = NULL;

static EffectRef fx_learn_spell_ref = { "Spell:Learn", -1 };

// Like PyString_FromString(), but for ResRef
inline PyObject* PyString_FromResRef(char* ResRef)
{
	size_t i = strnlen(ResRef,sizeof(ieResRef));
	return PyString_FromStringAndSize( ResRef, i );
}

// Like PyString_FromString(), but for ResRef
inline PyObject* PyString_FromAnimID(const char* AnimID)
{
	unsigned int i = strnlen(AnimID,2);
	return PyString_FromStringAndSize( AnimID, i );
}

/* Sets RuntimeError exception and returns NULL, so this function
 * can be called in `return'.
 */
static PyObject* RuntimeError(const char* msg)
{
	Log(ERROR, "GUIScript", "Runtime Error:");
	PyErr_SetString( PyExc_RuntimeError, msg );
	if (QuitOnError) {
		core->ExitGemRB();
	}
	return NULL;
}

/* Prints error msg for invalid function parameters and also the function's
 * doc string (given as an argument). Then returns NULL, so this function
 * can be called in `return'. The exception should be set by previous
 * call to e.g. PyArg_ParseTuple()
 */
static PyObject* AttributeError(const char* doc_string)
{
	Log(ERROR, "GUIScript", "Syntax Error:");
	PyErr_SetString(PyExc_AttributeError, doc_string);
	if (QuitOnError) {
		core->ExitGemRB();
	}
	return NULL;
}

#define GET_GAME() \
	Game *game = core->GetGame(); \
	if (!game) { \
		return RuntimeError( "No game loaded!\n" ); \
	}

#define GET_MAP() \
	Map *map = game->GetCurrentArea(); \
	if (!map) { \
		return RuntimeError( "No current area!" ); \
	}

#define GET_GAMECONTROL() \
	GameControl *gc = core->GetGameControl(); \
	if (!gc) { \
		return RuntimeError("Can't find GameControl!"); \
	}

#define GET_ACTOR_GLOBAL() \
	Actor* actor; \
	if (globalID > 1000) { \
		actor = game->GetActorByGlobalID( globalID ); \
	} else { \
		actor = game->FindPC( globalID ); \
	} \
	if (!actor) { \
		return RuntimeError( "Actor not found!\n" ); \
	}

static Control *GetControl( int wi, int ci, int ct)
{
	char errorbuffer[256];

	Window* win = core->GetWindow( wi );
	if (win == NULL) {
		snprintf(errorbuffer, sizeof(errorbuffer), "Cannot find window index #%d (unloaded?)", wi);
		RuntimeError(errorbuffer);
		return NULL;
	}
	Control* ctrl = win->GetControl( ci );
	if (!ctrl) {
		snprintf(errorbuffer, sizeof(errorbuffer), "Cannot find control #%d", ci);
		RuntimeError(errorbuffer);
		return NULL;
	}
	if ((ct>=0) && (ctrl->ControlType != ct)) {
		snprintf(errorbuffer, sizeof(errorbuffer), "Invalid control type: %d!=%d", ctrl->ControlType, ct);
		RuntimeError(errorbuffer);
		return NULL;
	}
	return ctrl;
}

static int GetControlIndex(unsigned short wi, unsigned long ControlID)
{
	Window* win = core->GetWindow(wi);
	if (win == NULL) {
		return -1;
	}
	return win->GetControlIndex(ControlID);
}

//sets tooltip with Fx key prepended
static int SetFunctionTooltip(int WindowIndex, int ControlIndex, char *txt, int Function)
{
	if (txt) {
		ieDword ShowHotkeys = 0;
		core->GetDictionary()->Lookup("Hotkeys On Tooltips", ShowHotkeys);
		if (txt[0]) {
			if (!Function) {
				Function = ControlIndex+1;
			}
			int ret;
			if (ShowHotkeys) {
				char *txt2 = (char *) malloc(strlen(txt) + 10);
				sprintf(txt2, "F%d - %s", Function, txt);
				ret = core->SetTooltip((ieWord) WindowIndex, (ieWord) ControlIndex, txt2, Function);
				free(txt2);
			} else {
				ret = core->SetTooltip((ieWord) WindowIndex, (ieWord) ControlIndex, txt, Function);
			}
			core->FreeString(txt);
			return ret;
		}
		core->FreeString(txt);
	}
	return core->SetTooltip((ieWord) WindowIndex, (ieWord) ControlIndex, "", -1);
}

static int GetCreatureStrRef(Actor *actor, unsigned int Str)
{
	return actor->StrRefs[Str];
}


static inline bool CheckStat(Actor * actor, ieDword stat, ieDword value, int op)
{
	int dc = DiffCore(actor->GetBase(stat), value, op);
	return dc;
}

static bool StatIsASkill(unsigned int StatID) {
	// traps, lore, stealth, lockpicking, pickpocket
	if (StatID >= IE_LORE && StatID <= IE_PICKPOCKET) return true;

	// alchemy, animals, bluff, concentration, diplomacy, intimidate, search, spellcraft, magicdevice
	// NOTE: change if you want to use IE_PROFICIENCYCLUB or IE_EXTRAPROFICIENCY2 etc., as they use the same values
	if (StatID >= IE_ALCHEMY && StatID <= IE_MAGICDEVICE) return true;

	// Hide, Wilderness_Lore
	if (StatID == IE_HIDEINSHADOWS || StatID == IE_TRACKING) return true;

	return false;
}

static int GetCreatureStat(Actor *actor, unsigned int StatID, int Mod)
{
	//this is a hack, if more PCStats fields are needed, improve it
	if (StatID&EXTRASETTINGS) {
		PCStatsStruct *ps = actor->PCStats;
		if (!ps) {
			//the official invalid value in GetStat
			return 0xdadadada;
		}
		StatID&=15;
		return ps->ExtraSettings[StatID];
	}
	if (Mod) {
		if (core->HasFeature(GF_3ED_RULES) && StatIsASkill(StatID)) {
			return actor->GetSkill(StatID);
		} else {
			if (StatID != IE_HITPOINTS || actor->HasVisibleHP()) {
				return actor->GetStat(StatID);
			} else {
				return 0xdadadada;
			}
		}
	}
	return actor->GetBase( StatID );
}

static int SetCreatureStat(Actor *actor, unsigned int StatID, int StatValue, bool pcf)
{
	// special AC handling
	if (StatID == IE_ARMORCLASS) {
		actor->AC.SetNatural(StatValue);
		return 1;
	} else if (StatID == IE_TOHIT) {
		actor->ToHit.SetBase(StatValue);
		return 1;
	}
	//this is a hack, if more PCStats fields are needed, improve it
	if (StatID&EXTRASETTINGS) {
		PCStatsStruct *ps = actor->PCStats;
		if (!ps) {
			return 0;
		}
		StatID&=15;
		ps->ExtraSettings[StatID] = StatValue;
		actor->ApplyExtraSettings();
		return 1;
	}

	if (pcf) {
		actor->SetBase( StatID, StatValue );
	} else {
		actor->SetBaseNoPCF( StatID, StatValue );
	}
	actor->CreateDerivedStats();
	return 1;
}

PyDoc_STRVAR( GemRB_SetInfoTextColor__doc,
"===== SetInfoTextColor =====\n\
\n\
**Prototype:** GemRB.SetInfoTextColor (red, green, blue[, alpha])\n\
\n\
**Description:**\n\
Sets the color of floating messages in GameControl. Floating messages are\n\
 in-game messages issued by actors, or information text coming from game objects.\n\
\n\
**Parameters:** red, green, blue, alpha - color code, alpha defaults to 255 (completely opaque)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Label_SetTextColor]], [[guiscript:Button_SetTextColor]], [[guiscript:WorldMap_SetTextColor]]"
);

static PyObject* GemRB_SetInfoTextColor(PyObject*, PyObject* args)
{
	int r, g, b, a;

	if (!PyArg_ParseTuple( args, "iii|i", &r, &g, &b, &a)) {
		return AttributeError( GemRB_SetInfoTextColor__doc );
	}
	const Color c = {(ieByte) r,(ieByte) g,(ieByte) b,(ieByte) a};
	core->SetInfoTextColor( c );
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_UnhideGUI__doc,
"===== UnhideGUI =====\n\
\n\
**Prototype:** GemRB.UnhideGUI ()\n\
\n\
**Description:** Shows the Game GUI and redraws windows.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:HideGUI]], [[guiscript:Window_Invalidate]], [[guiscript:Window_SetVisible]]\n\
"
);

static PyObject* GemRB_UnhideGUI(PyObject*, PyObject* /*args*/)
{
	//this is not the usual gc retrieval
	GameControl* gc = core->GetGameControl();
	if (!gc) {
		return RuntimeError("No gamecontrol!");
	}
	gc->SetGUIHidden(false);
	//this enables mouse in dialogs, which is wrong
	//gc->SetCutSceneMode( false );
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_HideGUI__doc,
"===== HideGUI =====\n\
\n\
**Prototype:** GemRB.HideGUI ()\n\
\n\
**Description:**\n\
Hides the game GUI (all windows except the GameControl window). It is also used\n\
 before a major change is made on a GUI window to avoid flickering. After the\n\
 changes, an UnhideGUI() command should be issued too.\n\
\n\
A list of reserved names (variables) and what they hold:\n\
  * MessageWindow - contains a TextArea for ingame messages/dialogue\n\
  * OptionsWindow - a series of buttons for Inventory/Spellbook/Journal,etc\n\
  * PortraitWindow - a series of portrait buttons\n\
  * ActionsWindow - a series of buttons to Attack/Talk, etc.\n\
  * TopWindow - unused (might be removed later)\n\
  * OtherWindow - this window usually covers the GameControl, it is used to display  maps, inventory, journal, etc.\n\
  * FloatWindow - special window which floats on top of the GameControl\n\
\n\
All these windows are associated with a position variable too, these are\n\
 MessagePosition, OptionsPosition, etc.\n\
The position value tells the engine the window's relative position to the\n\
 GameControl GUI. The engine doesn't make any distinction between these\n\
 windows based on their reference name. The differences come from the\n\
position value:\n\
  * -1 - no position (floats over GameControl)\n\
  * 0  - left\n\
  * 1  - bottom\n\
  * 2  - right\n\
  * 3  - top\n\
  * 4  - bottom (cumulative)\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** 1 on success\n\
\n\
**See also:** [[guiscript:UnhideGUI]], [[guiscript:Window_Invalidate]], [[guiscript:Window_SetVisible]]\n\
"
);

static PyObject* GemRB_HideGUI(PyObject*, PyObject* /*args*/)
{
	//it is no problem if the gamecontrol couldn't be found here?
	GameControl* gc = (GameControl *) GetControl( 0, 0, IE_GUI_GAMECONTROL);
	if (!gc) {
		return PyInt_FromLong( 0 );
	}
	int ret = gc->SetGUIHidden(true);

	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_GetGameString__doc,
"===== GetGameString =====\n\
\n\
**Prototype:** GemRB.GetGameString (Index)\n\
\n\
**Description:** Returns a system variable of string type referenced by Index.\n\
\n\
**Parameters:** Index\n\
  * 0 - returns the loading picture's name (MOS resref)\n\
  * 1 - returns the current area's name (ARE resref)\n\
  * 2 - returns the table name for the text screen (2DA resref)\n\
\n\
**Return value:** string - the referenced system variable\n\
\n\
**See also:** [[guiscript:GetSystemVariable]], [[guiscript:GetToken]]"
);

static PyObject* GemRB_GetGameString(PyObject*, PyObject* args)
{
	int Index;

	if (!PyArg_ParseTuple( args, "i", &Index )) {
		return AttributeError( GemRB_GetGameString__doc );
	}
	switch(Index&0xf0) {
	case 0: //game strings
		Game *game = core->GetGame();
		if (!game) {
			return PyString_FromString("");
		}
		switch(Index&15) {
		case 0: // STR_LOADMOS
			return PyString_FromString( game->LoadMos );
		case 1: // STR_AREANAME
			return PyString_FromString( game->CurrentArea );
		case 2: // STR_TEXTSCREEN
			return PyString_FromString( game->TextScreen );
		}
	}

	return AttributeError( GemRB_GetGameString__doc );
}

PyDoc_STRVAR( GemRB_LoadGame__doc,
"===== LoadGame =====\n\
\n\
**Prototype:** GemRB.LoadGame (index[, version])\n\
\n\
**Description:**\n\
Loads a saved game. This must be done before party creation. \n\
You must set the variable called PlayMode before loading a game (see SetVar). \n\
The game won't be loaded before the current GUIScript function returns!\n\
\n\
**Parameters:**\n\
  * index - the saved game's index, -1 means new game.\n\
  * version - optional version to override some buggy default savegame versions\n\
  * PlayMode (variable) - 0 (single player), 1 (tutorial), 2 (multi player)\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    GemRB.SetVar ('PlayMode', 0)\n\
    GemRB.LoadGame (-1, 22)\n\
\n\
**See also:** [[guiscript:EnterGame]], [[guiscript:CreatePlayer]], [[guiscript:SetVar]], [[guiscript:SaveGame]]\n\
"
);

static PyObject* GemRB_LoadGame(PyObject*, PyObject* args)
{
	PyObject *obj;
	int VersionOverride = 0;

	if (!PyArg_ParseTuple( args, "O|i", &obj, &VersionOverride )) {
		return AttributeError( GemRB_LoadGame__doc );
	}
	CObject<SaveGame> save(obj);
	core->SetupLoadGame(save, VersionOverride);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_EnterGame__doc,
"===== EnterGame =====\n\
\n\
**Prototype:** GemRB.EnterGame ()\n\
\n\
**Description:** Starts new game and enters it. \n\
It destroys all existing windows, and creates a GameControl window as the 0th \n\
window (the GameControl object will be the games 0th control). \n\
You should already have loaded a game using LoadGame(), otherwise the engine \n\
may terminate. The game won't be entered until the execution of the current \n\
script ends, but a LoadGame() may precede EnterGame() in the same function\n\
(SetNextScript too).\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:QuitGame]], [[guiscript:LoadGame]], [[guiscript:SetNextScript]]"
);

static PyObject* GemRB_EnterGame(PyObject*, PyObject* /*args*/)
{
	core->QuitFlag|=QF_ENTERGAME;
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_QuitGame__doc,
"===== QuitGame =====\n\
\n\
**Prototype:** GemRB.QuitGame ()\n\
\n\
**Description:** Ends the current game session. \n\
To go back to the main screen, you must call SetNextScript. \n\
Automatically unloads all existing windows and resets the window variables\n\
used by HideGUI().\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:EnterGame]], [[guiscript:Quit]], [[guiscript:SetNextScript]], [[guiscript:HideGUI]]\n\
"
);
static PyObject* GemRB_QuitGame(PyObject*, PyObject* /*args*/)
{
	core->QuitFlag=QF_QUITGAME;
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_TextArea_SetChapterText__doc,
"===== TextArea_SetChapterText =====\n\
\n\
**Prototype:** GemRB.SetChapterText (Win, Ctrl, Text)\n\
\n\
**Metaclass Prototype:** SetChapterText (Text)\n\
\n\
**Description:**\n\
Sets up a TextArea for scrolling the chapter text from below the TextArea \n\
to beyond the top.\n\
\n\
**Parameters:** \n\
  * Win - window id\n\
  * Ctrl - textarea id\n\
  * Text - The text to set in the TA\n\
\n\
**Example:**\n\
  TextArea = ChapterWindow.GetControl (5)\n\
  TextArea.SetChapterText (text)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:**"
);

static PyObject* GemRB_TextArea_SetChapterText(PyObject * /*self*/, PyObject* args)
{
	int Win, Ctrl;
	char* text;

	if (!PyArg_ParseTuple( args, "iis", &Win, &Ctrl, &text)) {
		return AttributeError( GemRB_TextArea_SetChapterText__doc );
	}

	TextArea* ta = ( TextArea* ) GetControl( Win, Ctrl, IE_GUI_TEXTAREA);
	if (!ta) {
		return NULL;
	}

	ta->ClearText();
	// insert enough newlines to push the text offscreen
	int rowHeight = ta->GetRowHeight();
	size_t lines = ta->Height / rowHeight;
	ta->AppendText(String(lines, L'\n'));
	String* chapText = StringFromCString(text);
	ta->AppendText(*chapText);
	// add instead of set, because we also scroll to be out of sight so keep the newline count
	lines += ta->RowCount();
	delete chapText;
	// animate the scroll: duration = 2500 * lines of text
	ta->ScrollToY((int)(rowHeight * lines), NULL, lines * 2500);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_StatComment__doc,
"===== StatComment =====\n\
\n\
**Prototype:** GemRB.StatComment (Strref, X, Y)\n\
\n\
**Description:**\n\
Replaces %%d's with the values of X and Y in a string referenced by strref.\n\
\n\
PST uses %%d values in place of tokens, thus it requires a special command. \n\
In other engines use GetString after setting the needed tokens with \n\
SetToken (if you need to set them at all).\n\
\n\
**Parameters:**\n\
  * Strref - a string reference from the dialog.tlk table.\n\
  * X, Y   - two numerical values to be replaced in place of %%d's.\n\
\n\
**Return value:** A string with resolved %%d's.\n\
\n\
**Example:**\n\
def IntPress():\n\
    global Int, StatTable, TextArea\n\
    TextArea.SetText (18488)\n\
    intComment = StatTable.GetValue (Int, 1)\n\
    TextArea.Append (GemRB.StatComment (intComment, 0, 0))\n\
\n\
The above example comes directly from our PST script, it will display the \n\
description of the intelligence stat (strref==18488), adding a comment \n\
based on the current Int variable. StatTable (a 2da table) contains the \n\
comment strref values associated with an intelligence value.\n\
\n\
**See also:** [[guiscript:GetString]], [[guiscript:SetToken]],\n\
[[guiscript:Table_GetValue]], [[guiscript:Control_SetText]],[[guiscript:LoadTable]],\n\
[[guiscript:TextArea_Append]]\n\
"
);

static PyObject* GemRB_StatComment(PyObject * /*self*/, PyObject* args)
{
	ieStrRef Strref;
	int X, Y;
	PyObject* ret;

	if (!PyArg_ParseTuple( args, "iii", &Strref, &X, &Y )) {
		return AttributeError( GemRB_StatComment__doc );
	}
	char* text = core->GetCString( Strref );
	size_t bufflen = strlen( text ) + 12;
	if (bufflen<12) {
		return AttributeError( GemRB_StatComment__doc );
	}
	char* newtext = ( char* ) malloc( bufflen );
	//this could be DANGEROUS, not anymore (snprintf is your friend)
	snprintf( newtext, bufflen, text, X, Y );
	core->FreeString( text );
	ret = PyString_FromString( newtext );
	free( newtext );
	return ret;
}

PyDoc_STRVAR( GemRB_GetString__doc,
"===== GetString =====\n\
\n\
**Prototype:** GemRB.GetString (Strref[, Flags])\n\
\n\
**Description:** Returns string for given strref. Usually, you don't need to \n\
resolve a string before use, as you can use SetText with a strref parameter. \n\
This command lets you alter the string. For example, if you want to add a \n\
level value without a token, you'll need this.\n\
\n\
**Parameters:** \n\
  * Strref - a string reference from the dialog.tlk table\n\
  * Flags - a bitfield:\n\
    * 1   - display strrefs on\n\
    * 2   - play attached sound\n\
    * 4   - speech (stop previous sound)\n\
    * 256 - strref off (overrides cfg)\n\
\n\
**Return value:** A string with resolved tokens. To resolve %d's, you must\n\
either use StatComment or do it manually.\n\
\n\
**Example:**\n\
   Level = GemRB.GetPlayerStat (pc, IE_LEVEL) # 1 at character generation\n\
   Label.SetText (GemRB.GetString(12137) + str(Level)) \n\
The above example will display 'Level: 1' in the addressed label.\n\
\n\
**See also:** [[guiscript:StatComment]], [[guiscript:Control_SetText]]\n\
"
);

static PyObject* GemRB_GetString(PyObject * /*self*/, PyObject* args)
{
	ieStrRef strref;
	int flags = 0;
	PyObject* ret;

	if (!PyArg_ParseTuple( args, "i|i", &strref, &flags )) {
		return AttributeError( GemRB_GetString__doc );
	}

	char *text = core->GetCString( strref, flags );
	ret=PyString_FromString( text );
	core->FreeString( text );
	return ret;
}

PyDoc_STRVAR( GemRB_EndCutSceneMode__doc,
"===== EndCutSceneMode =====\n\
\n\
**Prototype:** EndCutSceneMode ()\n\
\n\
**Description:** Exits the CutScene Mode. It is similar to the gamescript \n\
command of the same name. It gives back the cursor and shows the game GUI \n\
windows hidden by the CutSceneMode() gamescript action. \n\
This is mainly a debugging command.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:HideGUI]], [[guiscript:UnhideGUI]]\n\
"
);

static PyObject* GemRB_EndCutSceneMode(PyObject * /*self*/, PyObject* /*args*/)
{
	core->SetCutSceneMode( false );
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_ReassignControls__doc,
"===== Window_ReassignControls =====\n\
\n\
**Prototype:** ReassignControls (windowIndex, origIDtuple, newIDtuple)\n\
\n\
**Metaclass Prototype:** ReassignControls (origIDtuple, newIDtuple)\n\
\n\
**Description:** Mass-reassigns control IDs for each element in both \n\
parameters. The two tuples need to be of same length.\n\
\n\
**Parameters:** WindowIndex - the index returned by LoadWindow()\n\
  * origIDtuple, newIDtuple - python tuples of control IDs\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_Window_ReassignControls(PyObject * /*self*/, PyObject* args)
{
	PyObject *origIDtuple = NULL;
	PyObject *newIDtuple = NULL;
	int win = 0;

	if (!PyArg_ParseTuple(args, "iOO", &win, &origIDtuple, &newIDtuple)) {
		Log(ERROR, "GUIScript", "ReassignControls: param parsing");
		return AttributeError(GemRB_Window_ReassignControls__doc);
	}
	if (!PyObject_TypeCheck(origIDtuple, &PyTuple_Type)) {
		Log(ERROR, "GUIScript", "ReassignControls: first tuple type");
		return AttributeError(GemRB_Window_ReassignControls__doc);
	}
	if (!PyObject_TypeCheck(newIDtuple, &PyTuple_Type)) {
		Log(ERROR, "GUIScript", "ReassignControls: second tuple type");
		return AttributeError(GemRB_Window_ReassignControls__doc);
	}
	int length = PyTuple_Size(origIDtuple);
	if (length != PyTuple_Size(newIDtuple)) {
		Log(ERROR, "GUIScript", "ReassignControls: tuple size mismatch");
		return AttributeError(GemRB_Window_ReassignControls__doc);
	}

	for (int i=0;i < length; i++) {
		PyObject *poid = PyTuple_GetItem(origIDtuple, i);
		PyObject *pnid = PyTuple_GetItem(newIDtuple, i);

		if (!PyObject_TypeCheck(poid, &PyInt_Type)) {
			Log(ERROR, "GUIScript", "ReassignControls: tuple1 member %d not int", i);
			return AttributeError(GemRB_Window_ReassignControls__doc);
		}
		if (!PyObject_TypeCheck(pnid, &PyInt_Type)) {
			Log(ERROR, "GUIScript", "ReassignControls: tuple2 member %d not int", i);
			return AttributeError(GemRB_Window_ReassignControls__doc);
		}
		int oid = PyInt_AsLong(poid);
		int nid = PyInt_AsLong(pnid);

		// child control
		int ctrlindex = GetControlIndex(win, oid);
		if (ctrlindex == -1) {
			Log(ERROR, "GUIScript", "ReassignControls: Control (ID: %d) was not found!", oid);
			return RuntimeError("Control was not found!");
		}
		Control *ctrl = GetControl(win, ctrlindex, -1);
		if (!ctrl) {
			Log(ERROR, "GUIScript", "ReassignControls: Control (ID: %d) was not found!", oid);
			return RuntimeError("Control was not found!");
		}

		// finally it's time for reassignment
		ctrl->SetControlID(nid);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_LoadWindowPack__doc,
"===== LoadWindowPack =====\n\
\n\
**Prototype:** GemRB.LoadWindowPack (CHUIResRef[, Width=0, Height=0])\n\
\n\
**Description:** Loads a WindowPack into the Window Manager Module. \n\
Only one windowpack may be open at a time, but once a window was selected \n\
by LoadWindow, you can get a new windowpack.\n\
\n\
**Parameters:** \n\
  * CHUIResRef: the name of the GUI set (.CHU resref)\n\
  * Width, Height: if nonzero, they set the natural screen size for which \n\
    the windows in the winpack are positioned. LoadWindow() uses this \n\
    information to automatically reposition loaded windows.\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    LoadWindowPack ('START', 640, 480)\n\
\n\
**See also:** [[guiscript:LoadWindow]], [[guiscript:LoadWindowFrame]]\n\
"
);

static PyObject* GemRB_LoadWindowPack(PyObject * /*self*/, PyObject* args)
{
	char* string;
	int width = 0, height = 0;

	if (!PyArg_ParseTuple( args, "s|ii", &string, &width, &height )) {
		return AttributeError( GemRB_LoadWindowPack__doc );
	}

	if (!core->LoadWindowPack( string )) {
		return RuntimeError("Can't find resource");
	}

	CHUWidth = width;
	CHUHeight = height;

	if ( (width && (width>core->Width)) ||
			(height && (height>core->Height)) ) {
		Log(ERROR, "GUIScript", "Screen is too small! This window requires %d x %d resolution.",
			width, height);
		return RuntimeError("Please change your settings.");
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_LoadWindow__doc,
"===== LoadWindow =====\n\
\n\
**Prototype:** GemRB.LoadWindow (WindowID)\n\
\n\
**Description:** Returns a Window. You must call LoadWindowPack before using \n\
this command. The window won't be displayed. If LoadWindowPack() set nonzero \n\
natural screen size with Width and Height parameters, the loaded window is \n\
then moved by (screen size - winpack size) / 2\n\
\n\
**Parameters:** a window ID, see the .chu file specification\n\
\n\
**Return value:** GWindow (index)\n\
\n\
**See also:** [[guiscript:LoadWindowPack]], [[guiscript:Window_GetControl]], [[guiscript:Window_SetVisible]], [[guiscript:Window_ShowModal]], [[guiscript:accessing_gui_controls]]"
);

static PyObject* GemRB_LoadWindow(PyObject * /*self*/, PyObject* args)
{
	int WindowID;

	if (!PyArg_ParseTuple( args, "i", &WindowID )) {
		return AttributeError( GemRB_LoadWindow__doc );
	}

	int ret = core->LoadWindow( WindowID );
	if (ret == -1) {
		char buf[256];
		snprintf( buf, sizeof( buf ), "Can't find window #%d!", WindowID );
		return RuntimeError(buf);
	}

	// If the current winpack windows are placed for screen resolution
	// other than the current one, reposition them
	Window* win = core->GetWindow( ret );
	if (CHUWidth && CHUWidth != core->Width)
		win->XPos += (core->Width - CHUWidth) / 2;
	if (CHUHeight && CHUHeight != core->Height)
		win->YPos += (core->Height - CHUHeight) / 2;

	return gs->ConstructObject("Window", ret);
}

PyDoc_STRVAR( GemRB_Window_SetSize__doc,
"===== Window_SetSize =====\n\
\n\
**Prototype:** GemRB.SetWindowSize (WindowIndex, Width, Height)\n\
\n\
**Metaclass Prototype:** SetSize (Width, Height)\n\
\n\
**Description:** Resizes a Window.\n\
\n\
**Parameters:**\n\
  * WindowIndex   - the index returned by LoadWindow()\n\
  * Width, Height - the new dimensions of the window\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadWindow]], [[guiscript:Window_SetPos]], [[guiscript:Control_SetSize]]"
);

static PyObject* GemRB_Window_SetSize(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, Width, Height;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &Width, &Height )) {
		return AttributeError( GemRB_Window_SetSize__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!\n");
	}

	win->Width = Width;
	win->Height = Height;
	win->Invalidate();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_SetFrame__doc,
"===== Window_SetFrame =====\n\
\n\
**Prototype:** GemRB.SetWindowFrame (WindowIndex)\n\
\n\
**Metaclass Prototype:** SetFrame ()\n\
\n\
**Description:** Sets Window to have a frame used to fill screen on higher \n\
resolutions. At present all windows having frames have the same one.\n\
\n\
To automatically move the windows from the edges and to let GemRB know \n\
how much space is there, the LoadWindowPack() function should be called \n\
with size parameters.\n\
\n\
Make sure to set the frame only for the window on the bottom, because the \n\
frames will erase the whole screen before drawing.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the index returned by LoadWindow()\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadWindowFrame]], [[guiscript:LoadWindowPack]], [[guiscript:LoadWindow]], [[guiscript:Window_SetPos]]"
);

static PyObject* GemRB_Window_SetFrame(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;

	if (!PyArg_ParseTuple( args, "i", &WindowIndex )) {
		return AttributeError( GemRB_Window_SetFrame__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!\n");
	}

	win->SetFrame();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_LoadWindowFrame__doc,
"===== LoadWindowFrame =====\n\
\n\
**Prototype:** GemRB.LoadWindowFrame (MOSResRef_Left, MOSResRef_Right, MOSResRef_Top, MOSResRef_Bottom))\n\
\n\
**Description:** Load the parts of window frame used to decorate windows \n\
on higher resolutions.\n\
\n\
**Parameters:**\n\
  * MOSResRef_Left, MOSResRef_Right,\n\
  * MOSResRef_Top, MOSResRef_Bottom: names of MOS files with frame parts\n\
\n\
**Return value:** N/A\n\
\n\
**Example:** (from bg2's Start.py)\n\
  # Find proper window border for higher resolutions\n\
  screen_width = GemRB.GetSystemVariable (SV_WIDTH)\n\
  screen_height = GemRB.GetSystemVariable (SV_HEIGHT)\n\
  if screen_width == 800:\n\
    GemRB.LoadWindowFrame ('STON08L', 'STON08R', 'STON08T', 'STON08B')\n\
  elif screen_width == 1024:\n\
    GemRB.LoadWindowFrame ('STON10L', 'STON10R', 'STON10T', 'STON10B')\n\
\n\
  # Windows in this winpack were originally made and placed \n\
  # for 640x480 screen size\n\
  GemRB.LoadWindowPack ('START', 640, 480)\n\
  StartWindow = GemRB.LoadWindow (0)\n\
  GemRB.SetWindowFrame (StartWindow)\n\
\n\
**See also:** [[guiscript:Window_SetFrame]], [[guiscript:LoadWindowPack]]"
);

static PyObject* GemRB_LoadWindowFrame(PyObject * /*self*/, PyObject* args)
{
	char* ResRef[4];

	if (!PyArg_ParseTuple( args, "ssss", &ResRef[0], &ResRef[1], &ResRef[2], &ResRef[3] )) {
		return AttributeError( GemRB_LoadWindowFrame__doc );
	}


	for (int i = 0; i < 4; i++) {
		if (ResRef[i] == 0) {
			return AttributeError( GemRB_LoadWindowFrame__doc );
		}

		ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(ResRef[i]);
		if (im == nullptr) {
			return NULL;
		}

		Sprite2D* Picture = im->GetSprite2D();
		if (Picture == NULL) {
			return NULL;
		}

		core->SetWindowFrame(i, Picture);
	}

	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_EnableCheatKeys__doc,
"===== EnableCheatKeys =====\n\
\n\
**Prototype:** GemRB.EnableCheatKeys (flag)\n\
\n\
**Description:** Turns the debug keys on or off. \n\
They are currently turned on by default.\n\
\n\
**Parameters:** flag - boolean\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** GameControl.cpp for actual cheat key functions"
);

static PyObject* GemRB_EnableCheatKeys(PyObject * /*self*/, PyObject* args)
{
	int Flag;

	if (!PyArg_ParseTuple( args, "i", &Flag )) {
		return AttributeError( GemRB_EnableCheatKeys__doc );
	}

	core->EnableCheatKeys( Flag );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_SetPicture__doc,
"===== Window_SetPicture =====\n\
\n\
**Prototype:** GemRB.SetWindowPicture (WindowIndex, MosResRef)\n\
\n\
**Metaclass Prototype:** SetPicture (MosResRef)\n\
\n\
**Description:** Changes the background of a Window.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the index returned by LoadWindow()\n\
  * MosResRef   - the name of the background image (.mos resref)\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    LoadPic = GemRB.GetGameString (STR_LOADMOS)\n\
    if LoadPic=='':\n\
        LoadPic = 'GUILS0' + str(GemRB.Roll (1,9,0))\n\
    GemRB.SetWindowPicture (LoadScreen, LoadPic)\n\
The above snippet is responsible to generate a random loading screen.\n\
\n\
**See also:** [[guiscript:Button_SetPicture]]"
);

static PyObject* GemRB_Window_SetPicture(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;
	char* MosResRef;

	if (!PyArg_ParseTuple( args, "is", &WindowIndex, &MosResRef )) {
		return AttributeError( GemRB_Window_SetPicture__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!\n");
	}

	ResourceHolder<ImageMgr> mos = GetResourceHolder<ImageMgr>(MosResRef);
	if (mos != nullptr) {
		win->SetBackGround( mos->GetSprite2D(), true );
	}
	win->Invalidate();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_SetPos__doc,
"===== Window_SetPos =====\n\
\n\
**Prototype:** GemRB.SetWindowPos (WindowIndex, X, Y, [Flags=WINDOW_TOPLEFT])\n\
\n\
**Metaclass Prototype:** SetPos (X, Y, [Flags=WINDOW_TOPLEFT])\n\
\n\
**Description:** Moves a Window.\n\
\n\
**Parameters:** \n\
  * WindowIndex - the index returned by LoadWindow()\n\
  * X,Y - placement of the window, see Flags below for meaning for each flag\n\
  * Flags - bitmask of WINDOW_TOPLEFT (etc.), used to modify the meaning of X and Y.\n\
    * TOPLEFT  : X, Y are coordinates of upper-left corner.\n\
    * CENTER   : X, Y are coordinates of window's center.\n\
    * ABSCENTER: window is placed at screen center, moved by X, Y.\n\
    * RELATIVE : window is moved by X, Y.\n\
    * SCALE    : window is moved by diff of screen size and X, Y, divided by 2.\n\
    * BOUNDED  : the window is kept within screen boundaries.\n\
\n\
**Note:** All flags except WINDOW_BOUNDED are mutually exclusive\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    x, y = GemRB.GetVar ('MenuX'), GemRB.GetVar ('MenuY')\n\
    GemRB.SetWindowPos (Window, x, y, WINDOW_CENTER | WINDOW_BOUNDED)\n\
\n\
The above example is from the PST FloatMenuWindow script. It centers \n\
pie-menu window around the mouse cursor, but keeps it within screen.\n\
\n\
**See also:** [[guiscript:Window_SetSize]], [[guiscript:Control_SetPos]]"
);

static PyObject* GemRB_Window_SetPos(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, X, Y;
	int Flags = WINDOW_TOPLEFT;

	if (!PyArg_ParseTuple( args, "iii|i", &WindowIndex, &X, &Y, &Flags )) {
		return AttributeError( GemRB_Window_SetPos__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!\n");
	}

	if (Flags & WINDOW_CENTER) {
		X -= win->Width / 2;
		Y -= win->Height / 2;
	}
	else if (Flags & WINDOW_ABSCENTER) {
		X += (core->Width - win->Width) / 2;
		Y += (core->Height - win->Height) / 2;
	}
	else if (Flags & WINDOW_RELATIVE) {
		X += win->XPos;
		Y += win->YPos;
	}
	else if (Flags & WINDOW_SCALE) {
		X = win->XPos + (core->Width - X) / 2;
		Y = win->YPos + (core->Height - Y) / 2;
	}

	// Keep window within screen
	// FIXME: keep it within gamecontrol
	if (Flags & WINDOW_BOUNDED) {
		if ((ieWordSigned) X < 0)
			X = 0;
		if ((ieWordSigned) Y < 0)
			Y = 0;

		if (X + win->Width >= core->Width)
			X = core->Width - win->Width;
		if (Y + win->Height >= core->Height)
			Y = core->Height - win->Height;
	}

	win->XPos = X;
	win->YPos = Y;
	core->RedrawAll();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_GetRect__doc,
"===== Window_GetRect =====\n\
\n\
**Prototype:** GemRB.GetRect (WindowIndex)\n\
\n\
**Metaclass Prototype:** GetRect ()\n\
\n\
**Description:** Returns a dict with the window position and size.\n\
\n\
**Parameters:** WindowIndex is the index returned by LoadWindow()\n\
\n\
**Return value:** dict\n\
\n\
**See also:** [[guiscript:LoadWindow]]"
);

static PyObject* GemRB_Window_GetRect(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;

	if (!PyArg_ParseTuple( args, "i", &WindowIndex)) {
		return AttributeError( GemRB_Window_GetRect__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!\n");
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "X", PyInt_FromLong( win->XPos ));
	PyDict_SetItemString(dict, "Y", PyInt_FromLong( win->YPos ));
	PyDict_SetItemString(dict, "Width", PyInt_FromLong( win->Width ));
	PyDict_SetItemString(dict, "Height", PyInt_FromLong( win->Height ));
	return dict;
}

PyDoc_STRVAR( GemRB_LoadTable__doc,
"===== LoadTable =====\n\
\n\
**Prototype:** GemRB.LoadTable (2DAResRef[, ignore_error=0])\n\
\n\
**Description:** Loads a 2DA Table. In case it was already loaded, it \n\
will return the table's existing reference (won't load it again).\n\
\n\
**Parameters:** \n\
  * 2DAResRef    - the table's name (.2da resref)\n\
  * ignore_error - boolean, if set, handle missing files gracefully\n\
\n\
**Return value:** GTable\n\
\n\
**See also:** [[guiscript:UnloadTable]], [[guiscript:LoadSymbol]]"
);

static PyObject* GemRB_LoadTable(PyObject * /*self*/, PyObject* args)
{
	char* tablename;
	int noerror = 0;

	if (!PyArg_ParseTuple( args, "s|i", &tablename, &noerror )) {
		return AttributeError( GemRB_LoadTable__doc );
	}

	int ind = gamedata->LoadTable( tablename );
	if (ind == -1) {
		if (noerror) {
			Py_RETURN_NONE;
		} else {
			return RuntimeError("Can't find resource");
		}
	}
	return gs->ConstructObject("Table", ind);
}

PyDoc_STRVAR( GemRB_Table_Unload__doc,
"===== UnloadTable =====\n\
\n\
**Prototype:** GemRB.UnloadTable (TableIndex)\n\
\n\
**Metaclass Prototype:** Unload ()\n\
\n\
**Description:** Unloads a 2DA Table.\n\
\n\
**Parameters:**\n\
  * TableIndex - returned by a previous LoadTable command.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadTable]]"
);

static PyObject* GemRB_Table_Unload(PyObject * /*self*/, PyObject* args)
{
	int ti;

	if (!PyArg_ParseTuple( args, "i", &ti )) {
		return AttributeError( GemRB_Table_Unload__doc );
	}

	int ind = gamedata->DelTable( ti );
	if (ind == -1) {
		return RuntimeError("Can't find resource");
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Table_GetValue__doc,
"===== Table_GetValue =====\n\
\n\
**Prototype:** GemRB.GetTableValue (TableIndex, RowIndex/RowString, ColIndex/ColString, Type)\n\
\n\
**Metaclass Prototype:** GetValue (RowIndex/RowString, ColIndex/ColString[, Type])\n\
\n\
**Description:** Returns a field of a 2DA Table. The row and column indices \n\
must be of same type (either string or numeric), the return value will be \n\
of the same type, unless Type is specified and different.\n\
\n\
**Parameters:**\n\
  * TableIndex - returned by a previous LoadTable command.\n\
  * RowIndex, ColIndex - numeric row/column indices\n\
  * RowString, ColString - the row/column names as written in the 2da file\n\
  * Type - forces a specific return type (GUIDefines.py)\n\
    * -1 - default\n\
    * GTV_STR 0 - string\n\
    * GTV_INT 1 - int\n\
    * GTV_STAT 2 - stat symbol (translated to numeric - value of stat)\n\
    * GTV_REF 3 - string reference (expanded to string)\n\
\n\
**Return value:** numeric or string, based on the indices or type\n\
\n\
**See also:** [[guiscript:GetSymbolValue]], [[guiscript:Table_FindValue]], [[guiscript:LoadTable]]"
);

static PyObject* GemRB_Table_GetValue(PyObject * /*self*/, PyObject* args)
{
	PyObject* ti, * row, * col;
	PyObject* type = NULL;
	int which = -1;

	if (!PyArg_UnpackTuple( args, "ref", 3, 4, &ti, &row, &col, &type )) {
		return AttributeError( GemRB_Table_GetValue__doc );
	}
	if (type!=NULL) {
		if (!PyObject_TypeCheck( type, &PyInt_Type )) {
			return AttributeError( GemRB_Table_GetValue__doc );
		}
		which = PyInt_AsLong( type );
	}

	if (!PyObject_TypeCheck( ti, &PyInt_Type )) {
		return AttributeError( GemRB_Table_GetValue__doc );
	}
	long TableIndex = PyInt_AsLong( ti );
	if (( !PyObject_TypeCheck( row, &PyInt_Type ) ) &&
		( !PyObject_TypeCheck( row, &PyString_Type ) )) {
		return AttributeError( GemRB_Table_GetValue__doc );
	}
	if (( !PyObject_TypeCheck( col, &PyInt_Type ) ) &&
		( !PyObject_TypeCheck( col, &PyString_Type ) )) {
		return AttributeError( GemRB_Table_GetValue__doc );
	}
	if (PyObject_TypeCheck( row, &PyInt_Type ) &&
		( !PyObject_TypeCheck( col, &PyInt_Type ) )) {
		Log(ERROR, "GUIScript", "Type Error: RowIndex/RowString and ColIndex/ColString must be the same type");
		return NULL;
	}
	if (PyObject_TypeCheck( row, &PyString_Type ) &&
		( !PyObject_TypeCheck( col, &PyString_Type ) )) {
		Log(ERROR, "GUIScript", "Type Error: RowIndex/RowString and ColIndex/ColString must be the same type");
		return NULL;
	}
	Holder<TableMgr> tm = gamedata->GetTable( TableIndex );
	if (!tm) {
		return RuntimeError("Can't find resource");
	}
	const char* ret;
	if (PyObject_TypeCheck( row, &PyString_Type )) {
		char* rows = PyString_AsString( row );
		char* cols = PyString_AsString( col );
		ret = tm->QueryField( rows, cols );
	} else {
		long rowi = PyInt_AsLong( row );
		long coli = PyInt_AsLong( col );
		ret = tm->QueryField( rowi, coli );
	}
	if (ret == NULL)
		return NULL;

	long val;
	//if which = 0, then return string
	if (!which) {
		return PyString_FromString( ret );
	}
	//if which = 3 then return resolved string
	bool valid = valid_number(ret, val);
	if (which == 3) {
		return PyString_FromString(core->GetCString(val));
	}
	//if which = 1 then return number
	//if which = -1 (omitted) then return the best format
	if (valid || (which == 1)) {
		return PyInt_FromLong( val );
	}
	if (which==2) {
		val = core->TranslateStat(ret);
		return PyInt_FromLong( val );
	}
	return PyString_FromString( ret );
}

PyDoc_STRVAR( GemRB_Table_FindValue__doc,
"===== Table_FindValue =====\n\
\n\
**Prototype:** GemRB.FindTableValue (TableIndex, ColumnIndex, Value[, StartRow])\n\
\n\
**Metaclass Prototype:** FindValue (ColumnIndex, Value[, StartRow])\n\
\n\
**Description:** Returns the first row index of a field value in a 2DA \n\
Table. If StartRowis omitted, the search starts from the beginning.\n\
\n\
**Parameters:**\n\
  * TableIndex - integer, returned by a previous LoadTable command.\n\
  * Column - index or name of the column in which to look for value.\n\
  * Value - value to find in the table\n\
  * StartRow - integer, starting row (offset)\n\
\n\
**Return value:** numeric, -1 if the value isn't to be found\n\
\n\
**See also:** [[guiscript:LoadTable]], [[guiscript:Table_GetValue]]"
);

static PyObject* GemRB_Table_FindValue(PyObject * /*self*/, PyObject* args)
{
	int ti, col;
	int start = 0;
	long Value;
	char* colname = NULL;
	char* strvalue = NULL;

	if (!PyArg_ParseTuple( args, "iil|i", &ti, &col, &Value, &start )) {
		PyErr_Clear(); //clearing the exception
		col = -1;
		if (!PyArg_ParseTuple( args, "isl|i", &ti, &colname, &Value, &start )) {
			PyErr_Clear(); //clearing the exception
			col = -2;
			if (!PyArg_ParseTuple( args, "iss|i", &ti, &colname, &strvalue, &start )) {
				return AttributeError( GemRB_Table_FindValue__doc );
			}
		}
	}

	Holder<TableMgr> tm = gamedata->GetTable( ti );
	if (tm == nullptr) {
		return RuntimeError("Can't find resource");
	}
	if (col == -1) {
		return PyInt_FromLong(tm->FindTableValue(colname, Value, start));
	} else if (col == -2) {
		return PyInt_FromLong(tm->FindTableValue(colname, strvalue, start));
	} else {
		return PyInt_FromLong(tm->FindTableValue(col, Value, start));
	}
}

PyDoc_STRVAR( GemRB_Table_GetRowIndex__doc,
"===== Table_GetRowIndex =====\n\
\n\
**Prototype:** GemRB.GetTableRowIndex (TableIndex, RowName)\n\
\n\
**Metaclass Prototype:** GetRowIndex (RowName)\n\
\n\
**Description:** Returns the index of a row in a 2DA Table.\n\
\n\
**Parameters:**\n\
  * TableIndex - returned by a previous LoadTable command.\n\
  * RowName - a row label\n\
\n\
**Return value:** numeric, -1 if row doesn't exist\n\
\n\
**See also:** [[guiscript:LoadTable]]"
);

static PyObject* GemRB_Table_GetRowIndex(PyObject * /*self*/, PyObject* args)
{
	int ti;
	char* rowname;

	if (!PyArg_ParseTuple( args, "is", &ti, &rowname )) {
		return AttributeError( GemRB_Table_GetRowIndex__doc );
	}

	Holder<TableMgr> tm = gamedata->GetTable( ti );
	if (tm == nullptr) {
		return RuntimeError("Can't find resource");
	}
	int row = tm->GetRowIndex( rowname );
	//no error if the row doesn't exist
	return PyInt_FromLong( row );
}

PyDoc_STRVAR( GemRB_Table_GetRowName__doc,
"===== Table_GetRowName =====\n\
\n\
**Prototype:** GemRB.GetTableRowName (TableIndex, RowIndex)\n\
\n\
**Metaclass Prototype:** GetRowName (RowIndex)\n\
\n\
**Description:** Returns the name of a Row in a 2DA Table.\n\
\n\
**Parameters:**\n\
  * TableIndex - returned by a previous LoadTable command.\n\
  * RowIndex - the numeric index of the row.\n\
\n\
**Return value:** string\n\
\n\
**See also:** [[guiscript:LoadTable]], [[guiscript:Table_GetColumnName]]"
);

static PyObject* GemRB_Table_GetRowName(PyObject * /*self*/, PyObject* args)
{
	int ti, row;

	if (!PyArg_ParseTuple( args, "ii", &ti, &row )) {
		return AttributeError( GemRB_Table_GetRowName__doc );
	}

	Holder<TableMgr> tm = gamedata->GetTable( ti );
	if (tm == nullptr) {
		return RuntimeError("Can't find resource");
	}
	const char* str = tm->GetRowName( row );
	if (str == NULL) {
		return NULL;
	}

	return PyString_FromString( str );
}

PyDoc_STRVAR( GemRB_Table_GetColumnIndex__doc,
"===== Table_GetColumnIndex =====\n\
\n\
**Prototype:** GemRB.GetTableColumnIndex (TableIndex, ColumnName)\n\
\n\
**Metaclass Prototype:** GetColumnIndex (ColumnName)\n\
\n\
**Description:** Returns the index of a column in a 2DA Table.\n\
\n\
**Parameters:**\n\
  * TableIndex - returned by a previous LoadTable command.\n\
  * ColumnName - a column label\n\
\n\
**Return value:** numeric, -1 if column doesn't exist\n\
\n\
**See also:** [[guiscript:LoadTable]], [[guiscript:Table_GetRowIndex]]"
);

static PyObject* GemRB_Table_GetColumnIndex(PyObject * /*self*/, PyObject* args)
{
	int ti;
	char* colname;

	if (!PyArg_ParseTuple( args, "is", &ti, &colname )) {
		return AttributeError( GemRB_Table_GetColumnIndex__doc );
	}

	Holder<TableMgr> tm = gamedata->GetTable( ti );
	if (tm == nullptr) {
		return RuntimeError("Can't find resource");
	}
	int col = tm->GetColumnIndex( colname );
	//no error if the column doesn't exist
	return PyInt_FromLong( col );
}

PyDoc_STRVAR( GemRB_Table_GetColumnName__doc,
"===== Table_GetColumnName =====\n\
\n\
**Prototype:** GemRB.GetTableColumnName (TableIndex, ColumnIndex)\n\
\n\
**Metaclass Prototype:** GetColumnName (ColumnIndex)\n\
\n\
**Description:** Returns the name of a Column in a 2DA Table.\n\
\n\
**Parameters:**\n\
  * TableIndex - returned by a previous LoadTable command.\n\
  * ColumnIndex - the numeric index of the column.\n\
\n\
**Return value:** string\n\
\n\
**See also:** [[guiscript:LoadTable]], [[guiscript:Table_GetRowName]]"
);

static PyObject* GemRB_Table_GetColumnName(PyObject * /*self*/, PyObject* args)
{
	int ti, col;

	if (!PyArg_ParseTuple( args, "ii", &ti, &col )) {
		return AttributeError( GemRB_Table_GetColumnName__doc );
	}

	Holder<TableMgr> tm = gamedata->GetTable( ti );
	if (tm == nullptr) {
		return RuntimeError("Can't find resource");
	}
	const char* str = tm->GetColumnName( col );
	if (str == NULL) {
		return NULL;
	}

	return PyString_FromString( str );
}

PyDoc_STRVAR( GemRB_Table_GetRowCount__doc,
"===== Table_GetRowCount =====\n\
\n\
**Prototype:** GemRB.GetTableRowCount (TableIndex)\n\
\n\
**Metaclass Prototype:** GetRowCount ()\n\
\n\
**Description:** Returns the number of rows in a 2DA Table.\n\
\n\
**Parameters:** TableIndex - returned by a previous LoadTable command.\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:LoadTable]], [[guiscript:Table_GetColumnCount]]"
);

static PyObject* GemRB_Table_GetRowCount(PyObject * /*self*/, PyObject* args)
{
	int ti;

	if (!PyArg_ParseTuple( args, "i", &ti )) {
		return AttributeError( GemRB_Table_GetRowCount__doc );
	}

	Holder<TableMgr> tm = gamedata->GetTable( ti );
	if (tm == nullptr) {
		return RuntimeError("Can't find resource");
	}

	return PyInt_FromLong( tm->GetRowCount() );
}

PyDoc_STRVAR( GemRB_Table_GetColumnCount__doc,
"===== Table_GetColumnCount =====\n\
\n\
**Prototype:** GemRB.GetTableColumnCount (TableIndex[, Row])\n\
\n\
**Metaclass Prototype:** GetColumnCount ([Row])\n\
\n\
**Description:** Returns the column count of the specified row in a 2DA Table.\n\
\n\
**Parameters:**\n\
  * TableIndex - returned by a previous LoadTable command.\n\
  * Row        - the row of the table, if omitted, defaults to 0\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:LoadTable]], [[guiscript:Table_GetRowCount]]"
);

static PyObject* GemRB_Table_GetColumnCount(PyObject * /*self*/, PyObject* args)
{
	int ti;
	int row = 0;

	if (!PyArg_ParseTuple( args, "i|i", &ti, &row )) {
		return AttributeError( GemRB_Table_GetColumnCount__doc );
	}

	Holder<TableMgr> tm = gamedata->GetTable( ti );
	if (tm == nullptr) {
		return RuntimeError("Can't find resource");
	}

	return PyInt_FromLong( tm->GetColumnCount(row) );
}

PyDoc_STRVAR( GemRB_LoadSymbol__doc,
"===== LoadSymbol =====\n\
\n\
**Prototype:** GemRB.LoadSymbol (IDSResRef)\n\
\n\
**Description:** Loads a IDS Symbol List. In case it was already loaded, \n\
it will return the list's existing reference (won't load it again).\n\
\n\
**Parameters:**\n\
  * IDSResRef - the symbol list's name (.ids resref)\n\
\n\
**Return value:** Symbol table reference index\n\
\n\
**See also:** [[guiscript:UnloadSymbol]]\n\
"
);

static PyObject* GemRB_LoadSymbol(PyObject * /*self*/, PyObject* args)
{
	const char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		return AttributeError( GemRB_LoadSymbol__doc );
	}

	int ind = core->LoadSymbol( string );
	if (ind == -1) {
		return NULL;
	}

	return gs->ConstructObject("Symbol", ind);
}

PyDoc_STRVAR( GemRB_Symbol_Unload__doc,
"===== UnloadSymbol =====\n\
\n\
**Prototype:** GemRB.UnloadSymbol (SymbolIndex)\n\
\n\
**Metaclass Prototype:** Unload ()\n\
\n\
**Description:** Unloads an IDS symbol list.\n\
\n\
**Parameters:**\n\
  * SymbolIndex - returned by a previous LoadSymbol command.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadSymbol]], [[guiscript:UnloadTable]]"
);

static PyObject* GemRB_Symbol_Unload(PyObject * /*self*/, PyObject* args)
{
	int si;

	if (!PyArg_ParseTuple( args, "i", &si )) {
		return AttributeError( GemRB_Symbol_Unload__doc );
	}

	int ind = core->DelSymbol( si );
	if (ind == -1) {
		return NULL;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Symbol_GetValue__doc,
"===== GetSymbolValue =====\n\
\n\
**Prototype:** GemRB.GetSymbolValue (SymbolIndex, StringVal|IntVal)\n\
\n\
**Metaclass Prototype:** GetValue (StringVal|IntVal)\n\
\n\
**Description:** Returns a field of a IDS Symbol Table.\n\
\n\
**Parameters:**\n\
  * SymbolIndex - returned by a previous LoadSymbol command\n\
  * StringVal - name of the symbol to resolve (first column of .ids file)\n\
  * IntVal - value of the symbol to find (second column of .ids file)\n\
\n\
**Return value:**\n\
  * numeric, if the symbol's name was given (the value of the symbol)\n\
  * string, if the value of the symbol was given (the symbol's name)\n\
\n\
**Example:**\n\
    align = GemRB.GetPlayerStat (pc, IE_ALIGNMENT)\n\
    ss = GemRB.LoadSymbol ('ALIGN')\n\
    sym = GemRB.GetSymbolValue (ss, align)\n\
The above example will find the symbolic name of the player's alignment.\n\
\n\
**See also:** [[guiscript:LoadSymbol]], [[guiscript:Table_GetValue]]"
);

static PyObject* GemRB_Symbol_GetValue(PyObject * /*self*/, PyObject* args)
{
	PyObject* si, * sym;

	if (PyArg_UnpackTuple( args, "ref", 2, 2, &si, &sym )) {
		if (!PyObject_TypeCheck( si, &PyInt_Type )) {
			return AttributeError( GemRB_Symbol_GetValue__doc );
		}
		long SymbolIndex = PyInt_AsLong( si );
		if (PyObject_TypeCheck( sym, &PyString_Type )) {
			char* syms = PyString_AsString( sym );
			Holder<SymbolMgr> sm = core->GetSymbol( SymbolIndex );
			if (!sm)
				return NULL;
			long val = sm->GetValue( syms );
			return PyInt_FromLong( val );
		}
		if (PyObject_TypeCheck( sym, &PyInt_Type )) {
			long symi = PyInt_AsLong( sym );
			Holder<SymbolMgr> sm = core->GetSymbol( SymbolIndex );
			if (!sm)
				return NULL;
			const char* str = sm->GetValue( symi );
			return PyString_FromString( str );
		}
	}
	return AttributeError( GemRB_Symbol_GetValue__doc );
}

PyDoc_STRVAR( GemRB_Window_GetControl__doc,
"===== Window_GetControl =====\n\
\n\
**Prototype:** GemRB.GetControl (WindowID, ControlID)\n\
\n\
**Metaclass Prototype:** GetControl (ControlID)\n\
\n\
**Description:** Returns a control.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the return value of a previous LoadWindow call.\n\
  * ControlID - a control ID, see the .chu file specification\
\n\
**Return value:** a GControl object\n\
\n\
**See also:** [[guiscript:LoadWindowPack]], [[guiscript:LoadWindow]]"
);

static PyObject* GemRB_Window_GetControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlID )) {
		return AttributeError( GemRB_Window_GetControl__doc );
	}

	int ctrlindex = GetControlIndex(WindowIndex, ControlID);
	if (ctrlindex == -1) {
		char tmp[40];
		snprintf(tmp, sizeof(tmp), "Control (ID: %d) was not found!", ControlID);
		return RuntimeError(tmp);
	}

	PyObject* ret = 0;
	Control *ctrl = GetControl(WindowIndex, ctrlindex, -1);
	if (!ctrl) {
		return RuntimeError( "Control is not found" );
	}
	const char* type = "Control";
	switch(ctrl->ControlType) {
	case IE_GUI_LABEL:
		type = "Label";
		break;
	case IE_GUI_EDIT:
		type = "TextEdit";
		break;
	case IE_GUI_SCROLLBAR:
		type = "ScrollBar";
		break;
	case IE_GUI_TEXTAREA:
		type = "TextArea";
		break;
	case IE_GUI_BUTTON:
		type = "Button";
		break;
	case IE_GUI_WORLDMAP:
		type = "WorldMap";
		break;
	default:
		break;
	}
	PyObject* ctrltuple = Py_BuildValue("(ii)", WindowIndex, ctrlindex);
	ret = gs->ConstructObject(type, ctrltuple);
	Py_DECREF(ctrltuple);

	if (!ret) {
		char buf[256];
		snprintf( buf, sizeof( buf ), "Couldn't construct Control object for control %d in window %d!", ControlID, WindowIndex );
		return RuntimeError(buf);
	}
	return ret;
}

PyDoc_STRVAR( GemRB_Window_HasControl__doc,
"===== Window_HasControl =====\n\
\n\
**Prototype:** GemRB.HasControl (WindowIndex, ControlID[, ControlType])\n\
\n\
**Metaclass Prototype:** HasControl (ControlID[, ControlType])\n\
\n\
**Description:** Returns true if the control exists.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the window control id\n\
  * ControlID - the id of the target control\n\
  * ControlType - limit to controls of this type (see GUIDefines.py)\n\
\n\
**Return value:** bool"
);

static PyObject* GemRB_Window_HasControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;
	int Type = -1;

	if (!PyArg_ParseTuple( args, "ii|i", &WindowIndex, &ControlID, &Type )) {
		return AttributeError( GemRB_Window_HasControl__doc );
	}
	int ret = GetControlIndex( WindowIndex, ControlID );
	if (ret == -1) {
		return PyInt_FromLong( 0 );
	}

	if (Type!=-1) {
		Control *ctrl = GetControl(WindowIndex, ControlID, -1);
		if (ctrl->ControlType!=Type) {
			return PyInt_FromLong( 0 );
		}
	}
	return PyInt_FromLong( 1 );
}

PyDoc_STRVAR( GemRB_Control_QueryText__doc,
"===== Control_QueryText =====\n\
\n\
**Prototype:** GemRB.QueryText (WindowIndex, ControlIndex)\n\
\n\
**Metaclass Prototype:** QueryText ()\n\
\n\
**Description:** Returns the Text of a TextEdit/TextArea/Label control. \n\
In case of a TextArea, it will return the selected row, not the entire \n\
textarea.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
\n\
**Return value:** string, may be empty\n\
\n\
**Example:**\n\
  Name = NameField.QueryText ()\n\
  GemRB.SetToken ('CHARNAME', Name)\n\
The above example retrieves the character's name typed into the TextEdit control and stores it in a Token (a string variable accessible to gamescripts, the engine core and to the guiscripts too).\n\
\n\
  GemRB.SetToken ('VoiceSet', TextAreaControl.QueryText ())\n\
The above example sets the VoiceSet token to the value of the selected string in a TextArea control. Later this voiceset could be stored in the character sheet.\n\
\n\
**See also:** [[guiscript:Control_SetText]], [[guiscript:SetToken]], [[guiscript:accessing_gui_controls]]"
);

static PyObject* GemRB_Control_QueryText(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;

	if (!PyArg_ParseTuple( args, "ii", &wi, &ci )) {
		return AttributeError( GemRB_Control_QueryText__doc );
	}

	Control *ctrl = GetControl(wi, ci, -1);
	if (!ctrl) {
		return NULL;
	}

	String wstring = ctrl->QueryText();
	std::string nstring(wstring.begin(), wstring.end());
	char * cStr = ConvertCharEncoding(nstring.c_str(),
		core->TLKEncoding.encoding.c_str(), core->SystemEncoding);
	if (cStr) {
		PyObject* pyStr = PyString_FromString(cStr);
		free(cStr);
		return pyStr;
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_TextEdit_SetBufferLength__doc,
"===== TextEdit_SetBufferLength =====\n\
\n\
**Prototype:** GemRB.SetBufferLength (WindowIndex, ControlIndex, Length)\n\
\n\
**Metaclass Prototype:** SetBufferLength (Length)\n\
\n\
**Description:**  Sets the maximum text length of a TextEdit control. It \n\
cannot be more than 65535.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the window control id\n\
  * ControlID - the id of the target control\n\
  * Length - the maximum text length\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_TextEdit_SetBufferLength(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Length;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &Length)) {
		return AttributeError( GemRB_TextEdit_SetBufferLength__doc );
	}

	TextEdit* te = (TextEdit *) GetControl( WindowIndex, ControlIndex, IE_GUI_EDIT );
	if (!te)
		return NULL;

	if ((ieDword) Length>0xffff) {
		return AttributeError( GemRB_Control_QueryText__doc );
	}

	te->SetBufferLength((ieWord) Length );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Control_SetText__doc,
"===== Control_SetText =====\n\
\n\
**Prototype:** GemRB.SetText (WindowIndex, ControlIndex, String|Strref)\n\
\n\
**Metaclass Prototype:** SetText (String|Strref)\n\
\n\
**Description:** Sets the Text of a control in a Window. In case of \n\
strrefs, any tokens contained by the string will be resolved. (For \n\
example the substring '<CHARNAME>' will be replaced by the 'CHARNAME' \n\
token.) -1 is a special Strref, it will be resolved to the name/version \n\
of the running engine.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * String - an arbitrary string\n\
  * Strref - a string index from the dialog.tlk table.\n\
\n\
**Return value:** 0 on success, -1 otherwise\n\
\n\
**See also:** [[guiscript:Control_QueryText]], [[guiscript:DisplayString]], [[guiscript:Window_GetControl]], [[guiscript:accessing_gui_controls]]"
);

static PyObject* GemRB_Control_SetText(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	PyObject* str;

	if (!PyArg_ParseTuple( args, "iiO", &WindowIndex, &ControlIndex, &str)) {
		return AttributeError( GemRB_TextEdit_SetBufferLength__doc );
	}

	Control *ctrl = GetControl(WindowIndex, ControlIndex, -1);
	if (!ctrl) {
		return RuntimeError("Invalid Control");
	}

	if (PyObject_TypeCheck( str, &PyInt_Type )) { // strref
		ieStrRef StrRef = (ieStrRef)PyInt_AsLong( str );
		String* string = core->GetString( StrRef );
		ctrl->SetText(string);
		delete string;
	} else if (str == Py_None) {
		// clear the text
		ctrl->SetText(NULL);
	} else { // string value of the object
		const String* string = StringFromCString( PyString_AsString( str ) );
		ctrl->SetText(string);
		delete string;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_TextArea_Append__doc,
"===== TextArea_Append =====\n\
\n\
**Prototype:** GemRB.TextAreaAppend (WindowIndex, ControlIndex, String|Strref [, Row[, Flag]])\n\
\n\
**Metaclass Prototype:** Append (String|Strref [, Row[, Flag]])\n\
\n\
**Description:** Appends the Text to the TextArea Control in the Window. \n\
If row is specificed, it can also append text to existing rows.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * String - literal text, it could have embedded colour codes\n\
  * Strref - a string index from the dialog.tlk table.\n\
  * Row - the row of text to add the text to, if omitted, the text will be added (in a new row) after the last row.\n\
  * Flag - the flags for QueryText (if strref resolution is used)\n\
    * 1 - strrefs displayed (even if not enabled by default)\n\
    * 2 - sound (plays the associated sound)\n\
    * 4 - speech (works only with if sound was set)\n\
    * 256 - strrefs not displayed (even if allowed by default)\n\
\n\
**Return value:** Index of the row appended or changed\n\
\n\
**See also:** [[guiscript:TextArea_Clear]], [[guiscript:Control_SetText]], [[guiscript:Control_QueryText]]"
);

static PyObject* GemRB_TextArea_Append(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci, * pystr;
	PyObject* flag = NULL;
	long WindowIndex, ControlIndex;

	if (!PyArg_UnpackTuple( args, "ref", 3, 4, &wi, &ci, &pystr, &flag )) {
		return AttributeError( GemRB_TextArea_Append__doc );
	}
	if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
		!PyObject_TypeCheck( ci, &PyInt_Type ) ||
		( !PyObject_TypeCheck( pystr, &PyString_Type ) &&
		!PyObject_TypeCheck( pystr, &PyInt_Type ) )) {
		return AttributeError( GemRB_TextArea_Append__doc );
	}
	WindowIndex = PyInt_AsLong( wi );
	ControlIndex = PyInt_AsLong( ci );

	TextArea* ta = ( TextArea* ) GetControl( WindowIndex, ControlIndex, IE_GUI_TEXTAREA);
	if (!ta) {
		return NULL;
	}

	String* str = NULL;
	if (PyObject_TypeCheck( pystr, &PyString_Type )) {
		str = StringFromCString(PyString_AsString( pystr ));
	} else {
		ieDword flags = 0;
		if (flag) {
			if (!PyObject_TypeCheck( flag, &PyInt_Type )) {
				Log(ERROR, "GUIScript", "Syntax Error: GetString flag must be integer");
				return NULL;
			}
			flags = (ieDword)PyInt_AsLong( flag );
		}
		str = core->GetString( (ieStrRef)PyInt_AsLong( pystr ), flags );
	}
	if (str) {
		ta->AppendText( *str );
		delete str;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_TextArea_Clear__doc,
"===== TextArea_Clear =====\n\
\n\
**Prototype:** GemRB.TextAreaClear (WindowIndex, ControlIndex)\n\
\n\
**Metaclass Prototype:** Clear ()\n\
\n\
**Description:** Clears the Text from the TextArea Control in the Window.\n\
\n\
**Parameters:** WindowIndex, ControlIndex - the control's reference\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:TextArea_Append]]"
);

static PyObject* GemRB_TextArea_Clear(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci;
	long WindowIndex, ControlIndex;

	if (!PyArg_UnpackTuple( args, "ref", 2, 2, &wi, &ci )) {
		return AttributeError( GemRB_TextArea_Clear__doc );
	}
	if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
		!PyObject_TypeCheck( ci, &PyInt_Type )) {
		return AttributeError( GemRB_TextArea_Clear__doc );
	}
	WindowIndex = PyInt_AsLong( wi );
	ControlIndex = PyInt_AsLong( ci );
	TextArea* ta = ( TextArea* ) GetControl( WindowIndex, ControlIndex, IE_GUI_TEXTAREA);
	if (!ta) {
		return NULL;
	}
	ta->ClearText();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Control_SetTooltip__doc,
"===== Control_SetTooltip =====\n\
\n\
**Prototype:** GemRB.SetTooltip (WindowIndex, ControlIndex, String|Strref[, Function])\n\
\n\
**Metaclass Prototype:** SetTooltip (String|Strref[, Function])\n\
\n\
**Description:** Sets control's tooltip. Any control may have a tooltip.\n\
\n\
The tooltip's visual properties must be set in the gemrb.ini file:\n\
  * TooltipFont - Font used to display tooltips\n\
  * TooltipBack - Sprite displayed behind the tooltip text, if any\n\
  * TooltipMargin - Space between tooltip text and sides of TooltipBack (x2)\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * String - an arbitrary string\n\
  * Strref - a string index from the dialog.tlk table.\n\
  * Function - (optional) function key to prepend\n\
\n\
**Return value:** 0 on success, -1 on error\n\
\n\
**See also:** [[guiscript:Control_SetText]]"
);

static PyObject* GemRB_Control_SetTooltip(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci, * str;
	PyObject* function = NULL;
	long WindowIndex, ControlIndex, StrRef;
	char* string;
	int ret;
	int Function = 0;

	if (!PyArg_UnpackTuple( args, "ref", 3, 4, &wi, &ci, &str, &function)) {
		return AttributeError( GemRB_Control_SetTooltip__doc );
	}
	if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
		!PyObject_TypeCheck( ci, &PyInt_Type ) ||
		( !PyObject_TypeCheck( str, &PyString_Type ) &&
		!PyObject_TypeCheck( str, &PyInt_Type ) )) {
		return AttributeError( GemRB_Control_SetTooltip__doc );
	}

	WindowIndex = PyInt_AsLong( wi );
	ControlIndex = PyInt_AsLong( ci );
	if (function) {
		if (!PyObject_TypeCheck(function, &PyInt_Type) ) {
			return AttributeError( GemRB_Control_SetTooltip__doc );
		}
		Function = PyInt_AsLong( function);
	}
	if (PyObject_TypeCheck( str, &PyString_Type )) {
		string = PyString_AsString( str );
		if (string == NULL) {
			return RuntimeError("Null string received");
		}
		if (Function) {
			ret = SetFunctionTooltip( (ieWord) WindowIndex, (ieWord) ControlIndex, string, Function);
		} else {
			ret = core->SetTooltip( (ieWord) WindowIndex, (ieWord) ControlIndex, string );
		}
		if (ret == -1) {
			return RuntimeError("Cannot set tooltip");
		}
	} else {
		StrRef = PyInt_AsLong( str );
		if (StrRef == -1) {
			ret = core->SetTooltip( (ieWord) WindowIndex, (ieWord) ControlIndex, GEMRB_STRING );
		} else {
			char* str = core->GetCString( StrRef );

			if (Function) {
				ret = SetFunctionTooltip( (ieWord) WindowIndex, (ieWord) ControlIndex, str, Function );
			} else {
				ret = core->SetTooltip( (ieWord) WindowIndex, (ieWord) ControlIndex, str );
				core->FreeString( str );
			}
		}
		if (ret == -1) {
			return RuntimeError("Cannot set tooltip");
		}
	}

	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_Window_SetVisible__doc,
"===== Window_SetVisible =====\n\
\n\
**Prototype:** GemRB.SetVisible (WindowIndex, Visible)\n\
\n\
**Metaclass Prototype:** SetVisible (Visible)\n\
\n\
**Description:** Sets the Visibility Flag of a Window.\n\
Window index 0 is the game control window (GameWindow python object)\n\
\n\
**Parameters:**\n\
  * WindowIndex - the index returned by LoadWindow()\n\
  * Visible:**\n\
    * WINDOW_INVISIBLE - Window is invisible\n\
    * WINDOW_VISIBLE - Window is visible\n\
    * WINDOW_GRAYED - Window is shaded\n\
    * WINDOW_FRONT - Window is drawn in the foreground\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_ShowModal]], [[guiscript:Window_SetFrame]]"
);

static PyObject* GemRB_Window_SetVisible(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;
	int visible;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &visible )) {
		return AttributeError( GemRB_Window_SetVisible__doc );
	}

	int ret = core->SetVisible( WindowIndex, visible );
	if (ret == -1) {
		return RuntimeError("Invalid window in SetVisible");
	}
	if (!WindowIndex) {
		core->SetEventFlag(EF_CONTROL);
	}

	Py_RETURN_NONE;
}

//useful only for ToB and HoW, sets masterscript/worldmap name
PyDoc_STRVAR( GemRB_SetMasterScript__doc,
"===== SetMasterScript =====\n\
\n\
**Prototype:** GemRB.SetMasterScript (ScriptResRef, WMPResRef[, WMPResRef2])\n\
\n\
**Description:** Sets the worldmap and master script names. This function \n\
 is required if you want to alter the worldmap or the master script \n\
 (simulating the ToB or HoW expansions).\n\
\n\
**Parameters:** \n\
  * ScriptResRef - the name of the master script (.bcs resref). \n\
  * WMPResRef    - the name of the worldmap (.wmp resref).\n\
  * WMPResRef2   - the name of the extra worldmap (optional).\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadGame]]\n\
"
);

static PyObject* GemRB_SetMasterScript(PyObject * /*self*/, PyObject* args)
{
	char* script;
	char* worldmap1;
	char* worldmap2 = NULL;

	if (!PyArg_ParseTuple( args, "ss|s", &script, &worldmap1, &worldmap2 )) {
		return AttributeError( GemRB_SetMasterScript__doc );
	}
	strnlwrcpy( core->GlobalScript, script, 8 );
	strnlwrcpy( core->WorldMapName[0], worldmap1, 8 );
	if (!worldmap2) {
		memset(core->WorldMapName[1], 0, 8);
	} else {
		strnlwrcpy( core->WorldMapName[1], worldmap2, 8 );
	}
	core->UpdateMasterScript();
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_ShowModal__doc,
"===== Window_ShowModal =====\n\
\n\
**Prototype:** GemRB.ShowModal (WindowIndex, [Shadow=MODAL_SHADOW_NONE])\n\
\n\
**Metaclass Prototype:** ShowModal ([Shadow=MODAL_SHADOW_NONE])\n\
\n\
**Description:** Show a Window on Screen setting the Modal Status. If \n\
Shadow is MODAL_SHADOW_GRAY, other windows are grayed. If Shadow is \n\
MODAL_SHADOW_BLACK, they are blacked out.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the index returned by LoadWindow()\n\
  * Shadow:\n\
    * MODAL_SHADOW_NONE = 0\n\
    * MODAL_SHADOW_GRAY = 1 (translucent)\n\
    * MODAL_SHADOW_BLACK = 2\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_SetVisible]]"
);

static PyObject* GemRB_Window_ShowModal(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, Shadow = MODAL_SHADOW_NONE;

	if (!PyArg_ParseTuple( args, "i|i", &WindowIndex, &Shadow )) {
		return AttributeError( GemRB_Window_ShowModal__doc );
	}

	int ret = core->ShowModal( WindowIndex, (MODAL_SHADOW)Shadow );
	if (ret == -1) {
		return NULL;
	}

	core->PlaySound(DS_WINDOW_OPEN, SFX_CHAN_GUI);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetTimedEvent__doc,
"===== SetTimedEvent =====\n\
\n\
**Prototype:** GemRB.SetTimedEvent (FunctionName, rounds)\n\
\n\
**Description:** Sets a timed event to be called by the Game object. If \n\
there is no game loaded, this command is ignored. If the game is unloaded, \n\
the event won't be called.\n\
\n\
**Parameters:** \n\
  * FunctionName - a python function object\n\
  * rounds       - the delay with which the function should be called\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Control_SetEvent]]\n\
"
);

static PyObject* GemRB_SetTimedEvent(PyObject * /*self*/, PyObject* args)
{
	PyObject* function;
	int rounds;

	if (!PyArg_ParseTuple( args, "Oi", &function, &rounds )) {
		return AttributeError( GemRB_SetTimedEvent__doc );
	}

	EventHandler handler = NULL;
	if (function != Py_None && PyCallable_Check(function)) {
		handler = new PythonCallback(function);
	} else {
		char buf[256];
		snprintf(buf, sizeof(buf), "Can't set timed event handler %s!", PyEval_GetFuncName(function));
		return RuntimeError(buf);
	}
	Game *game = core->GetGame();
	if (game) {
		game->SetTimedEvent(handler, rounds);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Control_SetEvent__doc,
"===== Control_SetEvent =====\n\
\n\
**Prototype:** _GemRB.Control_SetEvent (WindowIndex, ControlIndex, EventMask, PythonFunction)\n\
\n\
**Metaclass Prototype:** SetEvent (EventMask, PythonFunction)\n\
\n\
**Description:** Ties an event of a control to a python function\
\n\
**Parameters:** \n\
  * EventMask - a dword describing the event. Its high byte is actually the control's type.\n\
    * IE_GUI_BUTTON_ON_PRESS    = 0x00000000, the user pressed the button.\n\
    * IE_GUI_MOUSE_OVER_BUTTON  = 0x00000001, the user hovered the mouse over the button.\n\
    * IE_GUI_MOUSE_ENTER_BUTTON = 0x00000002, the user just moved the mouse onto the button.\n\
    * IE_GUI_MOUSE_LEAVE_BUTTON = 0x00000003, the mouse just left the button\n\
    * IE_GUI_BUTTON_ON_SHIFT_PRESS = 0x00000004, the button was pressed along with the shift key.\n\
    * IE_GUI_BUTTON_ON_RIGHT_PRESS = 0x00000005, the button was right clicked\n\
    * IE_GUI_BUTTON_ON_DRAG_DROP   = 0x00000006, the button was clicked during a drag&drop action.\n\
    * IE_GUI_PROGRESS_END_REACHED = 0x01000000, the progressbar received a 100 percent value.\n\
    * IE_GUI_SLIDER_ON_CHANGE   = 0x02000000, the slider's knob position has changed.\n\
    * IE_GUI_EDIT_ON_CHANGE     = 0x03000000, the text in the editbox has changed.\n\
    * IE_GUI_TEXTAREA_ON_CHANGE = 0x05000000, the text in the textarea has changed.\n\
    * IE_GUI_LABEL_ON_PRESS     = 0x06000000, the label was pressed.\n\
    * IE_GUI_SCROLLBAR_ON_CHANGE= 0x07000000, the scrollbar's knob position has changed.\n\
    * ... See GUIDefines.py for all event types\n\
  * PythonFunction - the callback function\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
    Bar.SetEvent (IE_GUI_PROGRESS_END_REACHED, EndLoadScreen)\n\
    ...\n\
  def EndLoadScreen ():\n\
    Skull = LoadScreen.GetControl (1)\n\
    Skull.SetMOS ('GSKULON')\n\
The above example changes the image on the loadscreen when the progressbar reaches the end.\n\
\n\
  Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, Buttons.YesButton)\n\
The above example sets up the 'YesButton' function from the Buttons module to be called when the button is pressed.\n\
\n\
  Button.SetEvent (IE_GUI_MOUSE_OVER_BUTTON, ChaPress)\n\
The above example shows how to implement 'context sensitive help'. The 'ChaPress' function displays a help text on the screen when you hover the mouse over a button.\n\
\n\
**See also:** [[guiscript:Window_GetControl]], [[guiscript:Control_SetVarAssoc]], [[guiscript:SetTimedEvent]], [[guiscript:accessing_gui_controls]]"
);

static PyObject* GemRB_Control_SetEvent(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int event;
	PyObject* func;

	if (!PyArg_ParseTuple(args, "iiiO", &WindowIndex, &ControlIndex,
				&event, &func)) {
		return AttributeError(GemRB_Control_SetEvent__doc);
	}

	Control* ctrl = GetControl(WindowIndex, ControlIndex, -1);
	if (!ctrl)
		return NULL;

	ControlEventHandler handler = NULL;
	if (func != Py_None && PyCallable_Check(func)) {
		handler = new PythonControlCallback(func);
	}
	if (!ctrl->SetEvent(event, handler)) {
		char buf[256];
		snprintf(buf, sizeof(buf), "Can't set event handler %s!", PyEval_GetFuncName(func));
		return RuntimeError(buf);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_SetKeyPressEvent__doc,
"===== Window_SetKeyPressEvent =====\n\
\n\
**Prototype:** _GemRB.Window_SetKeyPressEvent (callback)\n\
\n\
**Description:** Sets a callback function to handle key press event on window scopes. \n\
\n\
**Parameters:**\n\
  * callback - Python function that accepts (windowIndex, key, mod) arguments and returns\n\
	* 					 1 indicating successful key press consumption or 0 otherwise. \n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    Window.SetKeyPressEvent (KeyPressCallback)\n\
    return\n\
\n\
**See also:** [[guiscript:QuitGame]]\n\
"
);

static PyObject* GemRB_Window_SetKeyPressEvent(PyObject* /*self*/, PyObject *args) {
	int windowIndex;
	PyObject *fn;

	if(!PyArg_ParseTuple(args, "iO", &windowIndex, &fn)) {
		return AttributeError(GemRB_Window_SetKeyPressEvent__doc);
	}

	WindowKeyPressHandler handler = NULL;
	if(fn != Py_None && PyCallable_Check(fn)) {
		handler = new PythonObjectCallback<WindowKeyPress>(fn);
	}

	Window *window = core->GetWindow(windowIndex);
	if(window) {
		window->SetKeyPressEvent(handler);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetNextScript__doc,
"===== SetNextScript =====\n\
\n\
**Prototype:** GemRB.SetNextScript (scriptname)\n\
\n\
**Description:** Instructs the GUIScript engine to load the script when \n\
this script has terminated.\n\
\n\
**Parameters:**\n\
  * scriptname - name of the python script to be executed. May not exceed 60 characters.\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    GemRB.SetNextScript ('CharGen')\n\
    return\n\
\n\
**See also:** [[guiscript:QuitGame]]\n\
"
);

static PyObject* GemRB_SetNextScript(PyObject * /*self*/, PyObject* args)
{
	const char* funcName;

	if (!PyArg_ParseTuple( args, "s", &funcName )) {
		return AttributeError( GemRB_SetNextScript__doc );
	}

	if (!strcmp(funcName, "")) {
		return AttributeError( GemRB_SetNextScript__doc );
	}

	core->SetNextScript(funcName);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Control_SetStatus__doc,
"===== SetControlStatus =====\n\
\n\
**Prototype:** GemRB.SetControlStatus (WindowIndex, ControlIndex, State)\n\
\n\
**Metaclass Prototype:** SetStatus (State)\n\
\n\
**Description:** Sets the state of a Control. For buttons, this is the \n\
same as SetButtonState. You can additionally use 0x80 for a focused \n\
control (IE_GUI_CONTROL_FOCUSED).\n\
For other controls, this command will set the common value of the \n\
control, which has various uses.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * Button States:**\n\
    * IE_GUI_BUTTON_ENABLED    = 0x00000000, default state\n\
    * IE_GUI_BUTTON_UNPRESSED  = 0x00000000, same as above\n\
    * IE_GUI_BUTTON_PRESSED    = 0x00000001, the button is pressed\n\
    * IE_GUI_BUTTON_SELECTED   = 0x00000002, the button stuck in pressed state\n\
    * IE_GUI_BUTTON_DISABLED   = 0x00000003, the button is disabled \n\
    * IE_GUI_BUTTON_LOCKED     = 0x00000004, the button is inactive (like DISABLED, but processes MouseOver events and draws UNPRESSED bitmap)\n\
    * IE_GUI_BUTTON_FAKEDISABLED = 0x00000005, draws DISABLED bitmap, but it isn't disabled\n\
    * IE_GUI_BUTTON_FAKEPRESSED = 0x00000006, draws PRESSED bitmap, but it isn't shifted\n\
  * Text Edit states\n\
    * IE_GUI_EDIT_NUMBER    =  0x030000001, the textedit will accept only digits\n\
  * Map Control States (add 0x09000000 to these):\n\
    * IE_GUI_MAP_NO_NOTES   =  0, no mapnotes visible\n\
    * IE_GUI_MAP_VIEW_NOTES =  1, view notes (no setting)\n\
    * IE_GUI_MAP_SET_NOTE   =  2, allow setting notes\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetState]]"
);

static PyObject* GemRB_Control_SetStatus(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int status;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &status )) {
		return AttributeError( GemRB_Control_SetStatus__doc );
	}

	int ret = core->SetControlStatus( WindowIndex, ControlIndex, status );
	switch (ret) {
	case -1:
		return RuntimeError( "Control is not found." );
	case -2:
		return RuntimeError( "Control type is not matching." );
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Control_AttachScrollBar__doc,
"===== Control_AttachScrollBar =====\n\
\n\
**Prototype:** GemRB.AttachScrollBar (WindowIndex, ControlIndex, ScrollBarControlIndex)\n\
\n\
**Metaclass Prototype:** AttachScrollBar (ScrollBarControlIndex)\n\
\n\
**Description:** Attaches a ScrollBar to another control. If the control \n\
receives mousewheel events, it will be relayed to the ScrollBar. TextArea \n\
controls will also be synchronised with the scrollbar. If there is a \n\
single ScrollBar on the window, or the ScrollBar was set with \n\
SetDefaultScrollBar, this command is not needed.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * ScrollBarControlIndex - the scrollbar's index on the same window\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:ScrollBar_SetDefaultScrollBar]]"
);

static PyObject* GemRB_Control_AttachScrollBar(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, ScbControlIndex;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &ScbControlIndex )) {
		return AttributeError( GemRB_Control_AttachScrollBar__doc );
	}

	Control *ctrl = GetControl(WindowIndex, ControlIndex, -1);
	if (!ctrl) {
		return NULL;
	}

	Control *scb = NULL;

	if (ScbControlIndex != -1) {
		scb = GetControl(WindowIndex, ScbControlIndex, IE_GUI_SCROLLBAR);
		if (!scb) {
			return NULL;
		}
	}

	int ret = ctrl->SetScrollBar( scb );
	if (ret == -1) {
		return NULL;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Control_SetVarAssoc__doc,
"===== Control_SetVarAssoc =====\n\
\n\
**Prototype:** GemRB.SetVarAssoc (WindowIndex, ControlIndex, VariableName, LongValue)\n\
\n\
**Metaclass Prototype:** SetVarAssoc (VariableName, LongValue)\n\
\n\
**Description:** It associates a variable name and value with a control. \n\
The control uses this associated value differently, depending on the \n\
control. See more about this in 'data_exchange'.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex  - the control's reference\n\
  * Variablename - string, a Global Dictionary Name associated with the control\n\
  * LongValue - numeric, a value associated with the control\n\
\n\
**Return value:** N/A\n\
\n\
**Special:** If the 'DialogChoose' variable was set to -1 or 0 during a dialog session, it will terminate (-1) or pick the first available option (0) from the dialog automatically. (0 is used for 'continue', -1 is used for 'end dialogue').\n\
\n\
**See also:** [[guiscript:Button_SetFlags]], [[guiscript:SetVar]], [[guiscript:GetVar]], [[guiscript:data_exchange]], [[guiscript:accessing_gui_controls]]"
);

static PyObject* GemRB_Control_SetVarAssoc(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	ieDword Value;
	char* VarName;

	if (!PyArg_ParseTuple( args, "iisi", &WindowIndex, &ControlIndex,
			&VarName, &Value )) {
		return AttributeError( GemRB_Control_SetVarAssoc__doc );
	}

	Control* ctrl = GetControl( WindowIndex, ControlIndex, -1 );
	if (!ctrl) {
		return NULL;
	}

	//max variable length is not 32, but 40 (in guiscripts), but that includes zero terminator!
	strnlwrcpy( ctrl->VarName, VarName, MAX_VARIABLE_LENGTH-1 );
	ctrl->Value = Value;
	/** setting the correct state for this control */
	/** it is possible to set up a default value, if Lookup returns false, use it */
	Value = 0;
	core->GetDictionary()->Lookup( VarName, Value );
	Window* win = core->GetWindow( WindowIndex );
	win->RedrawControls(VarName, Value);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_Unload__doc,
"===== Window_Unload =====\n\
\n\
**Prototype:** GemRB.UnloadWindow (WindowIndex)\n\
\n\
**Metaclass Prototype:** Unload ()\n\
\n\
**Description:** Unloads a previously loaded Window. EnterGame() and \n\
QuitGame() automatically unload all loaded windows.\n\
\n\
**Parameters:** WindowIndex - the index returned by LoadWindow()\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadWindow]], [[guiscript:EnterGame]], [[guiscript:QuitGame]]"
);

static PyObject* GemRB_Window_Unload(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;

	if (!PyArg_ParseTuple( args, "i", &WindowIndex )) {
		return AttributeError( GemRB_Window_Unload__doc );
	}

	unsigned short arg = (unsigned short) WindowIndex;
	if (arg == 0xffff) {
		return AttributeError( "Feature unsupported! ");
	}

	//Don't bug if the window wasn't loaded
	if (core->GetWindow(arg) ) {
		int ret = core->DelWindow( arg );
		if (ret == -1) {
			return RuntimeError( "Can't unload window!" );
		}

		core->PlaySound(DS_WINDOW_CLOSE, SFX_CHAN_GUI);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_Invalidate__doc,
"===== Window_Invalidate =====\n\
\n\
**Prototype:** GemRB.InvalidateWindow (WindowIndex)\n\
\n\
**Metaclass Prototype:** Invalidate ()\n\
\n\
**Description:** Invalidates the given Window so it will be redrawn entirely.\n\
\n\
**Parameters:** WindowIndex is the index returned by LoadWindow()\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadWindow]]"
);

static PyObject* GemRB_Window_Invalidate(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;

	if (!PyArg_ParseTuple( args, "i", &WindowIndex )) {
		return AttributeError( GemRB_Window_Invalidate__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	win->Invalidate();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_CreateWindow__doc,
"===== CreateWindow =====\n\
\n\
**Prototype:** GemRB.CreateWindow (WindowID, X, Y, Width, Height, MosResRef)\n\
\n\
**Description:** Creates a new empty window and returns its index.\n\
\n\
**Parameters:** \n\
  * WindowID - the window's ID\n\
  * X, Y - the window's position\n\
  * Width, Height - the window's dimensions\n\
  * MosResRef - the background image (.mos image)\n\
\n\
**Return value:** a window index\n\
\n\
**See also:**"
);

static PyObject* GemRB_CreateWindow(PyObject * /*self*/, PyObject* args)
{
	int WindowID, x, y, w, h;
	char* Background;

	if (!PyArg_ParseTuple( args, "iiiiis", &WindowID, &x, &y,
			&w, &h, &Background )) {
		return AttributeError( GemRB_CreateWindow__doc );
	}
	int WindowIndex = core->CreateWindow( WindowID, x, y, w, h, Background );
	if (WindowIndex == -1) {
		return RuntimeError( "Can't create window" );
	}

	return PyInt_FromLong( WindowIndex );
}

PyDoc_STRVAR( GemRB_Button_CreateLabelOnButton__doc,
"===== Button_CreateLabelOnButton =====\n\
\n\
**Prototype:** GemRB.CreateLabelOnButton (WindowIndex, ControlIndex, NewID, font, align)\n\
\n\
**Metaclass Prototype:** CreateLabelOnButton (NewID, font, align)\n\
\n\
**Description:** Creates and adds a new Label to a Window, based on the \n\
dimensions of an existing button. If the NewID is the same as the old \n\
button ID, the old button will be converted to this Label (the old button \n\
will be removed). \n\
\n\
**Parameters:**\n\
  * WindowIndex     - the value returned from LoadWindow\n\
  * ButtonControlID - the button control to be copied/converted\n\
  * NewID           - the new control will be available via this controlID\n\
  * font            - a .bam resref which must be listed in fonts.2da too\n\
  * align           - label text alignment\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_CreateButton]], [[guiscript:Window_CreateLabel]]"
);

static PyObject* GemRB_Button_CreateLabelOnButton(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, ControlID, align;
	char *font;

	if (!PyArg_ParseTuple( args, "iiisi", &WindowIndex, &ControlIndex,
			&ControlID, &font, &align )) {
		return AttributeError( GemRB_Button_CreateLabelOnButton__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	Control *btn = GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}
	Region frame = btn->ControlFrame();
	frame.y += 5;
	frame.h -= 10;
	Label* lbl = new Label(frame, core->GetFont( font ), L"" );
	lbl->ControlID = ControlID;
	lbl->SetAlignment( align );
	win->AddControl( lbl );

	int ret = GetControlIndex( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_Window_CreateLabel__doc,
"===== Window_CreateLabel =====\n\
\n\
**Prototype:** GemRB.CreateLabel(WindowIndex, ControlID, x, y, w, h, font, text, align)\n\
\n\
**Metaclass Prototype:** CreateLabel(ControlID, x, y, w, h, font, text, align)\n\
\n\
**Description:** Creates and adds a new Label to a Window.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the value returned from LoadWindow\n\
  * ControlID   - the new control will be available via this controlID\n\
  * x,y,w,h     - X position, Y position, Width and Height of the control\n\
  * font        - a .bam resref which must be listed in fonts.2da too\n\
  * text        - initial text of the label (must be string)\n\
  * align       - label text alignment\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
  StartWindow.CreateLabel (0x0fff0000, 0,415,640,30, 'EXOFONT', '', 1)\n\
  Label = StartWindow.GetControl (0x0fff0000)\n\
  Label.SetText (GEMRB_VERSION)\n\
The above lines add the GemRB version string to the PST main screen.\n\
\n\
**See also:** [[guiscript:Window_CreateButton]], [[guiscript:Control_SetText]]"
);

static PyObject* GemRB_Window_CreateLabel(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, align;
	Region rgn;
	char *font, *text;

	if (!PyArg_ParseTuple( args, "iiiiiissi", &WindowIndex, &ControlID, &rgn.x,
			&rgn.y, &rgn.w, &rgn.h, &font, &text, &align )) {
		return AttributeError( GemRB_Window_CreateLabel__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	String* string = StringFromCString(text);
	Label* lbl = new Label(rgn, core->GetFont( font ), (string) ? *string : L"" );
	delete string;

	lbl->ControlID = ControlID;
	lbl->SetAlignment( align );
	win->AddControl( lbl );

	int ret = GetControlIndex( WindowIndex, ControlID );
	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_Label_SetTextColor__doc,
"===== Label_SetTextColor =====\n\
\n\
**Prototype:** GemRB.SetLabelTextColor (WindowIndex, ControlIndex, red, green, blue)\n\
\n\
**Metaclass Prototype:** SetTextColor (red, green, blue)\n\
\n\
**Description:** Sets the Text Color of a Label Control. If the the Font \n\
has no own palette, you can set a default palette by this command.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * red, green, blue - the control's desired text color\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Label_SetUseRGB]]"
);
static PyObject* GemRB_Label_SetTextColor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int r, g, b;

	if (!PyArg_ParseTuple( args, "iiiii", &WindowIndex, &ControlIndex, &r, &g,
			&b )) {
		return AttributeError( GemRB_Label_SetTextColor__doc );
	}

	Label* lab = ( Label* ) GetControl(WindowIndex, ControlIndex, IE_GUI_LABEL);
	if (!lab) {
		return NULL;
	}

	const Color fore = { (ieByte) r, (ieByte) g, (ieByte) b, 0};
	lab->SetColor( fore, ColorBlack );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_CreateTextEdit__doc,
"===== Window_CreateTextEdit =====\n\
\n\
**Prototype:** GemRB.CreateTextEdit (WindowIndex, ControlID, x, y, w, h, font, text)\n\
\n\
**Metaclass Prototype:** CreateTextEdit (ControlID, x, y, w, h, font, text)\n\
\n\
**Description:** Creates and adds a new TextEdit field to a Window. Used \n\
in PST MapNote editor. The maximum length of the edit field is 500 characters.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the value returned from LoadWindow\n\
  * ControlID   - the new control will be available via this controlID\n\
  * x,y,w,h     - X position, Y position, Width and Height of the control\n\
  * font        - font BAM ResRef\n\
  * text        - initial text\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_CreateLabel]], [[guiscript:Window_CreateMapControl]], [[guiscript:Window_CreateWorldMapControl]], [[guiscript:Window_CreateButton]]"
);

static PyObject* GemRB_Window_CreateTextEdit(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;
	Region rgn;
	char *font, *cstr;

	if (!PyArg_ParseTuple( args, "iiiiiiss", &WindowIndex, &ControlID, &rgn.x,
			&rgn.y, &rgn.w, &rgn.h, &font, &cstr )) {
		return AttributeError( GemRB_Window_CreateTextEdit__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	//there is no need to set these differently, currently
	TextEdit* edit = new TextEdit(rgn, 500, 0, 0);
	edit->SetFont( core->GetFont( font ) );
	edit->ControlID = ControlID;
	String* text = StringFromCString(cstr);
	edit->Control::SetText( text );
	delete text;
	win->AddControl( edit );

	Sprite2D* spr = core->GetCursorSprite();
	if (spr)
		edit->SetCursor( spr );
	else
		return RuntimeError( "Cursor BAM not found" );

	int ret = GetControlIndex( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_Window_CreateScrollBar__doc,
"===== Window_CreateScrollBar =====\n\
\n\
**Prototype:** GemRB.CreateScrollBar (WindowIndex, ControlID, x, y, w, h, ResRef)\n\
\n\
**Metaclass Prototype:** CreateScrollBar (ControlID, x, y, w, h, ResRef)\n\
\n\
**Description:** Creates and adds a new ScrollBar to a Window.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the window control\n\
  * ControlID - the id of the new control\n\
  * x, y - position\
  * w - width\n\
  * h - height\n\
  * ResRef - the resref for the sprite set\n\
\n\
**Return value:** ControlIndex\n\
\n\
**See also:** [[guiscript:ScrollBar_SetDefaultScrollBar]], [[guiscript:Control_AttachScrollBar]]"
);

static PyObject* GemRB_Window_CreateScrollBar(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;
	char* resRef;
	Region rgn;

	// hidden parameters from CreateScrollBar decorator
	int cycle, up, upPr, down, downPr, trough, slider;

	if (!PyArg_ParseTuple( args, "iiiiiisiiiiiii", &WindowIndex, &ControlID, &rgn.x, &rgn.y,
			&rgn.w, &rgn.h, &resRef, &cycle, &up, &upPr, &down, &downPr, &trough, &slider )) {
		return AttributeError( GemRB_Window_CreateScrollBar__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}

	AnimationFactory* af = ( AnimationFactory* )
	gamedata->GetFactoryResource( resRef, IE_BAM_CLASS_ID, IE_NORMAL );
	if (!af) {
		char tmpstr[24];

		snprintf(tmpstr,sizeof(tmpstr),"%s BAM not found", resRef);
		return RuntimeError( tmpstr );
	}

	Sprite2D* images[IE_SCROLLBAR_IMAGE_COUNT];
	images[IE_GUI_SCROLLBAR_UP_UNPRESSED] = af->GetFrame( up, cycle );
	images[IE_GUI_SCROLLBAR_UP_PRESSED] = af->GetFrame( upPr, cycle );
	images[IE_GUI_SCROLLBAR_DOWN_UNPRESSED] = af->GetFrame( down, cycle );
	images[IE_GUI_SCROLLBAR_DOWN_PRESSED] = af->GetFrame( downPr, cycle );
	images[IE_GUI_SCROLLBAR_TROUGH] = af->GetFrame( trough, cycle );
	images[IE_GUI_SCROLLBAR_SLIDER] = af->GetFrame( slider, cycle );

	ScrollBar* sb = new ScrollBar(rgn, images);
	sb->ControlID = ControlID;
	win->AddControl( sb );

	int ret = GetControlIndex( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	win->Link(sb->ControlID, -1);

	return PyInt_FromLong( ret );
}


PyDoc_STRVAR( GemRB_Window_CreateButton__doc,
"===== Window_CreateButton =====\n\
\n\
**Prototype:** GemRB.CreateButton (WindowIndex, ControlID, x, y, w, h)\n\
\n\
**Metaclass Prototype:** CreateButton (ControlID, x, y, w, h)\n\
\n\
**Description:** Creates and adds a new Button to a Window.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the value returned from LoadWindow\n\
  * ControlID - the new control will be available via this controlID\n\
  * x, y, w, h - X position, Y position, Width and Height of the control\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_CreateLabel]], [[guiscript:Window_CreateMapControl]], [[guiscript:Window_CreateWorldMapControl]], [[guiscript:Window_CreateTextEdit]]"
);

static PyObject* GemRB_Window_CreateButton(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;
	Region rgn;

	if (!PyArg_ParseTuple( args, "iiiiii", &WindowIndex, &ControlID, &rgn.x, &rgn.y,
			&rgn.w, &rgn.h )) {
		return AttributeError( GemRB_Window_CreateButton__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}

	Button* btn = new Button(rgn);
	btn->ControlID = ControlID;
	win->AddControl( btn );

	int ret = GetControlIndex( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_TextEdit_SetBackground__doc,
"===== TextEdit_SetBackground =====\n\
\n\
**Prototype:** _GemRB.TextEdit_SetBackground (WindowIndex, ControlIndex, ResRef)\n\
\n\
**Metaclass Prototype:** SetBackground (ResRef)\n\
\n\
**Description:** Sets the background MOS for a TextEdit control.\n\
\n\
**Parameters:** \n\
  * WindowIndex - the window control id\n\
  * ControlID - the id of the target control\n\
  * ResRef - MOS to use\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetMOS]], [[guiscript:Button_SetBAM]], [[guiscript:Button_SetPicture]], [[guiscript:Button_SetSprites]]"
);

static PyObject* GemRB_TextEdit_SetBackground(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char *ResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex,&ResRef) ) {
		return AttributeError( GemRB_TextEdit_SetBackground__doc );
	}
	TextEdit* te = ( TextEdit* ) GetControl(WindowIndex, ControlIndex, IE_GUI_EDIT);
	if (!te) {
		return NULL;
	}

	if (ResRef[0]) {
		ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(ResRef);
		if (im == nullptr) {
			return RuntimeError("Picture resource not found!\n");
		}

		Sprite2D* Picture = im->GetSprite2D();
		if (Picture == NULL) {
			return RuntimeError("Failed to acquire the picture!\n");
		}
		te->SetBackGround(Picture);
	} else {
		te->SetBackGround(NULL);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetSprites__doc,
"===== Button_SetSprites =====\n\
\n\
**Prototype:** GemRB.SetButtonSprites (WindowIndex, ControlIndex, ResRef, Cycle, UnpressedFrame, PressedFrame, SelectedFrame, DisabledFrame)\n\
\n\
**Metaclass Prototype:** SetSprites (ResRef, Cycle, UnpressedFrame, PressedFrame, SelectedFrame, DisabledFrame)\n\
\n\
**Description:** Sets the Button's images. You can disable the images by \n\
setting the IE_GUI_BUTTON_NO_IMAGE flag on the control.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * ResRef - a .bam animation resource (.bam resref)\n\
  * Cycle - the cycle of the .bam from which all frames of this button will come\n\
  * UnpressedFrame - the frame which will be displayed by default\n\
  * PressedFrame - the frame which will be displayed when the button is pressed \n\
  * SelectedFrame - this is for selected checkboxes\n\
  * DisabledFrame - this is for inactivated buttons\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetFlags]], [[guiscript:Button_SetBAM]], [[guiscript:Button_SetPicture]]"
);

static PyObject* GemRB_Button_SetSprites(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, cycle, unpressed, pressed, selected,
		disabled;
	char *ResRef;

	if (!PyArg_ParseTuple( args, "iisiiiii", &WindowIndex, &ControlIndex,
			&ResRef, &cycle, &unpressed, &pressed, &selected, &disabled )) {
		return AttributeError( GemRB_Button_SetSprites__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		btn->SetImage( BUTTON_IMAGE_NONE, NULL );
		Py_RETURN_NONE;
	}

	AnimationFactory* af = ( AnimationFactory* )
		gamedata->GetFactoryResource( ResRef,
				IE_BAM_CLASS_ID, IE_NORMAL );
	if (!af) {
		char tmpstr[24];

		snprintf(tmpstr,sizeof(tmpstr),"%s BAM not found", ResRef);
		return RuntimeError( tmpstr );
	}
	Sprite2D *tspr = af->GetFrame(unpressed, (unsigned char)cycle);
	btn->SetImage( BUTTON_IMAGE_UNPRESSED, tspr );
	tspr = af->GetFrame( pressed, (unsigned char) cycle);
	btn->SetImage( BUTTON_IMAGE_PRESSED, tspr );
	tspr = af->GetFrame( selected, (unsigned char) cycle);
	btn->SetImage( BUTTON_IMAGE_SELECTED, tspr );
	tspr = af->GetFrame( disabled, (unsigned char) cycle);
	btn->SetImage( BUTTON_IMAGE_DISABLED, tspr );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetOverlay__doc,
"===== Button_SetOverlay =====\n\
\n\
**Prototype:** GemRB.SetButtonOverlay (WindowIndex, ControlIndex, Current, Max, r,g,b,a, r,g,b,a)\n\
\n\
**Metaclass Prototype:** SetOverlay (ratio, r1,g1,b1,a1, r2,g2,b2,a2)\n\
\n\
**Description:** Sets ratio (0-1.0) of height to which button picture will \n\
be overlaid in a different colour. The colour will fade from the first rgba \n\
values to the second.\n\
\n\
**Parameters:** \n\
  * Window, Button - the control's reference\n\
  * ClippingRatio  - a floating point value from the 0-1 interval\n\
  * rgba1          - source colour\n\
  * rgba2          - target colour\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetPictureClipping]]"
);

static PyObject* GemRB_Button_SetOverlay(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	double Clipping;
	int r1,g1,b1,a1;
	int r2,g2,b2,a2;

	if (!PyArg_ParseTuple( args, "iidiiiiiiii", &WindowIndex, &ControlIndex,
		&Clipping, &r1, &g1, &b1, &a1, &r2, &g2, &b2, &a2)) {
		return AttributeError( GemRB_Button_SetOverlay__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	const Color src = { (ieByte) r1, (ieByte) g1, (ieByte) b1, (ieByte) a1 };
	const Color dest = { (ieByte) r2, (ieByte) g2, (ieByte) b2, (ieByte) a2 };

	if (Clipping<0.0) Clipping = 0.0;
	else if (Clipping>1.0) Clipping = 1.0;
	//can't call clipping, because the change of ratio triggers color change
	btn->SetHorizontalOverlay(Clipping, src, dest);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetBorder__doc,
"===== Button_SetBorder =====\n\
\n\
**Prototype:** GemRB.SetButtonBorder (WindowIndex, ControlIndex, BorderIndex, dx1, dy1, dx2, dy2, R, G, B, A, [enabled, filled])\n\
\n\
**Metaclass Prototype:** SetBorder (BorderIndex, dx1, dy1, dx2, dy2, R, G, B, A, [enabled, filled])\n\
\n\
**Description:** Sets border/frame/overlay parameters for a button. This \n\
command can be used for drawing a border around a button, or to overlay \n\
it with a tint (like with unusable or unidentified item's icons).\n\
\n\
**Parameters:** \n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * BorderIndex - 0, 1 or 2\n\
  * dx1,dy1 - Upper left corner\n\
  * dx2,dy2 - Offset from the lower right corner\n\
  * RGBA - red,green,blue,opacity components of the border colour\n\
  * enabled - 1 means enable it immediately\n\
  * filled - 1 means draw it filled (overlays)\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
  GemRB.SetButtonBorder (Window, Icon, 0,  0, 0, 0, 0,  0, 0, 0, 160,  0, 1)\n\
Not known spells are drawn darkened (the whole button will be overlaid).\n\
\n\
  Button.SetBorder (FRAME_PC_SELECTED, 1, 1, 2, 2, 0, 255, 0, 255)\n\
This will draw a green frame around the portrait.\n\
\n\
**See also:** [[guiscript:Button_EnableBorder]]"
);

static PyObject* GemRB_Button_SetBorder(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, BorderIndex, dx1, dy1, dx2, dy2, enabled = 0, filled = 0;
	int r, g, b, a;

	if (!PyArg_ParseTuple( args, "iiiiiiiiiii|ii", &WindowIndex, &ControlIndex,
		&BorderIndex, &dx1, &dy1, &dx2, &dy2, &r, &g, &b, &a, &enabled, &filled)) {
		return AttributeError( GemRB_Button_SetBorder__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	const Color color = { (ieByte) r, (ieByte) g, (ieByte) b, (ieByte) a };
	btn->SetBorder( BorderIndex, dx1, dy1, dx2, dy2, color, (bool)enabled, (bool)filled );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_EnableBorder__doc,
"===== Button_EnableBorder =====\n\
\n\
**Prototype:** GemRB.EnableButtonBorder (WindowIndex, ControlIndex, BorderIndex, enabled)\n\
\n\
**Metaclass Prototype:** EnableBorder (BorderIndex, enabled)\n\
\n\
**Description:** Enable or disable specified button border/frame/overlay.\n\
\n\
**Parameters:** \n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * BorderIndex - 0, 1 or 2\n\
  * enabled - boolean, true enables the border\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetBAM]], [[guiscript:Button_SetFlags]], [[guiscript:Button_SetBorder]]"
);

static PyObject* GemRB_Button_EnableBorder(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, BorderIndex, enabled;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex,
			&BorderIndex, &enabled)) {
		return AttributeError( GemRB_Button_EnableBorder__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	btn->EnableBorder( BorderIndex, (bool)enabled );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetFont__doc,
"===== Button_SetFont =====\n\
\n\
**Prototype:** GemRB.SetButtonFont (WindowIndex, ControlIndex, FontResRef)\n\
\n\
**Metaclass Prototype:** SetFont (FontResRef)\n\
\n\
**Description:** Sets font used for drawing button text.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex  - the control's reference\n\
  * FontResref - a .bam resref which must be listed in fonts.2da\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_CreateLabel]]"
);

static PyObject* GemRB_Button_SetFont(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char *FontResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex,
			&FontResRef)) {
		return AttributeError( GemRB_Button_SetFont__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	btn->SetFont( core->GetFont( FontResRef ));

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetAnchor__doc,
"===== Button_SetAnchor =====\n\
\n\
**Prototype:** GemRB.SetButtonAnchor (WindowIndex, ControlIndex, x, y)\n\
\n\
**Metaclass Prototype:** SetAnchor (x, y)\n\
\n\
**Description:** Sets explicit anchor point used for drawing button label.\n\
\n\
**Parameters:** \n\
  * WindowIndex, ControlIndex - control IDs\n\
  * x, y - anchor position \n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetPushOffset]]"
);

static PyObject* GemRB_Button_SetAnchor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, x, y;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &x, &y)) {
		return AttributeError( GemRB_Button_SetAnchor__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	btn->SetAnchor(x, y);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetPushOffset__doc,
"===== Button_SetPushOffset =====\n\
\n\
**Prototype:** GemRB.SetButtonPushOffset (WindowIndex, ControlIndex, x, y)\n\
\n\
**Metaclass Prototype:** SetPushOffset (x, y)\n\
\n\
**Description:** Sets the amount pictures and label move on button press.\n\
\n\
**Parameters:** \n\
  * WindowIndex, ControlIndex - control IDs\n\
  * x, y - anchor position \n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetAnchor]]"
);

static PyObject* GemRB_Button_SetPushOffset(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, x, y;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &x, &y)) {
		return AttributeError( GemRB_Button_SetPushOffset__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	btn->SetPushOffset(x, y);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetTextColor__doc,
"===== Button_SetTextColor =====\n\
\n\
**Prototype:** GemRB.SetButtonTextColor (WindowIndex, ControlIndex, red, green, blue[, invert=0])\n\
\n\
**Metaclass Prototype:** SetTextColor (red, green, blue[, invert=0])\n\
\n\
**Description:** Sets the text color of a Button control. Invert is used \n\
for fonts with swapped background and text colors.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * red, green, blue - the rgb color values\n\
  * invert - swap background and text colors?\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Label_SetUseRGB]], [[guiscript:Label_SetTextColor]]"
);

static PyObject* GemRB_Button_SetTextColor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, swap = 0;
	int r, g, b;

	if (!PyArg_ParseTuple( args, "iiiii|i", &WindowIndex, &ControlIndex, &r, &g, &b, &swap )) {
		return AttributeError( GemRB_Button_SetTextColor__doc );
	}

	Button* but = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!but) {
		return NULL;
	}

	const Color fore = { (ieByte) r, (ieByte) g, (ieByte) b, 0}, back = {0, 0, 0, 0};

	// FIXME: swap is a hack for fonts which apparently have swapped f & B
	// colors. Maybe it depends on need_palette?
	if (! swap)
		but->SetTextColor( fore, back );
	else
		but->SetTextColor( back, fore );


	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Window_DeleteControl__doc,
"===== Window_DeleteControl =====\n\
\n\
**Prototype:** GemRB.DeleteControl (WindowIndex, ControlID)\n\
\n\
**Metaclass Prototype:** DeleteControl (ControlID)\n\
\n\
**Description:** Deletes a control from a Window.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the return value of a previous LoadWindow call.\n\
  * ControlID   - a control ID, see the .chu file specification\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_GetControl]]"
);

static PyObject* GemRB_Window_DeleteControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlID)) {
		return AttributeError( GemRB_Window_DeleteControl__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	int CtrlIndex = GetControlIndex( WindowIndex, ControlID );
	if (CtrlIndex != -1) {
		delete win->RemoveControl(CtrlIndex);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_AddNewArea__doc,
"===== AddNewArea =====\n\
\n\
**Prototype:** GemRB.AddNewArea (2daresref)\n\
\n\
**Description:**  Adds the extension areas to the game. \n\
Used in bg2 with xnewarea.2da for ToB.\n\
\n\
**Parameters:** \n\
  * 2daresref - 2da table with new area mappings\n\
\n\
**Return value:** N/A\n\
\n\
**See also:**"
);

static PyObject* GemRB_AddNewArea(PyObject * /*self*/, PyObject* args)
{
	const char *resref;

	if (!PyArg_ParseTuple( args, "s", &resref)) {
		return AttributeError( GemRB_AddNewArea__doc );
	}

	AutoTable newarea(resref);
	if (!newarea) {
		return RuntimeError( "2da not found!\n");
	}

	WorldMap *wmap = core->GetWorldMap();
	if (!wmap) {
		return RuntimeError( "no worldmap loaded!");
	}

	const char *enc[5];
	int k;
	int links[4];
	int indices[4];
	int rows = newarea->GetRowCount();
	for(int i=0;i<rows;i++) {
		const char *area   = newarea->QueryField(i,0);
		const char *script = newarea->QueryField(i,1);
		int flags          = atoi(newarea->QueryField(i,2));
		int icon           = atoi(newarea->QueryField(i,3));
		int locx           = atoi(newarea->QueryField(i,4));
		int locy           = atoi(newarea->QueryField(i,5));
		int label          = atoi(newarea->QueryField(i,6));
		int name           = atoi(newarea->QueryField(i,7));
		const char *ltab   = newarea->QueryField(i,8);
		links[WMP_NORTH]   = atoi(newarea->QueryField(i,9));
		links[WMP_EAST]    = atoi(newarea->QueryField(i,10));
		links[WMP_SOUTH]   = atoi(newarea->QueryField(i,11));
		links[WMP_WEST]    = atoi(newarea->QueryField(i,12));
		//this is the number of links in the 2da, we don't need it
		int linksto        = atoi(newarea->QueryField(i,13));

		unsigned int local = 0;
		int linkcnt = wmap->GetLinkCount();
		for (k=0;k<4;k++) {
			indices[k] = linkcnt;
			linkcnt += links[k];
			local += links[k];
		}
		unsigned int total = linksto+local;

		AutoTable newlinks(ltab);
		if (!newlinks || total != newlinks->GetRowCount() ) {
			return RuntimeError( "invalid links 2da!");
		}

		WMPAreaEntry *entry = wmap->GetNewAreaEntry();
		strnuprcpy(entry->AreaName, area, 8);
		strnuprcpy(entry->AreaResRef, area, 8);
		strnuprcpy(entry->AreaLongName, script, 32);
		entry->SetAreaStatus(flags, OP_SET);
		entry->IconSeq = icon;
		entry->X = locx;
		entry->Y = locy;
		entry->LocCaptionName = label;
		entry->LocTooltipName = name;
		memset(entry->LoadScreenResRef, 0, sizeof(ieResRef));
		memcpy(entry->AreaLinksIndex, indices, sizeof(entry->AreaLinksIndex) );
		memcpy(entry->AreaLinksCount, links, sizeof(entry->AreaLinksCount) );

		int thisarea = wmap->GetEntryCount();
		wmap->AddAreaEntry(entry);
		for (unsigned int j=0;j<total;j++) {
			const char *larea = newlinks->QueryField(j,0);
			int lflags        = atoi(newlinks->QueryField(j,1));
			const char *ename = newlinks->QueryField(j,2);
			int distance      = atoi(newlinks->QueryField(j,3));
			int encprob       = atoi(newlinks->QueryField(j,4));
			for(k=0;k<5;k++) {
				enc[k]    = newlinks->QueryField(i,5+k);
			}
			int linktodir     = atoi(newlinks->QueryField(j,10));

			unsigned int areaindex;
			WMPAreaEntry *oarea = wmap->GetArea(larea, areaindex);
			if (!oarea) {
				//blabla
				return RuntimeError("cannot establish area link!");
			}
			WMPAreaLink *link = new WMPAreaLink();
			strnuprcpy(link->DestEntryPoint, ename, 32);
			link->DistanceScale = distance;
			link->DirectionFlags = lflags;
			link->EncounterChance = encprob;
			for(k=0;k<5;k++) {
				if (enc[k][0]=='*') {
					memset(link->EncounterAreaResRef[k],0,sizeof(ieResRef));
				} else {
					strnuprcpy(link->EncounterAreaResRef[k], enc[k], 8);
				}
			}

			//first come the local links, then 'links to' this area
			//local is total-linksto
			if (j<local) {
				link->AreaIndex = thisarea;
				//linktodir may need translation
				wmap->InsertAreaLink(areaindex, linktodir, link);
				delete link;
			} else {
				link->AreaIndex = areaindex;
				wmap->AddAreaLink(link);
			}
		}
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_WorldMap_AdjustScrolling__doc,
"===== WorldMap_AdjustScrolling =====\n\
\n\
**Prototype:** GemRB.AdjustScrolling (WindowIndex, ControlIndex, x, y)\n\
\n\
**Metaclass Prototype:** AdjustScrolling (x, y)\n\
\n\
**Description:** Sets the scrolling offset of a WorldMapControl.\n\
\n\
**Parameters:** \n\
  * WindowIndex - the windows's reference\n\
  * ControlIndex - the control's reference\n\
  * x, y - scrolling offset values\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    # northeast\n\
    Button = GemRB.GetControl (Window, 9)\n\
    Button.SetEvent (IE_GUI_BUTTON_ON_PRESS, 'MapNE')\n\
...\n\
\n\
**def MapNE():**\n\
    WorldMapControl.AdjustScrolling (10, -10)\n\
    return\
The above lines set up a button event. When the button is pressed the worldmap will be shifted in the northeastern direction.\n\
\n\
**See also:** [[guiscript:Window_CreateWorldMapControl]"
);

static PyObject* GemRB_WorldMap_AdjustScrolling(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, x, y;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &x, &y )) {
		return AttributeError( GemRB_WorldMap_AdjustScrolling__doc );
	}

	core->AdjustScrolling( WindowIndex, ControlIndex, x, y );
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_CreateMovement__doc,
"===== CreateMovement =====\n\
\n\
**Prototype:** GemRB.CreateMovement (Area, Entrance[, Direction])\n\
\n\
**Description:** Moves some or all actors of the current area to the destination area.\n\
\n\
**Parameters:**\n\
  * Area - The area resource reference where the player(s) should arrive.\n\
  * Entrance - The area entrance in the destination area.\n\
  * Direction - The direction flag (from WMP) to use if the entrance doesn't exist.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:WorldMap_GetDestinationArea]]\n\
"
);

static PyObject* GemRB_CreateMovement(PyObject * /*self*/, PyObject* args)
{
	int everyone;
	char *area;
	char *entrance;
	int direction = 0;

	if (!PyArg_ParseTuple( args, "ss|i", &area, &entrance, &direction)) {
		return AttributeError( GemRB_CreateMovement__doc );
	}
	if (core->HasFeature(GF_TEAM_MOVEMENT) ) {
		everyone = CT_WHOLE;
	} else {
		everyone = CT_GO_CLOSER;
	}
	GET_GAME();

	GET_MAP();

	map->MoveToNewArea(area, entrance, (unsigned int)direction, everyone, NULL);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_UpdateWorldMap__doc,
"===== UpdateWorldMap =====\n\
\n\
**Prototype:** GemRB.UpdateWorldMap (ResRef, [AreaResRef])\n\
\n\
**Description:** Reloads the world map from ResRef. \n\
If AreaResRef is given only updates if that area is missing.\n\
\n\
**Parameters:**\n\
  * ResRef - worldmap resref to reload from\n\
  * AreaResRef - missing area resref (optional)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetWorldMap]]\n\
"
);

static PyObject* GemRB_UpdateWorldMap(PyObject * /*self*/, PyObject* args)
{
	char *wmResRef, *areaResRef = NULL;
	bool update = true;

	if (!PyArg_ParseTuple( args, "s|s", &wmResRef, &areaResRef)) {
		return AttributeError( GemRB_UpdateWorldMap__doc );
	}

	if (areaResRef != NULL) {
		unsigned int i;
		update = (core->GetWorldMap()->GetArea( areaResRef, i ) == NULL);
	}

	if (update)
		core->UpdateWorldMap(wmResRef);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_WorldMap_GetDestinationArea__doc,
"===== WorldMap_GetDestinationArea =====\n\
\n\
**Prototype:** GemRB.GetDestinationArea (WindowIndex, ControlID[, RndEncounter])\n\
\n\
**Metaclass Prototype:** GetDestinationArea ([RndEncounter])\n\
\n\
**Description:** Returns a dictionary of the selected area by the worldmap \n\
control. If the route is blocked, then Distance will return a negative \n\
value and Destination/Entrance won't be set. Random encounters could be \n\
optionally evaluated. If the random encounter flag is set, the random \n\
encounters will be evaluated too.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - designate a worldmap control\n\
  * RndEncounter - check for random encounters?\n\
\n\
**Return value:** Dictionary\n\
  * Target      - The target area selected by the player\n\
  * Distance    - The traveling distance, if it is negative, the way is blocked\n\
  * Destination - The area resource reference where the player arrives (if there was a random encounter, it differs from Target)\n\
  * Entrance    - The area entrance in the Destination area, it could be empty, in this casethe player should appear in middle of the area\n\
\n\
**See also:** [[guiscript:Window_CreateWorldMapControl]], [[guiscript:CreateMovement]], [[guiscript:accessing_gui_controls]]"
);

static PyObject* GemRB_WorldMap_GetDestinationArea(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int eval = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &WindowIndex, &ControlIndex, &eval)) {
		return AttributeError( GemRB_WorldMap_GetDestinationArea__doc );
	}

	WorldMapControl* wmc = (WorldMapControl *) GetControl(WindowIndex, ControlIndex, IE_GUI_WORLDMAP);
	if (!wmc) {
		return NULL;
	}
	//no area was pointed on
	if (!wmc->Area) {
		Py_RETURN_NONE;
	}
	WorldMap *wm = core->GetWorldMap();
	PyObject* dict = PyDict_New();
	//the area the user clicked on
	PyDict_SetItemString(dict, "Target", PyString_FromString (wmc->Area->AreaName) );
	PyDict_SetItemString(dict, "Destination", PyString_FromString (wmc->Area->AreaName) );

	if (!strnicmp(wmc->Area->AreaName, core->GetGame()->CurrentArea, 8)) {
		PyDict_SetItemString(dict, "Distance", PyInt_FromLong (-1) );
		return dict;
	}

	bool encounter;
	int distance;
	WMPAreaLink *wal = wm->GetEncounterLink(wmc->Area->AreaName, encounter);
	if (!wal) {
		PyDict_SetItemString(dict, "Distance", PyInt_FromLong (-1) );
		return dict;
	}
	PyDict_SetItemString(dict, "Entrance", PyString_FromString (wal->DestEntryPoint) );
	PyDict_SetItemString(dict, "Direction", PyInt_FromLong (wal->DirectionFlags) );
	distance = wm->GetDistance(wmc->Area->AreaName);

	if (eval) {
		wm->ClearEncounterArea();

		//evaluate the area the user will fall on in a random encounter
		if (encounter) {

			if(wal->EncounterChance>=100) {
				wal->EncounterChance-=100;
			}

			//bounty encounter
			ieResRef tmpresref;
			WMPAreaEntry *linkdest = wm->GetEntry(wal->AreaIndex);

			CopyResRef(tmpresref, linkdest->AreaResRef);
			if (core->GetGame()->RandomEncounter(tmpresref)) {
				displaymsg->DisplayConstantString(STR_AMBUSH, DMC_BG2XPGREEN);
				PyDict_SetItemString(dict, "Destination", PyString_FromString (tmpresref) );
				PyDict_SetItemString(dict, "Entrance", PyString_FromString ("") );
				distance = wm->GetDistance(linkdest->AreaResRef) - (wal->DistanceScale * 4 / 2);
				wm->SetEncounterArea(tmpresref, wal);
			} else {
				//regular random encounter, find a valid encounter area
				int i = RAND(0, 4);

				for(int j=0;j<5;j++) {
					const char *area = wal->EncounterAreaResRef[(i+j)%5];

					if (area[0]) {
						displaymsg->DisplayConstantString(STR_AMBUSH, DMC_BG2XPGREEN);
						PyDict_SetItemString(dict, "Destination", PyString_FromString (area) );
						//drop player in the middle of the map
						PyDict_SetItemString(dict, "Entrance", PyString_FromString ("") );
						PyDict_SetItemString(dict, "Direction", PyInt_FromLong (ADIRF_CENTER) );
						//only count half the distance of the final link
						distance = wm->GetDistance(linkdest->AreaResRef) - (wal->DistanceScale * 4 / 2);
						wm->SetEncounterArea(area, wal);
						break;
					}
				}
			}
		}
	}

	PyDict_SetItemString(dict, "Distance", PyInt_FromLong (distance));
	return dict;
}

PyDoc_STRVAR( GemRB_Window_CreateWorldMapControl__doc,
"===== Window_CreateWorldMapControl =====\n\
\n\
**Prototype:** GemRB.CreateWorldMapControl (WindowIndex, ControlID, x, y, w, h, direction[, font, recolor])\n\
\n\
**Metaclass Prototype:** CreateWorldMapControl (ControlID, x, y, w, h, direction[, font, recolor])\n\
\n\
**Description:** This command creates a special WorldMapControl, which is \n\
currently unavailable via .chu files. If WindowIndex and ControlID (not \n\
ControlIndex!) point to a valid control, it will replace that control with \n\
the WorldMapControl using the original control's dimensions (x,y,w,h are \n\
ignored).\n\
\n\
**Parameters:** \n\
  * WindowIndex - the value returned from LoadWindow\n\
  * ControlID   - the new control will be available via this controlID\n\
  * x,y,w,h     - X position, Y position, Width and Height of the control\n\
  * direction   - travel direction (-1 to ignore)\n\
  * font        - font used to display names of map locations\n\
  * recolor     - recolor map icons (bg1)\n\
\n\
**Return value:** N/A\n\
\n\
**Example:** \n\
     Window = GemRB.LoadWindow (0)\n\
     Window.CreateWorldMapControl (4, 0, 62, 640, 418, Travel, 'floattxt')\n\
     WorldMapControl = Window.GetControl (4)\n\
\n\
**See also:** [[guiscript:WorldMap_GetDestinationArea]], [[guiscript:CreateMovement]]"
);

static PyObject* GemRB_Window_CreateWorldMapControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, direction, recolor = 0;
	Region rgn;
	char *font=NULL;

	if (!PyArg_ParseTuple( args, "iiiiiii|si", &WindowIndex, &ControlID, &rgn.x,
			&rgn.y, &rgn.w, &rgn.h, &direction, &font, &recolor )) {
		return AttributeError( GemRB_Window_CreateWorldMapControl__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	int CtrlIndex = GetControlIndex( WindowIndex, ControlID );
	if (CtrlIndex != -1) {
		Control *ctrl = win->GetControl( CtrlIndex );
		rgn = ctrl->ControlFrame();
		//flags = ctrl->Value;
		delete win->RemoveControl( CtrlIndex );
	}

	WorldMapControl* wmap = new WorldMapControl(rgn, font?font:"", direction );
	wmap->ControlID = ControlID;
	wmap->SetOverrideIconPalette(recolor);
	win->AddControl( wmap );

	int ret = GetControlIndex( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_WorldMap_SetTextColor__doc,
"===== WorldMap_SetTextColor =====\n\
\n\
**Prototype:** GemRB.SetWorldMapTextColor (WindowIndex, ControlIndex, which, red, green, blue)\n\
\n\
**Metaclass Prototype:** SetTextColor (which, red, green, blue)\n\
\n\
**Description:** Sets the label colors of a WorldMap Control. 'which' \n\
selects the color affected.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the window control id\n\
  * ControlID - the id of the target control\n\
  * which - selects the color affected:\n\
    * IE_GUI_WMAP_COLOR_BACKGROUND\n\
    * IE_GUI_WMAP_COLOR_NORMAL - main text color\n\
    * IE_GUI_WMAP_COLOR_SELECTED - color of hovered on text\n\
    * IE_GUI_WMAP_COLOR_NOTVISITED - color of unvisited entries\n\
  * red - red value\n\
  * green - green value\n\
  * blue - blue value\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_WorldMap_SetTextColor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, which;
	int r, g, b, a;

	if (!PyArg_ParseTuple( args, "iiiiiii", &WindowIndex, &ControlIndex, &which, &r, &g, &b, &a )) {
		return AttributeError( GemRB_WorldMap_SetTextColor__doc );
	}

	WorldMapControl* wmap = ( WorldMapControl* ) GetControl( WindowIndex, ControlIndex, IE_GUI_WORLDMAP);
	if (!wmap) {
		return NULL;
	}

	const Color color = { (ieByte) r, (ieByte) g, (ieByte) b, (ieByte) a};
	wmap->SetColor( which, color );

	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_Window_CreateMapControl__doc,
"===== Window_CreateMapControl =====\n\
\n\
**Prototype:** GemRB.CreateMapControl (WindowIndex, ControlID, x, y, w, h, [LabelID, FlagResRef[, Flag2ResRef]])\n\
\n\
**Metaclass Prototype:** CreateMapControl (ControlID, x, y, w, h, [LabelID, FlagResRef [, Flag2ResRef]]\n\
\n\
**Description:**  Creates and adds a new Area Map Control to a Window. If \n\
WindowIndex and ControlID (not ControlIndex!) point to a valid control, it \n\
will replace that control with the MapControl using the original control's \n\
dimensions (x,y,w,h are ignored). It is possible to associate a variable \n\
with the MapControl, in this case, the associated variable will enable or \n\
disable mapnotes (you must supply a LabelID and the resources for the pins).\n\
\n\
**Parameters:**\n\
  * WindowIndex - the value returned from LoadWindow\n\
  * ControlID   - the new control will be available via this controlID\n\
  * x,y,w,h     - X position, Y position, Width and Height of the control\n\
  * LabelID     - associated control ID to display mapnotes, it must be a label\n\
  * FlagResRef  - Resource Reference for the pins, if no Flag2ResRef is given, this should be a .bam resref. If there is a second resref, then both must be .bmp.\n\
  * Flag2ResRef - the readonly mapnotes are marked by this .bam (red pin)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Control_SetVarAssoc]], [[guiscript:SetMapnote]]"
);

static PyObject* GemRB_Window_CreateMapControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;
	Region rgn;
	int LabelID = -1;
	char *Flag=NULL;
	char *Flag2=NULL;

	if (!PyArg_ParseTuple( args, "iiiiii|iss", &WindowIndex, &ControlID,
			&rgn.x, &rgn.y, &rgn.w, &rgn.h, &LabelID, &Flag, &Flag2))
	{
		return AttributeError( GemRB_Window_CreateMapControl__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	int CtrlIndex = GetControlIndex( WindowIndex, ControlID );
	if (CtrlIndex != -1) {
		Control *ctrl = win->GetControl( CtrlIndex );
		rgn = ctrl->ControlFrame();
		// do *not* delete the existing control, we want to replace
		// it in the sort order!
		//win->DelControl( CtrlIndex );
	}

	MapControl* map = new MapControl(rgn);
	map->ControlID = ControlID;

	if (LabelID >= 0) {
		CtrlIndex = GetControlIndex( WindowIndex, LabelID );
		Control* lc = win->GetControl( CtrlIndex );
		if (lc == NULL) {
			delete map;
			return RuntimeError("Cannot find label!");
		}
		map->LinkedLabel = lc;
	}

	if (Flag2) { //pst flavour
		map->convertToGame = false;

		ResourceHolder<ImageMgr> anim = GetResourceHolder<ImageMgr>(Flag);
		if (anim) {
			map->Flag[0] = anim->GetSprite2D();
		}
		ResourceHolder<ImageMgr> anim2 = GetResourceHolder<ImageMgr>(Flag2);
		if (anim2) {
			map->Flag[1] = anim2->GetSprite2D();
		}
	} else if (Flag) {
		AnimationFactory* af = ( AnimationFactory* )
			gamedata->GetFactoryResource( Flag,
					IE_BAM_CLASS_ID, IE_NORMAL );
		if (af) {
			for (int i=0;i<8;i++) {
				map->Flag[i] = af->GetFrame(0,i);
			}

		}
	}
	win->AddControl( map );

	int ret = GetControlIndex( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
}


PyDoc_STRVAR( GemRB_Control_SubstituteForControl__doc,
"===== Control_SubstituteForControl =====\n\
\n\
**Prototype:** GemRB.SubstituteForControl (WindowIndex, ControlIndex, TWindowIndex, TControlIndex)\n\
\n\
**Metaclass Prototype:** SubstituteForControl (TControl)\n\
\n\
**Description:** Substitute a control with another control (even of \n\
different type), keeping its ControlID, size and scrollbar.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the value returned from LoadWindow for the substitute control\n\
  * ControlID - the substitute control's controlID\n\
  * TWindowIndex - the value returned from LoadWindow for the target control\n\
  * TControlID - the old control's controlID\n\
  * TControl - target control object\n\
\n\
**Return value:** The new ControlID"
);

static PyObject* GemRB_Control_SubstituteForControl(PyObject * /*self*/, PyObject* args)
{
	int SubWindowIndex, SubControlID;
	int WindowIndex, ControlID;

	if (!PyArg_ParseTuple( args, "iiii", &SubWindowIndex, &SubControlID, &WindowIndex, &ControlID)) {
		return AttributeError( GemRB_Control_SubstituteForControl__doc );
	}

	int subIdx = SubControlID;//GetControlIndex(SubWindowIndex, SubControlID);
	int targetIdx = ControlID;//GetControlIndex(WindowIndex, ControlID);
	Control* substitute = GetControl(SubWindowIndex, subIdx, -1);
	Control* target = GetControl(WindowIndex, targetIdx, -1);
	if (!substitute || !target) {
		return RuntimeError("Cannot find control!");
	}
	substitute->Owner->RemoveControl(subIdx);
	Window* targetWin = target->Owner;
	substitute->SetControlFrame(target->ControlFrame());

	substitute->ControlID = target->ControlID;
	ieDword sbid = (target->sb) ? target->sb->ControlID : -1;
	targetWin->AddControl( substitute ); // deletes target!
	targetWin->Link( sbid, substitute->ControlID );

	PyObject* ctrltuple = Py_BuildValue("(ii)", WindowIndex, substitute->ControlID);
	PyObject* ret = GemRB_Window_GetControl(NULL, ctrltuple);
	Py_DECREF(ctrltuple);
	return ret;
}

PyDoc_STRVAR( GemRB_Control_SetPos__doc,
"===== Control_SetPos =====\n\
\n\
**Prototype:** GemRB.SetControlPos (WindowIndex, ControlIndex, X, Y)\n\
\n\
**Metaclass Prototype:** SetPos (X, Y)\n\
\n\
**Description:** Moves a Control.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * X,Y - the new position of the control relative to the window\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Control_SetSize]], [[guiscript:accessing_gui_controls]]"
);

static PyObject* GemRB_Control_SetPos(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, X, Y;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &X, &Y )) {
		return AttributeError( GemRB_Control_SetPos__doc );
	}

	Control* ctrl = GetControl(WindowIndex, ControlIndex, -1);
	if (!ctrl) {
		return NULL;
	}

	ctrl->XPos = X;
	ctrl->YPos = Y;

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Control_GetRect__doc,
"===== Control_GetRect =====\n\
\n\
**Prototype:** GemRB.GetRect (WindowIndex, ControlIndex)\n\
\n\
**Metaclass Prototype:** GetRect ()\n\
\n\
**Description:** Returns a dict with the control position and size.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
\n\
**Return value:** dict\n\
\n\
**See also:** [[guiscript:Control_SetSize]], [[guiscript:accessing_gui_controls]]"
);

static PyObject* GemRB_Control_GetRect(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlIndex)) {
		return AttributeError( GemRB_Control_GetRect__doc );
	}

	Control* ctrl = GetControl(WindowIndex, ControlIndex, -1);
	if (!ctrl) {
		return NULL;
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "X", PyInt_FromLong( ctrl->XPos ));
	PyDict_SetItemString(dict, "Y", PyInt_FromLong( ctrl->YPos ));
	PyDict_SetItemString(dict, "Width", PyInt_FromLong( ctrl->Width ));
	PyDict_SetItemString(dict, "Height", PyInt_FromLong( ctrl->Height ));
	return dict;
}

PyDoc_STRVAR( GemRB_Control_SetSize__doc,
"===== Control_SetSize =====\n\
\n\
**Prototype:** GemRB.SetControlSize (WindowIndex, ControlIndex, Width, Height)\n\
\n\
**Metaclass Prototype:** SetSize (Width, Height)\n\
\n\
**Description:** Resizes a Control.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex  - the control's reference\n\
  * Width, Height - the new dimensions of the control\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Control_SetPos]], [[guiscript:accessing_gui_controls]]"
);

static PyObject* GemRB_Control_SetSize(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Width, Height;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &Width,
			&Height )) {
		return AttributeError( GemRB_Control_SetSize__doc );
	}

	Control* ctrl = GetControl(WindowIndex, ControlIndex, -1);
	if (!ctrl) {
		return NULL;
	}

	ctrl->Width = Width;
	ctrl->Height = Height;

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Label_SetFont__doc,
"===== Label_SetFont =====\n\
\n\
**Prototype:** GemRB.SetLabelFont (WindowIndex, ControlIndex, FontResRef)\n\
\n\
**Metaclass Prototype:** SetFont (FontResRef)\n\
\n\
**Description:** Sets font used for drawing the label.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex  - the control's reference\n\
  * FontResref - a .bam resref which must be listed in fonts.2da\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_CreateLabel]]"
);

static PyObject* GemRB_Label_SetFont(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char *FontResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex,
			&FontResRef)) {
		return AttributeError( GemRB_Label_SetFont__doc );
	}

	Label *lbl = (Label *) GetControl(WindowIndex, ControlIndex, IE_GUI_LABEL);
	if (!lbl) {
		return NULL;
	}

	lbl->SetFont( core->GetFont( FontResRef ));

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Label_SetUseRGB__doc,
"===== Label_SetUseRGB =====\n\
\n\
**Prototype:** GemRB.SetLabelUseRGB (WindowIndex, ControlIndex, status)\n\
\n\
**Metaclass Prototype:** SetUseRGB (status)\n\
\n\
**Description:** Sets a Label control to use the colors coming from the \n\
Font. If the font has its own color, you must set this. If a label was set \n\
to not use the Font's colors you can alter its color by SetLabelTextColor().\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * status                    - boolean\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Label_SetTextColor]]"
);

static PyObject* GemRB_Label_SetUseRGB(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, status;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &status )) {
		return AttributeError( GemRB_Label_SetUseRGB__doc );
	}

	Label* lab = (Label *) GetControl(WindowIndex, ControlIndex, IE_GUI_LABEL);
	if (!lab) {
		return NULL;
	}

	lab->useRGB = ( status != 0 );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GameSetPartySize__doc,
"===== GameSetPartySize =====\n\
\n\
**Prototype:** GemRB.GameSetPartySize (Size)\n\
\n\
**Description:** Sets the maximum number of PCs. This command works only \n\
after a LoadGame(). If the party size was set to 0, then it means unlimited size.\n\
\n\
**Parameters:**\n\
 * Size - must be 0-10; 7 or more requires the 10pp mod for existing games\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetPartySize]]\n\
"
);

static PyObject* GemRB_GameSetPartySize(PyObject * /*self*/, PyObject* args)
{
	int Flags;

	if (!PyArg_ParseTuple( args, "i", &Flags )) {
		return AttributeError( GemRB_GameSetPartySize__doc );
	}

	GET_GAME();

	game->SetPartySize( Flags );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GameSetProtagonistMode__doc,
"===== GameSetProtagonistMode =====\n\
\n\
**Prototype:** GemRB.GameSetProtagonistMode (Mode)\n\
\n\
**Description:** Sets how the game handles the game over event. This action \n\
works only after a LoadGame().\n\
\n\
**Parameters:**\n\
  *  Mode:\n\
    * 0 no check\n\
    * 1 game over when protagonist dies\n\
    * 2 game over when whole party is dead\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadGame]], [[guiscript:GameSetPartySize]]\n\
"
);

static PyObject* GemRB_GameSetProtagonistMode(PyObject * /*self*/, PyObject* args)
{
	int Flags;

	if (!PyArg_ParseTuple( args, "i", &Flags )) {
		return AttributeError( GemRB_GameSetProtagonistMode__doc );
	}

	GET_GAME();

	game->SetProtagonistMode( Flags );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GameGetExpansion__doc,
"===== GameGetExpansion =====\n\
\n\
**Prototype:** GemRB.GameGetExpansion ()\n\
\n\
**Description:** Gets the expansion mode.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** int\n\
\n\
**See also:** [[guiscript:GameSetExpansion]]"
);
static PyObject* GemRB_GameGetExpansion(PyObject * /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyInt_FromLong( game->Expansion );
}

PyDoc_STRVAR( GemRB_GameSetExpansion__doc,
"===== GameSetExpansion =====\n\
\n\
**Prototype:** GemRB.GameSetExpansion (mode)\n\
\n\
**Description:** Sets the expansion mode. Most games were created in a two \n\
in one and could start the game as expansion only (or transfer to the expansion). \n\
This command selects between these two modes.\n\
\n\
**Parameters:**\n\
  * mode - 0 or 1\n\
\n\
**Return value:** false if already set\n\
\n\
**See also:** [[guiscript:LoadGame]], [[guiscript:GameGetExpansion]], GameType(variable)\n\
"
);

static PyObject* GemRB_GameSetExpansion(PyObject * /*self*/, PyObject* args)
{
	int value;

	if (!PyArg_ParseTuple( args, "i", &value )) {
		return AttributeError( GemRB_GameSetExpansion__doc );
	}

	GET_GAME();

	if ((unsigned int) value<=game->Expansion) {
		Py_INCREF( Py_False );
		return Py_False;
	}
	game->SetExpansion(value);
	Py_INCREF( Py_True );
	return Py_True;
}

PyDoc_STRVAR( GemRB_GameSetScreenFlags__doc,
"===== GameSetScreenFlags =====\n\
\n\
**Prototype:** GemRB.GameSetScreenFlags (Bits, Operation)\n\
\n\
**Description:** Sets the Display Flags of the main game screen (pane \n\
status, dialog textarea size).\n\
\n\
**Parameters:**\n\
  * Bits - This depends on the game. The lowest 2 bits are the message window size\n\
  * Operation - The usual bit operations\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_SetVisible]], [[guiscript:bit_operation]]"
);

static PyObject* GemRB_GameSetScreenFlags(PyObject * /*self*/, PyObject* args)
{
	int Flags, Operation;

	if (!PyArg_ParseTuple( args, "ii", &Flags, &Operation )) {
		return AttributeError( GemRB_GameSetScreenFlags__doc );
	}
	if (Operation < OP_SET || Operation > OP_NAND) {
		Log(ERROR, "GUIScript", "Syntax Error: operation must be 0-4");
		return NULL;
	}

	GET_GAME();

	game->SetControlStatus( Flags, Operation );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GameControlSetScreenFlags__doc,
"===== GameControlSetScreenFlags =====\n\
\n\
**Prototype:** GemRB.GameControlSetScreenFlags (Mode, Operation)\n\
\n\
**Description:** Sets screen flags, like cutscene mode, disable mouse, etc. \n\
Don't confuse it with the saved screen flags set by GameSetScreenFlags.\n\
\n\
**Parameters:**\n\
  * Mode - bitfield:\n\
    * 1 - disable mouse\n\
    * 2 - center on actor (one time)\n\
    * 4 - center on actor (always)\n\
    * 8 - enable gui\n\
    * 16 - lock scroll\n\
    * 32 - cutscene (no action queueing)\n\
  * Operation - bit operation to use\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GameSetScreenFlags]], [[guiscript:bit_operation]]\n\
"
);

static PyObject* GemRB_GameControlSetScreenFlags(PyObject * /*self*/, PyObject* args)
{
	int Flags, Operation;

	if (!PyArg_ParseTuple( args, "ii", &Flags, &Operation )) {
		return AttributeError( GemRB_GameControlSetScreenFlags__doc );
	}
	if (Operation < OP_SET || Operation > OP_NAND) {
		return AttributeError("Operation must be 0-4\n");
	}

	GET_GAMECONTROL();

	gc->SetScreenFlags( Flags, Operation );

	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_GameControlSetTargetMode__doc,
"===== GameControlSetTargetMode =====\n\
\n\
**Prototype:** GemRB.GameControlSetTargetMode (Mode[, Types])\n\
\n\
**Description:** Sets the targeting mode of the main game screen control \n\
(attack, cast spell, ...) and type of target (ally, enemy and/or neutral; \n\
all by default). Changes the cursor.\n\
\n\
**Parameters:**\n\
  *  Mode\n\
    * 0 TARGET_MODE_NONE\n\
    * 1 TARGET_MODE_TALK\n\
    * 2 TARGET_MODE_ATTACK (also for bashing)\n\
    * 4 TARGET_MODE_CAST\n\
    * 8 TARGET_MODE_DEFEND\n\
    * 16 TARGET_MODE_PICK\n\
  * (target) Types - bitfield:\n\
    * GA_SELECT (selectable actor)\n\
    * GA_NO_DEAD\n\
    * GA_POINT - any point could be selected (area effect)\n\
    * GA_NO_HIDDEN\n\
    * GA_NO_ALLY\n\
    * GA_NO_ENEMY\n\
    * GA_NO_NEUTRAL\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GameControlSetScreenFlags]], [[guiscript:GameControlGetTargetMode]]\n\
"
);

static PyObject* GemRB_GameControlSetTargetMode(PyObject * /*self*/, PyObject* args)
{
	int Mode;
	int Types = GA_SELECT | GA_NO_DEAD | GA_NO_HIDDEN | GA_NO_UNSCHEDULED;

	if (!PyArg_ParseTuple( args, "i|i", &Mode, &Types )) {
		return AttributeError( GemRB_GameControlSetTargetMode__doc );
	}

	GET_GAMECONTROL();

	//target mode is only the low bits (which is a number)
	gc->SetTargetMode(Mode&GA_ACTION);
	//target type is all the bits
	gc->target_types = (Mode&GA_ACTION)|Types;
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GameControlGetTargetMode__doc,
"===== GameControlGetTargetMode =====\n\
\n\
**Prototype:** GemRB.GameControlGetTargetMode ()\n\
\n\
**Description:** Returns the current target mode.\n\
\n\
**Return value:** numeric (see GameControlSetTargetMode)\n\
\n\
**See also:** [[guiscript:GameControlSetTargetMode]], [[guiscript:GameControlSetScreenFlags]]"
);

static PyObject* GemRB_GameControlGetTargetMode(PyObject * /*self*/, PyObject* /*args*/)
{
	GET_GAMECONTROL();

	return PyInt_FromLong(gc->GetTargetMode());
}

PyDoc_STRVAR( GemRB_GameControlToggleAlwaysRun__doc,
"===== GameControlToggleAlwaysRun =====\n\
\n\
**Prototype:** GemRB.GameControlToggleAlwaysRun ()\n\
\n\
**Description:** Toggles using running instead of walking by default.\n\
\n\
**Return value:** N/A\n\
"
);

static PyObject* GemRB_GameControlToggleAlwaysRun(PyObject * /*self*/, PyObject* /*args*/)
{

	GET_GAMECONTROL();

	gc->ToggleAlwaysRun();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetFlags__doc,
"===== Button_SetFlags =====\n\
\n\
**Prototype:** GemRB.SetButtonFlags (WindowIndex, ControlIndex, Flags, Operation)\n\
\n\
**Metaclass Prototype:** SetFlags (Flags, Operation)\n\
\n\
**Description:** Sets the Display Flags of a Button. \n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * Flags - various bits altering the behaviour of the control\n\
    * IE_GUI_BUTTON_NO_IMAGE   = 0x00000001, no button image set by SetButtonSprites\n\
    * IE_GUI_BUTTON_PICTURE    = 0x00000002, has picture set by other SetButton* commands\n\
    * IE_GUI_BUTTON_SOUND      = 0x00000004, clicking the button has a sound\n\
    * IE_GUI_BUTTON_CAPS       = 0x00000008, uppercase the button label\n\
    * IE_GUI_BUTTON_CHECKBOX   = 0x00000010, it is a checkbox\n\
    * IE_GUI_BUTTON_RADIOBUTTON= 0x00000020, it is a radio button\n\
    * IE_GUI_BUTTON_DEFAULT    = 0x00000040, it is the default button\n\
    * IE_GUI_BUTTON_DRAGGABLE  = 0x00000080, the image of the button is draggable?\n\
  * Operation - bit operation to perform on the current button flags\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
  GemRB.SetButtonFlags (window, button, IE_GUI_BUTTON_CHECKBOX, OP_OR)\n\
This command will make the button behave like a checkbox.\n\
\n\
  Button.SetFlags (IE_GUI_BUTTON_NO_IMAGE, OP_NAND)\n\
This command will re-enable the images of the button (making it visible).\n\
\n\
**See also:** [[guiscript:Button_SetSprites]], [[guiscript:Button_SetPicture]], [[guiscript:Button_SetBAM]], [[guiscript:bit_operation]]"
);

static PyObject* GemRB_Button_SetFlags(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Flags, Operation;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &Flags, &Operation )) {
		return AttributeError( GemRB_Button_SetFlags__doc );
	}
	if (Operation < OP_SET || Operation > OP_NAND) {
		Log(ERROR, "GUIScript", "Syntax Error: operation must be 0-4");
		return NULL;
	}

	Control* btn = ( Control* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	if (btn->SetFlags( Flags, Operation ) ) {
		Log(ERROR, "GUIScript", "Flag cannot be set!");
		return NULL;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_TextArea_SetFlags__doc,
"===== TextArea_SetFlags =====\n\
\n\
**Prototype:** GemRB.SetTextAreaFlags (WindowIndex, ControlIndex, Flags, Operation)\n\
\n\
**Metaclass Prototype:** SetFlags (Flags, Operation)\n\
\n\
**Description:** Sets the Flags of a TextArea.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * Flags - various bits altering the behaviour of the control\n\
    * IE_GUI_TEXTAREA_AUTOSCROLL   - 0x05000001\n\
    * IE_GUI_TEXTAREA_SMOOTHSCROLL - 0x05000002 slowly scroll contents. If it is out of text, it will call the TEXTAREA_OUT_OF_TEXT callback.\n\
    * IE_GUI_TEXTAREA_HISTORY      - 0x05000004 drop some of the scrolled out text.\n\
  * Operation - bit operation to perform, default is OP_SET.\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    TextAreaControl = SoundWindow.GetControl (45)\n\
    TextAreaControl.SetFlags (IE_GUI_TEXTAREA_HISTORY, OP_OR)\n\
    TextAreaControl.SetVarAssoc ('Sound', 0)\n\
    RowCount = TextAreaControl.GetCharSounds ()\n\
The above code will set up the TextArea as a ListBox control, by reading the names of available character soundsets into the TextArea and setting it up as selectable. When the user clicks on row, the 'Sound' variable will be assigned a row number.\n\
\n\
**See also:** [[guiscript:RewindTA]], [[guiscript:SetTAHistory]], [[guiscript:GetCharSounds]], [[guiscript:GetCharacters]], [[guiscript:Control_QueryText]], [[guiscript:accessing_gui_controls]], [[guiscript:bit_operation]]"
);

static PyObject* GemRB_TextArea_SetFlags(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Flags;
	int Operation=0;

	if (!PyArg_ParseTuple( args, "iii|i", &WindowIndex, &ControlIndex, &Flags, &Operation )) {
		return AttributeError( GemRB_TextArea_SetFlags__doc );
	}
	if (Operation < OP_SET || Operation > OP_NAND) {
		Log(ERROR, "GUIScript", "Syntax Error: operation must be 0-4");
		return NULL;
	}

	Control* ta = ( Control* ) GetControl(WindowIndex, ControlIndex, IE_GUI_TEXTAREA);
	if (!ta) {
		return NULL;
	}

	if (ta->SetFlags( Flags, Operation )) {
		Log(ERROR, "GUIScript", "Flag cannot be set!");
		return NULL;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_ScrollBar_SetDefaultScrollBar__doc,
"===== ScrollBar_SetDefaultScrollBar =====\n\
\n\
**Prototype:** GemRB.SetDefaultScrollBar (WindowIndex, ControlIndex)\n\
\n\
**Metaclass Prototype:** SetDefaultScrollBar ()\n\
\n\
**Description:** Sets a ScrollBar as default on a window. If any control \n\
receives mousewheel events, it will be relayed to this ScrollBar, unless \n\
there is another attached to the control.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the ScrollBar control's reference\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Control_AttachScrollBar]]"
);

static PyObject* GemRB_ScrollBar_SetDefaultScrollBar(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlIndex)) {
		return AttributeError( GemRB_ScrollBar_SetDefaultScrollBar__doc );
	}

	Control* sb = ( Control* ) GetControl(WindowIndex, ControlIndex, IE_GUI_SCROLLBAR);
	if (!sb) {
		return NULL;
	}

	sb->SetFlags((IE_GUI_SCROLLBAR<<24) | IE_GUI_SCROLLBAR_DEFAULT, OP_OR);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetState__doc,
"===== Button_SetState =====\n\
\n\
**Prototype:** GemRB.SetButtonState (WindowIndex, ControlIndex, State)\n\
\n\
**Metaclass Prototype:** SetState (State)\n\
\n\
**Description:** Sets the state of a Button Control. Doesn't work if the button \n\
is a checkbox or a radio button though, their states are handled internally.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex  - the control's reference\n\
  * State - the new state of the button:\n\
    * IE_GUI_BUTTON_ENABLED    = 0x00000000, default state\n\
    * IE_GUI_BUTTON_UNPRESSED  = 0x00000000, same as above\n\
    * IE_GUI_BUTTON_PRESSED    = 0x00000001, the button is pressed\n\
    * IE_GUI_BUTTON_SELECTED   = 0x00000002, the button stuck in pressed state\n\
    * IE_GUI_BUTTON_DISABLED   = 0x00000003, the button is disabled \n\
    * IE_GUI_BUTTON_LOCKED     = 0x00000004, the button is inactive (like DISABLED, but processes MouseOver events and draws UNPRESSED bitmap)\n\
    * IE_GUI_BUTTON_FAKEDISABLED      = 0x00000005, draws DISABLED bitmap, but it isn't disabled\n\
    * IE_GUI_BUTTON_FAKEPRESSED     = 0x00000006, draws PRESSED bitmap, but it isn't shifted\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetFlags]]"
);

static PyObject* GemRB_Button_SetState(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, state;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &state )) {
		return AttributeError( GemRB_Button_SetState__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	btn->SetState( state );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetPictureClipping__doc,
"===== Button_SetPictureClipping =====\n\
\n\
**Prototype:** GemRB.SetButtonPictureClipping (Window, Button, ClippingRatio)\n\
\n\
**Metaclass Prototype:** SetPictureClipping (ClippingRatio)\n\
\n\
**Description:** Sets percent (0-1.0) of width to which button picture \n\
will be clipped. This clipping cannot be used simultaneously with \n\
SetButtonOverlay().\n\
\n\
**Parameters:** \n\
  * Window, Button - the control's reference\n\
  * ClippingRatio  - a floating point value from the 0-1 interval\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetPicture]], [[guiscript:Button_SetOverlay]]"
);

static PyObject* GemRB_Button_SetPictureClipping(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	double Clipping;

	if (!PyArg_ParseTuple( args, "iid", &WindowIndex, &ControlIndex, &Clipping )) {
		return AttributeError( GemRB_Button_SetPictureClipping__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	if (Clipping<0.0) Clipping = 0.0;
	else if (Clipping>1.0) Clipping = 1.0;
	btn->SetPictureClipping( Clipping );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetPicture__doc,
"===== Button_SetPicture =====\n\
\n\
**Prototype:** GemRB.SetButtonPicture (WindowIndex, ControlIndex, PictureResRef, DefaultResRef)\n\
\n\
**Metaclass Prototype:** SetPicture (PictureResRef, DefaultResRef)\n\
\n\
**Description:** Sets the Picture of a Button Control from a BMP file.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * PictureResRef - the name of the picture (a .bmp resref)\n\
  * DefaultResRef - an alternate bmp should the picture be nonexistent\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetBAM]], [[guiscript:Button_SetPLT]], [[guiscript:Button_SetSprites]], [[guiscript:Button_SetPictureClipping]], [[guiscript:Window_SetPicture]]"
);

static PyObject* GemRB_Button_SetPicture(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char *ResRef;
	char *DefResRef = NULL;

	if (!PyArg_ParseTuple( args, "iis|s", &WindowIndex, &ControlIndex, &ResRef, &DefResRef )) {
		return AttributeError( GemRB_Button_SetPicture__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return RuntimeError("Cannot find the button!\n");
	}

	if (ResRef[0] == 0) {
		btn->SetPicture( NULL );
		Py_RETURN_NONE;
	}

	ImageFactory* fact = ( ImageFactory* )
		gamedata->GetFactoryResource(ResRef, IE_BMP_CLASS_ID, IE_NORMAL, true);

	//if the resource doesn't exist, but we have a default resource
	//use this resource
	if (!fact && DefResRef) {
		fact = ( ImageFactory* )
			gamedata->GetFactoryResource( DefResRef, IE_BMP_CLASS_ID, IE_NORMAL );
	}

	if (!fact) {
		return RuntimeError("Picture resource not found!\n");
	}

	Sprite2D* Picture = fact->GetSprite2D();
	if (Picture == NULL) {
		return RuntimeError("Failed to acquire the picture!\n");
	}

	btn->SetPicture( Picture );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetSprite2D__doc,
"===== Button_SetSprite2D =====\n\
\n\
**Prototype:** GemRB.Button.SetSprite2D (WindowIndex, ControlIndex, Sprite2D)\n\
\n\
**Metaclass Prototype:** SetSprite2D (Sprite2D)\n\
\n\
**Description:** Sets a Sprite2D onto a button as its picture.\n\
\n\
**Parameters:** \n\
  * WindowIndex, ControlIndex - control ID and index\n\
  * Sprite2D - sprite object\n\
\n\
**Return value:** N/A\n\
\n\
**See also:**"
);

static PyObject* GemRB_Button_SetSprite2D(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	PyObject *obj;

	if (!PyArg_ParseTuple( args, "iiO", &wi, &ci, &obj )) {
		return AttributeError( GemRB_Button_SetSprite2D__doc );
	}
	Button* btn = ( Button* ) GetControl( wi, ci, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	CObject<Sprite2D> spr(obj);

	btn->SetPicture( spr.get() );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetMOS__doc,
"===== Button_SetMOS =====\n\
\n\
**Prototype:** GemRB.SetButtonMOS (WindowIndex, ControlIndex, MOSResRef)\n\
\n\
**Metaclass Prototype:** SetMOS (MOSResRef)\n\
\n\
**Description:** Sets the Picture of a Button Control from a MOS file.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * MOSResRef - the name of the picture (a .mos resref)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetBAM]], [[guiscript:Button_SetPLT]], [[guiscript:Button_SetPicture]]"
);

static PyObject* GemRB_Button_SetMOS(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char *ResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex, &ResRef )) {
		return AttributeError( GemRB_Button_SetMOS__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		btn->SetPicture( NULL );
		Py_RETURN_NONE;
	}

	ResourceHolder<ImageMgr> im = GetResourceHolder<ImageMgr>(ResRef);
	if (im == nullptr) {
		return RuntimeError("Picture resource not found!\n");
	}

	Sprite2D* Picture = im->GetSprite2D();
	if (Picture == NULL) {
		return RuntimeError("Failed to acquire the picture!\n");
	}

	btn->SetPicture( Picture );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetPLT__doc,
"===== Button_SetPLT =====\n\
\n\
**Prototype:** GemRB.SetButtonPLT (WindowIndex, ControlIndex, PLTResRef, col1, col2, col3, col4, col5, col6, col7, col8[, type])\n\
\n\
**Metaclass Prototype:** SetPLT (PLTResRef, col1, col2, col3, col4, col5, col6, col7, col8[, type])\n\
\n\
**Description:** Sets the Picture of a Button Control from a PLT file. \n\
Sets up the palette based on the eight given gradient colors.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * PLTResRef - the name of the picture (a .plt resref)\n\
  * col1-8 - color gradients\n\
  * type - the byte to use from the gradients:\n\
    * 0 Body (robe or armour)\n\
    * 1 Weapon\
    * 2 Shield\n\
    * 3 Helmet\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetBAM]]"
);

static PyObject* GemRB_Button_SetPLT(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	ieDword col[8];
	int type = 0;
	char *ResRef;

	memset(col,-1,sizeof(col));
	if (!PyArg_ParseTuple( args, "iisiiiiiiii|i", &WindowIndex, &ControlIndex,
			&ResRef, &(col[0]), &(col[1]), &(col[2]), &(col[3]),
			&(col[4]), &(col[5]), &(col[6]), &(col[7]), &type) ) {
		return AttributeError( GemRB_Button_SetPLT__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	//empty image
	if (ResRef[0] == 0 || ResRef[0]=='*') {
		btn->SetPicture( NULL );
		Py_RETURN_NONE;
	}

	Sprite2D *Picture;
	Sprite2D *Picture2=NULL;

	// NOTE: it seems nobody actually wants to use external palettes!
	// the only users with external plts are in bg2, but they don't match the bam:
	//   lvl9 shapeshift targets: troll, golem, fire elemental, illithid, wolfwere
	// 1pp deliberately breaks palettes for the bam to be used (so the original did support)
	// TODO: if this turns out to be resiliently true, also remove-revert useCorrupt
	//ResourceHolder<PalettedImageMgr> im(ResRef, false, true);

//	if (im == NULL) {
		AnimationFactory* af = ( AnimationFactory* )
			gamedata->GetFactoryResource( ResRef,
			IE_BAM_CLASS_ID, IE_NORMAL );
		if (!af) {
			Log(WARNING, "GUISCript", "PLT/BAM not found for ref: %s", ResRef);
			Py_RETURN_NONE;
		}

		Picture = af->GetPaperdollImage(col[0]==0xFFFFFFFF?0:col, Picture2,(unsigned int)type);
		if (Picture == NULL) {
			Log(ERROR, "Button_SetPLT", "Paperdoll picture == NULL (%s)", ResRef);
			Py_RETURN_NONE;
		}
/*	} else {
		Picture = im->GetSprite2D(type, col);
		if (Picture == NULL) {
			Log(ERROR, "Button_SetPLT", "Picture == NULL (%s)", ResRef);
			return NULL;
		}
	}*/

	if (type == 0)
		btn->ClearPictureList();
	btn->StackPicture(Picture);
	if (Picture2) {
		btn->SetFlags (IE_GUI_BUTTON_BG1_PAPERDOLL, OP_OR);
		btn->StackPicture( Picture2 );
	} else if (type == 0) {
		btn->SetFlags (IE_GUI_BUTTON_BG1_PAPERDOLL, OP_NAND);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetBAM__doc,
"===== Button_SetBAM =====\n\
\n\
**Prototype:** GemRB.SetButtonBAM (WindowIndex, ControlIndex, BAMResRef, CycleIndex, FrameIndex, col1)\n\
\n\
**Metaclass Prototype:** SetBAM (BAMResRef, CycleIndex, FrameIndex[, col1])\n\
\n\
**Description:** Sets the Picture of a Button Control from a BAM file. If \n\
the supplied color gradient value is the default -1, then no palette change, \n\
if it is >=0, then it changes the 4-16 palette entries of the bam. Since it \n\
uses 12 colors palette, it has issues in PST.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * BAMResRef - the name of the BAM animation (a .bam resref)\n\
  * CycleIndex, FrameIndex - the cycle and frame index of the picture in the bam\n\
  * col1 - the gradient number, (-1 no gradient)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetPLT]], [[guiscript:Button_SetPicture]], [[guiscript:Button_SetSprites]]"
);

static PyObject* SetButtonBAM(int wi, int ci, const char *ResRef, int CycleIndex, int FrameIndex, int col1)
{
	Button* btn = ( Button* ) GetControl(wi, ci, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		btn->SetPicture( NULL );
		//no incref! (happens in caller if necessary)
		return Py_None;
	}

	AnimationFactory* af = ( AnimationFactory* )
		gamedata->GetFactoryResource( ResRef,
		IE_BAM_CLASS_ID, IE_NORMAL );
	if (!af)
		return NULL;
	Sprite2D* Picture = af->GetFrame ( FrameIndex, CycleIndex );

	if (Picture == NULL) {
		return NULL;
	}

	if (col1 >= 0) {
		Sprite2D* old = Picture;
		Picture = old->copy();
		Sprite2D::FreeSprite(old);

		Palette* newpal = Picture->GetPalette()->Copy();
		core->GetPalette( col1, 12, &newpal->col[4]);
		Picture->SetPalette( newpal );
		newpal->release();
	}

	btn->SetPicture( Picture );

	//no incref! (happens in caller if necessary)
	return Py_None;
}

static PyObject* GemRB_Button_SetBAM(PyObject * /*self*/, PyObject* args)
{
	int wi, ci, CycleIndex, FrameIndex, col1 = -1;
	char *ResRef;

	if (!PyArg_ParseTuple( args, "iisii|i", &wi, &ci,
			&ResRef, &CycleIndex, &FrameIndex, &col1 )) {
		return AttributeError( GemRB_Button_SetBAM__doc );
	}

	PyObject *ret = SetButtonBAM(wi,ci, ResRef, CycleIndex, FrameIndex,col1);
	if (ret) {
		Py_INCREF(ret);
	}
	return ret;
}

PyDoc_STRVAR( GemRB_Control_SetAnimationPalette__doc,
"===== Control_SetAnimationPalette =====\n\
\n\
**Prototype:** GemRB.SetAnimationPalette (WindowIndex, ControlIndex, col1, col2, col3, col4, col5, col6, col7, col8)\n\
\n\
**Metaclass Prototype:** SetAnimationPalette (col1, col2, col3, col4, col5, col6, col7, col8)\n\
\n\
**Description:** Sets the palette of an animation already assigned to the control.\n\
\n\
**Parameters:** \n\
  * WindowIndex - the window control id\n\
  * ControlID - the id of the target control\n\
  * col1 - col8 - colors for the palette\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Control_SetAnimation]]"
);

static PyObject* GemRB_Control_SetAnimationPalette(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	ieDword col[8];

	memset(col,-1,sizeof(col));
	if (!PyArg_ParseTuple( args, "iiiiiiiiii", &wi, &ci,
			&(col[0]), &(col[1]), &(col[2]), &(col[3]),
			&(col[4]), &(col[5]), &(col[6]), &(col[7])) ) {
		return AttributeError( GemRB_Control_SetAnimationPalette__doc );
	}

	Control* ctl = GetControl(wi, ci, -1);
	if (!ctl) {
		return NULL;
	}

	ControlAnimation* anim = ctl->animation;
	if (!anim) {
		return RuntimeError("No animation!");
	}

	anim->SetPaletteGradients(col);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Control_HasAnimation__doc,
"===== Control_HasAnimation =====\n\
\n\
**Prototype:** GemRB.HasAnimation (WindowIndex, ControlIndex, BAMResRef[, Cycle])\n\
\n\
**Metaclass Prototype:** HasAnimation (BAMResRef[, Cycle])\n\
\n\
**Description:** Checks whether a Control (usually a Button) has a given animation set.\n\
\n\
**Parameters:** \n\
  * WindowIndex, ControlIndex - control IDs\n\
  * BAMResRef - animation file to search for\n\
  * Cycle - require a certain bam cycle to be used\n\
\n\
**Return value:** integer, 0 if false"
);

static PyObject* GemRB_Control_HasAnimation(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	char *ResRef;
	int Cycle = 0;

	if (!PyArg_ParseTuple( args, "iis|i", &wi, &ci, &ResRef, &Cycle )) {
		return AttributeError( GemRB_Control_HasAnimation__doc );
	}

	Control* ctl = GetControl(wi, ci, -1);
	if (ctl && ctl->animation) {
		return PyBool_FromLong(ctl->animation->SameResource(ResRef, Cycle));
	}

	return PyBool_FromLong(0);
}

PyDoc_STRVAR( GemRB_Control_SetAnimation__doc,
"===== Control_SetAnimation =====\n\
\n\
**Prototype:** GemRB.SetAnimation (WindowIndex, ControlIndex, BAMResRef[, Cycle, Blend])\n\
\n\
**Metaclass Prototype:** SetAnimation (BAMResRef[, Cycle, Blend])\n\
\n\
**Description:**  Sets the animation of a Control (usually a Button) from \n\
a BAM file. Optionally an animation cycle could be set too.\n\
\n\
**Parameters:** \n\
  * WindowIndex - the window control id\n\
  * ControlID - the id of the target control\n\
  * BAMResRef - resref of the animation\
  * Cycle - (optional) number of the cycle to use\n\
  * Blend - (optional) toggle use of blending\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Control_SetAnimationPalette]]"
);

static PyObject* GemRB_Control_SetAnimation(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	char *ResRef;
	int Cycle = 0;
	int Blend = 0;

	if (!PyArg_ParseTuple( args, "iis|ii", &wi, &ci, &ResRef, &Cycle, &Blend )) {
		return AttributeError( GemRB_Control_SetAnimation__doc );
	}

	Control* ctl = GetControl(wi, ci, -1);
	if (!ctl) {
		return NULL;
	}

	//who knows, there might have been an active animation lurking
	if (ctl->animation) {
		//if this control says the resource is the same
		//we wanted to set, we don't reset it
		//but we must reinitialize it, if it was play once
		if(ctl->animation->SameResource(ResRef, Cycle) && !(ctl->Flags&IE_GUI_BUTTON_PLAYONCE)) {
			Py_RETURN_NONE;
		}
		delete ctl->animation;
		ctl->animation = NULL;
	}

	if (ResRef[0] == 0) {
		ctl->SetAnimPicture( NULL );
		Py_RETURN_NONE;
	}

	ControlAnimation* anim = new ControlAnimation( ctl, ResRef, Cycle );
	if (!anim->HasControl()) Py_RETURN_NONE;

	if (Blend) {
		anim->SetBlend(true);
	}
	anim->UpdateAnimation(false);

	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_ValidTarget__doc,
"===== ValidTarget =====\n\
\n\
**Prototype:** GemRB.ValidTarget (PartyID, flags)\n\
\n\
**Description:** Checks if an actor is valid for various purposes, like \n\
being visible, selectable, dead, etc.\n\
\n\
**Parameters:** \n\
  * PartyID - party ID or global ID of the actor to check\n\
  * flags   - bits to check against (see GameControlSetTargetMode)\n\
\n\
**See also:** [[guiscript:GameControlSetTargetMode]]\n\
\n\
**Return value:** boolean"
);

static PyObject* GemRB_ValidTarget(PyObject * /*self*/, PyObject* args)
{
	int globalID, flags;

	if (!PyArg_ParseTuple( args, "ii", &globalID, &flags )) {
		return AttributeError( GemRB_ValidTarget__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (actor->ValidTarget(flags, actor)) {
		Py_INCREF( Py_True );
		return Py_True;
	} else {
		Py_INCREF( Py_False );
		return Py_False;
	}
}


PyDoc_STRVAR( GemRB_VerbalConstant__doc,
"===== VerbalConstant =====\n\
\n\
**Prototype:** GemRB.VerbalConstant (globalID, str)\n\
\n\
**Description:**  Plays a Character's SoundSet entry.\n\
\n\
**Parameters:** \n\
  * globalID - party ID or global ID of the actor to use\n\
  * str - verbal constant index (0-100)\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_VerbalConstant(PyObject * /*self*/, PyObject* args)
{
	int globalID, str;
	char Sound[_MAX_PATH];
	unsigned int channel;

	if (!PyArg_ParseTuple( args, "ii", &globalID, &str )) {
		return AttributeError( GemRB_VerbalConstant__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (str<0 || str>=VCONST_COUNT) {
		return AttributeError( "SoundSet Entry is too large" );
	}

	//get soundset based string constant
	snprintf(Sound, _MAX_PATH, "%s/%s%02d",
		actor->PCStats->SoundFolder, actor->PCStats->SoundSet, str);
	channel = actor->InParty ? SFX_CHAN_CHAR0 + actor->InParty - 1 : SFX_CHAN_DIALOG;
	core->GetAudioDrv()->Play(Sound, channel, 0, 0, GEM_SND_RELATIVE|GEM_SND_SPEECH);
	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_PlaySound__doc,
"===== PlaySound =====\n\
\n\
**Prototype:** GemRB.PlaySound (SoundResource[, channel, xpos, ypos, type])\n\
**Prototype:** GemRB.PlaySound (DefSoundIndex[, channel])\n\
**Prototype:** GemRB.PlaySound (None)\n\
\n\
**Description:** Plays a sound identified by resource reference or \n\
defsound.2da index. If there is a single PC selected, then it will play the \n\
sound as if it was said by that PC (EAX).\n\
\n\
**Parameters:**\n\
  * SoundResource - a sound resref (the format could be raw pcm, wavc or  ogg; 8/16 bit; mono/stereo). Use the None python object to simply stop the previous sound.\n\
  * channel - the name of the channel the sound should be played on (optional, defaults to 'GUI'\n\
  * xpos - x coordinate of the position where the sound should be played (optional)\n\
  * ypos - y coordinate of the position where the sound should be played (optional)\n\
  * type - defaults to 1, use 4 for speeches or other sounds that should stop the previous sounds (optional)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadMusicPL]]"
);

static PyObject* GemRB_PlaySound(PyObject * /*self*/, PyObject* args)
{
	char *ResRef;
	char *channel_name = NULL;
	int xpos = 0;
	int ypos = 0;
	unsigned int flags = GEM_SND_RELATIVE;
	unsigned int channel = SFX_CHAN_GUI;
	int index;

	if (PyArg_ParseTuple( args, "i|z", &index, &channel_name) ) {
		if (channel_name != NULL) {
			channel = core->GetAudioDrv()->GetChannel(channel_name);
		}
		core->PlaySound(index, channel);
	} else {
		PyErr_Clear(); //clearing the exception
		if (!PyArg_ParseTuple(args, "z|ziii", &ResRef, &channel_name, &xpos, &ypos, &flags)) {
			return AttributeError( GemRB_PlaySound__doc );
		}

		if (channel_name != NULL) {
			channel = core->GetAudioDrv()->GetChannel(channel_name);
		}

		core->GetAudioDrv()->Play(ResRef, channel, xpos, ypos, flags);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_DrawWindows__doc,
"===== DrawWindows =====\n\
\n\
**Prototype:** GemRB.DrawWindows ()\n\
\n\
**Description:** Refreshes the User Interface by redrawing invalidated controls.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_Invalidate]], [[guiscript:UnhideGUI]]\n\
"
);

static PyObject* GemRB_DrawWindows(PyObject * /*self*/, PyObject * /*args*/)
{
	core->DrawWindows();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Quit__doc,
"===== Quit =====\n\
\n\
**Prototype:** GemRB.Quit ()\n\
\n\
**Description:** Quits GemRB immediately.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:QuitGame]]\n\
"
);

static PyObject* GemRB_Quit(PyObject * /*self*/, PyObject * /*args*/)
{
	core->ExitGemRB();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_LoadMusicPL__doc,
"===== LoadMusicPL =====\n\
\n\
**Prototype:** LoadMusicPL (MusicPlayListResource[, HardEnd])\n\
\n\
**Description:** Loads and starts a Music PlayList.\n\
\n\
**Parameters:**\n\
  * MusicPlayListResource - a .mus resref\n\
  * HardEnd - off by default, set to 1 to disable the fading at the end\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:SoftEndPL]], [[guiscript:HardEndPL]]"
);

static PyObject* GemRB_LoadMusicPL(PyObject * /*self*/, PyObject* args)
{
	char *ResRef;
	int HardEnd = 0;

	if (!PyArg_ParseTuple( args, "s|i", &ResRef, &HardEnd )) {
		return AttributeError( GemRB_LoadMusicPL__doc );
	}

	core->GetMusicMgr()->SwitchPlayList( ResRef, (bool) HardEnd );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SoftEndPL__doc,
"===== SoftEndPL =====\n\
\n\
**Prototype:** GemRB.SoftEndPL ()\n\
\n\
**Description:** Ends the currently playing Music Playlist softly.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:HardEndPL]]"
);

static PyObject* GemRB_SoftEndPL(PyObject * /*self*/, PyObject * /*args*/)
{
	core->GetMusicMgr()->End();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_HardEndPL__doc,
"===== HardEndPL =====\n\
\n\
**Prototype:** GemRB.HardEndPL ()\n\
\n\
**Description:** Ends a Music Playlist immediately.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:SoftEndPL]]\n\
"
);

static PyObject* GemRB_HardEndPL(PyObject * /*self*/, PyObject * /*args*/)
{
	core->GetMusicMgr()->HardEnd();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetToken__doc,
"===== SetToken =====\n\
\n\
**Prototype:** GemRB.SetToken(VariableName, Value)\n\
\n\
**Description:** Set/Create a token to be replaced in StrRefs. QueryText() \n\
will use the actual value of the token when it encounters '<token>' \n\
substrings in a retrieved dialog.tlk entry. Some tokens are hardcoded and \n\
you can't affect QueryText() by setting them. Usage of those tokens should \n\
be avoided. The hardcoded token list:\n\
  * FIGHTERTYPE  - always resolves to strref #10174\n\
  * RACE         - always resolves to the race of the last speaker\n\
  * GABBER       - always resolves to the longname of the last speaker\n\
  * SIRMAAM      - strref #27473/#27475 depending on last speaker's gender\n\
  * GIRLBOY      - strref #27477/#27476 depending on last speaker's gender\n\
  * BROTHERSISTER- strref #27478/#27479 ...\n\
  * LADYLORD     - strref #27481/#27480 ...\n\
  * MALEFEMALE   - strref #27483/#27482 ...\n\
  * HESHE        - strref #27485/#27484 ...\n\
  * HISHER       - strref #27487/#27486 ...\n\
  * HIMHER       - strref #27487/#27488 ...\n\
  * MANWOMAN     - strref #27490/#27489 ...\n\
  * PRO_*        - same as above with protagonist\n\
\n\
**Parameters:**\n\
  *  VariableName - the name of the variable (shorter than 32!)\n\
  *  Value        - string, the value of the token\n\
\n\
**Example:**\n\
  ClassTitle = CommonTables.Classes.GetValue (Class, 'CAP_REF', GTV_REF)\n\
  GemRB.SetToken ('CLASS', ClassTitle)\n\
  # force an update of the string by refetching it\n\
  TA.SetText (GemRB.GetString (16480))\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetToken]], [[guiscript:Control_QueryText]], [[guiscript:Control_SetText]]\n\
"
);

static PyObject* GemRB_SetToken(PyObject * /*self*/, PyObject* args)
{
	char *Variable;
	char *value;

	if (!PyArg_ParseTuple( args, "ss", &Variable, &value )) {
		return AttributeError( GemRB_SetToken__doc );
	}
	core->GetTokenDictionary()->SetAtCopy( Variable, value );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetVar__doc,
"===== SetVar =====\n\
\n\
**Prototype:** GemRB.SetVar (VariableName, Value)\n\
\n\
**Description:** Set a Variable of the Global Dictionary. This is an \n\
independent dictionary from the gamescript. It contains configuration \n\
variables, and provides a flexible interface between guiscript and the \n\
engine core. There are some reserved names that are referenced from the \n\
core, these are described in different places:\n\
  * variable  : described in\n\
  * PlayMode  - LoadGame\n\
  * *Window   - HideGUI\n\
  * *Position - HideGUI\n\
  * Progress  - data_exchange\n\
\n\
**Parameters:**\n\
  * VariableName - the name of the variable (shorter than 32!)\n\
  * Value        - numeric, the new value\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
  GemRB.SetVar('ActionsWindow', ActionsWindow)\n\
  GemRB.SetVar('OptionsWindow', OptionsWindow)\n\
  GemRB.SetVar('MessageWindow', MessageWindow)\n\
  GemRB.SetVar('ActionsPosition', 4) #BottomAdded\n\
  GemRB.SetVar('OptionsPosition', 0) #Left\n\
  GemRB.SetVar('MessagePosition', 4) #BottomAdded\n\
The above lines set up some windows of the main game screen.\n\
\n\
**See also:** [[guiscript:Control_SetVarAssoc]], [[guiscript:SetToken]], [[guiscript:LoadGame]], [[guiscript:HideGUI]], [[guiscript:data_exchange]]"
);

static PyObject* GemRB_SetVar(PyObject * /*self*/, PyObject* args)
{
	char *Variable;
	//this should be 32 bits, always, but i cannot tell that to Python
	unsigned long value;

	if (!PyArg_ParseTuple( args, "sl", &Variable, &value )) {
		return AttributeError( GemRB_SetVar__doc );
	}

	core->GetDictionary()->SetAt( Variable, (ieDword) value );

	//this is a hack to update the settings deeper in the core
	UpdateActorConfig();
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetMessageWindowSize__doc,
"===== GetMessageWindowSize =====\n\
\n\
**Prototype:** GemRB.GetMessageWindowSize ()\n\
\n\
**Description:** Returns current MessageWindowSize. Works only when a game is loaded.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** int (GS_ flag bits)\n\
\n\
**See also:**"
);

static PyObject* GemRB_GetMessageWindowSize(PyObject * /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyInt_FromLong( game->ControlStatus );
}

PyDoc_STRVAR( GemRB_GetToken__doc,
"===== GetToken =====\n\
\n\
**Prototype:** GemRB.GetToken (VariableName)\n\
\n\
**Description:** Get a Variable value from the Token Dictionary. Tokens are \n\
string values, used both by the game scripts and the GUI scripts.\n\
\n\
**Parameters:**\n\
  * VariableName - name of the variable (shorter than 32!)\n\
\n\
**Return value:** string, the value of the token\n\
\n\
**Example:**\n\
  TextArea.Append (GemRB.GetToken ('CHARNAME'))\n\
The above example will add the protagonist's name to the TextArea (if the token was set correctly).\n\
\n\
**See also:** [[guiscript:SetToken]], [[guiscript:Control_QueryText]]\n\
"
);

static PyObject* GemRB_GetToken(PyObject * /*self*/, PyObject* args)
{
	const char *Variable;
	char *value;

	if (!PyArg_ParseTuple( args, "s", &Variable )) {
		return AttributeError( GemRB_GetToken__doc );
	}

	//returns only the pointer
	if (!core->GetTokenDictionary()->Lookup( Variable, value )) {
		return PyString_FromString( "" );
	}

	return PyString_FromString( value );
}

PyDoc_STRVAR( GemRB_GetVar__doc,
"===== GetVar =====\n\
\n\
**Prototype:** GemRB.GetVar (VariableName)\n\
\n\
**Description:** Get a Variable value from the Global Dictionary. Controls \n\
could be set up to be associated with such a variable. Even multiple \n\
controls could affect the same variable.\n\
\n\
**Parameters:**\n\
  * VariableName - name of the variable (shorter than 32!)\n\
\n\
**Return value:** numeric, 0 if the variable doesn't exist\n\
\n\
**Examples:**\n\
  selected = GemRB.GetVar ('SelectedMovie')\n\
\n\
**See also:** [[guiscript:SetVar]], [[guiscript:Control_SetVarAssoc]], [[guiscript:data_exchange]]\n\
"
);

static PyObject* GemRB_GetVar(PyObject * /*self*/, PyObject* args)
{
	const char *Variable;
	ieDword value;

	if (!PyArg_ParseTuple( args, "s", &Variable )) {
		return AttributeError( GemRB_GetVar__doc );
	}

	if (!core->GetDictionary()->Lookup( Variable, value )) {
		return PyInt_FromLong( 0 );
	}

	// A PyInt is internally (probably) a long. Since we sometimes set
	// variables to -1, cast value to a signed integer first, so it is
	// sign-extended into a long if long is larger than int.
	return PyInt_FromLong( (int)value );
}

PyDoc_STRVAR( GemRB_CheckVar__doc,
"===== CheckVar =====\n\
\n\
**Prototype:** GemRB.CheckVar (VariableName, Context)\n\
\n\
**Description:** Return (and output on terminal) the value of a Game \n\
Variable. It executes the CheckVariable gamescript function in the last \n\
actor's context, or, short of that, in the current area's context. If there \n\
is no running game, it terminates the script. \n\
GetGameVar('variable') is effectively the same as CheckVar('variable','GLOBAL').\n\
\n\
**Parameters:**\n\
  * VariableName - must be shorter than 32 bytes\n\
  * Context      - must be exactly 6 bytes long\n\
  * Special cases for Context: LOCALS, GLOBAL, MYAREA\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:GetGameVar]]"
);

static PyObject* GemRB_CheckVar(PyObject * /*self*/, PyObject* args)
{
	char *Variable;
	char *Context;

	if (!PyArg_ParseTuple( args, "ss", &Variable, &Context )) {
		return AttributeError( GemRB_CheckVar__doc );
	}
	GET_GAMECONTROL();

	Scriptable *Sender = (Scriptable *) gc->GetLastActor();
	if (!Sender) {
		GET_GAME();

		Sender = (Scriptable *) game->GetCurrentArea();
	}

	if (!Sender) {
		Log(ERROR, "GUIScript", "No Sender!");
		return NULL;
	}
	long value =(long) CheckVariable(Sender, Variable, Context);
	Log(DEBUG, "GUISCript", "%s %s=%ld",
		Context, Variable, value);
	return PyInt_FromLong( value );
}

PyDoc_STRVAR( GemRB_SetGlobal__doc,
"===== SetGlobal =====\n\
\n\
**Prototype:** GemRB.SetGlobal (VariableName, Context, Value)\n\
\n\
**Description:** Sets a gamescript variable to the specificed numeric value.\n\
\n\
**Parameters:** \n\
  * VariableName - name of the variable\n\
  * Context - LOCALS, GLOBALS, MYAREA or area specific\n\
  * Value - value to set\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:SetVar]], [[guiscript:Control_SetVarAssoc]], [[guiscript:SetToken]], [[guiscript:data_exchange]]"
);

static PyObject* GemRB_SetGlobal(PyObject * /*self*/, PyObject* args)
{
	char *Variable;
	char *Context;
	int Value;

	if (!PyArg_ParseTuple( args, "ssi", &Variable, &Context, &Value )) {
		return AttributeError( GemRB_SetGlobal__doc );
	}

	Scriptable *Sender = NULL;

	GET_GAME();

	if (!strnicmp(Context, "MYAREA", 6) || !strnicmp(Context, "LOCALS", 6)) {
		GET_GAMECONTROL();

		Sender = (Scriptable *) gc->GetLastActor();
		if (!Sender) {
			Sender = (Scriptable *) game->GetCurrentArea();
		}
		if (!Sender) {
			Log(ERROR, "GUIScript", "No Sender!");
			return NULL;
		}
	} // else GLOBAL, area name or KAPUTZ

	SetVariable(Sender, Variable, Context, (ieDword) Value);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetGameVar__doc,
"===== GetGameVar =====\n\
\n\
**Prototype:** GemRB.GetGameVar (VariableName)\n\
\n\
**Description:** Get a Variable value from the Game Global Dictionary. This \n\
is what gamescripts know as GLOBAL variables. \n\
\n\
**Parameters:**\n\
  * VariableName - name of the variable\n\
\n\
**Return value:**\n\
\n\
**Example:** Chapter = GemRB.GetGameVar ('chapter')\n\
\n\
**See also:** [[guiscript:GetVar]], [[guiscript:GetToken]], [[guiscript:CheckVar]]"
);

static PyObject* GemRB_GetGameVar(PyObject * /*self*/, PyObject* args)
{
	const char *Variable;
	ieDword value;

	if (!PyArg_ParseTuple( args, "s", &Variable )) {
		return AttributeError( GemRB_GetGameVar__doc );
	}

	GET_GAME();

	if (!game->locals->Lookup( Variable, value )) {
		return PyInt_FromLong( ( unsigned long ) 0 );
	}

	return PyInt_FromLong( (unsigned long) value );
}

PyDoc_STRVAR( GemRB_PlayMovie__doc,
"===== PlayMovie =====\n\
\n\
**Prototype:** GemRB.PlayMovie (MOVResRef[, flag=0])\n\
\n\
**Description:** Plays the named movie. Sets the configuration variable \n\
MOVResRef to 1. If flag was set to 1 it won't play the movie if the \n\
configuration variable was already set (saved in game ini).\n\
\n\
**Parameters:**\n\
  * MOVResRef - a .mve/.bik (or vlc compatible) resource reference.\n\
  * flag:\n\
      * 0 - only play the movie if it has never been played before\n\
      * 1 - always play\n\
\n\
**Return value:**\n\
  * 0 - movie played\n\
  * -1 - error occurred\n\
  * 1 - movie skipped\n\
\n\
**See also:** [[guiscript:SetVar]], [[guiscript:GetVar]]\n\
"
);

static PyObject* GemRB_PlayMovie(PyObject * /*self*/, PyObject* args)
{
	const char *string;
	int flag = 0;

	if (!PyArg_ParseTuple( args, "s|i", &string, &flag )) {
		return AttributeError( GemRB_PlayMovie__doc );
	}

	ieDword ind = 0;

	//Lookup will leave the flag untouched if it doesn't exist yet
	core->GetDictionary()->Lookup(string, ind);
	if (flag)
		ind = 0;
	if (!ind) {
		ind = core->PlayMovie( string );
	}
	//don't return NULL
	return PyInt_FromLong( ind );
}

PyDoc_STRVAR( GemRB_DumpActor__doc,
"===== DumpActor =====\n\
\n\
**Prototype:** GemRB.DumpActor (globalID)\n\
\n\
**Description:** Prints the character's debug dump\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** N/A"
);
static PyObject* GemRB_DumpActor(PyObject * /*self*/, PyObject * args)
{
	int globalID;

	if (!PyArg_ParseTuple( args, "i", &globalID )) {
		return AttributeError( GemRB_DumpActor__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->dump();
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SaveCharacter__doc,
"===== SaveCharacter =====\n\
\n\
**Prototype:** GemRB.SaveCharacter (PartyID, filename)\n\
\n\
**Description:** Saves (exports) the designated partymember into the Characters \n\
directory of the game. This character is importable later by a special \n\
CreatePlayer call.\n\
\n\
**Parameters:**\n\
  * PartyID  - the saved character's position in the party\n\
  * filename - the filename of the character\n\
\n\
**Return value:** N/A\n\
\n\
**Example:** \n\
  pc = GemRB.GameGetSelectedPCSingle ()\n\
  GemRB.SaveCharacter (pc, ExportFileName)\n\
The above example exports the currently selected character.\n\
\n\
**See also:** [[guiscript:CreatePlayer]]"
);

static PyObject* GemRB_SaveCharacter(PyObject * /*self*/, PyObject * args)
{
	int globalID;
	const char *name;

	if (!PyArg_ParseTuple( args, "is", &globalID, &name )) {
		return AttributeError( GemRB_SaveCharacter__doc );
	}
	if (!name[0]) {
		return AttributeError( GemRB_SaveCharacter__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyInt_FromLong(core->WriteCharacter(name, actor) );
}


PyDoc_STRVAR( GemRB_SaveConfig__doc,
"===== SaveConfig =====\n\
\n\
**Prototype:** GemRB.SaveConfig ()\n\
\n\
**Description:** Exports the game configuration to a file.\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_SaveConfig(PyObject * /*self*/, PyObject * /*args*/)
{
	UpdateActorConfig(); // Button doesn't trigger this in its OnMouseUp handler where it calls SetVar
	return PyBool_FromLong(core->SaveConfig());
}

PyDoc_STRVAR( GemRB_SaveGame__doc,
"===== SaveGame =====\n\
\n\
**Prototype:** GemRB.SaveGame (savegame, description[, version])\n\
**Prototype:** GemRB.SaveGame (position[, version])\n\
\n\
**Description:** Saves the current game. If version is given, it will save \n\
to a specific SAV version.\n\
\n\
**Parameters:**\n\
  * position - the saved game's index; 0 and 1 are reserved\n\
  * savegame - a save game python object (GetSaveGames)\n\
  * description - the string that will also appear in the filename\n\
  * version - an optional SAV version override\n\
\n\
**Return value:** N/A\n\
\n\
**Example:** \n\
  GemRB.SaveGame (10, 'After meeting Dhall')\n\
\n\
**See also:** [[guiscript:LoadGame]], [[guiscript:SaveCharacter]]"
);

static PyObject* GemRB_SaveGame(PyObject * /*self*/, PyObject * args)
{
	PyObject *obj;
	int slot = -1;
	int Version = -1;
	const char *folder;

	if (!PyArg_ParseTuple( args, "Os|i", &obj, &folder, &Version )) {
		PyErr_Clear();
		if (!PyArg_ParseTuple( args, "i|i", &slot, &Version)) {
			return AttributeError( GemRB_SaveGame__doc );
		}
	}

	GET_GAME();

	SaveGameIterator *sgip = core->GetSaveGameIterator();
	if (!sgip) {
		return RuntimeError("No savegame iterator");
	}

	if (Version>0) {
		game->version = Version;
	}
	if (slot == -1) {
		CObject<SaveGame> save(obj);

		return PyInt_FromLong(sgip->CreateSaveGame(save, folder) );
	} else {
		return PyInt_FromLong(sgip->CreateSaveGame(slot, core->MultipleQuickSaves) );
	}
}

PyDoc_STRVAR( GemRB_GetSaveGames__doc,
"===== GetSaveGames =====\n\
\n\
**Prototype:** GemRB.GetSaveGameCount ()\n\
\n\
**Description:** Returns a list of saved games.\n\
\n\
**Return value:** python list"
);

static PyObject* GemRB_GetSaveGames(PyObject * /*self*/, PyObject * /*args*/)
{
	return MakePyList<SaveGame>(core->GetSaveGameIterator()->GetSaveGames());
}

PyDoc_STRVAR( GemRB_DeleteSaveGame__doc,
"===== DeleteSaveGame =====\n\
\n\
**Prototype:** GemRB.DeleteSaveGame (Slot)\n\
\n\
**Description:** Deletes a saved game folder completely.\n\
\n\
**Parameters:**\n\
  * Slot - saved game object\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetSaveGames]]"
);

static PyObject* GemRB_DeleteSaveGame(PyObject * /*self*/, PyObject* args)
{
	PyObject *Slot;

	if (!PyArg_ParseTuple( args, "O", &Slot )) {
		return AttributeError( GemRB_DeleteSaveGame__doc );
	}

	CObject<SaveGame> game(Slot);
	core->GetSaveGameIterator()->DeleteSaveGame( game );
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SaveGame_GetName__doc,
"===== SaveGame_GetName =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetName ()\n\
\n\
**Metaclass Prototype:** GetName ()\n\
\n\
**Description:**  Returns name of the saved game.\n\
\n\
**Return value:** string/int\n\
\n\
**See also:**"
);

static PyObject* GemRB_SaveGame_GetName(PyObject * /*self*/, PyObject* args)
{
	PyObject* Slot;

	if (!PyArg_ParseTuple( args, "O", &Slot )) {
		return AttributeError( GemRB_SaveGame_GetName__doc );
	}

	CObject<SaveGame> save(Slot);
	return PyString_FromString(save->GetName());
}

PyDoc_STRVAR( GemRB_SaveGame_GetDate__doc,
"===== SaveGame_GetDate =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetDate ()\n\
\n\
**Metaclass Prototype:** GetDate ()\n\
\n\
**Description:** Returns date of the saved game.\n\
\n\
**Return value:** string/int\n\
\n\
**See also:**"
);

static PyObject* GemRB_SaveGame_GetDate(PyObject * /*self*/, PyObject* args)
{
	PyObject* Slot;

	if (!PyArg_ParseTuple( args, "O", &Slot )) {
		return AttributeError( GemRB_SaveGame_GetDate__doc );
	}

	CObject<SaveGame> save(Slot);
	return PyString_FromString(save->GetDate());
}

PyDoc_STRVAR( GemRB_SaveGame_GetGameDate__doc,
"===== SaveGame_GetGameDate =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetGameDate ()\n\
\n\
**Metaclass Prototype:** GetGameDate ()\n\
\n\
**Description:** Returns game date of the saved game.\n\
\n\
**Return value:** string/int\n\
\n\
**See also:**"
);

static PyObject* GemRB_SaveGame_GetGameDate(PyObject * /*self*/, PyObject* args)
{
	PyObject* Slot;

	if (!PyArg_ParseTuple( args, "O", &Slot )) {
		return AttributeError( GemRB_SaveGame_GetGameDate__doc );
	}

	CObject<SaveGame> save(Slot);
	return PyString_FromString(save->GetGameDate());
}

PyDoc_STRVAR( GemRB_SaveGame_GetSaveID__doc,
"===== SaveGame_GetSaveID =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetSaveID ()\n\
\n\
**Metaclass Prototype:** GetSaveID ()\n\
\n\
**Description:** Returns ID of the saved game.\n\
\n\
**Return value:** string/int\n\
\n\
**See also:**"
);

static PyObject* GemRB_SaveGame_GetSaveID(PyObject * /*self*/, PyObject* args)
{
	PyObject* Slot;

	if (!PyArg_ParseTuple( args, "O", &Slot )) {
		return AttributeError( GemRB_SaveGame_GetSaveID__doc );
	}

	CObject<SaveGame> save(Slot);
	return PyInt_FromLong(save->GetSaveID());
}

PyDoc_STRVAR( GemRB_SaveGame_GetPreview__doc,
"===== SaveGame_GetPreview =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetPreview ()\n\
\n\
**Metaclass Prototype:** GetPreview ()\n\
\n\
**Description:** Returns preview of the saved game.\n\
\n\
**Return value:** string/int\n\
\n\
**See also:**"
);

static PyObject* GemRB_SaveGame_GetPreview(PyObject * /*self*/, PyObject* args)
{
	PyObject* Slot;

	if (!PyArg_ParseTuple( args, "O", &Slot )) {
		return AttributeError( GemRB_SaveGame_GetPreview__doc );
	}

	CObject<SaveGame> save(Slot);
	return CObject<Sprite2D>(save->GetPreview());
}

PyDoc_STRVAR( GemRB_SaveGame_GetPortrait__doc,
"===== SaveGame_GetPortrait =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetPortrait (save, index)\n\
\n\
**Metaclass Prototype:** GetPortrait (index)\n\
\n\
**Description:** Returns portrait of the saved game.\n\
\n\
**Parameters:** \n\
  * save - save game object\n\
  * index - portrait index\n\
\n\
**Return value:** string/int\n\
\n\
**See also:**"
);

static PyObject* GemRB_SaveGame_GetPortrait(PyObject * /*self*/, PyObject* args)
{
	PyObject* Slot;
	int index;

	if (!PyArg_ParseTuple( args, "Oi", &Slot, &index )) {
		return AttributeError( GemRB_SaveGame_GetPortrait__doc );
	}

	CObject<SaveGame> save(Slot);
	return CObject<Sprite2D>(save->GetPortrait(index));
}

PyDoc_STRVAR( GemRB_GetGamePreview__doc,
"===== GetGamePreview =====\n\
\n\
**Prototype:** GemRB.GetGamePreview ()\n\
\n\
**Description:** Gets current game area preview.\n\
\n\
**Return value:** python image object"
);

static PyObject* GemRB_GetGamePreview(PyObject * /*self*/, PyObject* args)
{
	if (!PyArg_ParseTuple( args, "" )) {
		return AttributeError( GemRB_GetGamePreview__doc );
	}

	GET_GAMECONTROL();
	return CObject<Sprite2D>(gc->GetPreview());
}

PyDoc_STRVAR( GemRB_GetGamePortraitPreview__doc,
"===== GetGamePortraitPreview =====\n\
\n\
**Prototype:** GemRB.GetGamePortraitPreview (PCSlot)\n\
\n\
**Description:** Gets a current game PC portrait.\n\
\n\
**Parameters:** \n\
  * PCSlot - party ID\n\
\n\
**Return value:** python image object"
);

static PyObject* GemRB_GetGamePortraitPreview(PyObject * /*self*/, PyObject* args)
{
	int PCSlotCount;

	if (!PyArg_ParseTuple( args, "i", &PCSlotCount )) {
		return AttributeError( GemRB_GetGamePreview__doc );
	}

	GET_GAMECONTROL();
	return CObject<Sprite2D>(gc->GetPortraitPreview(PCSlotCount));
}

PyDoc_STRVAR( GemRB_Roll__doc,
"===== Roll =====\n\
\n\
**Prototype:** GemRB.Roll (Dice, Size, Add)\n\
\n\
**Description:** Calls traditional dice roll calculation.\n\
\n\
**Parameters:**\n\
  * Dice - the number of the dice.\n\
  * Size - the size of the die.\n\
  * Add  - add this value directly to the sum\n\
\n\
**Return value:** numeric\n\
\n\
**Example:** \n\
  dice = 3\n\
  size = 5\n\
  v = GemRB.Roll (dice, size, 3)\n\
The above example generates a 3d5+3 number."
);

static PyObject* GemRB_Roll(PyObject * /*self*/, PyObject* args)
{
	int Dice, Size, Add;

	if (!PyArg_ParseTuple( args, "iii", &Dice, &Size, &Add )) {
		return AttributeError( GemRB_Roll__doc );
	}
	return PyInt_FromLong( core->Roll( Dice, Size, Add ) );
}


PyDoc_STRVAR( GemRB_Window_CreateTextArea__doc,
"===== Window_CreateTextArea =====\n\
\n\
**Prototype:** GemRB.CreateTextArea (WindowIndex, ControlID, x, y, w, h, font, alignment)\n\
\n\
**Metaclass Prototype:** CreateTextArea (ControlID, x, y, w, h, font, alignment)\n\
\n\
**Description:** Creates and adds a new TextArea to a Window. Used \n\
in PST MapNote editor. The maximum length of the edit field is 500 characters.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the value returned from LoadWindow\n\
  * ControlID   - the new control will be available via this controlID\n\
  * x,y,w,h     - X position, Y position, Width and Height of the control\n\
  * font        - font BAM ResRef\n\
  * alignment   - text alignment\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_CreateLabel]], [[guiscript:Window_CreateMapControl]], [[guiscript:Window_CreateWorldMapControl]], [[guiscript:Window_CreateButton]], [[guiscript:Window_CreateTextEdit]]"
);

static PyObject* GemRB_Window_CreateTextArea(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;
	int alignment; // TODO: currently unused
	Region rgn;
	char *font;

	if (!PyArg_ParseTuple( args, "iiiiiisi", &WindowIndex, &ControlID, &rgn.x,
						  &rgn.y, &rgn.w, &rgn.h, &font, &alignment )) {
		return AttributeError( GemRB_Window_CreateTextArea__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	TextArea* ta = new TextArea(rgn, core->GetFont( font ));
	ta->ControlID = ControlID;
	win->AddControl( ta );

	int ret = GetControlIndex( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
}

// FIXME: probably could use a better home, and probably vary from game to game;
static const Color Hover = {255, 180, 0, 0};
static const Color Selected = {255, 100, 0, 0};

PyDoc_STRVAR( GemRB_TextArea_ListResources__doc,
"===== TextArea_ListResources =====\n\
\n\
**Prototype:** GemRB.ListResources (WindowIndex, ControlIndex, type [, flags])\n\
\n\
**Metaclass Prototype:** ListResources (type [, flags])\n\
\n\
**Description:** Lists the resources of 'type' as selectable options in the TextArea.\n\
\n\
**Parameters:** \n\
  * WindowIndex, ControlIndex - control IDs\n\
  * type - one of CHR_PORTRAITS, CHR_SOUNDS, CHR_EXPORTS or CHR_SCRIPTS\n\
  * flags -  currently only used for CHR_PORTRAITS: 0 means the portraits with 'M' as the suffix, anything else 'S'\n\
\n\
**Return value:** int - the number of options added to the TextArea"
);

static PyObject* GemRB_TextArea_ListResources(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	RESOURCE_DIRECTORY type;
	int flags = 0;

	if (!PyArg_ParseTuple( args, "iii|i", &wi, &ci, &type, &flags )) {
		return AttributeError( GemRB_TextArea_ListResources__doc );
	}
	TextArea* ta = ( TextArea* ) GetControl( wi, ci, IE_GUI_TEXTAREA );
	if (!ta) {
		return NULL;
	}

	struct LastCharFilter : DirectoryIterator::FileFilterPredicate {
		char lastchar;
		LastCharFilter(char lastchar) {
			this->lastchar = tolower(lastchar);
		}

		bool operator()(const char* fname) const {
			const char* extpos = strrchr(fname, '.');
			if (extpos) {
				extpos--;
				return tolower(*extpos) == lastchar;
			}
			return false;
		}
	};

	DirectoryIterator dirit = core->GetResourceDirectory(type);
	bool dirs = false;
	char suffix = 'S';
	switch (type) {
		case DIRECTORY_CHR_PORTRAITS:
			if (flags&1) suffix = 'M';
			if (flags&2) suffix = 'L';
			dirit.SetFilterPredicate(new LastCharFilter(suffix), true);
			break;
		case DIRECTORY_CHR_SOUNDS:
			if (core->HasFeature( GF_SOUNDFOLDERS )) {
				dirs = true;
			} else {
				dirit.SetFilterPredicate(new LastCharFilter('A'), true);
			}
			break;
		case DIRECTORY_CHR_EXPORTS:
		case DIRECTORY_CHR_SCRIPTS:
		default:
			break;
	}

	std::vector<String> strings;
	if (dirit) {
		do {
			const char *name = dirit.GetName();
			if (name[0] == '.' || dirit.IsDirectory() != dirs)
				continue;

			char * str = ConvertCharEncoding(name, core->SystemEncoding, core->TLKEncoding.encoding.c_str());
			String* string = StringFromCString(str);
			free(str);

			if (dirs == false) {
				size_t pos = string->find_last_of(L'.');
				if (pos == String::npos || (type == DIRECTORY_CHR_SOUNDS && pos-- == 0)) {
					delete string;
					continue;
				}
				string->resize(pos);
			}
			strings.push_back(*string);
			delete string;
		} while (++dirit);
	}

	std::vector<SelectOption> TAOptions;
	std::sort(strings.begin(), strings.end());
	for (size_t i =0; i < strings.size(); i++) {
		TAOptions.push_back(std::make_pair(i, strings[i]));
	}
	ta->SetSelectOptions(TAOptions, false, NULL, &Hover, &Selected);

	return PyInt_FromLong( TAOptions.size() );
}


PyDoc_STRVAR( GemRB_TextArea_SetOptions__doc,
"===== TextArea_SetOptions =====\n\
\n\
**Prototype:** GemRB.SetOptions (WindowIndex, ControlIndex, Options)\n\
\n\
**Metaclass Prototype:** SetOptions (Options)\n\
\n\
**Description:** Set the selectable options for the TextArea\n\
\n\
**Parameters:** \n\
  * WindowIndex, ControlIndex - control IDs\n\
  * Options - python list of options\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_TextArea_SetOptions(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	PyObject* list;

	if (!PyArg_ParseTuple( args, "iiO", &wi, &ci, &list )) {
		return AttributeError( GemRB_TextArea_SetOptions__doc );
	}

	if (!PyList_Check(list)) {
		return AttributeError( GemRB_TextArea_SetOptions__doc );
	}

	TextArea* ta = ( TextArea* ) GetControl( wi, ci, IE_GUI_TEXTAREA );
	if (!ta) {
		return NULL;
	}

	// FIXME: should we have an option to sort the list?
	// PyList_Sort(list);
	std::vector<SelectOption> TAOptions;
	PyObject* item = NULL;
	for (int i = 0; i < PyList_Size(list); i++) {
		item = PyList_GetItem(list, i);
		String* string = NULL;
		if(!PyString_Check(item)) {
			if (PyInt_Check(item)) {
				string = core->GetString(PyInt_AsLong(item));
			} else {
				return AttributeError( GemRB_TextArea_SetOptions__doc );
			}
		} else {
			string = StringFromCString(PyString_AsString(item));
		}
		TAOptions.push_back(std::make_pair(i, *string));
		delete string;
	}
	ta->SetSelectOptions(TAOptions, false, NULL, &Hover, &Selected);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetPartySize__doc,
"===== GetPartySize =====\n\
\n\
**Prototype:** GemRB.GetPartySize ()\n\
\n\
**Description:** Returns the actual number of PCs (dead included). This \n\
command works only after a LoadGame().\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** numeric (0-10)\n\
\n\
**See also:** [[guiscript:LoadGame]], [[guiscript:QuitGame]], [[guiscript:GameSetPartySize]]"
);

static PyObject* GemRB_GetPartySize(PyObject * /*self*/, PyObject * /*args*/)
{
	GET_GAME();

	return PyInt_FromLong( game->GetPartySize(0) );
}

PyDoc_STRVAR( GemRB_GetGameTime__doc,
"===== GetGameTime =====\n\
\n\
**Prototype:** GemRB.GetGameTime ()\n\
\n\
**Description:** Returns current game time (seconds since start).\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:GameGetPartyGold]], [[guiscript:GetPartySize]]\n\
"
);

static PyObject* GemRB_GetGameTime(PyObject * /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	unsigned long GameTime = game->GameTime/AI_UPDATE_TIME;
	return PyInt_FromLong( GameTime );
}

PyDoc_STRVAR( GemRB_GameGetReputation__doc,
"===== GameGetReputation =====\n\
\n\
**Prototype:** GemRB.GameGetReputation ()\n\
\n\
**Description:** Returns current party's reputation.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:GetPlayerStat]], [[guiscript:GameSetReputation]]\n\
"
);

static PyObject* GemRB_GameGetReputation(PyObject * /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	int Reputation = (int) game->Reputation;
	return PyInt_FromLong( Reputation );
}

PyDoc_STRVAR( GemRB_GameSetReputation__doc,
"===== GameSetReputation =====\n\
\n\
**Prototype:** GemRB.GameSetReputation (Reputation)\n\
\n\
**Description:** Sets current party's reputation.\n\
\n\
**Parameters:**\n\
  * Reputation - amount to be set. It is divided by ten when displayed.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GameGetReputation]], [[guiscript:IncreaseReputation]]"
);

static PyObject* GemRB_GameSetReputation(PyObject * /*self*/, PyObject* args)
{
	int Reputation;

	if (!PyArg_ParseTuple( args, "i", &Reputation )) {
		return AttributeError( GemRB_GameSetReputation__doc );
	}
	GET_GAME();

	game->SetReputation( (unsigned int) Reputation );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_IncreaseReputation__doc,
"===== IncreaseReputation =====\n\
\n\
**Prototype:** GemRB.IncreaseReputation (Donation)\n\
\n\
**Description:** Increases party's reputation based on Donation. (see reputatio.2da)\n\
\n\
**Parameters:**\n\
  * donation - gold spent to increase reputation. You have to change the\n\
  party's gold separately.\n\
\n\
**Return value:** Nonzero if the reputation has been increased. The amount\n\
of increase is multiplied by ten.\n\
\n\
**See also:** [[guiscript:GameGetReputation]], [[guiscript:GameGetPartyGold]], [[guiscript:GameSetPartyGold]]"
);

static PyObject* GemRB_IncreaseReputation(PyObject * /*self*/, PyObject* args)
{
	int Donation;
	int Increase = 0;

	if (!PyArg_ParseTuple( args, "i", &Donation )) {
		return AttributeError( GemRB_IncreaseReputation__doc );
	}

	GET_GAME();

	int Limit = core->GetReputationMod(8);
	if (Limit > Donation) {
		return PyInt_FromLong (0);
	}
	Increase = core->GetReputationMod(4);
	if (Increase) {
		game->SetReputation( game->Reputation + Increase );
	}
	return PyInt_FromLong ( Increase );
}

PyDoc_STRVAR( GemRB_GameGetPartyGold__doc,
"===== GameGetPartyGold =====\n\
\n\
**Prototype:** GemRB.GameGetPartyGold ()\n\
\n\
**Description:** Returns current party gold.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:GetPlayerStat]], [[guiscript:GameSetPartyGold]]"
);

static PyObject* GemRB_GameGetPartyGold(PyObject * /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	int Gold = game->PartyGold;
	return PyInt_FromLong( Gold );
}

PyDoc_STRVAR( GemRB_GameSetPartyGold__doc,
"===== GameSetPartyGold =====\n\
\n\
**Prototype:** GemRB.GameSetPartyGold (Gold)\n\
\n\
**Description:** Sets current party gold.\n\
\n\
**Parameters:**\n\
  * Gold - the target party gold amount\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GameGetPartyGold]]"
);

static PyObject* GemRB_GameSetPartyGold(PyObject * /*self*/, PyObject* args)
{
	int Gold, flag = 0;

	if (!PyArg_ParseTuple( args, "i|i", &Gold, &flag )) {
		return AttributeError( GemRB_GameSetPartyGold__doc );
	}
	GET_GAME();

	if (flag) {
		game->AddGold((ieDword) Gold);
	} else {
		game->PartyGold=Gold;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GameGetFormation__doc,
"===== GameGetFormation =====\n\
\n\
**Prototype:** GemRB.GameGetFormation ([Which])\n\
\n\
**Description:** Returns current party formation. The formations are stored \n\
in the GemRB specific formatio.2da table. If Which was supplied, it returns \n\
one of the preset formations.\n\
\n\
**Parameters:**\n\
  * Which - optionally return a preset formation (index)\n\
\n\
**Return value:** integer\n\
\n\
**See also:** [[guiscript:GameSetFormation]]"
);

static PyObject* GemRB_GameGetFormation(PyObject * /*self*/, PyObject* args)
{
	int Which = -1;
	int Formation;

	if (!PyArg_ParseTuple( args, "|i", &Which )) {
		return AttributeError( GemRB_GameGetFormation__doc );
	}
	GET_GAME();

	if (Which<0) {
		Formation = game->WhichFormation;
	} else {
		if (Which>4) {
			return AttributeError( GemRB_GameGetFormation__doc );
		}
		Formation = game->Formations[Which];
	}
	return PyInt_FromLong( Formation );
}

PyDoc_STRVAR( GemRB_GameSetFormation__doc,
"===== GameSetFormation =====\n\
\n\
**Prototype:** GemRB.GameSetFormation (Formation[, Which])\n\
\n\
**Description:** Sets party formation. If Which was supplied, it sets one \n\
of the preset formations.\n\
\n\
**Parameters:**\n\
  * Formation - the row index of formatio.2da\n\
  * Which - the preset formation to use (0-4)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GameGetFormation]]\n\
"
);

static PyObject* GemRB_GameSetFormation(PyObject * /*self*/, PyObject* args)
{
	int Formation, Which=-1;

	if (!PyArg_ParseTuple( args, "i|i", &Formation, &Which )) {
		return AttributeError( GemRB_GameSetFormation__doc );
	}
	GET_GAME();

	if (Which<0) {
		game->WhichFormation = Formation;
	} else {
		if (Which>4) {
			return AttributeError( GemRB_GameSetFormation__doc );
		}
		game->Formations[Which] = Formation;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetJournalSize__doc,
"===== GetJournalSize =====\n\
\n\
**Prototype:** GemRB.GetJournalSize(chapter[, section])\n\
\n\
**Description:** Returns the number of entries in the given section of \n\
journal. Please note that various engines implemented the chapter/sections \n\
at various degree. For example PST has none of these. Section will default \n\
to zero.\n\
\n\
**Parameters:**\n\
  * chapter - the chapter of the journal page\n\
  * section - 0,1,2 or 4 - general, quest, solved quest or user notes.\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:GetJournalEntry]], [[guiscript:SetJournalEntry]]\n\
"
);

static PyObject* GemRB_GetJournalSize(PyObject * /*self*/, PyObject * args)
{
	int chapter;
	int section = -1;

	if (!PyArg_ParseTuple( args, "i|i", &chapter, &section )) {
		return AttributeError( GemRB_GetJournalSize__doc );
	}

	GET_GAME();

	int count = 0;
	for (unsigned int i = 0; i < game->GetJournalCount(); i++) {
		GAMJournalEntry* je = game->GetJournalEntry( i );
		if ((section == -1 || section == je->Section) && (chapter==je->Chapter) )
			count++;
	}

	return PyInt_FromLong( count );
}

PyDoc_STRVAR( GemRB_GetJournalEntry__doc,
"===== GetJournalEntry =====\n\
\n\
**Prototype:** GemRB.GetJournalEntry (chapter, index[, section])\n\
\n\
**Description:** Returns dictionary representing journal entry with given \n\
chapter, section and index. Section will default to zero.\n\
\n\
**Parameters:**\n\
  * chapter - the chapter of the journal entry\n\
  * index - the index of the entry in the given section/chapter\n\
  * section - the section of the journal to use\n\
\n\
**Return value:** dictionary with the following fields:\n\
  * 'Text'     - strref of the journal entry\n\
  * 'GameTime' - time of entry\n\
  * 'Section'  - same as the input parameter\n\
  * 'Chapter'  - same as the input parameter\n\
\n\
**See also:** [[guiscript:GetJournalSize]], [[guiscript:SetJournalEntry]]"
);

static PyObject* GemRB_GetJournalEntry(PyObject * /*self*/, PyObject * args)
{
	int section=-1, index, chapter;

	if (!PyArg_ParseTuple( args, "ii|i", &chapter, &index, &section )) {
		return AttributeError( GemRB_GetJournalEntry__doc );
	}

	GET_GAME();

	int count = 0;
	for (unsigned int i = 0; i < game->GetJournalCount(); i++) {
		GAMJournalEntry* je = game->GetJournalEntry( i );
		if ((section == -1 || section == je->Section) && (chapter == je->Chapter)) {
			if (index == count) {
				PyObject* dict = PyDict_New();
				PyDict_SetItemString(dict, "Text", PyInt_FromLong ((signed) je->Text));
				PyDict_SetItemString(dict, "GameTime", PyInt_FromLong (je->GameTime));
				PyDict_SetItemString(dict, "Section", PyInt_FromLong (je->Section));
				PyDict_SetItemString(dict, "Chapter", PyInt_FromLong (je->Chapter));

				return dict;
			}
			count++;
		}
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetJournalEntry__doc,
"===== SetJournalEntry =====\n\
\n\
**Prototype:** GemRB.SetJournalEntry (strref[, section, chapter])\n\
\n\
**Description:** Sets a journal journal entry with given chapter and section. \n\
If section was not given, then it will delete the entry. Chapter is \n\
optional, if it is omitted, then the current chapter will be used. If \n\
strref is -1, then it will delete the whole journal.\n\
\n\
**Parameters:**\n\
  * strref - strref of the journal entry\n\
  * section - the section of the journal (only if the journal has sections)\n\
  * chapter - the chapter of the journal entry\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetJournalEntry]]\n\
"
);

static PyObject* GemRB_SetJournalEntry(PyObject * /*self*/, PyObject * args)
{
	int section=-1, chapter = -1, strref;

	if (!PyArg_ParseTuple( args, "i|ii", &strref, &section, &chapter )) {
		return AttributeError( GemRB_SetJournalEntry__doc );
	}

	GET_GAME();

	if (strref == -1) {
		//delete the whole journal
		section = -1;
	}

	if (section==-1) {
		//delete one or all entries
		game->DeleteJournalEntry( strref);
	} else {
		if (chapter == -1) {
			ieDword tmp = -1;
			game->locals->Lookup("CHAPTER", tmp);
			chapter = (int) tmp;
		}
		game->AddJournalEntry( chapter, section, strref);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GameIsBeastKnown__doc,
"===== GameIsBeastKnown =====\n\
\n\
**Prototype:** GameIsBeastKnown (index)\n\
\n\
**Description:** Returns whether beast with given index is known to PCs. \n\
Works only in PST.\n\
\n\
**Parameters:**\n\
  * index - the beast's index as of beast.ini\n\
\n\
**Return value:** boolean, 1 means beast is known.\n\
\n\
**See also:** [[guiscript:GetINIBeastsKey]]\n\
"
);

static PyObject* GemRB_GameIsBeastKnown(PyObject * /*self*/, PyObject * args)
{
	int index;
	if (!PyArg_ParseTuple( args, "i", &index )) {
		return AttributeError( GemRB_GameIsBeastKnown__doc );
	}

	GET_GAME();

	return PyInt_FromLong( game->IsBeastKnown( index ));
}

PyDoc_STRVAR( GemRB_GetINIPartyCount__doc,
"===== GetINIPartyCount =====\n\
\n\
**Prototype:** GemRB.GetINIPartyCount ()\n\
\n\
**Description:** Returns the Number of Parties defined in Party.ini. \n\
Works only in IWD2.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** the number of predefined parties in party.ini\n\
\n\
**See also:** [[guiscript:GetINIPartyKey]]\n\
"
);

static PyObject* GemRB_GetINIPartyCount(PyObject * /*self*/,
	PyObject * /*args*/)
{
	if (!core->GetPartyINI()) {
		return RuntimeError( "INI resource not found!\n" );
	}
	return PyInt_FromLong( core->GetPartyINI()->GetTagsCount() );
}

PyDoc_STRVAR( GemRB_GetINIQuestsKey__doc,
"===== GetINIQuestsKey =====\n\
\n\
**Prototype:** GemRB.GetINIQuestsKey (Tag, Key, Default)\n\
\n\
**Description:** Returns a Value from the quests.ini file. \n\
Works only in PST.\n\
\n\
**Parameters:**\n\
  * Tag - a section in the quests.ini file\n\
  * Key - a field in the section\n\
  * Default - default value in case the entry doesn't exist\n\
\n\
**Return value:** string, the entry from the ini file\n\
\n\
**See also:** [[guiscript:GetINIBeastsKey]]\n\
"
);

static PyObject* GemRB_GetINIQuestsKey(PyObject * /*self*/, PyObject* args)
{
	char *Tag, *Key, *Default;

	if (!PyArg_ParseTuple( args, "sss", &Tag, &Key, &Default )) {
		return AttributeError( GemRB_GetINIQuestsKey__doc );
	}
	if (!core->GetQuestsINI()) {
		return RuntimeError( "INI resource not found!\n" );
	}
	return PyString_FromString(
			core->GetQuestsINI()->GetKeyAsString( Tag, Key, Default ) );
}

PyDoc_STRVAR( GemRB_GetINIBeastsKey__doc,
"===== GetINIBeastsKey =====\n\
\n\
**Prototype:** GemRB.GetINIBeastsKey (Tag, Key, Default)\n\
\n\
**Description:** Returns a Value from the beast.ini file. \n\
Works only in PST.\n\
\n\
**Parameters:**\n\
  * Tag - a section in the beast.ini file\n\
  * Key - a field in the section\n\
  * Default - default value in case the entry doesn't exist\n\
\n\
**Return value:** string, the entry from the ini file\n\
\n\
**See also:** [[guiscript:GetINIQuestsKey]]"
);
static PyObject* GemRB_GetINIBeastsKey(PyObject * /*self*/, PyObject* args)
{
	char *Tag, *Key, *Default;

	if (!PyArg_ParseTuple( args, "sss", &Tag, &Key, &Default )) {
		return AttributeError( GemRB_GetINIBeastsKey__doc );
	}
	if (!core->GetBeastsINI()) {
		return NULL;
	}
	return PyString_FromString(
			core->GetBeastsINI()->GetKeyAsString( Tag, Key, Default ) );
}

PyDoc_STRVAR( GemRB_GetINIPartyKey__doc,
"===== GetINIPartyKey =====\n\
\n\
**Prototype:** GemRB.GetINIPartyKey(Tag, Key, Default)\n\
\n\
**Description:** Returns a Value from the party.ini file. \n\
Works only in IWD2.\n\
\n\
**Parameters:**\n\
  * Tag - a section in the party.ini file\n\
  * Key - a field in the section\n\
  * Default - default value in case the entry doesn't exist\n\
\n\
**Return value:** string, the entry from the ini file\n\
\n\
**See also:** [[guiscript:GetINIPartyCount]]"
);

static PyObject* GemRB_GetINIPartyKey(PyObject * /*self*/, PyObject* args)
{
	const char *Tag, *Key, *Default;

	if (!PyArg_ParseTuple( args, "sss", &Tag, &Key, &Default )) {
		return AttributeError( GemRB_GetINIPartyKey__doc );
	}
	if (!core->GetPartyINI()) {
		return RuntimeError( "INI resource not found!\n" );
	}
	return PyString_FromString(
			core->GetPartyINI()->GetKeyAsString( Tag, Key, Default ) );
}

PyDoc_STRVAR( GemRB_CreatePlayer__doc,
"===== CreatePlayer =====\n\
\n\
**Prototype:** CreatePlayer (CREResRef, Slot [,Import, VersionOverride])\n\
\n\
**Description:** Adds an actor (PC) to the current game. It works only \n\
after a LoadGame() was executed, and should be used before an EnterGame(). \n\
It is also used to import a .chr file as a PC. A new character will need \n\
additional SetPlayerStat() and FillPlayerInfo() calls to be a working \n\
character. \n\
Note: if the slot is already filled, it will delete that pc instead!\n\
\n\
**Parameters:**\n\
  * CREResRef - name of the creature to use, usually 'charbase'\n\
  * Slot      - The player character's position in the party\n\
  * Import    - Set it to 1 if you want to import a .chr instead of creating a new character\n\
  * VersionOverride - Force CRE version of new actor.\n\
\n\
**Return value:** the new player's index in the game structure\n\
\n\
**Examples:**\n\
  MyChar = GemRB.GetVar ('Slot')\n\
  GemRB.CreatePlayer ('charbase', MyChar)\n\
The above example will create a new player character in the slot selected\n\
by the Slot variable.\n\
\n\
  MyChar = GemRB.GetVar ('Slot')\n\
  ImportName = 'avenger'\n\
  GemRB.CreatePlayer (ImportName, MyChar, 1)\n\
The above example would import avenger.chr into the slot selected by the \n\
Slot Variable. (If it exists in the Characters directory of the game).\n\
\n\
**See also:** [[guiscript:LoadGame]], [[guiscript:EnterGame]], [[guiscript:QuitGame]], [[guiscript:FillPlayerInfo]], [[guiscript:SetPlayerStat]]"
);

static PyObject* GemRB_CreatePlayer(PyObject * /*self*/, PyObject* args)
{
	const char *CreResRef;
	int PlayerSlot, Slot;
	int Import=0;
	int VersionOverride = -1;

	if (!PyArg_ParseTuple( args, "si|ii", &CreResRef, &PlayerSlot, &Import, &VersionOverride)) {
		return AttributeError( GemRB_CreatePlayer__doc );
	}
	//PlayerSlot is zero based
	Slot = ( PlayerSlot & 0x7fff );
	GET_GAME();

	//FIXME:overwriting original slot
	//is dangerous if the game is already loaded
	//maybe the actor should be removed from the area first
	if (PlayerSlot & 0x8000) {
		PlayerSlot = game->FindPlayer( Slot );
		if (PlayerSlot >= 0) {
			game->DelPC(PlayerSlot, true);
		}
	} else {
		PlayerSlot = game->FindPlayer( PlayerSlot );
		if (PlayerSlot >= 0) {
			return RuntimeError("Slot is already filled!\n");
		}
	}
	if (CreResRef[0]) {
		PlayerSlot = gamedata->LoadCreature( CreResRef, Slot, (bool) Import, VersionOverride );
	} else {
		//just destroyed the previous actor, not going to create one
		PlayerSlot = 0;
	}
	if (PlayerSlot < 0) {
		return RuntimeError("File not found!\n");
	}
	return PyInt_FromLong( PlayerSlot );
}

PyDoc_STRVAR( GemRB_GetPlayerStates__doc,
"===== GetPlayerStates =====\n\
\n\
**Prototype:** GemRB.GetPlayerStates (PartyID)\n\
\n\
**Description:** Returns the active spell states on the player. The state \n\
descriptions are in the statdesc.2da file which comes with the original \n\
games. The values in the character array equal to the corresponding cycle \n\
number in states.bam. To reference statdesc.2da, subtract 65 from the \n\
values.\n\
\n\
**Parameters:**\n\
  * PartyID - the character's position in the party\n\
\n\
**Return value:** a string whose letters are greater or equal ascii 65. \n\
Using the states.bam font, they will be drawn as the status icons.\n\
\n\
**See also:** [[guiscript:GetPlayerName]], [[guiscript:GetPlayerStat]]\n\
"
);

static PyObject* GemRB_GetPlayerStates(PyObject * /*self*/, PyObject* args)
{
	int globalID;

	if (!PyArg_ParseTuple( args, "i", &globalID )) {
		return AttributeError( GemRB_GetPlayerStates__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyString_FromString((const char *) actor->GetStateString() );
}

PyDoc_STRVAR( GemRB_GetPlayerName__doc,
"===== GetPlayerName =====\n\
\n\
**Prototype:** GemRB.GetPlayerName (PartyID[, LongOrShort])\n\
\n\
**Description:** Queries the player character's (script)name.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party (1 based)\n\
  * LongOrShort - which name to query\n\
    * -1: default name\n\
    * 0: shortname\n\
    * 1: longname\n\
    * 2: scripting name\n\
\n\
**Return value:** string\n\
\n\
**See also:** [[guiscript:SetPlayerName]], [[guiscript:GetPlayerStat]]"
);

static PyObject* GemRB_GetPlayerName(PyObject * /*self*/, PyObject* args)
{
	int globalID, Which;

	Which = 0;
	if (!PyArg_ParseTuple( args, "i|i", &globalID, &Which )) {
		return AttributeError( GemRB_GetPlayerName__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Which == 2) {
		return PyString_FromString( actor->GetScriptName() );
	}

	return PyString_FromString( actor->GetName(Which) );
}

PyDoc_STRVAR( GemRB_SetPlayerName__doc,
"===== SetPlayerName =====\n\
\n\
**Prototype:** GemRB.SetPlayerName(Slot, Name[, LongOrShort])\n\
\n\
**Description:** Sets the player name. Each actor has 2 names, this \n\
command can set either or both.\n\
\n\
**Parameters:**\n\
  * Slot - numeric, the character's slot\n\
  * Name - string, the name\n\
  * LongOrShort - 0 (both), 1 (short), 2 (long)\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
  GemRB.SetPlayerName (MyChar, GemRB.GetToken('CHARNAME'), 0)\n\
In the above example we set the player's name to a previously set Token (global string).\n\
\n\
**See also:** [[guiscript:Control_QueryText]], [[guiscript:GetToken]]"
);

static PyObject* GemRB_SetPlayerName(PyObject * /*self*/, PyObject* args)
{
	const char *Name=NULL;
	int globalID, Which;

	Which = 0;
	if (!PyArg_ParseTuple( args, "is|i", &globalID, &Name, &Which )) {
		return AttributeError( GemRB_SetPlayerName__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetName(Name, Which);
	actor->SetMCFlag(MC_EXPORTABLE,OP_OR);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_CreateString__doc,
"===== CreateString =====\n\
\n\
**Prototype:** GemRB.CreateString (Strref, Text)\n\
\n\
**Description:** Creates or updates a custom string.\n\
\n\
**Parameters:**\n\
  * Strref - string index to use\n\
  * Text - string contents"
);

static PyObject* GemRB_CreateString(PyObject * /*self*/, PyObject* args)
{
	const char *Text;
	ieStrRef strref;

	if (!PyArg_ParseTuple( args, "is", &strref, &Text )) {
		return AttributeError( GemRB_CreateString__doc );
	}
	GET_GAME();

	strref = core->UpdateString(strref, Text);
	return PyInt_FromLong( strref );
}

PyDoc_STRVAR( GemRB_SetPlayerString__doc,
"===== SetPlayerString =====\n\
Missing function already used in bg2 biography (cg uses a token)\n\
\n\
**Prototype:** GemRB.SetPlayerString (PlayerSlot, StringSlot, StrRef)\n\
\n\
**Description:** Sets one of the player character's verbal constants. \n\
Mostly useful for setting the biography.\n\
\n\
**Parameters:**\n\
  * PlayerSlot - party ID\n\
  * StringSlot - verbal constant index (0-99)\n\
  * StrRef - new string resref\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_SetPlayerString(PyObject * /*self*/, PyObject* args)
{
	int globalID, StringSlot, StrRef;

	if (!PyArg_ParseTuple( args, "iii", &globalID, &StringSlot, &StrRef )) {
		return AttributeError( GemRB_SetPlayerString__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (StringSlot>=VCONST_COUNT) {
		return AttributeError( "StringSlot is out of range!\n" );
	}

	actor->StrRefs[StringSlot]=StrRef;

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetPlayerSound__doc,
"===== SetPlayerSound =====\n\
\n\
**Prototype:** GemRB.SetPlayerSound (Slot, SoundFolder)\n\
\n\
**Description:** Sets the player character's soundset.\n\
\n\
**Parameters:**\n\
  * Slot        - numeric, the character's slot\n\
  * SoundFolder - string, a folder in Sounds (iwd2 style), or a filename (bg2 style)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetPlayerSound]], [[guiscript:FillPlayerInfo]], [[guiscript:SetPlayerString]]"
);

static PyObject* GemRB_SetPlayerSound(PyObject * /*self*/, PyObject* args)
{
	const char *Sound=NULL;
	int globalID;

	if (!PyArg_ParseTuple( args, "is", &globalID, &Sound )) {
		return AttributeError( GemRB_SetPlayerSound__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetSoundFolder(Sound);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetPlayerSound__doc,
"===== GetPlayerSound =====\n\
\n\
**Prototype:** GemRB.GetPlayerSound (Slot[, flags])\n\
\n\
**Description:**  Gets the player character's sound set.\n\
\n\
**Parameters:** \n\
  * Slot - party slot\n\
  * flags - if set, the whole subpath will be returned for games with sound subfolders\n\
\n\
**Return value:** string\n\
\n\
**See also:** [[guiscript:SetPlayerSound]]"
);

static PyObject* GemRB_GetPlayerSound(PyObject * /*self*/, PyObject* args)
{
	char Sound[42];
	int globalID;
	int flag = 0;

	if (!PyArg_ParseTuple( args, "i|i", &globalID, &flag )) {
		return AttributeError( GemRB_GetPlayerSound__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->GetSoundFolder(Sound, flag);
	return PyString_FromString(Sound);
}

PyDoc_STRVAR( GemRB_GetSlotType__doc,
"===== GetSlotType =====\n\
\n\
**Prototype:** GemRB.GetSlotType (idx[, PartyID])\n\
\n\
**Description:** Returns dictionary of an itemslot type (slottype.2da).\n\
\n\
**Parameters:**\n\
  * idx - a row number of slottype.2da\n\
  * PartyID - optional actor ID for a richer dictionary\n\
\n\
**Return value:** dictionary\n\
'Type'   - bitfield, The inventory slot's type.\n\
'ID'     - the gui button's controlID which belongs to this slot.\n\
'Tip'    - the tooltip resref for this slot.\n\
'ResRef' - the background .bam of the slot.\n\
\n\
**See also:** [[guiscript:Button_SetItemIcon]]\n\
"
);

static PyObject* GemRB_GetSlotType(PyObject * /*self*/, PyObject* args)
{
	int idx;
	int PartyID = 0;
	Actor *actor = NULL;

	if (!PyArg_ParseTuple( args, "i|i", &idx, &PartyID )) {
		return AttributeError( GemRB_GetSlotType__doc );
	}

	if (PartyID) {
		GET_GAME();

		actor = game->FindPC( PartyID );
	}

	PyObject* dict = PyDict_New();
	if (idx==-1) {
		PyDict_SetItemString(dict, "Count", PyInt_FromLong(core->GetInventorySize()));
		return dict;
	}
	int tmp = core->QuerySlot(idx);
	if (core->QuerySlotEffects(idx)==0xffffffffu) {
		tmp=idx;
	}

	PyDict_SetItemString(dict, "Slot", PyInt_FromLong(tmp));
	PyDict_SetItemString(dict, "Type", PyInt_FromLong((int)core->QuerySlotType(tmp)));
	PyDict_SetItemString(dict, "ID", PyInt_FromLong((int)core->QuerySlotID(tmp)));
	PyDict_SetItemString(dict, "Tip", PyInt_FromLong((int)core->QuerySlottip(tmp)));
	//see if the actor shouldn't have some slots displayed
	if (!actor || !actor->PCStats) {
		goto has_slot;
	}
	//WARNING:idx isn't used any more, recycling it
	idx = actor->inventory.GetWeaponSlot();
	if (tmp<idx || tmp>idx+3) {
		goto has_slot;
	}
	if (actor->GetQuickSlot(tmp-idx)==0xffff) {
		PyDict_SetItemString(dict, "ResRef", PyString_FromString (""));
		goto continue_quest;
	}
has_slot:
	PyDict_SetItemString(dict, "ResRef", PyString_FromString (core->QuerySlotResRef(tmp)));
continue_quest:
	PyDict_SetItemString(dict, "Effects", PyInt_FromLong (core->QuerySlotEffects(tmp)));
	return dict;
}

PyDoc_STRVAR( GemRB_GetPCStats__doc,
"===== GetPCStats =====\n\
\n\
**Prototype:** GemRB.GetPCStats (PartyID)\n\
\n\
**Description:** Returns dictionary of PC's performance stats.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party (1 based)\n\
\n\
**Return value:** A Python dictionary containing the following items\n\
  * 'BestKilledName'   - strref of killed creature with biggest XP\n\
  * 'BestKilledXP'     - XP value of this creature\n\
  * 'JoinDate'         - date joined the team\n\
  * 'KillsChapterXP'   - total XP from kills gathered in this chapter\n\
  * 'KillsChapterCount'- total number of kills in this chapter\n\
  * 'KillsTotalXP'     - total XP from kills\n\
  * 'KillsTotalCount'  - total number of kills\n\
  * 'FavouriteSpell'   - spell used the most of the time\n\
  * 'FavouriteWeapon'  - weapon bringing the most kill XP\n\
\n\
**See also:** [[guiscript:GetPlayerStat]]"
);

static PyObject* GemRB_GetPCStats(PyObject * /*self*/, PyObject* args)
{
	int PartyID;

	if (!PyArg_ParseTuple( args, "i", &PartyID )) {
		return AttributeError( GemRB_GetPCStats__doc );
	}
	GET_GAME();

	Actor* MyActor = game->FindPC( PartyID );
	if (!MyActor || !MyActor->PCStats) {
		Py_RETURN_NONE;
	}

	PyObject* dict = PyDict_New();
	PCStatsStruct* ps = MyActor->PCStats;

	PyDict_SetItemString(dict, "BestKilledName", PyInt_FromLong ((signed) ps->BestKilledName));
	PyDict_SetItemString(dict, "BestKilledXP", PyInt_FromLong (ps->BestKilledXP));
	PyDict_SetItemString(dict, "AwayTime", PyInt_FromLong (ps->AwayTime));
	PyDict_SetItemString(dict, "JoinDate", PyInt_FromLong (ps->JoinDate));
	PyDict_SetItemString(dict, "KillsChapterXP", PyInt_FromLong (ps->KillsChapterXP));
	PyDict_SetItemString(dict, "KillsChapterCount", PyInt_FromLong (ps->KillsChapterCount));
	PyDict_SetItemString(dict, "KillsTotalXP", PyInt_FromLong (ps->KillsTotalXP));
	PyDict_SetItemString(dict, "KillsTotalCount", PyInt_FromLong (ps->KillsTotalCount));

	if (ps->FavouriteSpells[0][0]) {
		int largest = 0;

		// 0 already has the top candidate, but we double check for old saves
		for (int i = 1; i < 4; ++i) {
			if (ps->FavouriteSpellsCount[i] > ps->FavouriteSpellsCount[largest]) {
				largest = i;
			}
		}

		Spell* spell = gamedata->GetSpell(ps->FavouriteSpells[largest]);
		if (spell == NULL) {
			return NULL;
		}

		PyDict_SetItemString(dict, "FavouriteSpell", PyInt_FromLong ((signed) spell->SpellName));

		gamedata->FreeSpell( spell, ps->FavouriteSpells[largest], false );
	} else {
		PyDict_SetItemString(dict, "FavouriteSpell", PyInt_FromLong (-1));
	}

	if (ps->FavouriteWeapons[0][0]) {
		int largest = 0;

		for (int i = 1; i < 4; ++i) {
			if (ps->FavouriteWeaponsCount[i] > ps->FavouriteWeaponsCount[largest]) {
				largest = i;
			}
		}

		Item* item = gamedata->GetItem(ps->FavouriteWeapons[largest]);
		if (item == NULL) {
			return RuntimeError( "Item not found!\n" );
		}

		PyDict_SetItemString(dict, "FavouriteWeapon", PyInt_FromLong ((signed) item->GetItemName(true)));

		gamedata->FreeItem( item, ps->FavouriteWeapons[largest], false );
	} else {
		PyDict_SetItemString(dict, "FavouriteWeapon", PyInt_FromLong (-1));
	}

	return dict;
}


PyDoc_STRVAR( GemRB_GameSelectPC__doc,
"===== GameSelectPC =====\n\
\n\
**Prototype:** GemRB.GameSelectPC (PartyID, Selected[, Flags = SELECT_NORMAL])\n\
\n\
**Description:** Selects or deselects a PC. Note: some things use a \n\
different PC selection mechanism (dialogs and stores are not unified yet).\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party, 0 means ALL\n\
  * Selected - boolean\n\
  * Flags - bitflags\n\
    * SELECT_REPLACE - if set deselect all other actors\n\
    * SELECT_QUIET - do not run SelectionHandler (no GUI feedback)\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
def SelectAllOnPress ():\n\
  GemRB.GameSelectPC (0, 1)\n\
  return\n\
The above function is associated to the 'select all' button of the GUI screen.\n\
\n\
**See also:** [[guiscript:GameIsPCSelected]], [[guiscript:GameSelectPCSingle]], [[guiscript:GameGetSelectedPCSingle]], [[guiscript:GameGetFirstSelectedPC]]"
);

static PyObject* GemRB_GameSelectPC(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Select;
	int Flags = SELECT_NORMAL;

	if (!PyArg_ParseTuple( args, "ii|i", &PartyID, &Select, &Flags )) {
		return AttributeError( GemRB_GameSelectPC__doc );
	}
	GET_GAME();

	Actor* actor;
	if (PartyID > 0) {
		actor = game->FindPC( PartyID );
		if (!actor) {
			Py_RETURN_NONE;
		}
	} else {
		actor = NULL;
	}

	game->SelectActor( actor, (bool) Select, Flags );
	if (actor && (bool) Select && !(Flags&SELECT_QUIET)) {
		actor->PlaySelectionSound();
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GameIsPCSelected__doc,
"===== GameIsPCSelected =====\n\
\n\
**Prototype:** GemRB.GameIsPCSelected (Slot)\n\
\n\
**Description:** Returns true if the PC is selected.\n\
\n\
**Parameters:**\n\
  * Slot - the PC's position in the party (1 based)\n\
\n\
**Return value:** boolean, 1 if the PC is selected\n\
\n\
**See also:** [[guiscript:GameSelectPC]], [[guiscript:GameGetFirstSelectedPC]]\n\
"
);

static PyObject* GemRB_GameIsPCSelected(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot;

	if (!PyArg_ParseTuple( args, "i", &PlayerSlot )) {
		return AttributeError( GemRB_GameIsPCSelected__doc );
	}
	GET_GAME();

	Actor* MyActor = game->FindPC( PlayerSlot );
	if (!MyActor) {
		return PyInt_FromLong( 0 );
	}
	return PyInt_FromLong( MyActor->IsSelected() );
}


PyDoc_STRVAR( GemRB_GameSelectPCSingle__doc,
"===== GameSelectPCSingle =====\n\
\n\
**Prototype:** GemRB.GameSelectPCSingle (PartyID)\n\
\n\
**Description:** Selects one PC in non-walk environment (i.e. in shops, \n\
inventory, ...).\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GameSelectPC]], [[guiscript:GameGetSelectedPCSingle]]"
);

static PyObject* GemRB_GameSelectPCSingle(PyObject * /*self*/, PyObject* args)
{
	int index;

	if (!PyArg_ParseTuple( args, "i", &index )) {
		return AttributeError( GemRB_GameSelectPCSingle__doc );
	}

	GET_GAME();

	game->SelectPCSingle( index );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GameGetSelectedPCSingle__doc,
"===== GameGetSelectedPCSingle =====\n\
\n\
**Prototype:** GemRB.GameGetSelectedPCSingle (flag)\n\
\n\
**Description:** If flag is 0 or omitted, then returns currently active pc \n\
in non-walk environment (i.e. in shops, inventory, ...).  If flag is set to \n\
non-zero, then returns the currently speaking PC. \n\
If there is no such PC, then returns 0.\n\
\n\
**Parameters:**\n\
  * flag - 0/1\n\
\n\
**Return value:** PartyID (1-10)\n\
\n\
**See also:** [[guiscript:GameSelectPC]], [[guiscript:GameSelectPCSingle]]"
);

static PyObject* GemRB_GameGetSelectedPCSingle(PyObject * /*self*/, PyObject* args)
{
	int flag = 0;

	if (!PyArg_ParseTuple( args, "|i", &flag )) {
		return AttributeError( GemRB_GameGetSelectedPCSingle__doc );
	}
	GET_GAME();

	if (flag) {
		GET_GAMECONTROL();

		Actor *ac = gc->dialoghandler->GetSpeaker();
		int ret = 0;
		if (ac) {
			ret = ac->InParty;
		}
		return PyInt_FromLong( ret );
	}
	return PyInt_FromLong( game->GetSelectedPCSingle() );
}

PyDoc_STRVAR( GemRB_GameGetFirstSelectedPC__doc,
"===== GameGetFirstSelectedPC =====\n\
\n\
**Prototype:** GemRB.GameGetFirstSelectedPC ()\n\
\n\
**Description:** Returns index of the first selected PC or 0 if none.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** the first selected PC's position in the party (it will \n\
look in the original party order, thus the protagonist will be always \n\
first!)\n\
\n\
**See also:** [[guiscript:GameSelectPC]], [[guiscript:GameIsPCSelected]], [[guiscript:GameGetFirstSelectedActor]]\n\
"
);

static PyObject* GemRB_GameGetFirstSelectedPC(PyObject * /*self*/, PyObject* /*args*/)
{
	Actor *actor = core->GetFirstSelectedPC(false);
	if (actor) {
		return PyInt_FromLong( actor->InParty);
	}

	return PyInt_FromLong( 0 );
}

PyDoc_STRVAR( GemRB_GameGetFirstSelectedActor__doc,
"===== GameGetFirstSelectedActor =====\n\
\n\
**Prototype:** GemRB.GameGetFirstSelectedActor ()\n\
\n\
**Description:**  Returns the global ID of the first selected actor or 0 if none.\n\
\n\
**Return value:** int\n\
\n\
**See also:** [[guiscript:GameGetFirstSelectedPC]]"
);

static PyObject* GemRB_GameGetFirstSelectedActor(PyObject * /*self*/, PyObject* /*args*/)
{
	Actor *actor = core->GetFirstSelectedActor();
	if (actor) {
		return PyInt_FromLong( actor->GetGlobalID() );
	}

	return PyInt_FromLong( 0 );
}

PyDoc_STRVAR( GemRB_GameControlSetLastActor__doc,
"===== GameControlSetLastActor =====\n\
\n\
**Prototype:** GemRB.GameControlSetLastActor ([partyID])\n\
\n\
**Description:** Sets LastActor in the GameControl object. The LastActor \n\
object is the character which is currently being hovered over by the \n\
player. Its feet circle is flickering.\n\
\n\
**Parameters:**\n\
  * partyID - 0 to delete any previous settings, or the pc ID.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GameSelectPCSingle]]\n\
"
);

static PyObject* GemRB_GameControlSetLastActor(PyObject * /*self*/, PyObject* args)
{
	int PartyID = 0;

	if (!PyArg_ParseTuple( args, "|i", &PartyID )) {
		return AttributeError( GemRB_GameControlSetLastActor__doc );
	}

	GET_GAME();

	GET_GAMECONTROL();

	Actor* actor = game->FindPC( PartyID );
	gc->SetLastActor(actor, gc->GetLastActor() );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_ActOnPC__doc,
"===== ActOnPC =====\n\
\n\
**Prototype:** GemRB.ActOnPC (player)\n\
\n\
**Description:** Targets the selected PC for an action (cast spell, attack,  ...)\n\
\n\
**Parameters:**\n\
  * player - the pc's party position (1-10)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:ClearActions]], [[guiscript:SetModalState]], [[guiscript:SpellCast]]"
);

static PyObject* GemRB_ActOnPC(PyObject * /*self*/, PyObject* args)
{
	int PartyID;

	if (!PyArg_ParseTuple( args, "i", &PartyID )) {
		return AttributeError( GemRB_ActOnPC__doc );
	}
	GET_GAME();

	Actor* MyActor = game->FindPC( PartyID );
	if (MyActor) {
		GameControl* gc = core->GetGameControl();
		if(gc) {
			gc->PerformActionOn(MyActor);
		}
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetPlayerPortrait__doc,
"===== GetPlayerPortrait =====\n\
\n\
**Prototype:** GemRB.GetPlayerPortrait (Slot[, SmallOrLarge])\n\
\n\
**Description:** Queries the player's portrait. To set the portrait of a \n\
new character you must use FillPlayerInfo().\n\
\n\
**Parameters:**\n\
  * Slot         - the PC's position in the party\n\
  * SmallOrLarge - boolean, specify 1 if you want to get the large portrait\n\
\n\
**Return value:** the player's portrait name (image resref)\n\
\n\
**See also:** [[guiscript:FillPlayerInfo]]\n\
"
);

static PyObject* GemRB_GetPlayerPortrait(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, Which;

	Which = 0;
	if (!PyArg_ParseTuple( args, "i|i", &PlayerSlot, &Which )) {
		return AttributeError( GemRB_GetPlayerPortrait__doc );
	}
	GET_GAME();

	Actor* MyActor = game->FindPC( PlayerSlot );
	if (!MyActor) {
		return PyString_FromString( "");
	}
	return PyString_FromString( MyActor->GetPortrait(Which) );
}

PyDoc_STRVAR( GemRB_GetPlayerString__doc,
"===== GetPlayerString =====\n\
\n\
**Prototype:** GemRB.GetPlayerString (globalID, StringIndex)\n\
\n\
**Description:** Returns the string reference of a Verbal Constant set in the player. \n\
The biography string is an example of such a string.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * StringIndex - the verbal constant's index\n\
\n\
**Return value:** a string reference.\n\
\n\
**See also:** [[guiscript:GetPlayerName]], [[guiscript:GetPlayerStat]], [[guiscript:GetPlayerScript]]\n\
\n\
**See also:** sndslot.ids, soundoff.ids (it is a bit unclear which one is it)\n\
"
);

static PyObject* GemRB_GetPlayerString(PyObject * /*self*/, PyObject* args)
{
	int globalID, Index, StatValue;

	if (!PyArg_ParseTuple( args, "ii", &globalID, &Index)) {
		return AttributeError( GemRB_GetPlayerString__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Index>=VCONST_COUNT) {
		return RuntimeError("String reference is too high!\n");
	}
	StatValue = GetCreatureStrRef( actor, Index );
	return PyInt_FromLong( StatValue );
}

PyDoc_STRVAR( GemRB_GetPlayerStat__doc,
"===== GetPlayerStat =====\n\
\n\
**Prototype:** GemRB.GetPlayerStat(globalID, StatID[, Base])\n\
\n\
**Description:** Queries a stat of the player character. The stats are \n\
listed in ie_stats.py. For IWD2 skills, it takes all bonuses into account.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * StatID - stat index\n\
  * Base - if set to 1, the function will return the base instead of the modified (current) value\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:SetPlayerStat]], [[guiscript:GetPlayerName]], [[guiscript:GetPlayerStates]]"
);

static PyObject* GemRB_GetPlayerStat(PyObject * /*self*/, PyObject* args)
{
	int globalID, StatID, StatValue, BaseStat;

	BaseStat = 0;
	if (!PyArg_ParseTuple( args, "ii|i", &globalID, &StatID, &BaseStat )) {
		return AttributeError( GemRB_GetPlayerStat__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	//returning the modified stat if BaseStat was 0 (default)
	StatValue = GetCreatureStat( actor, StatID, !BaseStat );

	// special handling for the hidden hp
	if ((unsigned)StatValue == 0xdadadada) {
		return PyString_FromString("?");
	} else {
		return PyInt_FromLong(StatValue);
	}
}

PyDoc_STRVAR( GemRB_SetPlayerStat__doc,
"===== SetPlayerStat =====\n\
\n\
**Prototype:** GemRB.SetPlayerStat (globalID, ID, Value[, PCF])\n\
\n\
**Description:** Sets a player character's base stat. The stats are listed \n\
in ie_stats.py.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * ID - Stat index\n\
  * Value - New stat value\n\
  * PCF - Set to 0 if you don't want the stat's post-change function to be ran\n\
\n\
**Return value:** N/A\n\
\n\
**Example:** \n\
  PickedColor = ColorTable.GetValue (ColorIndex, GemRB.GetVar('Selected'))\n\
  GemRB.SetPlayerStat (pc, IE_MAJOR_COLOR, PickedColor)\n\
The above example sets the player's color just picked via the color customisation dialog. ColorTable holds the available colors.\n\
\n\
**See also:** [[guiscript:GetPlayerStat]], [[guiscript:SetPlayerName]], [[guiscript:ApplyEffect]]"
);

static PyObject* GemRB_SetPlayerStat(PyObject * /*self*/, PyObject* args)
{
	int globalID, StatID, StatValue;
	int pcf = 1;

	if (!PyArg_ParseTuple( args, "iii|i", &globalID, &StatID, &StatValue, &pcf)) {
		return AttributeError( GemRB_SetPlayerStat__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	//Setting the creature's base stat
	SetCreatureStat( actor, StatID, StatValue, pcf);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetPlayerScript__doc,
"===== GetPlayerScript =====\n\
\n\
**Prototype:** GemRB.GetPlayerScript (globalID[, Index])\n\
\n\
**Description:** Queries the player's script. If index is omitted, it will \n\
default to the class script slot (customisable by players).\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * Index - script index (see scrlev.ids)\n\
\n\
**Return value:** the player's script (.bcs or .baf resref)\n\
\n\
**See also:** [[guiscript:SetPlayerScript]]\n\
"
);

static PyObject* GemRB_GetPlayerScript(PyObject * /*self*/, PyObject* args)
{
	//class script is the custom slot for player scripts
	int globalID, Index = SCR_CLASS;

	if (!PyArg_ParseTuple( args, "i|i", &globalID, &Index )) {
		return AttributeError( GemRB_GetPlayerScript__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	const char *scr = actor->GetScript(Index);
	if (scr[0]==0) {
		Py_RETURN_NONE;
	}
	return PyString_FromString( scr );
}

PyDoc_STRVAR( GemRB_SetPlayerScript__doc,
"===== SetPlayerScript =====\n\
\n\
**Prototype:** GemRB.SetPlayerScript (globalID, ScriptName[, Index])\n\
\n\
**Description:** Sets the player character's script. Normally only the class \n\
script is customisable via the GUI (used if Index is omitted).\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * ScriptName - the script resource\n\
  * Index      - the script index (see scrlev.ids)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetPlayerScript]]"
);

static PyObject* GemRB_SetPlayerScript(PyObject * /*self*/, PyObject* args)
{
	const char *ScriptName;
	int globalID, Index = SCR_CLASS;

	if (!PyArg_ParseTuple( args, "is|i", &globalID, &ScriptName, &Index )) {
		return AttributeError( GemRB_SetPlayerScript__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetScript(ScriptName, Index, true);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetPlayerDialog__doc,
"===== SetPlayerDialog =====\n\
\n\
**Prototype:** GemRB.SetPlayerDialog (globalID, Resource)\n\
\n\
**Description:** Sets the dialog resource for a player.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * Resource - the dialog resource\n\
\n\
**Return value:** N/A\n\
"
);

static PyObject* GemRB_SetPlayerDialog(PyObject * /*self*/, PyObject* args)
{
	const char *DialogName;
	int globalID;

	if (!PyArg_ParseTuple( args, "is", &globalID, &DialogName )) {
		return AttributeError( GemRB_SetPlayerDialog__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetDialog(DialogName);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_FillPlayerInfo__doc,
"===== FillPlayerInfo =====\n\
\n\
**Prototype:** GemRB.FillPlayerInfo (globalID[, Portrait1, Portrait2, clear=0])\n\
\n\
**Description:** Fills basic character info that is not stored in stats. \n\
This command will generate an AnimationID for the character based on the \n\
avprefix.2da table, the character must have the stats referenced in the \n\
avprefix structure already set. It will also set the player's portraits if \n\
given. It will set the actor's area/position according to the 'PlayMode' \n\
variable and the Slot value (using the startpos.2da table). This command \n\
must be called once after a character was created and before EnterGame().\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * Portrait1 - medium (or large) portrait\n\
  * Portrait2 - small portrait\n\
  * clear - clear all the quickslot/spell/item fields?\n\
\n\
avprefix.2da is a gemrb specific table. Its first row contains the base animationID used for the actor. Its optional additional rows contain other table resrefs which refine the animationID by different player stats. The first row of these tables contain the stat which affects the animationID. The other rows assign cumulative values to the animationID. \n\
\n\
**For example:**\n\
avprefix.2da\n\
        RESOURCE\n\
0       0x6000\n\
1       avprefr\n\
2       avprefg\n\
3       avprefc\n\
\n\
avprefr.2da\n\
                RACE\n\
TYPE            201\n\
HUMAN           0\n\
ELF             1\n\
HALF_ELF        1\n\
GNOME           4\n\
HALFLING        3\n\
DWARF           2\n\
HALFORC         5\n\
\n\
Based on the avatar's stat (201 == race) the animationID (0x6000) will be increased by the given values. For example an elf's animationID will be 0x6001. The animationID will be further modified by gender and class.\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
  GemRB.FillPlayerInfo (MyChar, PortraitName+'M', PortraitName+'S')\n\
\n\
**See also:** [[guiscript:LoadGame]], [[guiscript:CreatePlayer]], [[guiscript:SetPlayerStat]], [[guiscript:EnterGame]]\n\
"
);

static PyObject* GemRB_FillPlayerInfo(PyObject * /*self*/, PyObject* args)
{
	int globalID, clear = 0;
	const char *Portrait1=NULL, *Portrait2=NULL;

	if (!PyArg_ParseTuple( args, "i|ssi", &globalID, &Portrait1, &Portrait2, &clear)) {
		return AttributeError( GemRB_FillPlayerInfo__doc );
	}
	// here comes some code to transfer icon/name to the PC sheet
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Portrait1) {
		actor->SetPortrait( Portrait1, 1);
	}
	if (Portrait2) {
		actor->SetPortrait( Portrait2, 2);
	}

	//set up animation ID
	switch(actor->UpdateAnimationID(0)) {
	case -1: return RuntimeError("avprefix table contains no entries." );
	case -2: return RuntimeError("Couldn't load avprefix table.");
	case -3: return RuntimeError("Couldn't load an avprefix subtable.");
	}

	// clear several fields (only useful for cg; currently needed only in iwd2, but that will change if its system is ported to the rest)
	// fixes random action bar mess, kill stats, join time ...
	if (clear) {
		actor->PCStats->Init(false);
	}

	actor->SetOver( false );
	actor->InitButtons(actor->GetActiveClass(), true); // force re-init of actor's action bar

	//what about multiplayer?
	if ((globalID == 1) && core->HasFeature(GF_HAS_DPLAYER) ) {
		actor->SetScript("DPLAYER3", SCR_DEFAULT, false);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_Button_SetSpellIcon__doc,
"===== Button_SetSpellIcon =====\n\
\n\
**Prototype:** GemRB.SetSpellIcon (WindowIndex, ControlIndex, SPLResRef[, Type, Tooltip, Function])\n\
\n\
**Metaclass Prototype:** SetSpellIcon (SPLResRef[, Type, Tooltip, Function])\n\
\n\
**Description:** Sets Spell icon image on a Button control. Type determines \n\
the icon type, if set to 1 it will use the Memorised Icon instead of the \n\
Spellbook Icon\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * SPLResRef - the name of the spell (.spl resref)\n\
  * Type - 0 (default, use parchment background) or 1 (use stone background)\n\
  * Tooltip - 0 (default); if 1, set the tooltip 'F<n> <spell_name>'\n\
  * Function - F-key number to be used in the tooltip above\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetItemIcon]]"
);

static PyObject *SetSpellIcon(int wi, int ci, const ieResRef SpellResRef, int type, int tooltip, int Function)
{
	Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
	if (!btn) {
		return NULL;
	}

	if (!SpellResRef[0]) {
		btn->SetPicture( NULL );
		//no incref here!
		return Py_None;
	}

	Spell* spell = gamedata->GetSpell( SpellResRef, 1 );
	if (spell == NULL) {
		btn->SetPicture( NULL );
		Log(ERROR, "GUIScript", "Spell not found :%.8s",
			SpellResRef);
		//no incref here!
		return Py_None;
	}

	const char* IconResRef;
	if (type) {
		IconResRef = spell->ext_headers[0].MemorisedIcon;
	}
	else {
		IconResRef = spell->SpellbookIcon;
	}
	AnimationFactory* af = ( AnimationFactory* )
		gamedata->GetFactoryResource( IconResRef,
				IE_BAM_CLASS_ID, IE_NORMAL, 1 );
	if (!af) {
		char tmpstr[24];

		snprintf(tmpstr,sizeof(tmpstr),"%s BAM not found", IconResRef);
		return RuntimeError( tmpstr );
	}
	//small difference between pst and others
	if (af->GetCycleSize(0)!=4) { //non-pst
		btn->SetPicture( af->GetFrame(0, 0));
	}
	else { //pst
		btn->SetImage( BUTTON_IMAGE_UNPRESSED, af->GetFrame(0, 0));
		btn->SetImage( BUTTON_IMAGE_PRESSED, af->GetFrame(1, 0));
		btn->SetImage( BUTTON_IMAGE_SELECTED, af->GetFrame(2, 0));
		btn->SetImage( BUTTON_IMAGE_DISABLED, af->GetFrame(3, 0));
	}
	if (tooltip) {
		char *str = core->GetCString(spell->SpellName,0);
		SetFunctionTooltip(wi, ci, str, Function); //will free str
	}
	gamedata->FreeSpell( spell, SpellResRef, false );
	//no incref here!
	return Py_None;
}

static PyObject* GemRB_Button_SetSpellIcon(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	const char *SpellResRef;
	int type=0;
	int tooltip=0;
	int Function=0;

	if (!PyArg_ParseTuple( args, "iis|iii", &wi, &ci, &SpellResRef, &type, &tooltip, &Function )) {
		return AttributeError( GemRB_Button_SetSpellIcon__doc );
	}
	PyObject *ret = SetSpellIcon(wi, ci, SpellResRef, type, tooltip, Function);
	if (ret) {
		Py_INCREF(ret);
	}
	return ret;
}


static Sprite2D* GetUsedWeaponIcon(Item *item, int which)
{
	ITMExtHeader *ieh = item->GetWeaponHeader(false);
	if (!ieh) {
		ieh = item->GetWeaponHeader(true);
	}
	if (ieh) {
		return gamedata->GetBAMSprite(ieh->UseIcon, -1, which, true);
	}
	return gamedata->GetBAMSprite(item->ItemIcon, -1, which, true);
}

static void SetItemText(Button* btn, int charges, bool oneisnone)
{
	if (!btn) return;

	wchar_t usagestr[10];
	if (charges && (charges>1 || !oneisnone) ) {
		swprintf(usagestr, sizeof(usagestr)/sizeof(usagestr[0]), L"%d", charges);
	} else {
		usagestr[0] = 0;
	}
	btn->SetText(usagestr);
}

PyDoc_STRVAR( GemRB_Button_SetItemIcon__doc,
"===== Button_SetItemIcon =====\n\
\n\
**Prototype:** GemRB.SetItemIcon (WindowIndex, ControlIndex, ITMResRef[, type, tooltip, Function, ITM2ResRef])\n\
\n\
**Metaclass Prototype:** SetItemIcon (ITMResRef[, Type, Tooltip, ITM2ResRef])\n\
\n\
**Description:** Sets Item icon image on a Button control.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the control's reference\n\
  * ITMResRef                 - the name of the item (.itm resref)\n\
  * Type                      - the icon's type\n\
    * 0 - Inventory Icon1\n\
    * 1 - Inventory Icon2\n\
    * 2 - Description Icon (for BG)\n\
    * 3 - No Icon (empty slot)\n\
    * 4 - Activation Icon1\n\
    * 5 - Activation Icon2\n\
    * 6 - Item ability icon for first extended header\n\
    * 7 - Item ability icon for second extended header\n\
    * 8 - etc.\n\
  * Tooltip  - if set to 1, the tooltip for the item will also be set\n\
  * ITM2ResRef - if set, a second item to display in the icon. ITM2 is drawn first. The tooltip of ITM is used. Only valid for Type 4 and 5\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetSpellIcon]], [[guiscript:Button_SetActionIcon]]"
);

static PyObject *SetItemIcon(int wi, int ci, const char *ItemResRef, int Which, int tooltip, int Function, const char *Item2ResRef)
{
	Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
	if (!btn) {
		return NULL;
	}

	if (!ItemResRef[0]) {
		btn->SetPicture( NULL );
		//no incref here!
		return Py_None;
	}
	Item* item = gamedata->GetItem(ItemResRef, true);
	if (item == NULL) {
		btn->SetPicture(NULL);
		//no incref here!
		return Py_None;
	}

	btn->SetFlags(IE_GUI_BUTTON_PICTURE, OP_OR);
	Sprite2D* Picture;
	bool setpicture = true;
	int i;
	switch (Which) {
	case 0: case 1:
		Picture = gamedata->GetBAMSprite(item->ItemIcon, -1, Which, true);
		break;
	case 2:
		btn->SetPicture( NULL ); // also calls ClearPictureList
		for (i=0;i<4;i++) {
			Picture = gamedata->GetBAMSprite(item->DescriptionIcon, -1, i, true);
			if (Picture)
				btn->StackPicture(Picture);
		}
		//fallthrough
	case 3:
		setpicture = false;
		Picture = NULL;
		break;
	case 4: case 5:
		Picture = GetUsedWeaponIcon(item, Which-4);
		if (Item2ResRef) {
			btn->SetPicture( NULL ); // also calls ClearPictureList
			Item* item2 = gamedata->GetItem(Item2ResRef, true);
			if (item2) {
				Sprite2D* Picture2;
				Picture2 = gamedata->GetBAMSprite(item2->ItemIcon, -1, Which-4, true);
				if (Picture2) btn->StackPicture(Picture2);
				gamedata->FreeItem( item2, Item2ResRef, false );
			}
			if (Picture) btn->StackPicture(Picture);
			setpicture = false;
		}
		break;
	default:
		ITMExtHeader *eh = item->GetExtHeader(Which-6);
		if (eh) {
			Picture = gamedata->GetBAMSprite(eh->UseIcon, -1, 0, true);
		}
		else {
			Picture = NULL;
		}
	}

	if (setpicture)
		btn->SetPicture( Picture );
	if (tooltip) {
		//later getitemname could also return tooltip stuff
		char *str = core->GetCString(item->GetItemName(tooltip==2),0);
		//this will free str, no need of freeing it
		SetFunctionTooltip(wi, ci, str, Function);
	}

	gamedata->FreeItem( item, ItemResRef, false );
	//no incref here!
	return Py_None;
}

static PyObject* GemRB_Button_SetItemIcon(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	const char *ItemResRef;
	int Which = 0;
	int tooltip = 0;
	int Function = 0;
	const char *Item2ResRef = NULL;

	if (!PyArg_ParseTuple( args, "iis|iiis", &wi, &ci, &ItemResRef, &Which, &tooltip, &Function, &Item2ResRef )) {
		return AttributeError( GemRB_Button_SetItemIcon__doc );
	}

	PyObject *ret = SetItemIcon(wi, ci, ItemResRef, Which, tooltip, Function, Item2ResRef);
	if (ret) {
		Py_INCREF(ret);
	}
	return ret;
}

PyDoc_STRVAR( GemRB_EnterStore__doc,
"===== EnterStore =====\n\
\n\
**Prototype:** GemRB.EnterStore (StoreResRef)\n\
\n\
**Description:** Loads a store, sets it as current and opens the window.\n\
\n\
**Parameters:**\n\
  * StoreResRef - the store's resource name\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetStore]], [[guiscript:GetStoreCure]], [[guiscript:GetStoreDrink]], [[guiscript:LeaveStore]], [[guiscript:SetPurchasedAmount]]\n\
"
);

static PyObject* GemRB_EnterStore(PyObject * /*self*/, PyObject* args)
{
	const char* StoreResRef;

	if (!PyArg_ParseTuple( args, "s", &StoreResRef )) {
		return AttributeError( GemRB_EnterStore__doc );
	}

	//stores are cached, bags could be opened while in shops
	//so better just switch to the requested store silently
	//the core will be intelligent enough to not do excess work
	core->SetCurrentStore( StoreResRef, 0 );

	core->SetEventFlag(EF_OPENSTORE);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_LeaveStore__doc,
"===== LeaveStore =====\n\
\n\
**Prototype:** GemRB.LeaveStore ()\n\
\n\
**Description:** Saves the current store to the Cache folder and removes it \n\
from memory. If there was no active store, this function causes a runtime \n\
error.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:EnterStore]], [[guiscript:GetStore]]\n\
"
);

static PyObject* GemRB_LeaveStore(PyObject * /*self*/, PyObject* /*args*/)
{
	core->CloseCurrentStore();
	core->ResetEventFlag(EF_OPENSTORE);
	core->SetEventFlag(EF_PORTRAIT);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_LoadRighthandStore__doc,
"===== LoadRighthandStore =====\n\
\n\
**Prototype:** GemRB.LoadRighthandStore (StoreResRef)\n\
\n\
**Description:** Loads a secondary (right-hand) store.  Used for trading to/from\n\
containers. The previous right-hand store, if any, is saved to cache.\n\
\n\
**Parameters:**\n\
  * StoreResRef - the store's resource name\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:CloseRighthandStore]], [[guiscript:GetStore]], [[guiscript:GetStoreItem]], [[guiscript:SetPurchasedAmount]]\n\
"
);

static PyObject* GemRB_LoadRighthandStore(PyObject * /*self*/, PyObject* args)
{
	const char* StoreResRef;
	if (!PyArg_ParseTuple(args, "s", &StoreResRef)) {
		return AttributeError(GemRB_LoadRighthandStore__doc);
	}

	Store *newrhstore = gamedata->GetStore(StoreResRef);
	if (rhstore && rhstore != newrhstore) {
		gamedata->SaveStore(rhstore);
	}
	rhstore = newrhstore;
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_CloseRighthandStore__doc,
"===== CloseRighthandStore =====\n\
\n\
**Prototype:** GemRB.CloseRighthandStore ()\n\
\n\
**Description:** Unloads the current right-hand store and saves it to cache.\n\
If there was no right-hand store opened, the function does nothing.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:LoadRighthandStore]]\n\
"
);

static PyObject* GemRB_CloseRighthandStore(PyObject * /*self*/, PyObject* /*args*/)
{
	gamedata->SaveStore(rhstore);
	rhstore = NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_LeaveContainer__doc,
"===== LeaveContainer =====\n\
\n\
**Prototype:** GemRB.LeaveContainer ()\n\
\n\
**Description:** Closes the current container by calling 'CloseContainerWindow' \n\
in the next update cycle. You cannot call 'CloseContainerWindow' directly, \n\
because the core system needs to know if the container subwindow is still \n\
open. This function will also remove empty ground piles.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetContainer]], [[guiscript:GetContainerItem]], [[guiscript:LeaveStore]]"
);

static PyObject* GemRB_LeaveContainer(PyObject * /*self*/, PyObject* /*args*/)
{
	core->CloseCurrentContainer();
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetContainer__doc,
"===== GetContainer =====\n\
\n\
**Prototype:** GemRB.GetContainer (PartyID[, autoselect])\n\
\n\
**Description:** Gets the current container's type and other basic header \n\
information. The player is always the first selected player. If PartyID is \n\
0 then the default PC is the first multiselected PC. Autoselect will always \n\
select a groundpile. If there is no container at the feet of the PC \n\
autoselect will create the container.\n\
\n\
**Parameters:**\n\
  * PartyID    - the PC's position in the party\n\
  * autoselect - is 1 if you call this function from a player inventory (so you select the pile at their feet)\n\
\n\
**Return value:** dictionary\n\
  * 'Type'      - the container's type, numeric (see IESDP)\n\
  * 'ItemCount' - the number of items in the container\n\
\n\
**See also:** [[guiscript:GetStore]], [[guiscript:GameGetFirstSelectedPC]], [[guiscript:GetContainerItem]]"
);

static PyObject* GemRB_GetContainer(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	int autoselect=0;

	if (!PyArg_ParseTuple( args, "i|i", &PartyID, &autoselect )) {
		return AttributeError( GemRB_GetContainer__doc );
	}

	Actor *actor;

	GET_GAME();

	if (PartyID) {
		actor = game->FindPC( PartyID );
	} else {
		actor = core->GetFirstSelectedPC(false);
	}
	if (!actor) {
		return RuntimeError( "Actor not found!\n" );
	}
	Container *container = NULL;
	if (autoselect) { //autoselect works only with piles
		Map *map = actor->GetCurrentArea();
		if (!map) {
			return RuntimeError("No current area!");
		}
		//GetContainer should create an empty container
		container = map->GetPile(actor->Pos);
	} else {
		container = core->GetCurrentContainer();
	}
	if (!container) {
		return RuntimeError("No current container!");
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "Type", PyInt_FromLong( container->Type ));
	PyDict_SetItemString(dict, "ItemCount", PyInt_FromLong( container->inventory.GetSlotCount() ));

	return dict;
}

PyDoc_STRVAR( GemRB_GetContainerItem__doc,
"===== GetContainerItem =====\n\
\n\
**Prototype:** GemRB.GetContainerItem (PartyID, index)\n\
\n\
**Description:** Returns the container item referenced by the index. If \n\
PartyID is 0 then the container was opened manually and should be the \n\
current container. If PartyID is not 0 then the container is autoselected \n\
and should be at the feet of the player.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party\n\
  * index   - the item's index in the container\n\
\n\
**Return value:** dictionary\n\
  * 'ItemResRef' - the ResRef of the item\n\
  * 'ItemName'   - the StrRef of the item's name (identified or not)\n\
  * 'Usages0'    - The primary charges of the item (or the item's stack amount if the item is stackable).\n\
  * 'Usages1'    - The secondary charges of the item.\n\
  * 'Usages2'    - The tertiary charges of the item.\n\
  * 'Flags'      - Item flags.\n\
\n\
**See also:** [[guiscript:GetContainer]], [[guiscript:GameGetFirstSelectedPC]], [[guiscript:GetStoreItem]]\n\
"
);

static PyObject* GemRB_GetContainerItem(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	int index;

	if (!PyArg_ParseTuple( args, "ii", &globalID, &index )) {
		return AttributeError( GemRB_GetContainerItem__doc );
	}
	Container *container;

	if (globalID) {
		GET_GAME();
		GET_ACTOR_GLOBAL();

		Map *map = actor->GetCurrentArea();
		if (!map) {
			return RuntimeError("No current area!");
		}
		container = map->TMap->GetContainer(actor->Pos, IE_CONTAINER_PILE);
	} else {
		container = core->GetCurrentContainer();
	}
	if (!container) {
		return RuntimeError("No current container!");
	}
	if (index>=(int) container->inventory.GetSlotCount()) {
		Py_RETURN_NONE;
	}
	PyObject* dict = PyDict_New();

	CREItem *ci=container->inventory.GetSlotItem( index );

	PyDict_SetItemString(dict, "ItemResRef", PyString_FromResRef( ci->ItemResRef ));
	PyDict_SetItemString(dict, "Usages0", PyInt_FromLong (ci->Usages[0]));
	PyDict_SetItemString(dict, "Usages1", PyInt_FromLong (ci->Usages[1]));
	PyDict_SetItemString(dict, "Usages2", PyInt_FromLong (ci->Usages[2]));
	PyDict_SetItemString(dict, "Flags", PyInt_FromLong (ci->Flags));

	Item *item = gamedata->GetItem(ci->ItemResRef, true);
	if (!item) {
		Log(MESSAGE, "GUIScript", "Cannot find container (%s) item %s!", container->GetScriptName(), ci->ItemResRef);
		Py_RETURN_NONE;
	}

	bool identified = ci->Flags & IE_INV_ITEM_IDENTIFIED;
	PyDict_SetItemString(dict, "ItemName", PyInt_FromLong( (signed) item->GetItemName( identified )) );
	PyDict_SetItemString(dict, "ItemDesc", PyInt_FromLong( (signed) item->GetItemDesc( identified )) );
	gamedata->FreeItem( item, ci->ItemResRef, false );
	return dict;
}

PyDoc_STRVAR( GemRB_ChangeContainerItem__doc,
"===== ChangeContainerItem =====\n\
\n\
**Prototype:** GemRB.ChangeContainerItem (PartyID, slot, action)\n\
\n\
**Description:** Moves an item from PC's inventory into a container or vice \n\
versa. If PartyID is 0 then PC is the first selected PC and container is \n\
the current container. If PartyID is not 0 then the container is the pile \n\
at the feet of that PC.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party\n\
  * slot    - the item's inventory or container slot\n\
  * action\n\
    * 0 - put item of PC into container\n\
    * 1 - get item from container and put it in PC's inventory\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetContainer]], [[guiscript:GetSlotItem]]\n\
"
);

static PyObject* GemRB_ChangeContainerItem(PyObject * /*self*/, PyObject* args)
{
	int globalID, Slot;
	int action;

	if (!PyArg_ParseTuple( args, "iii", &globalID, &Slot, &action)) {
		return AttributeError( GemRB_ChangeContainerItem__doc );
	}
	GET_GAME();

	Container *container;
	Actor *actor = NULL;

	if (globalID) {
		if (globalID > 1000) {
			actor = game->GetActorByGlobalID( globalID );
		} else {
			actor = game->FindPC( globalID );
		}
		if (!actor) {
			return RuntimeError( "Actor not found!\n" );
		}
		Map *map = actor->GetCurrentArea();
		if (!map) {
			return RuntimeError("No current area!");
		}
		container = map->TMap->GetContainer(actor->Pos, IE_CONTAINER_PILE);
	} else {
		actor = core->GetFirstSelectedPC(false);
		container = core->GetCurrentContainer();
	}
	if (!actor) {
		return RuntimeError( "Actor not found!\n" );
	}
	if (!container) {
		return RuntimeError("No current container!");
	}

	ieResRef Sound;
	CREItem *si;
	int res;

	Sound[0]=0;
	if (action) { //get stuff from container
		if (Slot<0 || Slot>=(int) container->inventory.GetSlotCount()) {
			return RuntimeError("Invalid Container slot!");
		}

		si = container->inventory.GetSlotItem(Slot);
		res = core->CanMoveItem(si);
		if (!res) { //cannot move
			Log(MESSAGE, "GUIScript", "Cannot move item, it is undroppable!");
			Py_RETURN_NONE;
		}

		// check for full inventory up front to prevent unnecessary shuffling of container
		// items; note that shuffling will still occur when picking up stacked items where
		// not the entire stack fits
		if (res == -1) { // not gold
			if (actor->inventory.FindCandidateSlot(SLOT_INVENTORY, 0, si->ItemResRef) == -1) {
				Py_RETURN_NONE;
			}
		}

		//this will update the container
		si = container->RemoveItem(Slot,0);
		if (!si) {
			Log(WARNING, "GUIScript", "Cannot move item, there is something weird!");
			Py_RETURN_NONE;
		}
		Item *item = gamedata->GetItem(si->ItemResRef);
		if (item) {
			if (core->HasFeature(GF_HAS_PICK_SOUND) && item->ReplacementItem[0]) {
				memcpy(Sound,item->ReplacementItem,sizeof(ieResRef));
			} else {
				gamedata->GetItemSound(Sound, item->ItemType, item->AnimationType, IS_DROP);
			}
			gamedata->FreeItem(item, si->ItemResRef,0);
		}
		if (res!=-1) { //it is gold!
			game->PartyGold += res;
			delete si;
		} else {
			res = actor->inventory.AddSlotItem(si, SLOT_ONLYINVENTORY);
			if (res !=ASI_SUCCESS) { //putting it back
				container->AddItem(si);
			}
		}
	} else { //put stuff in container, simple!
		res = core->CanMoveItem(actor->inventory.GetSlotItem(core->QuerySlot(Slot) ) );
		if (!res) { //cannot move
			Log(MESSAGE, "GUIScript","Cannot move item, it is undroppable!");
			Py_RETURN_NONE;
		}

		si = actor->inventory.RemoveItem(core->QuerySlot(Slot));
		if (!si) {
			Log(WARNING, "GUIScript", "Cannot move item, there is something weird!");
			Py_RETURN_NONE;
		}
		Item *item = gamedata->GetItem(si->ItemResRef);
		if (item) {
			if (core->HasFeature(GF_HAS_PICK_SOUND) && item->DescriptionIcon[0]) {
				memcpy(Sound,item->DescriptionIcon,sizeof(ieResRef));
			} else {
				gamedata->GetItemSound(Sound, item->ItemType, item->AnimationType, IS_GET);
			}
			gamedata->FreeItem(item, si->ItemResRef,0);
		}
		actor->ReinitQuickSlots();

		if (res!=-1) { //it is gold!
			game->PartyGold += res;
			delete si;
		} else {
			container->AddItem(si);
		}
	}

	if (Sound[0]) {
		core->GetAudioDrv()->Play(Sound, SFX_CHAN_GUI);
	}

	//keep weight up to date
	actor->CalculateSpeed(false);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetStore__doc,
"===== GetStore =====\n\
\n\
**Prototype:** GemRB.GetStore ([righthand])\n\
\n\
**Description:** Gets the basic header information of the current store and \n\
returns it in a dictionary.\n\
\n\
**Parameters:**\n\
  * righthand - set to non-zero to query the right-hand store (bag) instead\n\
\n\
**Return value:** dictionary\n\
  * 'StoreType'       - numeric (see IESDP)\n\
  * 'StoreName'       - the StrRef of the store name\n\
  * 'StoreDrinkCount' - the count of drinks served (tavern)\n\
  * 'StoreCureCount'  - the count of cures served (temple)\n\
  * 'StoreItemCount'  - the count of items sold, in case of PST the availability trigger is also checked\n\
  * 'StoreCapacity'   - the capacity of the store\n\
  * 'StoreRoomPrices' - a four elements tuple, negative if the room type is unavailable\n\
  * 'StoreButtons'    - a four elements tuple, possible actions\n\
  * 'StoreFlags'      - the store flags if you ever need them, StoreButtons is a digested information, but you might have something else in mind based on these\n\
  * 'TavernRumour'    - ResRef of tavern rumour dialog\n\
  * 'TempleRumour'    - ResRef of temple rumour dialog\n\
\n\
**See also:** [[guiscript:EnterStore]], [[guiscript:GetStoreCure]], [[guiscript:GetStoreDrink]], [[guiscript:GetRumour]]\n\
"
);

#define STOREBUTTON_COUNT 7
#define STORETYPE_COUNT 7
static int storebuttons[STORETYPE_COUNT][STOREBUTTON_COUNT]={
//store
{STA_BUYSELL,STA_IDENTIFY|STA_OPTIONAL,STA_STEAL|STA_OPTIONAL,STA_DONATE|STA_OPTIONAL,STA_CURE|STA_OPTIONAL,STA_DRINK|STA_OPTIONAL,STA_ROOMRENT|STA_OPTIONAL},
//tavern
{STA_DRINK,STA_BUYSELL|STA_OPTIONAL,STA_IDENTIFY|STA_OPTIONAL,STA_STEAL|STA_OPTIONAL,STA_DONATE|STA_OPTIONAL,STA_CURE|STA_OPTIONAL,STA_ROOMRENT|STA_OPTIONAL},
//inn
{STA_ROOMRENT,STA_BUYSELL|STA_OPTIONAL,STA_DRINK|STA_OPTIONAL,STA_STEAL|STA_OPTIONAL,STA_IDENTIFY|STA_OPTIONAL,STA_DONATE|STA_OPTIONAL,STA_CURE|STA_OPTIONAL},
//temple
{STA_CURE, STA_DONATE|STA_OPTIONAL,STA_BUYSELL|STA_OPTIONAL,STA_IDENTIFY|STA_OPTIONAL,STA_STEAL|STA_OPTIONAL,STA_DRINK|STA_OPTIONAL,STA_ROOMRENT|STA_OPTIONAL},
//iwd container
{STA_BUYSELL,-1,-1,-1,-1,-1,-1},
//no need to steal from your own container (original engine had STEAL instead of DRINK)
{STA_BUYSELL,STA_IDENTIFY|STA_OPTIONAL,STA_DRINK|STA_OPTIONAL,STA_CURE|STA_OPTIONAL,-1,-1,-1},
//gemrb specific store type: (temple 2), added steal, removed identify
{STA_BUYSELL,STA_STEAL|STA_OPTIONAL,STA_DONATE|STA_OPTIONAL,STA_CURE|STA_OPTIONAL} };

//buy/sell, identify, steal, cure, donate, drink, rent
static int storebits[7]={IE_STORE_BUY|IE_STORE_SELL,IE_STORE_ID,IE_STORE_STEAL,
IE_STORE_CURE,IE_STORE_DONATE,IE_STORE_DRINK,IE_STORE_RENT};

static PyObject* GemRB_GetStore(PyObject * /*self*/, PyObject* args)
{
	int rh = 0;
	if (!PyArg_ParseTuple( args, "|i", &rh )) {
		return AttributeError( GemRB_GetStore__doc );
	}

	Store *store;
	if (rh) {
		store = rhstore;
	} else {
		store = core->GetCurrentStore();
	}
	if (!store) {
		Py_INCREF( Py_None );
		return Py_None;
	}
	if (store->Type>STORETYPE_COUNT-1) {
		store->Type=STORETYPE_COUNT-1;
	}
	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "StoreType", PyInt_FromLong( store->Type ));
	PyDict_SetItemString(dict, "StoreName", PyInt_FromLong( (signed) store->StoreName ));
	PyDict_SetItemString(dict, "StoreDrinkCount", PyInt_FromLong( store->DrinksCount ));
	PyDict_SetItemString(dict, "StoreCureCount", PyInt_FromLong( store->CuresCount ));
	PyDict_SetItemString(dict, "StoreItemCount", PyInt_FromLong( store->GetRealStockSize() ));
	PyDict_SetItemString(dict, "StoreCapacity", PyInt_FromLong( store->Capacity ));
	PyDict_SetItemString(dict, "StoreOwner", PyInt_FromLong( store->GetOwnerID() ));
	PyObject* p = PyTuple_New( 4 );

	int i;
	int j=1;
	int k;
	for (i = 0; i < 4; i++) {
		if (store->AvailableRooms&j) {
			k = store->RoomPrices[i];
		}
		else k=-1;
		PyTuple_SetItem( p, i, PyInt_FromLong( k ) );
		j<<=1;
	}
	PyDict_SetItemString(dict, "StoreRoomPrices", p);

	p = PyTuple_New( STOREBUTTON_COUNT );
	j=0;
	for (i = 0; i < STOREBUTTON_COUNT; i++) {
		k = storebuttons[store->Type][i];
		if (k&STA_OPTIONAL) {
			k&=~STA_OPTIONAL;
			//check if the type was disabled
			if (!(store->Flags & storebits[k]) ) {
				continue;
			}
		}
		PyTuple_SetItem( p, j++, PyInt_FromLong( k ) );
	}
	for (; j < STOREBUTTON_COUNT; j++) {
		PyTuple_SetItem( p, j, PyInt_FromLong( -1 ) );
	}
	PyDict_SetItemString(dict, "StoreButtons", p);
	PyDict_SetItemString(dict, "StoreFlags", PyInt_FromLong( store->Flags ) );
	PyDict_SetItemString(dict, "TavernRumour", PyString_FromResRef( store->RumoursTavern ));
	PyDict_SetItemString(dict, "TempleRumour", PyString_FromResRef( store->RumoursTemple ));
	PyDict_SetItemString(dict, "IDPrice", PyInt_FromLong( store->IDPrice ) );
	PyDict_SetItemString(dict, "Lore", PyInt_FromLong( store->Lore ) );
	PyDict_SetItemString(dict, "Depreciation", PyInt_FromLong( store->DepreciationRate ) );
	PyDict_SetItemString(dict, "SellMarkup", PyInt_FromLong( store->SellMarkup ) );
	PyDict_SetItemString(dict, "BuyMarkup", PyInt_FromLong( store->BuyMarkup ) );
	PyDict_SetItemString(dict, "StealFailure", PyInt_FromLong( store->StealFailureChance ) );

	return dict;
}


PyDoc_STRVAR( GemRB_IsValidStoreItem__doc,
"===== IsValidStoreItem =====\n\
\n\
**Prototype:** GemRB.IsValidStoreItem (PartyID, slot[, type])\n\
\n\
**Description:** Returns if a pc's inventory item or a store item is valid \n\
for buying, selling, identifying or stealing. If Type is 1, then this is a \n\
 store item.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party\n\
  * slot    - the item's inventory or store slot\n\
  * type - which inventory to look at?\n\
    * 0 - PC\n\
    * 1 - store\n\
    * 2 - right-hand store (bag)\n\
\n\
**Return value:** bitfield\n\
  * 1 - valid for buy\n\
  * 2 - valid for sell\n\
  * 4 - valid for identify\n\
  * 8 - valid for steal\n\
  * 0x40 - selected for buy or sell\n\
\n\
**See also:** [[guiscript:EnterStore]], [[guiscript:GetSlotItem]], [[guiscript:GetStoreItem]], [[guiscript:ChangeStoreItem]]"
);

static PyObject* GemRB_IsValidStoreItem(PyObject * /*self*/, PyObject* args)
{
	int globalID, Slot, ret;
	int type = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &globalID, &Slot, &type)) {
		return AttributeError( GemRB_IsValidStoreItem__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}

	const char *ItemResRef;
	ieDword Flags;

	if (type) {
		STOItem* si = NULL;
		if (type != 2) {
			si = store->GetItem( Slot, true );
		} else if (rhstore) {
			si = rhstore->GetItem(Slot, true);
		}
		if (!si) {
			return PyInt_FromLong(0);
		}
		ItemResRef = si->ItemResRef;
		Flags = si->Flags;
	} else {
		CREItem* si = actor->inventory.GetSlotItem( core->QuerySlot(Slot) );
		if (!si) {
			return PyInt_FromLong(0);
		}
		ItemResRef = si->ItemResRef;
		Flags = si->Flags;
	}
	Item *item = gamedata->GetItem(ItemResRef, true);
	if (!item) {
		Log(ERROR, "GUIScript", "Invalid resource reference: %s",
			ItemResRef);
		return PyInt_FromLong(0);
	}

	ret = store->AcceptableItemType( item->ItemType, Flags, type == 0 || type == 2 );

	//don't allow putting a bag into itself
	if (!strnicmp(ItemResRef, store->Name, sizeof(ieResRef)) ) {
		ret &= ~IE_STORE_SELL;
	}
	//this is a hack to report on selected items
	if (Flags & IE_INV_ITEM_SELECTED) {
		ret |= IE_STORE_SELECT;
	}

	//don't allow overstuffing bags
	if (store->Capacity && store->Capacity<=store->GetRealStockSize()) {
		ret = (ret | IE_STORE_CAPACITY) & ~IE_STORE_SELL;
	}

	//buying into bags respects bags' limitations
	if (rhstore && type != 0) {
		int accept = rhstore->AcceptableItemType(item->ItemType, Flags, 1);
		if (!(accept & IE_STORE_SELL)) {
			ret &= ~IE_STORE_BUY;
		}
		//probably won't happen in sane games, but doesn't hurt to check
		if (!(accept & IE_STORE_BUY)) {
			ret &= ~IE_STORE_SELL;
		}

		if (rhstore->Capacity && rhstore->Capacity<=rhstore->GetRealStockSize()) {
			ret = (ret | IE_STORE_CAPACITY) & ~IE_STORE_BUY;
		}
	}

	gamedata->FreeItem( item, ItemResRef, false );
	return PyInt_FromLong(ret);
}

PyDoc_STRVAR( GemRB_FindStoreItem__doc,
"===== FindStoreItem =====\n\
\n\
**Prototype:** GemRB.FindStoreItem (resref)\n\
\n\
**Description:** Returns the amount of the specified items in the open \n\
store. 0 is also returned for an infinite amount.\n\
\n\
**Parameters:** \n\
  * resref - item resource\n\
\n\
**Return value:** integer\n\
"
);

static PyObject* GemRB_FindStoreItem(PyObject * /*self*/, PyObject* args)
{
	char *resref;

	if (!PyArg_ParseTuple( args, "s", &resref)) {
		return AttributeError( GemRB_FindStoreItem__doc );
	}

	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}

	int Slot = store->FindItem(resref, false);
	if (Slot == -1) {
		return PyInt_FromLong(0);
	}
	STOItem* si = store->GetItem( Slot, true );
	if (!si) {
		// shouldn't be possible, item vanished
		return PyInt_FromLong(0);
	}

	if (si->InfiniteSupply == -1) {
		// change this if it is ever needed for something else than depreciation calculation
		return PyInt_FromLong(0);
	} else {
		return PyInt_FromLong(si->AmountInStock);
	}
}

PyDoc_STRVAR( GemRB_SetPurchasedAmount__doc,
"===== SetPurchasedAmount =====\n\
\n\
**Prototype:** GemRB.SetPurchasedAmount (Index, Amount[, type])\n\
\n\
**Description:** Sets the amount of purchased items of a type. If it is 0, \n\
then the item will be deselected from the purchase list. This function \n\
works only with an active store.\n\
\n\
**Parameters:**\n\
  * Index  - the store item's index\n\
  * Amount - a numeric value not less than 0\n\
  * type - set to non-zero to affect right-hand store (bag)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:EnterStore]], [[guiscript:LeaveStore]], [[guiscript:SetPurchasedAmount]]\n\
"
);

static PyObject* GemRB_SetPurchasedAmount(PyObject * /*self*/, PyObject* args)
{
	int Slot, tmp;
	ieDword amount;
	int type = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &Slot, &tmp, &type)) {
		return AttributeError( GemRB_SetPurchasedAmount__doc );
	}
	amount = (ieDword) tmp;
	Store *store;
	if (type) {
		store = rhstore;
	} else {
		store = core->GetCurrentStore();
	}
	if (!store) {
		return RuntimeError("No current store!");
	}
	STOItem* si = store->GetItem( Slot, true );
	if (!si) {
		return RuntimeError("Store item not found!");
	}

	if (si->InfiniteSupply != -1) {
		if (si->AmountInStock<amount) {
			amount=si->AmountInStock;
		}
	}
	si->PurchasedAmount=amount;
	if (amount) {
		si->Flags |= IE_INV_ITEM_SELECTED;
	} else {
		si->Flags &= ~IE_INV_ITEM_SELECTED;
	}

	Py_RETURN_NONE;
}

// a bunch of duplicated code moved from GemRB_ChangeStoreItem()
static int SellBetweenStores(STOItem* si, int action, Store *store)
{
	CREItem ci(si);
	ci.Flags &= ~IE_INV_ITEM_SELECTED;
	if (action == IE_STORE_STEAL) {
		ci.Flags |= IE_INV_ITEM_STOLEN;
	}

	while (si->PurchasedAmount) {
		//store/bag is at full capacity
		if (store->Capacity && (store->Capacity <= store->GetRealStockSize())) {
			Log(MESSAGE, "GUIScript", "Store is full.");
			return ASI_FAILED;
		}
		if (si->InfiniteSupply!=-1) {
			if (!si->AmountInStock) {
				break;
			}
			si->AmountInStock--;
		}
		si->PurchasedAmount--;
		store->AddItem(&ci);
	}
	return ASI_SUCCESS;
}

PyDoc_STRVAR( GemRB_ChangeStoreItem__doc,
"===== ChangeStoreItem =====\n\
\n\
**Prototype:** GemRB.ChangeStoreItem (PartyID, slot, action)\n\
\n\
**Description:** Performs a buy, sell, identify or steal action. It has the \n\
same bit values as IsValidStoreItem. It can also toggle the selection of an item.\n\
If a right-hand store is currently loaded, it will be acted upon instead of\n\
the PC's inventory.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party\n\
  * slot    - the item's inventory or store slot\n\
  * action  - bitfield\n\
    * 1 - buy\n\
    * 2 - sell\n\
    * 4 - identify\n\
    * 8 - steal\n\
    * Add 0x40 for selection (in case of buy/sell only)\n\
\n\
**Return value:**\n\
  * 0 - failure\n\
  * 2 - success\n\
\n\
**See also:** [[guiscript:EnterStore]], [[guiscript:GetSlotItem]], [[guiscript:GetStoreItem]], [[guiscript:IsValidStoreItem]]\n\
"
);

static PyObject* GemRB_ChangeStoreItem(PyObject * /*self*/, PyObject* args)
{
	int globalID, Slot;
	int action;
	int res = ASI_FAILED;

	if (!PyArg_ParseTuple( args, "iii", &globalID, &Slot, &action)) {
		return AttributeError( GemRB_ChangeStoreItem__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}
	switch (action) {
	case IE_STORE_STEAL:
	case IE_STORE_BUY:
	{
		STOItem* si = store->GetItem( Slot, true );
		if (!si) {
			return RuntimeError("Store item not found!");
		}
		//always stealing only one item
		if (action == IE_STORE_STEAL) {
			si->PurchasedAmount=1;
		}
		if (!rhstore) {
			//the amount of items is stored in si->PurchasedAmount
			//it will adjust AmountInStock/PurchasedAmount
			actor->inventory.AddStoreItem(si, action == IE_STORE_STEAL ? STA_STEAL : STA_BUYSELL);
		} else {
			SellBetweenStores(si, action, rhstore);
		}
		if (si->PurchasedAmount) {
			//was not able to buy it due to lack of space
			res = ASI_FAILED;
			break;
		}
		// save the resref, since the pointer may get freed
		ieResRef itemResRef;
		CopyResRef(itemResRef, si->ItemResRef);
		//if no item remained, remove it
		if (si->AmountInStock) {
			si->Flags &= ~IE_INV_ITEM_SELECTED;
		} else {
			store->RemoveItem(si);
			delete si;
		}
		//keep encumbrance labels up to date
		if (!rhstore) {
			actor->CalculateSpeed(false);
		}

		// play the item's inventory sound
		ieResRef Sound;
		Item *item = gamedata->GetItem(itemResRef);
		if (item) {
			if (core->HasFeature(GF_HAS_PICK_SOUND) && item->ReplacementItem[0]) {
				memcpy(Sound, item->ReplacementItem, sizeof(ieResRef));
			} else {
				gamedata->GetItemSound(Sound, item->ItemType, item->AnimationType, IS_DROP);
			}
			gamedata->FreeItem(item, itemResRef, 0);
			if (Sound[0]) {
				// speech means we'll only play the last sound if multiple items were bought
				core->GetAudioDrv()->Play(Sound, SFX_CHAN_GUI, 0, 0, GEM_SND_SPEECH|GEM_SND_RELATIVE);
			}
		}
		res = ASI_SUCCESS;
		break;
	}
	case IE_STORE_ID:
	{
		if (!rhstore) {
			CREItem* si = actor->inventory.GetSlotItem( core->QuerySlot(Slot) );
			if (!si) {
				return RuntimeError( "Item not found!" );
			}
			si->Flags |= IE_INV_ITEM_IDENTIFIED;
		} else {
			STOItem* si = rhstore->GetItem( Slot, true );
			if (!si) {
				return RuntimeError("Bag item not found!");
			}
			si->Flags |= IE_INV_ITEM_IDENTIFIED;
		}
		res = ASI_SUCCESS;
		break;
	}
	case IE_STORE_SELECT|IE_STORE_BUY:
	{
		STOItem* si = store->GetItem( Slot, true );
		if (!si) {
			return RuntimeError("Store item not found!");
		}
		si->Flags ^= IE_INV_ITEM_SELECTED;
		if (si->Flags & IE_INV_ITEM_SELECTED) {
			si->PurchasedAmount=1;
		} else {
			si->PurchasedAmount=0;
		}
		res = ASI_SUCCESS;
		break;
	}

	case IE_STORE_SELECT|IE_STORE_SELL:
	case IE_STORE_SELECT|IE_STORE_ID:
	{
		if (!rhstore) {
			//this is not removeitem, because the item is just marked
			CREItem* si = actor->inventory.GetSlotItem( core->QuerySlot(Slot) );
			if (!si) {
				return RuntimeError( "Item not found!" );
			}
			si->Flags ^= IE_INV_ITEM_SELECTED;
		} else {
			STOItem* si = rhstore->GetItem( Slot, true );
			if (!si) {
				return RuntimeError("Bag item not found!");
			}
			si->Flags ^= IE_INV_ITEM_SELECTED;
                        if (si->Flags & IE_INV_ITEM_SELECTED) {
                                si->PurchasedAmount=1;
                        } else {
                                si->PurchasedAmount=0;
                        }
		}
		res = ASI_SUCCESS;
		break;
	}
	case IE_STORE_SELL:
	{
		//store/bag is at full capacity
		if (store->Capacity && (store->Capacity <= store->GetRealStockSize()) ) {
			Log(MESSAGE, "GUIScript", "Store is full.");
			res = ASI_FAILED;
			break;
		}

		if (rhstore) {
			STOItem *si = rhstore->GetItem(Slot, true);
			if (!si) {
				return RuntimeError("Bag item not found!");
			}
			res = SellBetweenStores(si, action, store);

			//if no item remained, remove it
			if (si->AmountInStock) {
				si->Flags &= ~IE_INV_ITEM_SELECTED;
			} else {
				rhstore->RemoveItem(si);
				delete si;
			}
		} else {
			//this is removeitem, because the item leaves our inventory
			CREItem* si = actor->inventory.RemoveItem( core->QuerySlot(Slot) );
			if (!si) {
				return RuntimeError( "Item not found!" );
			}
			//well, it shouldn't be sold at all, but if it is here
			//it will vanish!!!
			if (!si->Expired && (si->Flags& IE_INV_ITEM_RESELLABLE)) {
				si->Flags &= ~IE_INV_ITEM_SELECTED;
				store->AddItem( si );
			}
			delete si;
			//keep encumbrance labels up to date
			actor->CalculateSpeed(false);
			res = ASI_SUCCESS;
		}
		break;
	}
	}
	return PyInt_FromLong(res);
}

PyDoc_STRVAR( GemRB_GetStoreItem__doc,
"===== GetStoreItem =====\n\
\n\
**Prototype:** GemRB.GetStoreItem (index[, righthand])\n\
\n\
**Description:** Gets the item resref, price and other details of a store \n\
item referenced by the index. In case of PST stores the item's availability \n\
is also checked against the availability triggers.\n\
\n\
**Parameters:**\n\
  * index - the number of the item in the store list\n\
  * righthand - set to non-zero to query the right-hand store (bag) instead\n\
\n\
**Return value:** dictionary\n\
  * 'ItemResRef' - the ResRef of the item\n\
  * 'ItemName'   - the StrRef of the item's name (identified or not)\n\
  * 'ItemDesc'   - the StrRef of the item's description (identified or not)\n\
  * 'Price'      - the price of the item (subtract this from the party gold)\n\
  * 'Amount'     - the amount of item in store (-1 means infinite)\n\
  * 'Usages0'    - The primary charges of the item (or the item's stack amount if the item is stackable).\n\
  * 'Usages1'    - The secondary charges of the item.\n\
  * 'Usages2'    - The tertiary charges of the item.\n\
  * 'Flags'      - Item flags.\n\
  * 'Purchased'  - The count of purchased items of this type.\n\
\n\
**See also:** [[guiscript:EnterStore]], [[guiscript:GetStoreDrink]], [[guiscript:GetStoreCure]], [[guiscript:GetStore]], [[guiscript:GetSlotItem]]\n\
"
);

static PyObject* GemRB_GetStoreItem(PyObject * /*self*/, PyObject* args)
{
	int index;
	int rh = 0;

	if (!PyArg_ParseTuple( args, "i|i", &index, &rh )) {
		return AttributeError( GemRB_GetStoreItem__doc );
	}
	Store *store;
	if (rh) {
		store = rhstore;
	} else {
		store = core->GetCurrentStore();
	}
	if (!store) {
		return RuntimeError("No current store!");
	}
	if (index>=(int) store->GetRealStockSize()) {
		Log(WARNING, "GUIScript", "Item is not available???");
		Py_INCREF( Py_None );
		return Py_None;
	}
	PyObject* dict = PyDict_New();
	STOItem *si=store->GetItem( index, true );
	if (!si) {
		Log(WARNING, "GUIScript", "Item is not available???");
		Py_INCREF( Py_None );
		return Py_None;
	}
	PyDict_SetItemString(dict, "ItemResRef", PyString_FromResRef( si->ItemResRef ));
	PyDict_SetItemString(dict, "Usages0", PyInt_FromLong (si->Usages[0]));
	PyDict_SetItemString(dict, "Usages1", PyInt_FromLong (si->Usages[1]));
	PyDict_SetItemString(dict, "Usages2", PyInt_FromLong (si->Usages[2]));
	PyDict_SetItemString(dict, "Flags", PyInt_FromLong (si->Flags));
	PyDict_SetItemString(dict, "Purchased", PyInt_FromLong (si->PurchasedAmount) );

	if (si->InfiniteSupply==-1) {
		PyDict_SetItemString(dict, "Amount", PyInt_FromLong( -1 ) );
	} else {
		PyDict_SetItemString(dict, "Amount", PyInt_FromLong( si->AmountInStock ) );
	}

	Item *item = gamedata->GetItem(si->ItemResRef, true);
	if (!item) {
		Log(WARNING, "GUIScript", "Item is not available???");
		Py_INCREF( Py_None );
		return Py_None;
	}

	int identified = !!(si->Flags & IE_INV_ITEM_IDENTIFIED);
	PyDict_SetItemString(dict, "ItemName", PyInt_FromLong( (signed) item->GetItemName( (bool) identified )) );
	PyDict_SetItemString(dict, "ItemDesc", PyInt_FromLong( (signed) item->GetItemDesc( (bool) identified )) );

	int price = item->Price * store->SellMarkup / 100;
	//calculate depreciation too
	//store->DepreciationRate, mount

	price *= si->Usages[0];

	//is this correct?
	if (price<1) {
		price = 1;
	}
	PyDict_SetItemString(dict, "Price", PyInt_FromLong( price ) );

	gamedata->FreeItem( item, si->ItemResRef, false );
	return dict;
}

PyDoc_STRVAR( GemRB_GetStoreDrink__doc,
"===== GetStoreDrink =====\n\
\n\
**Prototype:** GemRB.GetStoreDrink (index)\n\
\n\
**Description:** Gets the name, strength and price of a store drink \n\
referenced by the index.\n\
\n\
**Parameters:**\n\
  * index - the number of the drink in the store list\n\
\n\
**Return value:** dictionary\n\
  * 'DrinkName' - the StrRef of the drink name\n\
  * 'Strength'  - the strength if the drink (affects rumour and intoxication)\n\
  * 'Price'     - the price of the drink (subtract this from the party gold)\n\
\n\
**See also:** [[guiscript:EnterStore]], [[guiscript:GetStoreCure]], [[guiscript:GetStore]]\n\
"
);

static PyObject* GemRB_GetStoreDrink(PyObject * /*self*/, PyObject* args)
{
	int index;

	if (!PyArg_ParseTuple( args, "i", &index )) {
		return AttributeError( GemRB_GetStoreDrink__doc );
	}
	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}
	if (index>=(int) store->DrinksCount) {
		Py_INCREF( Py_None );
		return Py_None;
	}
	PyObject* dict = PyDict_New();
	STODrink *drink=store->GetDrink(index);
	PyDict_SetItemString(dict, "DrinkName", PyInt_FromLong( (signed) drink->DrinkName ));
	PyDict_SetItemString(dict, "Price", PyInt_FromLong( drink->Price ));
	PyDict_SetItemString(dict, "Strength", PyInt_FromLong( drink->Strength ));
	return dict;
}

static void ReadUsedItems()
{
	int i;

	UsedItemsCount = 0;
	int table = gamedata->LoadTable("item_use");
	if (table>=0) {
		Holder<TableMgr> tab = gamedata->GetTable(table);
		if (!tab) goto table_loaded;
		UsedItemsCount = tab->GetRowCount();
		UsedItems = (UsedItemType *) malloc( sizeof(UsedItemType) * UsedItemsCount);
		for (i=0;i<UsedItemsCount;i++) {
			strnlwrcpy(UsedItems[i].itemname, tab->GetRowName(i),8 );
			strnlwrcpy(UsedItems[i].username, tab->QueryField(i,0),32 );
			if (UsedItems[i].username[0]=='*') {
				UsedItems[i].username[0] = 0;
			}
			//this is an strref
			UsedItems[i].value = atoi(tab->QueryField(i,1) );
			//1 - named actor cannot remove it
			//2 - anyone else cannot equip it
			//4 - can only swap it for something else
			//8 - (pst) can only be equipped in eye slots
			//16 - (pst) can only be equipped in ear slots
			UsedItems[i].flags = atoi(tab->QueryField(i,2) );
		}
table_loaded:
		gamedata->DelTable(table);
	}
}

static void ReadSpecialItems()
{
	int i;

	SpecialItemsCount = 0;
	int table = gamedata->LoadTable("itemspec");
	if (table>=0) {
		Holder<TableMgr> tab = gamedata->GetTable(table);
		if (!tab) goto table_loaded;
		SpecialItemsCount = tab->GetRowCount();
		SpecialItems = (SpellDescType *) malloc( sizeof(SpellDescType) * SpecialItemsCount);
		for (i=0;i<SpecialItemsCount;i++) {
			strnlwrcpy(SpecialItems[i].resref, tab->GetRowName(i),8 );
			//if there are more flags, compose this value into a bitfield
			SpecialItems[i].value = atoi(tab->QueryField(i,0) );
		}
table_loaded:
		gamedata->DelTable(table);
	}
}

static ieStrRef GetSpellDesc(ieResRef CureResRef)
{
	int i;

	if (StoreSpellsCount==-1) {
		StoreSpellsCount = 0;
		int table = gamedata->LoadTable("speldesc");
		if (table>=0) {
			Holder<TableMgr> tab = gamedata->GetTable(table);
			if (!tab) goto table_loaded;
			StoreSpellsCount = tab->GetRowCount();
			StoreSpells = (SpellDescType *) malloc( sizeof(SpellDescType) * StoreSpellsCount);
			for (i=0;i<StoreSpellsCount;i++) {
				strnlwrcpy(StoreSpells[i].resref, tab->GetRowName(i),8 );
				StoreSpells[i].value = atoi(tab->QueryField(i,0) );
			}
table_loaded:
			gamedata->DelTable(table);
		}
	}
	if (StoreSpellsCount==0) {
		Spell *spell = gamedata->GetSpell(CureResRef);
		if (!spell) {
			return 0;
		}
		int ret = spell->SpellDescIdentified;
		gamedata->FreeSpell(spell, CureResRef, false);
		return ret;
	}
	for (i=0;i<StoreSpellsCount;i++) {
		if (!strnicmp(StoreSpells[i].resref, CureResRef, 8) ) {
			return StoreSpells[i].value;
		}
	}
	return 0;
}

PyDoc_STRVAR( GemRB_GetStoreCure__doc,
"===== GetStoreCure =====\n\
\n\
**Prototype:** GemRB.GetStoreCure (index)\n\
\n\
**Description:** Gets the spell resref, price and description of a store \n\
cure referenced by the index.\n\
\n\
**Parameters:**\n\
  * index - the number of the cure in the store list\n\
\n\
**Return value:** dictionary\n\
  * 'CureResRef'  - the ResRef of the cure spell\n\
  * 'Description' - the StrRef of the spell's description\n\
  * 'Price'       - the price of the spell (subtract this from the party gold)\n\
\n\
**See also:** [[guiscript:EnterStore]], [[guiscript:GetStoreDrink]], [[guiscript:GetStore]]"
);

static PyObject* GemRB_GetStoreCure(PyObject * /*self*/, PyObject* args)
{
	int index;

	if (!PyArg_ParseTuple( args, "i", &index )) {
		return AttributeError( GemRB_GetStoreCure__doc );
	}
	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}
	if (index>=(int) store->CuresCount) {
		Py_INCREF( Py_None );
		return Py_None;
	}
	PyObject* dict = PyDict_New();
	STOCure *cure=store->GetCure(index);
	PyDict_SetItemString(dict, "CureResRef", PyString_FromResRef( cure->CureResRef ));
	PyDict_SetItemString(dict, "Price", PyInt_FromLong( cure->Price ));
	PyDict_SetItemString(dict, "Description", PyInt_FromLong( (signed) GetSpellDesc(cure->CureResRef) ) );
	return dict;
}

PyDoc_STRVAR( GemRB_ExecuteString__doc,
"===== ExecuteString =====\n\
\n\
**Prototype:** GemRB.ExecuteString (String[, Slot])\n\
\n\
**Description:** Executes an in-game script action in the current area \n\
script context. This means that LOCALS will be treated as the current \n\
area's variable. If a number was given, it will execute the action in the \n\
numbered actor's context.\n\
\n\
**Parameters:**\n\
  * String - a gamescript action\n\
  * Slot   - a player slot or global ID\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
  GemRB.ExecuteString('ActionOverride([PC], Attack(NearestEnemyOf(Myself)) )')\n\
The above example will force a player (most likely Player1) to attack an enemy, issuing the command as it would come from the current area's script. The current gametype must support the scripting action.\n\
\n\
\n\
  GemRB.ExecuteString('Attack(NearestEnemyOf(Myself))', 2)\n\
The above example will force Player2 to attack an enemy, as the example will run in that actor's script context.\n\
\n\
**See also:** [[guiscript:EvaluateString]], gamescripts\n\
"
);

static PyObject* GemRB_ExecuteString(PyObject * /*self*/, PyObject* args)
{
	char* String;
	int globalID = 0;

	if (!PyArg_ParseTuple( args, "s|i", &String, &globalID)) {
		return AttributeError( GemRB_ExecuteString__doc );
	}
	GET_GAME();

	if (globalID) {
		GET_ACTOR_GLOBAL();
		GameScript::ExecuteString(actor, String);
	} else {
		GameScript::ExecuteString( game->GetCurrentArea( ), String );
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_EvaluateString__doc,
"===== EvaluateString =====\n\
\n\
**Prototype:** GemRB.EvaluateString (String)\n\
\n\
**Description:** Evaluates an ingame script trigger in the current area \n\
script context. It prints the result. The command is more useful from the \n\
ingame debug console than from scripts.\n\
\n\
**Parameters:**\n\
  * String - a gamescript trigger\n\
\n\
**Return value:** N/A (the trigger's return value is printed)\n\
\n\
**See also:** [[guiscript:ExecuteString]]\n\
"
);

static PyObject* GemRB_EvaluateString(PyObject * /*self*/, PyObject* args)
{
	char* String;

	if (!PyArg_ParseTuple( args, "s", &String )) {
		return AttributeError( GemRB_EvaluateString__doc );
	}
	GET_GAME();

	if (GameScript::EvaluateString( game->GetCurrentArea( ), String )) {
		print("%s returned True", String);
	} else {
		print("%s returned False", String);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_UpdateMusicVolume__doc,
"===== UpdateMusicVolume =====\n\
\n\
**Prototype:** GemRB.UpdateMusicVolume ()\n\
\n\
**Description:** Updates music volume on-the-fly.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:UpdateAmbientsVolume]]"
);

static PyObject* GemRB_UpdateMusicVolume(PyObject * /*self*/, PyObject* /*args*/)
{
	core->GetAudioDrv()->UpdateVolume( GEM_SND_VOL_MUSIC );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_UpdateAmbientsVolume__doc,
"===== UpdateAmbientsVolume =====\n\
\n\
**Prototype:** GemRB.UpdateAmbientsVolume ()\n\
\n\
**Description:** Updates ambients volume on-the-fly.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:UpdateMusicVolume]]"
);

static PyObject* GemRB_UpdateAmbientsVolume(PyObject * /*self*/, PyObject* /*args*/)
{
	core->GetAudioDrv()->UpdateVolume( GEM_SND_VOL_AMBIENTS );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_MessageWindowDebug__doc,
			 "MessageWindowDebug(log_level)\n\n"
			 "Enable/Disable debug messages of log_level in the MessageWindow." );

static PyObject* GemRB_MessageWindowDebug(PyObject * /*self*/, PyObject* args)
{
	int logLevel;
	if (!PyArg_ParseTuple( args, "i", &logLevel )) {
		return AttributeError( GemRB_MessageWindowDebug__doc );
	}

	if (logLevel == -1) {
		RemoveLogger(getMessageWindowLogger());
	} else {
		// convert it to the internal representation
		getMessageWindowLogger(true)->SetLogLevel((log_level)logLevel);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetCurrentArea__doc,
"===== GetCurrentArea =====\n\
\n\
**Prototype:** GemRB.GetCurrentArea ()\n\
\n\
**Description:** Returns the resref of the current area. It is the same as \n\
GetGameString(1). It works only after a LoadGame() was issued.\n\
\n\
**Return value:** string, (ARE resref)\n\
\n\
**See also:** [[guiscript:GetGameString]]\n\
"
);

static PyObject* GemRB_GetCurrentArea(PyObject * /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyString_FromString( game->CurrentArea );
}

PyDoc_STRVAR( GemRB_MoveToArea__doc,
"===== MoveToArea =====\n\
\n\
**Prototype:** GemRB.MoveToArea (resref)\n\
\n\
**Description:** Moves the selected actors to the named area.\n\
\n\
**Parameters:**\n\
  * resref - The name of the area.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetCurrentArea]]\n\
"
);

static PyObject* GemRB_MoveToArea(PyObject * /*self*/, PyObject* args)
{
	const char *String;

	if (!PyArg_ParseTuple( args, "s", &String )) {
		return AttributeError( GemRB_MoveToArea__doc );
	}
	GET_GAME();

	Map* map2 = game->GetMap(String, true);
	if (!map2) {
		return RuntimeError( "Map not found!" );
	}
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (!actor->Selected) {
			continue;
		}
		Map* map1 = actor->GetCurrentArea();
		if (map1) {
			map1->RemoveActor( actor );
		}
		map2->AddActor( actor, true );
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetMemorizableSpellsCount__doc,
"===== GetMemorizableSpellsCount =====\n\
\n\
**Prototype:** GemRB.GetMemorizableSpellsCount (PartyID, SpellType, Level[, Bonus])\n\
\n\
**Description:** Returns number of memorizable spells of given type and \n\
level in a player character's spellbook.\n\
\n\
**Parameters:**\n\
  * PartyID   - the PC's position in the party\n\
  * SpellType - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level     - the memorized spell's level\n\
  * Bonus     - whether querying the (wisdom) modified or the base value\n\
\n\
**Return value:** numeric, -1 if the query is invalid (no spellcaster, bad spelltype, too high level).\n\
\n\
**See also:** [[guiscript:SetMemorizableSpellsCount]]\n\
"
);

static PyObject* GemRB_GetMemorizableSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Bonus=1;

	if (!PyArg_ParseTuple( args, "iii|i", &globalID, &SpellType, &Level, &Bonus )) {
		return AttributeError( GemRB_GetMemorizableSpellsCount__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	//this isn't in the actor's spellbook, handles Wisdom
	return PyInt_FromLong(actor->spellbook.GetMemorizableSpellsCount( (ieSpellType) SpellType, Level, (bool) Bonus ) );
}

PyDoc_STRVAR( GemRB_SetMemorizableSpellsCount__doc,
"===== SetMemorizableSpellsCount =====\n\
\n\
**Prototype:** GemRB.SetMemorizableSpellsCount (PartyID, Value, SpellType, Level)\n\
\n\
**Description:** Sets number of memorizable spells of given type and level \n\
in a player character's spellbook.\n\
\n\
**Parameters:**\n\
  * PartyID   - the PC's position in the party\n\
  * Value     - number of memorizable spells\n\
  * SpellType - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level     - the memorized spell's level\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetMemorizableSpellsCount]]\n\
"
);

static PyObject* GemRB_SetMemorizableSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int globalID, Value, SpellType, Level;

	if (!PyArg_ParseTuple( args, "iiii", &globalID, &Value, &SpellType, &Level)) {
		return AttributeError( GemRB_SetMemorizableSpellsCount__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	//the bonus increased value (with wisdom too) is handled by the core
	actor->spellbook.SetMemorizableSpellsCount( Value, (ieSpellType) SpellType, Level, 0 );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_CountSpells__doc,
"===== CountSpells =====\n\
\n\
**Prototype:** GemRB.CountSpells (PartyID, SpellName, SpellType, Flag)\n\
\n\
**Description:** Returns number of memorized spells of given name and type \n\
in PC's spellbook. If flag is set then spent spells are also count.\n\
\n\
**Parameters:**\n\
  * PartyID   - the PC's position in the party\n\
  * SpellName - spell to count\n\
  * SpellType - 0 - priest, 1 - wizard, 2 - innate\n\
  * Flag      - count depleted spells too?\n\
\n\
**Return value:** integer\n\
\n\
**See also:** [[guiscript:GetMemorizableSpellsCount]]"
);

static PyObject* GemRB_CountSpells(PyObject * /*self*/, PyObject* args)
{
	int globalID, SpellType = -1;
	char *SpellResRef;
	int Flag = 0;

	if (!PyArg_ParseTuple( args, "is|ii", &globalID, &SpellResRef, &SpellType, &Flag)) {
		return AttributeError( GemRB_CountSpells__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyInt_FromLong(actor->spellbook.CountSpells( SpellResRef, SpellType, Flag) );
}

PyDoc_STRVAR( GemRB_GetKnownSpellsCount__doc,
"===== GetKnownSpellsCount =====\n\
\n\
**Prototype:** GemRB.GetKnownSpellsCount (PartyID, SpellType[, Level])\n\
\n\
**Description:** Returns number of known spells of given type and level in \n\
a player character's spellbook. If Level isn't given, it will return the \n\
number of all spells of the given type.\n\
\n\
**Parameters:**\n\
  * PartyID   - the PC's position in the party\n\
  * SpellType - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level     - the known spell's level\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:GetMemorizedSpellsCount]], [[guiscript:GetKnownSpell]]\n\
"
);

static PyObject* GemRB_GetKnownSpellsCount(PyObject * /*self*/, PyObject* args)
{
	int globalID, SpellType, Level = -1;

	if (!PyArg_ParseTuple( args, "ii|i", &globalID, &SpellType, &Level)) {
		return AttributeError( GemRB_GetKnownSpellsCount__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Level<0) {
		int tmp = 0;
		for(int i=0;i<9;i++) {
			tmp += actor->spellbook.GetKnownSpellsCount( SpellType, i );
		}
		return PyInt_FromLong(tmp);
	}

	return PyInt_FromLong(actor->spellbook.GetKnownSpellsCount( SpellType, Level ) );
}

PyDoc_STRVAR( GemRB_GetKnownSpell__doc,
"===== GetKnownSpell =====\n\
\n\
**Prototype:** GemRB.GetKnownSpell (PartyID, SpellType, Level, Index)\n\
\n\
**Description:** Returns dictionary with specified known spell from PC's spellbook.\n\
\n\
**Parameters:**\n\
  * PartyID   - the PC's position in the party\n\
  * SpellType - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level     - the memorized spell's level\n\
  * Index     - the memorized spell's index\n\
\n\
**Return value:** dictionary\n\
  * 'SpellResRef' - The name of the spell (.spl resref)\n\
\n\
**See also:** [[guiscript:GetMemorizedSpell]]\n\
"
);

static PyObject* GemRB_GetKnownSpell(PyObject * /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index;

	if (!PyArg_ParseTuple( args, "iiii", &globalID, &SpellType, &Level, &Index)) {
		return AttributeError( GemRB_GetKnownSpell__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	CREKnownSpell* ks = actor->spellbook.GetKnownSpell( SpellType, Level, Index );
	if (! ks) {
		return RuntimeError( "Spell not found!" );
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "SpellResRef", PyString_FromResRef (ks->SpellResRef));
	//PyDict_SetItemString(dict, "Flags", PyInt_FromLong (ms->Flags));

	return dict;
}


PyDoc_STRVAR( GemRB_GetMemorizedSpellsCount__doc,
"===== GetMemorizedSpellsCount =====\n\
\n\
**Prototype:** GemRB.GetMemorizedSpellsCount (globalID, SpellType, Level, Castable)\n\
\n\
**Description:** Returns number of spells of given type and level in \n\
selected character's memory. If level is negative then it returns the \n\
number of distinct spells memorised.\n\
\n\
**Parameters:**\n\
  * globalID  - party ID or global ID of the actor to use\n\
  * SpellType - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level     - the memorized spell's level\n\
  * Castable  - ignore depleted spells?\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:GetMemorizedSpell]], [[guiscript:GetKnownSpellsCount]]"
);

static PyObject* GemRB_GetMemorizedSpellsCount(PyObject * /*self*/, PyObject* args)
{
	int globalID, SpellType, Level = -1;
	int castable;

	if (!PyArg_ParseTuple( args, "iiii", &globalID, &SpellType, &Level, &castable)) {
		return AttributeError( GemRB_GetMemorizedSpellsCount__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Level<0) {
		if (castable) {
			return PyInt_FromLong( actor->spellbook.GetSpellInfoSize( SpellType ) );
		} else {
			return PyInt_FromLong( actor->spellbook.GetMemorizedSpellsCount( SpellType, false ) );
		}
	} else {
		return PyInt_FromLong( actor->spellbook.GetMemorizedSpellsCount( SpellType, Level, castable ) );
	}
}

PyDoc_STRVAR( GemRB_GetMemorizedSpell__doc,
"===== GetMemorizedSpell =====\n\
\n\
**Prototype:** GemRB.GetMemorizedSpell (PartyID, SpellType, Level, Index)\n\
\n\
**Description:** Returns dict with specified memorized spell from PC's spellbook.\n\
\n\
**Parameters:** \n\
  * PartyID   - the PC's position in the party\n\
  * SpellType - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level     - the memorized spell's level\n\
  * Index     - the memorized spell's index\n\
\n\
**Return value:** dictionary\n\
  * 'SpellResRef' - The name of the spell (.spl resref)\n\
  * 'Flags'       - Is the spell castable, or already spent\n\
\n\
**See also:** [[guiscript:GetMemorizedSpellsCount]]\n\
"
);

static PyObject* GemRB_GetMemorizedSpell(PyObject * /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index;

	if (!PyArg_ParseTuple( args, "iiii", &globalID, &SpellType, &Level, &Index)) {
		return AttributeError( GemRB_GetMemorizedSpell__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	CREMemorizedSpell* ms = actor->spellbook.GetMemorizedSpell( SpellType, Level, Index );
	if (! ms) {
		return RuntimeError( "Spell not found!" );
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "SpellResRef", PyString_FromResRef (ms->SpellResRef));
	PyDict_SetItemString(dict, "Flags", PyInt_FromLong (ms->Flags));
	return dict;
}


PyDoc_STRVAR( GemRB_GetSpell__doc,
"===== GetSpell =====\n\
\n\
**Prototype:** GemRB.GetSpell (ResRef[, silent])\n\
\n\
**Description:** Returns dictionary with the specified spell's data. If silent \n\
is set, nothing will be printed to the console.\n\
\n\
**Parameters:**\n\
  * ResRef - the resource reference of the spell.\n\
  * silent - turn off verbose output.\n\
\n\
**Return value:** dictionary\n\
  * 'SpellName'       - strref of unidentified name.\n\
  * 'SpellDesc'       - strref of unidentified description.\n\
  * 'SpellbookIcon'   - the spell's icon (.bam resref)\n\
  * 'SpellExclusion'  - the excluded schools and alignments\n\
  * 'SpellDivine'     - this field tells divine magics apart\n\
  * 'SpellSchool'     - the spell's school (primary type)\n\
  * 'SpellType'       - the type of text that appears on spell dispelling\n\
  * 'SpellLevel'      - the spell's level\n\
  * 'Completion'      - the spell's completion sound\n\
  * 'SpellTargetType' - the spell's target type\n\
  * 'SpellSecondary'  - the spell's secondary type\n\
  * 'HeaderFlags'     - the spell's header flags\n\
  * 'NonHostile'      - is the spell considered hostile?\n\
  * 'SpellResRef'     - the spell's resource reference\n\
\n\
**See also:** [[guiscript:GetItem]], [[guiscript:Button_SetSpellIcon]], spell_structure(IESDP)\n\
"
);

static PyObject* GemRB_GetSpell(PyObject * /*self*/, PyObject* args)
{
	const char* ResRef;
	int silent = 0;

	if (!PyArg_ParseTuple( args, "s|i", &ResRef, &silent)) {
		return AttributeError( GemRB_GetSpell__doc );
	}

	if (silent && !gamedata->Exists(ResRef,IE_SPL_CLASS_ID, true)) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	Spell* spell = gamedata->GetSpell(ResRef, silent);
	if (spell == NULL) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "SpellType", PyInt_FromLong (spell->SpellType));
	PyDict_SetItemString(dict, "SpellName", PyInt_FromLong ((signed) spell->SpellName));
	PyDict_SetItemString(dict, "SpellDesc", PyInt_FromLong ((signed) spell->SpellDesc));
	PyDict_SetItemString(dict, "SpellbookIcon", PyString_FromResRef (spell->SpellbookIcon));
	PyDict_SetItemString(dict, "SpellExclusion", PyInt_FromLong (spell->ExclusionSchool)); //this will list school exclusions and alignment
	PyDict_SetItemString(dict, "SpellDivine", PyInt_FromLong (spell->PriestType)); //this will tell apart a priest spell from a druid spell
	PyDict_SetItemString(dict, "SpellSchool", PyInt_FromLong (spell->PrimaryType));
	PyDict_SetItemString(dict, "SpellSecondary", PyInt_FromLong (spell->SecondaryType));
	PyDict_SetItemString(dict, "SpellLevel", PyInt_FromLong (spell->SpellLevel));
	PyDict_SetItemString(dict, "Completion", PyString_FromResRef (spell->CompletionSound));
	PyDict_SetItemString(dict, "SpellTargetType", PyInt_FromLong (spell->GetExtHeader(0)->Target));
	PyDict_SetItemString(dict, "HeaderFlags", PyInt_FromLong (spell->Flags));
	PyDict_SetItemString(dict, "NonHostile", PyInt_FromLong (!(spell->Flags&SF_HOSTILE) && !spell->ContainsDamageOpcode()));
	PyDict_SetItemString(dict, "SpellResRef", PyString_FromResRef (spell->Name));
	gamedata->FreeSpell( spell, ResRef, false );
	return dict;
}


PyDoc_STRVAR( GemRB_CheckSpecialSpell__doc,
"===== CheckSpecialSpell =====\n\
\n\
**Prototype:** GemRB.CheckSpecialSpell (globalID, SpellResRef)\n\
\n\
**Description:** Checks if an actor's spell is considered special (splspec.2da).\n\
\n\
**Parameters:**\n\
  * globalID - global ID of the actor to use\n\
  * SpellResRef - spell resource to check\n\
\n\
**Return value:** bitfield\n\
  * 0 for normal ones\n\
  * SP_IDENTIFY - any spell that cannot be cast from the menu\n\
  * SP_SILENCE  - any spell that can be cast in silence\n\
  * SP_SURGE    - any spell that cannot be cast during a wild surge\n\
  * SP_REST     - any spell that is cast upon rest if memorized"
);

static PyObject* GemRB_CheckSpecialSpell(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	const char *SpellResRef;

	if (!PyArg_ParseTuple( args, "is", &globalID, &SpellResRef)) {
		return AttributeError( GemRB_CheckSpecialSpell__doc );
	}
	GET_GAME();

	Actor* actor = game->GetActorByGlobalID( globalID );
	if (!actor) {
		return RuntimeError( "Actor not found!\n" );
	}

	int ret = core->CheckSpecialSpell( SpellResRef, actor );
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_GetSpelldataIndex__doc,
"===== GetSpelldataIndex =====\n\
\n\
**Prototype:** GemRB.GetSpelldataIndex (globalID, SpellResRef, type)\n\
\n\
**Description:** Returns the index of the spell in the spellbook's \n\
spellinfo structure.\n\
\n\
**Parameters:**\n\
  * globalID - global ID of the actor to use\n\
  * SpellResRef - spell resource to check\n\
  * type - spell(book) type (0 means any)\n\
\n\
**Return value:** integer"
);

static PyObject* GemRB_GetSpelldataIndex(PyObject * /*self*/, PyObject* args)
{
	unsigned int globalID;
	const char *spellResRef;
	int type;

	if (!PyArg_ParseTuple( args, "isi", &globalID, &spellResRef, &type)) {
		return AttributeError( GemRB_GetSpelldataIndex__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	SpellExtHeader spelldata;
	int ret = actor->spellbook.FindSpellInfo(&spelldata, spellResRef, type);
	return PyInt_FromLong( ret-1 );
}

PyDoc_STRVAR( GemRB_GetSpelldata__doc,
"===== GetSpelldata =====\n\
\n\
**Prototype:** GemRB.GetSpelldata (globalID[, type])\n\
\n\
**Description:** Returns resrefs of the spells in the spellbook's spellinfo structure.\n\
\n\
**Parameters:**\n\
  * globalID - global ID of the actor to use\n\
  * type - spell(book) type (255 means any)\n\
\n\
**Return value:** tuple of spell resresfs\n\
"
);

static PyObject* GemRB_GetSpelldata(PyObject * /*self*/, PyObject* args)
{
	unsigned int globalID;
	int type = 255;

	if (!PyArg_ParseTuple( args, "i|i", &globalID, &type)) {
		return AttributeError( GemRB_GetSpelldata__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	SpellExtHeader spelldata;
	int i = 0;
	int count = actor->spellbook.GetSpellInfoSize(type);
	PyObject* spell_list = PyTuple_New(count);
	for (i=0; i < count; i++) {
		actor->spellbook.GetSpellInfo(&spelldata, type, i, 1);
		PyTuple_SetItem(spell_list, i, PyString_FromResRef(spelldata.spellname) );
	}
	return spell_list;
}


PyDoc_STRVAR( GemRB_LearnSpell__doc,
"===== LearnSpell =====\n\
\n\
**Prototype:** GemRB.LearnSpell (PartyID, SpellResRef[, Flags, BookType, Level])\n\
\n\
**Description:** Tries to learn the specified spell. Flags control xp \n\
granting, stat checks and feedback.\n\
\n\
**Parameters:**\n\
  * PartyID     - the PC's position in the party\n\
  * SpellResRef - the spell's Resource Reference\n\
  * Flags       - bitmap with the following bits (default is 0):\n\
    * 1 - Give XP for learning (Level * 100)\n\
    * 2 - Display message\n\
    * 4 - Check for insufficient stats\n\
    * 8 - Also memorize it\n\
  * BookType - override which spellbook to use\n\
  * Level - override at which level to learn it\n\
\n\
**Return value:** integer, 0 on success, nonzero on failure (LSR_*).\n\
\n\
**See also:** [[guiscript:MemorizeSpell]], [[guiscript:RemoveSpell]]\n\
"
);

static PyObject* GemRB_LearnSpell(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	const char *Spell;
	int Flags=0;
	int Booktype = -1;
	int Level = -1;

	if (!PyArg_ParseTuple(args, "is|iii", &globalID, &Spell, &Flags, &Booktype, &Level)) {
		return AttributeError( GemRB_LearnSpell__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	int ret = actor->LearnSpell(Spell, Flags, Booktype, Level); // returns 0 on success
	if (!ret) core->SetEventFlag( EF_ACTION );
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_DispelEffect__doc,
"===== DispelEffect =====\n\
\n\
**Prototype:** GemRB.DispelEffect (globalID, EffectName, Parameter2)\n\
\n\
**Description:** Removes all effects from target whose opcode and second \n\
parameter matches the arguments.\n\
\n\
**Parameters:** \n\
  * globalID  - party ID or global ID of the actor to use\n\
  * EffectName - effect reference name (eg. 'State:Helpless')\n\
  * Parameter2 - parameter2 of targeted effect\n\
\n\
**Return value:** N/A"
);

static EffectRef work_ref;

static PyObject* GemRB_DispelEffect(PyObject * /*self*/, PyObject* args)
{
	int globalID, Parameter2;
	const char *EffectName;

	if (!PyArg_ParseTuple( args, "isi", &globalID, &EffectName, &Parameter2 )) {
		return AttributeError( GemRB_DispelEffect__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	work_ref.Name=EffectName;
	work_ref.opcode=-1;
	actor->fxqueue.RemoveAllEffectsWithParam(work_ref, Parameter2);

	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_RemoveEffects__doc,
"===== RemoveEffects =====\n\
\n\
**Prototype:** GemRB.RemoveEffects (globalID, SpellResRef)\n\
\n\
**Description:** Removes all effects created by the spell/item named SpellResRef. \n\
This is useful for removing class abilities (CLAB/HLA AP_* entries).\n\
\n\
**Parameters:**\n\
  * globalID  - party ID or global ID of the actor to use\n\
  * SpellResRef - a spell or item resource reference (source of effects)\n\
\n\
**See also:** [[guiscript:RemoveSpell]], [[guiscript:RemoveItem]]\n\
"
);

static PyObject* GemRB_RemoveEffects(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	const char * SpellResRef;

	if (!PyArg_ParseTuple( args, "is", &globalID, &SpellResRef )) {
		return AttributeError( GemRB_RemoveEffects__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->fxqueue.RemoveAllEffects(SpellResRef);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_RemoveSpell__doc,
"===== RemoveSpell =====\n\
\n\
**Prototype:** GemRB.RemoveSpell (globalID, SpellType, Level, Index)\n\
**Prototype:** GemRB.RemoveSpell (globalID, SpellResRef)\n\
\n\
**Description:** Unlearns a specified known spell.\n\
\n\
**Parameters:**\n\
  * globalID  - party ID or global ID of the actor to use\n\
  * SpellType - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level     - the known spell's level\n\
  * Index     - the known spell's index\n\
  * SpellResRef - spell resource reference to remove by\n\
\n\
**Return value:** boolean, 1 on success\n\
\n\
**See also:** [[guiscript:UnmemorizeSpell]], [[guiscript:GetKnownSpellsCount]], [[guiscript:GetKnownSpell]], [[guiscript:LearnSpell]], [[guiscript:RemoveEffects]]\n\
"
);

static PyObject* GemRB_RemoveSpell(PyObject * /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index;
	const char *SpellResRef;

	GET_GAME();

	if (PyArg_ParseTuple( args, "is", &globalID, &SpellResRef) ) {
		GET_ACTOR_GLOBAL();
		int ret = actor->spellbook.KnowSpell(SpellResRef);
		actor->spellbook.RemoveSpell(SpellResRef);
		return PyInt_FromLong(ret);
	}
	PyErr_Clear(); //clear the type exception from above

	if (!PyArg_ParseTuple( args, "iiii", &globalID, &SpellType, &Level, &Index )) {
		return AttributeError( GemRB_RemoveSpell__doc );
	}

 	GET_ACTOR_GLOBAL();
	CREKnownSpell* ks = actor->spellbook.GetKnownSpell( SpellType, Level, Index );
	if (! ks) {
		return RuntimeError( "Spell not known!" );
	}

	return PyInt_FromLong( actor->spellbook.RemoveSpell( ks ) );
}

PyDoc_STRVAR( GemRB_RemoveItem__doc,
"===== RemoveItem =====\n\
\n\
**Prototype:** GemRB.RemoveItem (PartyID, Slot[, Count])\n\
\n\
**Description:** Removes and destroys an item in an actor's inventory. This \n\
works even if the item is cursed or indestructible. If an item has charges \n\
it decreases the charge count instead.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party\n\
  * Slot    - The inventory slot index of the item\n\
  * Count   - the number of items\n\
\n\
**Return value:** boolean, 1 on success\n\
\n\
**See also:** [[guiscript:CreateItem]]"
);

static PyObject* GemRB_RemoveItem(PyObject * /*self*/, PyObject* args)
{
	int globalID, Slot;
	int Count = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &globalID, &Slot, &Count )) {
		return AttributeError( GemRB_RemoveItem__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	int ok;

	Slot = core->QuerySlot(Slot);
	actor->inventory.UnEquipItem( Slot, false );
	CREItem *si = actor->inventory.RemoveItem( Slot, Count );
	if (si) {
		ok = true;
		delete si;
	} else {
		ok = false;
	}
	return PyInt_FromLong( ok );
}

PyDoc_STRVAR( GemRB_MemorizeSpell__doc,
"===== MemorizeSpell =====\n\
\n\
**Prototype:** GemRB.MemorizeSpell (PartyID, SpellType, Level, Index[, Enabled])\n\
\n\
**Description:** Memorizes specified known spell. If Enabled is set, the \n\
spell will be ready for use.\n\
\n\
**Parameters:**\n\
  * PartyID   - the PC's position in the party\n\
  * SpellType - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level     - the known spell's level\n\
  * Index     - the known spell's index\n\
  * Enabled   - defaults to 0, which means the spell is depleted\n\
\n\
**Return value:** boolean, 1 on success.\n\
\n\
**See also:** [[guiscript:GetKnownSpell]], [[guiscript:UnmemorizeSpell]]"
);

static PyObject* GemRB_MemorizeSpell(PyObject * /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index, enabled=0;

	if (!PyArg_ParseTuple( args, "iiii|i", &globalID, &SpellType, &Level, &Index, &enabled )) {
		return AttributeError( GemRB_MemorizeSpell__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	CREKnownSpell* ks = actor->spellbook.GetKnownSpell( SpellType, Level, Index );
	if (! ks) {
		return RuntimeError( "Spell not found!" );
	}

	// auto-refresh innates (memorisation defaults to depleted)
	if (core->HasFeature(GF_HAS_SPELLLIST)) {
		if (SpellType == IE_IWD2_SPELL_INNATE) enabled = 1;
	} else {
		if (SpellType == IE_SPELL_TYPE_INNATE) enabled = 1;
	}

	return PyInt_FromLong( actor->spellbook.MemorizeSpell( ks, enabled ) );
}


PyDoc_STRVAR( GemRB_UnmemorizeSpell__doc,
"===== UnmemorizeSpell =====\n\
\n\
**Prototype:** GemRB.UnmemorizeSpell (PartyID, SpellType, Level, Index[, onlydepleted])\n\
\n\
**Description:** Unmemorizes specified memorized spell. If onlydepleted is \n\
set, it will only remove an already depleted spell (with the same resref as \n\
the provided spell).\n\
\n\
**Parameters:**\n\
  * PartyID      - the PC's position in the party\n\
  * SpellType    - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level        - the memorized spell's level\n\
  * Index        - the memorized spell's index\n\
  * onlydepleted - remove only an already depleted spell with the same resref as the specified spell\n\
\n\
**Return value:** boolean, 1 on success\n\
\n\
**See also:** [[guiscript:MemorizeSpell]], [[guiscript:GetMemorizedSpellsCount]], [[guiscript:GetMemorizedSpell]]"
);

static PyObject* GemRB_UnmemorizeSpell(PyObject * /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index, onlydepleted=0;

	if (!PyArg_ParseTuple( args, "iiii|i", &globalID, &SpellType, &Level, &Index, &onlydepleted )) {
		return AttributeError( GemRB_UnmemorizeSpell__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	CREMemorizedSpell* ms = actor->spellbook.GetMemorizedSpell( SpellType, Level, Index );
	if (! ms) {
		return RuntimeError( "Spell not found!\n" );
	}
	if (onlydepleted)
		return PyInt_FromLong(actor->spellbook.UnmemorizeSpell(ms->SpellResRef, false, onlydepleted));
	else
		return PyInt_FromLong(actor->spellbook.UnmemorizeSpell(ms));
}

PyDoc_STRVAR( GemRB_GetSlotItem__doc,
"===== GetSlotItem =====\n\
\n\
**Prototype:** GemRB.GetSlotItem (PartyID, slot[, translated])\n\
\n\
**Description:** Returns dictionary with the specified actor's inventory \n\
slot data. Or the dragged item if globalID is 0. If translated is nonzero, \n\
the slot will not be looked up again.\n\
\n\
**Parameters:**\n\
  * PartyID   - the PC's position in the party\n\
  * slot      - the item's inventory slot\n\
  * translated - look up the slot again (useful for quickweapons)\n\
\n\
**Return value:** dictionary\n\
  * 'ItemResRef' - The name of the item (.itm resref)\n\
  * 'Usages0' - The primary charges of the item (or the item's stack amount if the item is stackable).\n\
  * 'Usages1' - The secondary charges of the item.\n\
  * 'Usages2' - The tertiary charges of the item.\n\
  * 'Flags'   - Item flags:\n\
    * IE_INV_ITEM_IDENTIFIED = 1,   The item is identified.\n\
    * IE_INV_ITEM_UNSTEALABLE = 2,  The item is unstealable.\n\
    * IE_INV_ITEM_STOLEN = 4,       The item is stolen.\n\
    * IE_INV_ITEM_UNDROPPABLE =8,   The item is undroppable.\n\
    * IE_INV_ITEM_ACQUIRED = 0x10,  The item was recently moved.\n\
    * IE_INV_ITEM_DESTRUCTIBLE = 0x20,  The item is removable (sellable or destructible).\n\
    * IE_INV_ITEM_EQUIPPED = 0x40,  The item is currently equipped.\n\
    * IE_INV_ITEM_STACKED = 0x80,   The item is a stacked item.\n\
  * 'Header'  - Item's extended header assigned to the inventory slot (the\n\
  ability to use). Only applicable to quickslots.\n\
\n\
**See also:** [[guiscript:GetItem]], [[guiscript:Button_SetItemIcon]], [[guiscript:ChangeItemFlag]]"
);

static PyObject* GemRB_GetSlotItem(PyObject * /*self*/, PyObject* args)
{
	int globalID, Slot;
	int translated = 0; // inventory slots are numbered differently in CRE and need to be remapped

	if (!PyArg_ParseTuple( args, "ii|i", &globalID, &Slot, &translated)) {
		return AttributeError( GemRB_GetSlotItem__doc );
	}
	CREItem *si;
	int header = -1;

	if (globalID==0) {
		si = core->GetDraggedItem();
	} else {
		GET_GAME();
		GET_ACTOR_GLOBAL();

		if (!translated) {
			Slot = core->QuerySlot(Slot);
		}
		header = actor->PCStats->GetHeaderForSlot(Slot);

		si = actor->inventory.GetSlotItem( Slot );
	}
	if (! si) {
		Py_INCREF( Py_None );
		return Py_None;
	}
	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "ItemResRef", PyString_FromResRef (si->ItemResRef));
	PyDict_SetItemString(dict, "Usages0", PyInt_FromLong (si->Usages[0]));
	PyDict_SetItemString(dict, "Usages1", PyInt_FromLong (si->Usages[1]));
	PyDict_SetItemString(dict, "Usages2", PyInt_FromLong (si->Usages[2]));
	PyDict_SetItemString(dict, "Flags", PyInt_FromLong (si->Flags));
	PyDict_SetItemString(dict, "Header", PyInt_FromLong (header));

	return dict;
}

PyDoc_STRVAR( GemRB_ChangeItemFlag__doc,
"===== ChangeItemFlag =====\n\
\n\
**Prototype:** GemRB.ChangeItemFlag (PartyID, slot, flags, mode)\n\
\n\
**Description:** Changes the flags of an inventory slot. For example, \n\
identifies an item.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party (1 based)\n\
  * slot    - inventory slot\n\
  * flags   - a bitfield, same as the GetSlotItem flags\n\
    * IE_INV_ITEM_IDENTIFIED    = 0x01 - the item is identified\n\
    * IE_INV_ITEM_UNSTEALABLE   = 0x02 - the item is unstealable\n\
    * IE_INV_ITEM_STOLEN        = 0x04 - the item is marked as stolen\n\
    * IE_INV_ITEM_UNDROPPABLE   = 0x08 - the item is undroppable (dragitem fails)\n\
    * IE_INV_ITEM_ACQUIRED      = 0x10 - the item was recently acquired\n\
    * IE_INV_ITEM_DESTRUCTIBLE  = 0x20 - the item is removable\n\
    * IE_INV_ITEM_EQUIPPED      = 0x40 - the item is equipped\n\
    * IE_INV_ITEM_STACKED       = 0x80 - the item is a stacked item\n\
  * mode    - binary operation type\n\
\n\
**Return value:** Returns 0 if the item was not found.\n\
\n\
**See also:** [[guiscript:GetSlotItem]], [[guiscript:bit_operation]]"
);

static PyObject* GemRB_ChangeItemFlag(PyObject * /*self*/, PyObject* args)
{
	int globalID, Slot, Flags, Mode;

	if (!PyArg_ParseTuple( args, "iiii", &globalID, &Slot, &Flags, &Mode)) {
		return AttributeError( GemRB_ChangeItemFlag__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (actor->inventory.ChangeItemFlag(core->QuerySlot(Slot), Flags, Mode)) {
		return PyInt_FromLong(1);
	}
	return PyInt_FromLong(0);
}


PyDoc_STRVAR( GemRB_CanUseItemType__doc,
"===== CanUseItemType =====\n\
\n\
**Prototype:** GemRB.CanUseItemType (slottype, itemname[, actor, equipped])\n\
\n\
**Description:** Checks the itemtype vs. slottype, and also checks the \n\
usability flags vs. actor's stats (alignment, class, race, kit etc.)\n\
\n\
**Parameters:**\n\
  * slottype    - the slot to check (See ie_slots.py)\n\
  * itemname    - the resource reference of the item\n\
  * actor       - the actor's PartyID (if 0, skips actor checks)\n\
  * equipped    - whether the item is equipped (if so, don't consider disabled items to be unusable)\n\
\n\
**Return value:** boolean\n\
\n\
**See also:** [[guiscript:DropDraggedItem]], [[guiscript:UseItem]]"
);

static PyObject* GemRB_CanUseItemType(PyObject * /*self*/, PyObject* args)
{
	int SlotType, globalID, Equipped;
	const char *ItemName;

	globalID = 0;
	if (!PyArg_ParseTuple( args, "is|ii", &SlotType, &ItemName, &globalID, &Equipped)) {
		return AttributeError( GemRB_CanUseItemType__doc );
	}
	if (!ItemName[0]) {
		return PyInt_FromLong(0);
	}
	Item *item = gamedata->GetItem(ItemName, true);
	if (!item) {
		Log(MESSAGE, "GUIScript", "Cannot find item %s to check!", ItemName);
		return PyInt_FromLong(0);
	}
	Actor* actor = NULL;
	if (globalID) {
		GET_GAME();

		if (globalID > 1000) {
			actor = game->GetActorByGlobalID( globalID );
		} else {
			actor = game->FindPC( globalID );
		}
		if (!actor) {
			return RuntimeError( "Actor not found!\n" );
		}
	}

	int ret=core->CanUseItemType(SlotType, item, actor, false, Equipped != 0);
	gamedata->FreeItem(item, ItemName, false);
	return PyInt_FromLong(ret);
}


PyDoc_STRVAR( GemRB_GetSlots__doc,
"===== GetSlots =====\n\
\n\
**Prototype:** GemRB.GetSlots (PartyID, SlotType[, Flag])\n\
\n\
**Description:** Returns the tuple of slots of a PC matching the SlotType \n\
criteria.\n\
\n\
**Parameters:**\n\
  * PartyID - a PC\n\
  * SlotType - bitfield, the inventory slot's type (32768 means inventory)\n\
  * Flag (defaults to 1)\n\
    * <0 - returns empty slots\n\
    * 0  - returns all slots.\n\
    * >0 - returns filled slots\n\
\n\
**Return value:** tuple\n\
\n\
**See also:** [[guiscript:GetSlotType]]\n\
"
);

static PyObject* GemRB_GetSlots(PyObject * /*self*/, PyObject* args)
{
	int SlotType, Count, MaxCount, globalID;
	int flag = 1;

	if (!PyArg_ParseTuple( args, "ii|i", &globalID, &SlotType, &flag)) {
		return AttributeError( GemRB_GetSlots__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	MaxCount = core->SlotTypes;
	int i;
	Count = 0;
	for (i=0;i<MaxCount;i++) {
		int id = core->QuerySlot(i);
		if ((core->QuerySlotType( id ) & (ieDword) SlotType) != (ieDword) SlotType) {
			continue;
		}
		CREItem *slot = actor->inventory.GetSlotItem( id );
		if (flag) {
			if(flag<0 && slot) continue;
			if(flag>0 && !slot) continue;
		}
		Count++;
	}

	PyObject* tuple = PyTuple_New( Count );
	Count = 0;
	for (i=0;i<MaxCount;i++) {
		int id = core->QuerySlot(i);
		if ((core->QuerySlotType( id ) & (ieDword) SlotType) != (ieDword) SlotType) {
			continue;
		}
		CREItem *slot = actor->inventory.GetSlotItem( id );
		if (flag) {
			if(flag<0 && slot) continue;
			if(flag>0 && !slot) continue;
		}
		PyTuple_SetItem( tuple, Count++, PyInt_FromLong( i ) );
	}

	return tuple;
}

PyDoc_STRVAR( GemRB_FindItem__doc,
"===== FindItem =====\n\
\n\
**Prototype:** GemRB.FindItem (globalID, itemname)\n\
\n\
**Description:** Returns the slot number of the actor's item (or -1).\n\
\n\
**Parameters:**\n\
  * globalID  - party ID or global ID of the actor to use\n\
  * itemname - item resource reference\n\
\n\
**Return value:** integer, -1 if not found\n\
\n\
**See also:** [[guiscript:GetItem]], [[guiscript:GetSlots]], [[guiscript:GetSlotType]]"
);

static PyObject* GemRB_FindItem(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	const char *ItemName;

	if (!PyArg_ParseTuple(args, "is", &globalID, &ItemName)) {
		return AttributeError( GemRB_FindItem__doc );
	}
	if (!ItemName[0]) {
		return PyInt_FromLong(-1);
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int slot = -1;
	slot = actor->inventory.FindItem(ItemName, IE_INV_ITEM_UNDROPPABLE);
	return PyInt_FromLong(slot);
}

PyDoc_STRVAR( GemRB_GetItem__doc,
"===== GetItem =====\n\
\n\
**Prototype:** GemRB.GetItem (ResRef[, PartyID])\n\
\n\
**Description:** Returns dictionary with the specified item's data.\n\
\n\
**Parameters:**\n\
  * ResRef - the resource reference of the item\n\
  * PartyID - the pc you want to check usability against (defaults to the selected one)\n\
\n\
**Return value:** dictionary\n\
  * 'ItemName'           - strref of unidentified name.\n\
  * 'ItemNameIdentified' - strref of identified name.\n\
  * 'ItemDesc'           - strref of unidentified description.\n\
  * 'ItemDescIdentified' - strref of identified description.\n\
  * 'ItemIcon'           - the item's icon (.bam resref)\n\
  * 'DescIcon'           - the description icon\n\
  * 'BrokenItem'         - the replacement item (used for items with broken sounds)\n\
  * 'MaxStackAmount'     - maximum stackable amount\n\
  * 'Dialog'             - item dialog (.dlg resref)\n\
  * 'DialogName'         - the item dialog name\n\
  * 'Price'              - the base item price\n\
  * 'Type'               - the item type (see itemtype.2da)\n\
  * 'AnimationType'      - the item animation ID\n\
  * 'Exclusion'          - the exclusion bit (used by eg. magic armor and rings of protection).\n\
  * 'LoreToID'           - the required lore to identify the item\n\
  * 'MaxCharge'          - the maximum amount of charges\n\
  * 'Tooltips'           - the item tooltips\n\
  * 'Locations'          - the item extended header's ability locations\n\
  * 'Spell'              - the spell's strref if the item is a copyable scroll\n\
  * 'Function'           - returns special function\n\
    * 0 - no special function\n\
    * 1 - item is a copyable scroll (2nd header's 1st feature is 'Learn spell')\n\
    * 2 - item is a drinkable potion \n\
    * 4 - item is a container\n\
    * 8 - item has selectable abilities (headers)\n\
\n\
**See also:** [[guiscript:GetSlotItem]], [[guiscript:GetSpell]], [[guiscript:Button_SetItemIcon]]"
);

#define CAN_DRINK 1  //potions
#define CAN_READ  2  //scrolls
#define CAN_STUFF 4  //containers
#define CAN_SELECT 8 //items with more abilities

static PyObject* GemRB_GetItem(PyObject * /*self*/, PyObject* args)
{
	char* ResRef;
	int PartyID = 0;
	Actor *actor = NULL;

	if (!PyArg_ParseTuple( args, "s|i", &ResRef, &PartyID)) {
		return AttributeError( GemRB_GetItem__doc );
	}
	//it isn't a problem if actor not found
	Game *game = core->GetGame();
	if (game) {
		if (!PartyID) {
			PartyID = game->GetSelectedPCSingle();
		}
		actor = game->FindPC( PartyID );
	}

	Item* item = gamedata->GetItem(ResRef, true);
	if (item == NULL) {
		Log(MESSAGE, "GUIScript", "Cannot get item %s!", ResRef);
		Py_INCREF( Py_None );
		return Py_None;
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "ItemName", PyInt_FromLong ((signed) item->GetItemName(false)));
	PyDict_SetItemString(dict, "ItemNameIdentified", PyInt_FromLong ((signed) item->GetItemName(true)));
	PyDict_SetItemString(dict, "ItemDesc", PyInt_FromLong ((signed) item->GetItemDesc(false)));
	PyDict_SetItemString(dict, "ItemDescIdentified", PyInt_FromLong ((signed)item->GetItemDesc(true)));
	PyDict_SetItemString(dict, "ItemIcon", PyString_FromResRef (item->ItemIcon));
	PyDict_SetItemString(dict, "DescIcon", PyString_FromResRef (item->DescriptionIcon));
	PyDict_SetItemString(dict, "BrokenItem", PyString_FromResRef (item->ReplacementItem));
	PyDict_SetItemString(dict, "MaxStackAmount", PyInt_FromLong (item->MaxStackAmount));
	PyDict_SetItemString(dict, "Dialog", PyString_FromResRef (item->Dialog));
	PyDict_SetItemString(dict, "DialogName", PyInt_FromLong ((signed)item->DialogName));
	PyDict_SetItemString(dict, "Price", PyInt_FromLong (item->Price));
	PyDict_SetItemString(dict, "Type", PyInt_FromLong (item->ItemType));
	PyDict_SetItemString(dict, "AnimationType", PyString_FromAnimID(item->AnimationType));
	PyDict_SetItemString(dict, "Exclusion", PyInt_FromLong(item->ItemExcl));
	PyDict_SetItemString(dict, "LoreToID", PyInt_FromLong(item->LoreToID));
	PyDict_SetItemString(dict, "MaxCharge", PyInt_FromLong(0) );

	int ehc = item->ExtHeaderCount;

	PyObject* tooltiptuple = PyTuple_New(ehc);
	PyObject* locationtuple = PyTuple_New(ehc);
	for(int i=0;i<ehc;i++) {
		ITMExtHeader *eh = item->ext_headers+i;
		PyTuple_SetItem(tooltiptuple, i, PyInt_FromLong(eh->Tooltip));
		PyTuple_SetItem(locationtuple, i, PyInt_FromLong(eh->Location));
		PyDict_SetItemString(dict, "MaxCharge", PyInt_FromLong(eh->Charges) );
	}

	PyDict_SetItemString(dict, "Tooltips", tooltiptuple);
	PyDict_SetItemString(dict, "Locations", locationtuple);

	int function=0;

	if (core->CanUseItemType(SLOT_POTION, item, actor, false) ) {
			function|=CAN_DRINK;
	}
	if (core->CanUseItemType(SLOT_SCROLL, item, actor, false) ) {
		ITMExtHeader *eh;
		Effect *f;
		//determining if this is a copyable scroll
		if (ehc<2) {
			goto not_a_scroll;
		}
		eh = item->ext_headers+1;
		if (eh->FeatureCount<1) {
			goto not_a_scroll;
		}
		f = eh->features; //+0

		//normally the learn spell opcode is 147
		EffectQueue::ResolveEffect(fx_learn_spell_ref);
		if (f->Opcode!=(ieDword) fx_learn_spell_ref.opcode) {
			goto not_a_scroll;
		}
		//maybe further checks for school exclusion?
		//no, those were done by CanUseItemType
		function|=CAN_READ;
		PyDict_SetItemString(dict, "Spell", PyString_FromResRef (f->Resource));
	} else if (ehc>1) {
		function|=CAN_SELECT;
	}
not_a_scroll:
	if (core->CanUseItemType(SLOT_BAG, item, NULL, false) ) {
		//allow the open container flag only if there is
		//a store file (this fixes pst eye items, which
		//got the same item type as bags)
		//while this isn't required anymore, as bag itemtypes are customisable
		//we still better check for the existence of the store, or we
		//get a crash somewhere.
		if (gamedata->Exists( ResRef, IE_STO_CLASS_ID) ) {
			function|=CAN_STUFF;
		}
	}
	PyDict_SetItemString(dict, "Function", PyInt_FromLong(function));
	gamedata->FreeItem( item, ResRef, false );
	return dict;
}

static void DragItem(CREItem *si)
{
	if (!si) {
		return;
	}
	Item *item = gamedata->GetItem (si->ItemResRef );
	if (!item) {
		return;
	}
	core->DragItem(si, item->ItemIcon);
	gamedata->FreeItem( item, si->ItemResRef, false );
}

static int CheckRemoveItem(Actor *actor, CREItem *si, int action)
{
	///check if item is undroppable because the actor likes it
	if (UsedItemsCount==-1) {
		ReadUsedItems();
	}
	unsigned int i=UsedItemsCount;

	while(i--) {
		if (UsedItems[i].itemname[0] && strnicmp(UsedItems[i].itemname, si->ItemResRef,8) ) {
			continue;
		}
		//true if names don't match
		int nomatch = (UsedItems[i].username[0] && strnicmp(UsedItems[i].username, actor->GetScriptName(), 32) );

		switch(action) {
		//the named actor cannot remove it
		case CRI_REMOVE:
			if (UsedItems[i].flags&1) {
				if (nomatch) continue;
			} else continue;
			break;
		//the named actor can equip it
		case CRI_EQUIP:
			if (UsedItems[i].flags&2) {
				if (!nomatch) continue;
			} else continue;
			break;
		//the named actor can swap it
		case CRI_SWAP:
			if (UsedItems[i].flags&4) {
				if (!nomatch) continue;
			} else continue;
			break;
		//the named actor cannot remove it except when initiating a swap (used for plain inventory slots)
		// and make sure not to treat earrings improperly
		case CRI_REMOVEFORSWAP:
			int flags = UsedItems[i].flags;
			if (!(flags&1) || flags&4) {
				continue;
			}
			break;
		}

		displaymsg->DisplayString(UsedItems[i].value, DMC_WHITE, IE_STR_SOUND);
		return 1;
	}
	return 0;
}

// TNO has an ear and an eye slot that share the same slot type, so normal checks fail
// return false if we're trying to stick an earing into our eye socket or vice versa
static bool CheckEyeEarMatch(CREItem *NewItem, int Slot) {
	if (UsedItemsCount==-1) {
		ReadUsedItems();
	}
	unsigned int i=UsedItemsCount;

	while(i--) {
		if (UsedItems[i].itemname[0] && strnicmp(UsedItems[i].itemname, NewItem->ItemResRef, 8)) {
			continue;
		}

		//8 - (pst) can only be equipped in eye slots
		//16 - (pst) can only be equipped in ear slots
		if (UsedItems[i].flags & 8) {
			return Slot == 1; // eye/left ear/helmet
		} else if (UsedItems[i].flags & 16) {
			return Slot == 7; //right ear/caleidoscope
		}

		return true;
	}
	return true;
}

static CREItem *TryToUnequip(Actor *actor, unsigned int Slot, unsigned int Count)
{
	//we should use getslotitem, because
	//getitem would remove the item from the inventory!
	CREItem *si = actor->inventory.GetSlotItem(Slot);
	if (!si) {
		return NULL;
	}

	//it is always possible to put these items into the inventory
	// however in pst, we need to ensure immovable swappables are swappable
	bool isdragging = core->GetDraggedItem() != NULL;
	if (core->QuerySlotType(Slot)&SLOT_INVENTORY) {
		if (CheckRemoveItem(actor, si, CRI_REMOVEFORSWAP)) {
			return NULL;
		}
	} else {
		if (CheckRemoveItem(actor, si, isdragging?CRI_SWAP:CRI_REMOVE)) {
			return NULL;
		}
	}
	///fixme: make difference between cursed/unmovable
	if (! actor->inventory.UnEquipItem( Slot, false )) {
		// Item is currently undroppable/cursed
		if (si->Flags&IE_INV_ITEM_CURSED) {
			displaymsg->DisplayConstantString(STR_CURSED, DMC_WHITE);
		} else {
			displaymsg->DisplayConstantString(STR_CANT_DROP_ITEM, DMC_WHITE);
		}
		return NULL;
	}
	si = actor->inventory.RemoveItem( Slot, Count );
	return si;
}

PyDoc_STRVAR( GemRB_DragItem__doc,
"===== DragItem =====\n\
\n\
**Prototype:** GemRB.DragItem (PartyID, Slot, ResRef, [Count=0, Type])\n\
\n\
**Description:** Start dragging specified item. If Count is given, it will \n\
try to split the item. If an  item is already dragged, it won't do \n\
anything. If Slot is negative, drag the actor's party portrait instead.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party\n\
  * Slot    - actor index in game structure\n\
  * ResRef  - item name (.itm resref)\n\
  * Count   - stack size (0 means all)\n\
  * Type    - if nonzero, drag from pile at actor's feet instead\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:DropDraggedItem]], [[guiscript:IsDraggingItem]]"
);

static PyObject* GemRB_DragItem(PyObject * /*self*/, PyObject* args)
{
	ieResRef Sound = {};
	int globalID, Slot, Count = 0, Type = 0;
	const char *ResRef;

	if (!PyArg_ParseTuple( args, "iis|ii", &globalID, &Slot, &ResRef, &Count, &Type)) {
		return AttributeError( GemRB_DragItem__doc );
	}

	// FIXME
	// we should Drop the Dragged item in place of the current item
	// but only if the current item is draggable, tough!
	if (core->GetDraggedItem()) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	GET_GAME();
	Actor* actor;
	if (globalID > 1000) {
		actor = game->GetActorByGlobalID( globalID );
	} else {
		actor = game->FindPC( globalID );
	}

	//allow -1,-1
	if (!actor && ( globalID || ResRef[0]) ) {
		return RuntimeError( "Actor not found!\n" );
	}

	//dragging a portrait
	if (!ResRef[0]) {
		core->SetDraggedPortrait(globalID, Slot);
		Py_INCREF( Py_None );
		return Py_None;
	}

	if ((unsigned int) Slot>core->GetInventorySize()) {
		return AttributeError( "Invalid slot" );
	}
	CREItem* si;
	if (Type) {
		Map *map = actor->GetCurrentArea();
		if (!map) {
			return RuntimeError("No current area!");
		}
		Container *cc = map->GetPile(actor->Pos);
		if (!cc) {
			return RuntimeError( "No current container!" );
		}
		si = cc->RemoveItem(Slot, Count);
	} else {
		si = TryToUnequip( actor, core->QuerySlot(Slot), Count );
		actor->RefreshEffects(NULL);
		// make sure the encumbrance labels stay correct
		actor->CalculateSpeed(false);
		actor->ReinitQuickSlots();
		core->SetEventFlag(EF_SELECTION);
	}
	if (! si) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	Item *item = gamedata->GetItem(si->ItemResRef);
	if (item) {
		if (core->HasFeature(GF_HAS_PICK_SOUND) && item->DescriptionIcon[0]) {
			memcpy(Sound,item->DescriptionIcon,sizeof(ieResRef));
		} else {
			gamedata->GetItemSound(Sound, item->ItemType, item->AnimationType, IS_GET);
		}
		gamedata->FreeItem(item, si->ItemResRef,0);
	}
	if (Sound[0]) {
		core->GetAudioDrv()->Play(Sound, SFX_CHAN_GUI);
	}

	//if res is positive, it is gold!
	int res = core->CanMoveItem(si);
	if (res>0) {
		game->AddGold(res);
		delete si;
		Py_INCREF( Py_None );
		return Py_None;
	}

	core->DragItem (si, ResRef);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_DropDraggedItem__doc,
"===== DropDraggedItem =====\n\
\n\
**Prototype:** GemRB.DropDraggedItem (PartyID, Slot)\n\
\n\
**Description:** Put currently dragged item to specified PC and slot. Stop \n\
dragging. Dropping the item in an invalid slot will result in 0. Partial \n\
success may happen if the item was dropped into a stack, but not all items \n\
were moved. The dragging status will be removed only after a complete \n\
success. Not all inventory slots may carry any type of item. The item could \n\
be dropped in an unspecified inventory slot, the ground, or an equippable \n\
slot fitting for the item.\n\
\n\
**Parameters:**\n\
  * PartyID - the actor's inparty index\n\
  * Slot    - the Inventory Slot or special values:\n\
    * -1 any equippable slot fitting for the item\n\
    * -2 ground\n\
    * -3 any empty inventory slot\n\
\n\
**Return value:** integer, 0 (failure), 1 (partial success), 2 (success) or 3 (swapped item)\n\
\n\
**See also:** [[guiscript:DragItem]], [[guiscript:IsDraggingItem]], [[guiscript:CanUseItemType]]\n\
"
);

static PyObject* GemRB_DropDraggedItem(PyObject * /*self*/, PyObject* args)
{
	ieResRef Sound;
	int globalID, Slot;

	if (!PyArg_ParseTuple( args, "ii", &globalID, &Slot)) {
		return AttributeError( GemRB_DropDraggedItem__doc );
	}

	// FIXME
	if (core->GetDraggedItem() == NULL) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	Label* l = core->GetMessageLabel();
	if (l) {
		// this is how BG2 behaves, not sure about others
		l->SetText(L""); // clear previous message
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int res;

	if (Slot==-2) {
		Map *map = actor->GetCurrentArea();
		if (!map) {
			return RuntimeError("No current area!");
		}
		Container *cc = map->GetPile(actor->Pos);
		if (!cc) {
			return RuntimeError( "No current container!" );
		}
		CREItem *si = core->GetDraggedItem();
		res = cc->AddItem(si);
		Item *item = gamedata->GetItem(si->ItemResRef);
		if (item) {
			if (core->HasFeature(GF_HAS_PICK_SOUND) && item->ReplacementItem[0]) {
				memcpy(Sound,item->ReplacementItem,sizeof(ieResRef));
			} else {
				gamedata->GetItemSound(Sound, item->ItemType, item->AnimationType, IS_DROP);
			}
			gamedata->FreeItem(item, si->ItemResRef,0);
			if (Sound[0]) {
				core->GetAudioDrv()->Play(Sound, SFX_CHAN_GUI);
			}
		}
		if (res == 2) {
			// Whole amount was placed
			core->ReleaseDraggedItem ();
		}
		return PyInt_FromLong( res );
	}

	int Slottype, Effect;
	switch(Slot) {
	case -1:
		//anything but inventory
		Slottype = ~SLOT_INVENTORY;
		Effect = 1;
		break;
	case -3:
		//only inventory
		Slottype = -1;
		Effect = 0;
		break;
	default:
		Slot = core->QuerySlot(Slot);
		Slottype = core->QuerySlotType( Slot );
		Effect = core->QuerySlotEffects( Slot );
	}

	// too far away?
	Actor *current = game->FindPC(game->GetSelectedPCSingle());
	if (current && current != actor &&
		(actor->GetCurrentArea() != current->GetCurrentArea() ||
		SquaredPersonalDistance(actor, current) > MAX_DISTANCE * MAX_DISTANCE)) {
		displaymsg->DisplayConstantString(STR_TOOFARAWAY, DMC_WHITE);
		return PyInt_FromLong(ASI_FAILED);
	}

	CREItem * slotitem = core->GetDraggedItem();
	Item *item = gamedata->GetItem( slotitem->ItemResRef );
	if (!item) {
		return PyInt_FromLong(ASI_FAILED);
	}
	bool ranged = item->GetWeaponHeader(true) != NULL;

	// can't equip item because of similar already equipped
	if (Effect) {
		if (item->ItemExcl & actor->inventory.GetEquipExclusion(Slot)) {
			displaymsg->DisplayConstantString(STR_ITEMEXCL, DMC_WHITE);
			//freeing the item before returning
			gamedata->FreeItem( item, slotitem->ItemResRef, false );
			return PyInt_FromLong(ASI_FAILED);
		}
	}

	if ((Slottype!=-1) && (Slottype & SLOT_WEAPON)) {
		CREItem *item = actor->inventory.GetUsedWeapon(false, Effect); //recycled variable
		if (item && (item->Flags & IE_INV_ITEM_CURSED)) {
			displaymsg->DisplayConstantString(STR_CURSED, DMC_WHITE);
			return PyInt_FromLong(ASI_FAILED);
		}
	}

	// can't equip item because it is not identified
	if ( (Slottype == SLOT_ITEM) && !(slotitem->Flags&IE_INV_ITEM_IDENTIFIED)) {
		ITMExtHeader *eh = item->GetExtHeader(0);
		if (eh && eh->IDReq) {
			displaymsg->DisplayConstantString(STR_ITEMID, DMC_WHITE);
			gamedata->FreeItem( item, slotitem->ItemResRef, false );
			return PyInt_FromLong(ASI_FAILED);
		}
	}

	//it is always possible to put these items into the inventory
	if (!(Slottype&SLOT_INVENTORY)) {
		if (CheckRemoveItem(actor, slotitem, CRI_EQUIP)) {
			return PyInt_FromLong(ASI_FAILED);
		}
	}

	//CanUseItemType will check actor's class bits too
	Slottype = core->CanUseItemType (Slottype, item, actor, true);
	//resolve the equipping sound, it needs to be resolved before
	//the item is freed
	if (core->HasFeature(GF_HAS_PICK_SOUND) && item->ReplacementItem[0]) {
		memcpy(Sound, item->ReplacementItem, 9);
	} else {
		gamedata->GetItemSound(Sound, item->ItemType, item->AnimationType, IS_DROP);
	}

	//freeing the item before returning
	gamedata->FreeItem( item, slotitem->ItemResRef, false );
	if ( !Slottype) {
		return PyInt_FromLong(ASI_FAILED);
	}
	res = actor->inventory.AddSlotItem(slotitem, Slot, Slottype, ranged);
	if (res) {
		//release it only when fully placed
		if (res==ASI_SUCCESS) {
			// make sure the encumbrance labels stay correct
			actor->CalculateSpeed(false);
			core->ReleaseDraggedItem ();
		}
		// res == ASI_PARTIAL
		//EquipItem (in AddSlotItem) already called RefreshEffects
		actor->ReinitQuickSlots();
	//couldn't place item there, try swapping (only if slot is explicit)
	} else if ( Slot >= 0 ) {
		//swapping won't cure this
		res = actor->inventory.WhyCantEquip(Slot, slotitem->Flags&IE_INV_ITEM_TWOHANDED, ranged);
		if (res) {
			displaymsg->DisplayConstantString(res, DMC_WHITE);
			return PyInt_FromLong(ASI_FAILED);
		}
		// pst: also check TNO earing/eye silliness: both share the same slot type
		if (Slottype == 1 && !CheckEyeEarMatch(slotitem, Slot)) {
			displaymsg->DisplayConstantString(STR_WRONGITEMTYPE, DMC_WHITE);
			return PyInt_FromLong(ASI_FAILED);
		}
		CREItem *tmp = TryToUnequip(actor, Slot, 0 );
		if (tmp) {
			//this addslotitem MUST succeed because the slot was
			//just emptied (canuseitemtype already confirmed too)
			actor->inventory.AddSlotItem( slotitem, Slot, Slottype );
			core->ReleaseDraggedItem ();
			DragItem(tmp);
			// switched items, not returned by normal AddSlotItem
			res = ASI_SWAPPED;
			//EquipItem (in AddSlotItem) already called RefreshEffects
			actor->RefreshEffects(NULL);
			// make sure the encumbrance labels stay correct
			actor->CalculateSpeed(false);
			actor->ReinitQuickSlots();
			core->SetEventFlag(EF_SELECTION);
		} else {
			res = ASI_FAILED;
		}
	} else {
		displaymsg->DisplayConstantString(STR_INVFULL, DMC_WHITE);
	}

	if (Sound[0]) {
		core->GetAudioDrv()->Play(Sound, SFX_CHAN_GUI);
	}
	return PyInt_FromLong( res );
}

PyDoc_STRVAR( GemRB_IsDraggingItem__doc,
"===== IsDraggingItem =====\n\
\n\
**Prototype:** GemRB.IsDraggingItem ()\n\
\n\
**Description:** Returns 1 if the player is dragging an item with the mouse \n\
(usually in the inventory screen). Returns 2 if the player is dragging a \n\
portrait (rearranging the party).\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** integer, 1 if we are dragging an item, 2 for portraits\n\
\n\
**See also:** [[guiscript:DropDraggedItem]], [[guiscript:DragItem]]"
);

static PyObject* GemRB_IsDraggingItem(PyObject * /*self*/, PyObject* /*args*/)
{
	if (core->GetDraggedPortrait()) {
		return PyInt_FromLong(2);
	}
	return PyInt_FromLong( core->GetDraggedItem() != NULL );
}

PyDoc_STRVAR( GemRB_GetSystemVariable__doc,
"===== GetSystemVariable =====\n\
\n\
**Prototype:** GemRB.GetSystemVariable (Index)\n\
\n\
**Description:** Returns the named Interface attribute.\n\
\n\
**Parameters:**\n\
  * Index could have the following values:\n\
    * SV_BPP = 0 - bpp (color resolution)\n\
    * SV_WIDTH = 1 - screen width\n\
    * SV_HEIGHT = 2 - screen height\n\
    * SV_GAMEPATH = 3 - game path\n\
    * SV_TOUCH = 4 - are we using touch input mode?\n\
\n\
**Return value:** This function returns -1 if the index is invalid.\n\
\n\
**See also:** [[guiscript:GetGameString]]\n\
"
);

static PyObject* GemRB_GetSystemVariable(PyObject * /*self*/, PyObject* args)
{
	int Variable, value = 0;
	char path[_MAX_PATH] = { '\0' };

	if (!PyArg_ParseTuple( args, "i", &Variable)) {
		return AttributeError( GemRB_GetSystemVariable__doc );
	}
	switch(Variable) {
		case SV_BPP: value = core->Bpp; break;
		case SV_WIDTH: value = core->Width; break;
		case SV_HEIGHT: value = core->Height; break;
		case SV_GAMEPATH: strlcpy(path, core->GamePath, _MAX_PATH); break;
		case SV_TOUCH: value = core->GetVideoDriver()->TouchInputEnabled(); break;
		default: value = -1; break;
	}
	if (path[0]) {
		return PyString_FromString(path);
	} else {
		return PyInt_FromLong( value );
	}
}

PyDoc_STRVAR( GemRB_CreateItem__doc,
"===== CreateItem =====\n\
\n\
**Prototype:** GemRB.CreateItem (PartyID, ItemResRef, [SlotID, Charge0, Charge1, Charge2])\n\
\n\
**Description:** Creates item in the inventory of the player character.\n\
\n\
**Parameters:** \n\
  * PartyID    - the PC's position in the party\n\
  * ItemResRef - the item's name (.itm resref)\n\
  * SlotID     - Inventory Slot (-1 means any backpack slot)\n\
  * Charge0-2  - the item's stack amount/charges\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_CreateItem(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	int SlotID=-1;
	int Charge0=1,Charge1=0,Charge2=0;
	const char *ItemResRef;

	if (!PyArg_ParseTuple( args, "is|iiii", &globalID, &ItemResRef, &SlotID, &Charge0, &Charge1, &Charge2)) {
		return AttributeError( GemRB_CreateItem__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (SlotID==-1) {
		//This is already a slot ID we need later
		SlotID=actor->inventory.FindCandidateSlot(SLOT_INVENTORY,0);
	} else {
		//I believe we need this only here
		SlotID = core->QuerySlot(SlotID);
	}

	if (SlotID==-1) {
		// Create item on ground
		Map *map = actor->GetCurrentArea();
		if (map) {
			CREItem *item = new CREItem();
			if (!CreateItemCore(item, ItemResRef, Charge0, Charge1, Charge2)) {
				delete item;
			} else {
				map->AddItemToLocation(actor->Pos, item);
			}
		}
	} else {
		// Note: this forcefully gets rid of any item currently
		// in the slot without properly unequipping it
		actor->inventory.SetSlotItemRes( ItemResRef, SlotID, Charge0, Charge1, Charge2 );
		actor->inventory.EquipItem(SlotID);
		//EquipItem already called RefreshEffects
		actor->ReinitQuickSlots();
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetAvatarsValue__doc,
"===== GetAvatarsValue =====\n\
\n\
**Prototype:** GemRB.GetAvatarsValue (globalID, column)\n\
\n\
**Description:** Returns an entry from the avatars.2da table, accounting \n\
for animation ID ranges.\n\
\n\
**Parameters:** \n\
  * globalID - party ID or global ID of the actor to use\n\
  * column - which armor level to use\n\
\n\
**Return value:** string, bam resref\n\
"
);
// NOTE: currently it can only lookup the animation prefixes!
static PyObject* GemRB_GetAvatarsValue(PyObject * /*self*/, PyObject* args)
{
	int globalID, col;

	if (!PyArg_ParseTuple(args, "ii", &globalID, &col)) {
		return AttributeError( GemRB_GetAvatarsValue__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	ieResRef prefix;
	strnuprcpy(prefix, actor->GetAnims()->GetArmourLevel(col), sizeof(ieResRef)-1);

	return PyString_FromResRef(prefix);
}

PyDoc_STRVAR( GemRB_SetMapAnimation__doc,
"===== SetMapAnimation =====\n\
\n\
**Prototype:** GemRB.SetMapAnimation (X, Y, BAMresref[, flags, cycle, height])\n\
\n\
**Description:** Creates an area animation. Used for PST Modron mazes.\n\
\n\
**Parameters:** \n\
  * X, Y - map position to create at\n\
  * BAMresref - bam animation to use\n\
  * flags - drawing flags\n\
  * cycle - select a different cycle to play\n\
  * height - vertical offset \n\
\n\
**Return value:** N/A\n\
"
);

static PyObject* GemRB_SetMapAnimation(PyObject * /*self*/, PyObject* args)
{
	int x,y;
	const char *ResRef;
	int Cycle = 0;
	int Flags = 0x19;
	int Height = 0x1e;
	//the animation is cloned by AddAnimation, so we can keep the original on
	//the stack
	AreaAnimation anim;

	if (!PyArg_ParseTuple( args, "iis|iii", &x, &y, &ResRef, &Flags, &Cycle, &Height)) {
		return AttributeError( GemRB_SetMapAnimation__doc );
	}

	GET_GAME();

	GET_MAP();

	anim.appearance=0xffffffff; //scheduled for every hour
	anim.Pos.x=(short) x;
	anim.Pos.y=(short) y;
	strnlwrcpy(anim.Name, ResRef, 8);
	strnlwrcpy(anim.BAM, ResRef, 8);
	anim.Flags=Flags;
	anim.sequence=Cycle;
	anim.height=Height;
	if (Flags&A_ANI_ACTIVE) {
		map->AddAnimation(&anim);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetMapnote__doc,
"===== SetMapnote =====\n\
\n\
**Prototype:** GemRB.SetMapnote(X, Y, color, text)\n\
\n\
**Description:** Adds or removes a mapnote to the current map (area).\n\
\n\
**Parameters:**\n\
  * X, Y - the position of the mapnote\n\
  * color - the color index (0-7) of the note (in case of PST it is only 0 or 1)\n\
  * text - string, the text of the note. If it's empty, the mapnote is removed.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_CreateMapControl]]\n\
"
);

static PyObject* GemRB_SetMapnote(PyObject * /*self*/, PyObject* args)
{
	int x,y;
	int color=0;
	const char *txt=NULL;

	if (!PyArg_ParseTuple( args, "ii|is", &x, &y, &color, &txt)) {
		return AttributeError( GemRB_SetMapnote__doc );
	}
	GET_GAME();

	GET_MAP();

	Point point;
	point.x=x;
	point.y=y;
	if (txt && txt[0]) {
		map->AddMapNote(point, color, StringFromCString(txt));
	} else {
		map->RemoveMapNote(point);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetMapDoor__doc,
"===== SetMapDoor =====\n\
\n\
**Prototype:** GemRB.SetMapDoor (DoorName, State)\n\
\n\
**Description:** Modifies a door's open state in the current area.\n\
\n\
**Parameters:** \n\
  * DoorName - scripting name of the targe door\n\
  * State - boolean, opened or closed\n\
\n\
**Return value:** N/A\n\
"
);

static PyObject* GemRB_SetMapDoor(PyObject * /*self*/, PyObject* args)
{
	const char *DoorName;
	int State;

	if (!PyArg_ParseTuple( args, "si", &DoorName, &State) ) {
		return AttributeError( GemRB_SetMapDoor__doc);
	}

	GET_GAME();

	GET_MAP();

	Door *door = map->TMap->GetDoor(DoorName);
	if (!door) {
		return RuntimeError( "No such door!" );
	}

	door->SetDoorOpen(State, 0, 0);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetMapExit__doc,
"===== SetMapExit =====\n\
\n\
**Prototype:** GemRB.SetMapExit (ExitName[, NewArea, NewEntrance])\n\
\n\
**Description:** Modifies the target of an exit in the current area. If no \n\
destination is given, then the exit will be disabled.\n\
\n\
**Parameters:** \n\
  * ExitName - scripting name\n\
  * NewArea - new exit target area\n\
  * NewEntrance - target areas entrance to link to\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_SetMapExit(PyObject * /*self*/, PyObject* args)
{
	const char *ExitName;
	const char *NewArea = NULL;
	const char *NewEntrance = NULL;

	if (!PyArg_ParseTuple( args, "s|ss", &ExitName, &NewArea, &NewEntrance)) {
		return AttributeError( GemRB_SetMapExit__doc );
	}

	GET_GAME();

	GET_MAP();

	InfoPoint *ip = map->TMap->GetInfoPoint(ExitName);
	if (!ip || ip->Type!=ST_TRAVEL) {
		return RuntimeError( "No such exit!" );
	}

	if (!NewArea) {
		//disable entrance
		ip->Flags|=TRAP_DEACTIVATED;
	} else {
		//activate entrance
		ip->Flags&=~TRAP_DEACTIVATED;
		//set destination area
		strnuprcpy(ip->Destination, NewArea, sizeof(ieResRef)-1 );
		//change entrance only if supplied
		if (NewEntrance) {
			strnuprcpy(ip->EntranceName, NewEntrance, sizeof(ieVariable)-1 );
		}
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetMapRegion__doc,
"===== SetMapRegion =====\n\
\n\
**Prototype:** GemRB.SetMapRegion (TrapName[, trapscript])\n\
\n\
**Description:** Enables or disables an infopoint in the current area.\n\
\n\
**Parameters:** \n\
  * TrapName - scripting name\n\
  * trapscript - new script to assign\n\
\n\
**Return value:** N/A\n\
"
);

static PyObject* GemRB_SetMapRegion(PyObject * /*self*/, PyObject* args)
{
	const char *Name;
	const char *TrapScript = NULL;

	if (!PyArg_ParseTuple( args, "s|s", &Name, &TrapScript)) {
		return AttributeError( GemRB_SetMapRegion__doc );
	}

	GET_GAME();
	GET_MAP();

	InfoPoint *ip = map->TMap->GetInfoPoint(Name);
	if (ip) {
		if (TrapScript && TrapScript[0]) {
			ip->Flags&=~TRAP_DEACTIVATED;
			ip->SetScript(TrapScript,0);
		} else {
			ip->Flags|=TRAP_DEACTIVATED;
		}
	}

	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_CreateCreature__doc,
"===== CreateCreature =====\n\
\n\
**Prototype:** GemRB.CreateCreature (globalID, CreResRef[, posX, posY])\n\
\n\
**Description:** Creates creature in the vicinity of the specified actor or \n\
at the specified point (takes precedence if both are specified).\n\
\n\
**Parameters:** \n\
  * globalID - party ID or global ID of the actor to use\n\
  * CreResRef  - the creature's name (.cre resref)\n\
  * posX, posY - position to create at\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:CreateItem]]"
);

static PyObject* GemRB_CreateCreature(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	const char *CreResRef;
	int PosX = -1, PosY = -1;

	if (!PyArg_ParseTuple( args, "is|ii", &globalID, &CreResRef, &PosX, &PosY)) {
		return AttributeError( GemRB_CreateCreature__doc );
	}

	GET_GAME();
	GET_MAP();

	if (PosX!=-1 && PosY!=-1) {
		map->SpawnCreature(Point(PosX, PosY), CreResRef);
	} else {
		GET_ACTOR_GLOBAL();
		map->SpawnCreature(actor->Pos, CreResRef, 10, 10);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_RevealArea__doc,
"===== RevealArea =====\n\
\n\
**Prototype:** GemRB.RevealArea (x, y, radius, type)\n\
\n\
**Description:** Reveals part of the area.\n\
\n\
**Parameters:** \n\
  * x - x coordinate of the center point\n\
  * y - y coordinate of the center point\n\
  * radius - radius of the circle to explore\n\
  * type - if positive will make the effect ignore blocked portions of the map\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:ExploreArea]]"
);

static PyObject* GemRB_RevealArea(PyObject * /*self*/, PyObject* args)
{
	int x,y;
	int radius;
	int Value;

	if (!PyArg_ParseTuple( args, "iiii", &x, &y, &radius, &Value)) {
		return AttributeError( GemRB_RevealArea__doc );
	}

	Point p(x,y);
	GET_GAME();

	GET_MAP();

	map->ExploreMapChunk( p, radius, Value );

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_ExploreArea__doc,
"===== ExploreArea =====\n\
\n\
**Prototype:** ExploreArea ([bitvalue=-1])\n\
\n\
**Description:** Explores or unexplores the whole area. Basically fills the \n\
explored bitmap with the value given. If there was no value given, it will \n\
fill with -1 (all bit set).\n\
\n\
**Parameters:**\n\
  * bitvalue:\n\
    * 0 - undo explore\n\
    * -1 - explore\n\
    * all other values give meaningless results\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:MoveToArea]], [[guiscript:RevealArea]]"
);

static PyObject* GemRB_ExploreArea(PyObject * /*self*/, PyObject* args)
{
	int Value=-1;

	if (!PyArg_ParseTuple( args, "|i", &Value)) {
		return AttributeError( GemRB_ExploreArea__doc );
	}
	GET_GAME();

	GET_MAP();

	map->Explore( Value );

	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_GetRumour__doc,
"===== GetRumour =====\n\
\n\
**Prototype:** GemRB.GetRumour (percent, DialogResRef)\n\
\n\
**Description:** Gets a rumour string reference from a rumour dialog.\n\
\n\
**Parameters:**\n\
  * percent - chance of not returning -1\n\
  * DialogResRef - a rumour dialog resource\n\
\n\
**Return value:** a string reference\n\
\n\
**See also:** [[guiscript:EnterStore]], [[guiscript:GetStoreDrink]], [[guiscript:GetStore]]\n\
"
);

static PyObject* GemRB_GetRumour(PyObject * /*self*/, PyObject* args)
{
	int percent;
	const char *ResRef;

	if (!PyArg_ParseTuple( args, "is", &percent, &ResRef)) {
		return AttributeError( GemRB_GetRumour__doc );
	}
	if (RAND(0, 99) >= percent) {
		return PyInt_FromLong( -1 );
	}

	ieStrRef strref = core->GetRumour( ResRef );
	return PyInt_FromLong( strref );
}

PyDoc_STRVAR( GemRB_GamePause__doc,
"===== GamePause =====\n\
\n\
**Prototype:** GemRB.GamePause (pause, quiet)\n\
\n\
**Description:** Pauses or unpauses the current game or toggle whatever was \n\
set. This affects all ingame events, including: scripts, animations, \n\
movement. It doesn't affect the GUI.\n\
\n\
**Parameters:**\n\
  * pause  - int,\n\
    * 0 = continue\n\
    * 1 = pause\n\
    * 2 = toggle pause\n\
    * 3 = query state\n\
  * quiet  - int bitfield,\n\
    * 1 - no feedback\n\
    * 2 - forced pause\n\
\n\
**Return value:** the resulting paused state"
);

static PyObject* GemRB_GamePause(PyObject * /*self*/, PyObject* args)
{
	int pause, quiet;
	int ret;

	if (!PyArg_ParseTuple( args, "ii", &pause, &quiet)) {
		return AttributeError( GemRB_GamePause__doc );
	}

	GET_GAMECONTROL();

	//this will trigger when pause is not 0 or 1
	switch(pause)
	{
	case 2:
		ret = core->TogglePause();
		break;
	case 0:
	case 1:
		core->SetPause((PauseSetting)pause, quiet);
		// fall through
	default:
		ret = gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS;
	}
	if (ret) {
		Py_INCREF( Py_True );
		return Py_True;
	} else {
		Py_INCREF( Py_False );
		return Py_False;
	}
}

PyDoc_STRVAR( GemRB_CheckFeatCondition__doc,
"===== CheckFeatCondition =====\n\
\n\
**Prototype:** GemRB.CheckFeatCondition (partyslot, a_stat, a_value, b_stat, b_value, c_stat, c_value, d_stat, d_value[, a_op, b_op, c_op, d_op])\n\
\n\
**Description:** Checks if a party character is eligible for a feat. The \n\
formula is: (stat[a]~a or stat[b]~b) and (stat[c]~c or stat[d]~d). Where ~ \n\
is a relational operator. If the operators are omitted, the default \n\
operator is >=.\n\
\n\
**Parameters:**\n\
  * partyslot - the characters position in the party\n\
  * a_stat ... d_stat - stat IDs\n\
  * a_value ... d_value - stat value limits\n\
  * a_op ... d_op - operator to use for comparing x_stat to x_value\n\
\n\
**Return value:** bool\n\
\n\
**See also:** [[guiscript:GetPlayerStat]], [[guiscript:SetPlayerStat]]\n\
"
);

static PyObject* GemRB_CheckFeatCondition(PyObject * /*self*/, PyObject* args)
{
	int i;
	const char *callback = NULL;
	PyObject* p[13];
	int v[13];
	for(i=9;i<13;i++) {
		p[i]=NULL;
		v[i]=GREATER_OR_EQUALS;
	}

	if (!PyArg_UnpackTuple( args, "ref", 9, 13, &p[0], &p[1], &p[2], &p[3], &p[4], &p[5], &p[6], &p[7], &p[8], &p[9], &p[10], &p[11], &p[12] )) {
		return AttributeError( GemRB_CheckFeatCondition__doc );
	}

	if (!PyObject_TypeCheck( p[0], &PyInt_Type )) {
		return AttributeError( GemRB_CheckFeatCondition__doc );
	}
	v[0]=PyInt_AsLong( p[0] ); //slot

	if (PyObject_TypeCheck( p[1], &PyInt_Type )) {
		v[1]=PyInt_AsLong( p[1] ); //a_stat
	} else {
		if (!PyObject_TypeCheck( p[1], &PyString_Type )) {
			return AttributeError( GemRB_CheckFeatCondition__doc );
		}
		callback = PyString_AsString( p[1] ); // callback
		if (callback == NULL) {
			return RuntimeError("Null string received");
		}
	}
	v[0]=PyInt_AsLong( p[0] );

	for(i=2;i<9;i++) {
		if (!PyObject_TypeCheck( p[i], &PyInt_Type )) {
			return AttributeError( GemRB_CheckFeatCondition__doc );
		}
		v[i]=PyInt_AsLong( p[i] );
	}

	if (p[9]) {
		for(i=9;i<13;i++) {
			if (!PyObject_TypeCheck( p[i], &PyInt_Type )) {
				return AttributeError( GemRB_CheckFeatCondition__doc );
			}
			v[i]=PyInt_AsLong( p[i] );
		}
	}

	GET_GAME();

	Actor *actor = game->FindPC(v[0]);
	if (!actor) {
		return RuntimeError( "Actor not found!\n" );
	}

	/* see if the special function exists */
	if (callback) {
		char fname[32];

		snprintf(fname, 32, "Check_%s", callback);
		PyObject* param = PyTuple_New( 11 );
		PyTuple_SetItem( param, 0, PyInt_FromLong(v[0]) );
		for (i = 3;i<13;i++) {
			PyTuple_SetItem( param, i-2, PyInt_FromLong( v[i] ) );
		}

		PyObject *pValue = gs->RunFunction("Feats", fname, param);

		/* we created this parameter, now we don't need it*/
		Py_DECREF( param );
		if (pValue) {
			/* don't think we need any incref */
			return pValue;
		}
		return RuntimeError( "Callback failed" );
	}

	bool ret = true;

	if (v[1] || v[2]) {
		ret = CheckStat(actor, v[1], v[2], v[9]);
	}

	if (v[3] || v[4]) {
		ret |= CheckStat(actor, v[3], v[4], v[10]);
	}

	if (!ret)
		goto endofquest;

	if (v[5] || v[6]) {
		// no | because the formula is (a|b) & (c|d)
		ret = CheckStat(actor, v[5], v[6], v[11]);
	}

	if (v[7] || v[8]) {
		ret |= CheckStat(actor, v[7], v[8], v[12]);
	}

endofquest:
	if (ret) {
		Py_INCREF( Py_True );
		return Py_True;
	} else {
		Py_INCREF( Py_False );
		return Py_False;
	}
}

PyDoc_STRVAR( GemRB_HasFeat__doc,
"===== HasFeat =====\n\
\n\
**Prototype:** GemRB.HasFeat (globalID, feat)\n\
\n\
**Description:** Returns the number of times this feat was taken if the \n\
actor has the passed feat id (from ie_feats.py).\n\
\n\
**Parameters:** \n\
  * globalID - party ID or global ID of the actor to use\n\
  * feat - feat index from ie_feats.py\n\
\n\
**Return value:** number of feat levels\n\
\n\
**See also:** [[guiscript:CheckFeatCondition]]\n\
"
);

static PyObject* GemRB_HasFeat(PyObject * /*self*/, PyObject* args)
{
	int globalID, featindex;

	if (!PyArg_ParseTuple( args, "ii", &globalID, &featindex )) {
		return AttributeError( GemRB_HasFeat__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();
	return PyInt_FromLong(actor->GetFeat(featindex));
}

PyDoc_STRVAR( GemRB_SetFeat__doc,
"===== SetFeat =====\n\
\n\
**Prototype:** GemRB.SetFeat (globalID, feat, value)\n\
\n\
**Description:** Sets a feat. Handles both boolean and numeric fields.\n\
\n\
**Parameters:** \n\
  * globalID - party ID or global ID of the actor to use\n\
  * feat - feat index from ie_feats.py\n\
  * value - target feat value\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:HasFeat]]"
);

static PyObject* GemRB_SetFeat(PyObject * /*self*/, PyObject* args)
{
	int globalID, featindex, value;

	if (!PyArg_ParseTuple( args, "iii", &globalID, &featindex, &value )) {
		return AttributeError( GemRB_SetFeat__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();
	actor->SetFeatValue(featindex, value, false);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetMaxEncumbrance__doc,
"===== GetMaxEncumbrance =====\n\
\n\
**Prototype:** GemRB.GetMaxEncumbrance (globalID)\n\
\n\
**Description:** Returns the maximum weight the PC may carry before \n\
becoming encumbered.\n\
\n\
**Parameters:** \n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** integer"
);

static PyObject* GemRB_GetMaxEncumbrance(PyObject * /*self*/, PyObject* args)
{
	int globalID;

	if (!PyArg_ParseTuple( args, "i", &globalID )) {
		return AttributeError( GemRB_GetMaxEncumbrance__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyInt_FromLong( actor->GetMaxEncumbrance() );
}

PyDoc_STRVAR( GemRB_GetAbilityBonus__doc,
"===== GetAbilityBonus =====\n\
\n\
**Prototype:** GemRB.GetAbilityBonus (stat, column, value[, ex])\n\
\n\
**Description:** Returns ability based values from different .2da files.\n\
\n\
**Parameters:**\n\
  * stat   - a stat ID, like IE_STR\n\
  * column - integer, the column index of the value in the .2da file:\n\
    * **IE_STR:** 0 - To hit, 1 - Damage, 2 - Open doors, 3 - Weight allowance\n\
    * **IE_INT:** 0 - learn spell, 1 - max spell level, 2 - max spell number on level\n\
    * **IE_DEX:** 0 - reaction adjustment, 1 - missile,  2 - AC\n\
    * **IE_CON:** 0 - normal hp, 1 - warrior hp, 2 - minimum hp roll, 3 - hp regen rate, 4 - fatigue\n\
    * **IE_CHR:** 0 - reaction\n\
    * **IE_LORE:** 0 - lore bonus (int+wis based)\n\
    * **IE_REPUTATION:** 0 - reaction (chr+reputation based)\n\
    * **IE_WIS:** 0 - percentile xp bonus\n\
  * value - stat value to check with\n\
  * ex - extra stat value to check with (used for extended strength)\n\
\n\
**Return value:** -9999 if the parameters are illegal, otherwise the required bonus\n\
\n\
**See also:** [[guiscript:SetPlayerStat]], [[guiscript:GetPlayerStat]], [[guiscript:Table_GetValue]]"
);

static PyObject* GemRB_GetAbilityBonus(PyObject * /*self*/, PyObject* args)
{
	int stat, column, value, ex = 0;
	int ret;

	if (!PyArg_ParseTuple( args, "iii|i", &stat, &column, &value, &ex)) {
		return AttributeError( GemRB_GetAbilityBonus__doc );
	}

	GET_GAME();

	Actor *actor = game->FindPC(game->GetSelectedPCSingle());
	if (!actor) {
		return RuntimeError( "Actor not found!\n" );
	}

	switch (stat) {
		case IE_STR:
			ret=core->GetStrengthBonus(column, value, ex);
			break;
		case IE_INT:
			ret=core->GetIntelligenceBonus(column, value);
			break;
		case IE_DEX:
			ret=core->GetDexterityBonus(column, value);
			break;
		case IE_CON:
			ret=core->GetConstitutionBonus(column, value);
			break;
		case IE_CHR:
			ret=core->GetCharismaBonus(column, value);
			break;
		case IE_LORE:
			ret=core->GetLoreBonus(column, value);
			break;
		case IE_REPUTATION: //both chr and reputation affect the reaction, but chr is already taken
			ret=GetReaction(actor, NULL); // this is used only for display, so the null is fine
			break;
		case IE_WIS:
			ret=core->GetWisdomBonus(column, value);
			break;
		default:
			return RuntimeError( "Invalid ability!");
	}
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_LeaveParty__doc,
"===== LeaveParty =====\n\
\n\
**Prototype:** GemRB.LeaveParty (globalID [, Dialog])\n\
\n\
**Description:** Removes the character from the party and initiates dialog \n\
if demanded.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * Dialog:\n\
    * if set to 1, initiate the dialog.\n\
    * if set to 2, execute 'SetLeavePartyDialogFile' and initiate dialog.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetPartySize]]\n\
"
);

static PyObject* GemRB_LeaveParty(PyObject * /*self*/, PyObject* args)
{
	int globalID, initDialog = 0;

	if (!PyArg_ParseTuple( args, "i|i", &globalID, &initDialog )) {
		return AttributeError( GemRB_LeaveParty__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (initDialog) {
		if (initDialog == 2)
		GameScript::SetLeavePartyDialogFile(actor, NULL);
		if(actor->GetBase(IE_HITPOINTS) > 0) {
			actor->Stop();
			actor->AddAction( GenerateAction("Dialogue([PC])") );
		}
	}
	game->LeaveParty (actor);

	Py_RETURN_NONE;
}

typedef union pack {
	ieDword data;
	ieByte bytes[4];
} packtype;

static void ReadActionButtons()
{
	unsigned int i;

	memset(GUIAction, -1, sizeof(GUIAction));
	memset(GUITooltip, -1, sizeof(GUITooltip));
	memset(GUIResRef, 0, sizeof(GUIResRef));
	memset(GUIEvent, 0, sizeof(GUIEvent));
	int table = gamedata->LoadTable( "guibtact" );
	if (table<0) {
		return;
	}
	Holder<TableMgr> tab = gamedata->GetTable( table );
	for (i = 0; i < MAX_ACT_COUNT; i++) {
		packtype row;

		row.bytes[0] = (ieByte) atoi( tab->QueryField(i,0) );
		row.bytes[1] = (ieByte) atoi( tab->QueryField(i,1) );
		row.bytes[2] = (ieByte) atoi( tab->QueryField(i,2) );
		row.bytes[3] = (ieByte) atoi( tab->QueryField(i,3) );
		GUIAction[i] = row.data;
		GUITooltip[i] = atoi( tab->QueryField(i,4) );
		strnlwrcpy(GUIResRef[i], tab->QueryField(i,5), 8);
		strncpy(GUIEvent[i], tab->GetRowName(i), 16);
	}
	gamedata->DelTable( table );
}

static void SetButtonCycle(AnimationFactory *bam, Button *btn, int cycle, unsigned char which)
{
	Sprite2D *tspr = bam->GetFrame( cycle, 0 );
	btn->SetImage( (BUTTON_IMAGE_TYPE)which, tspr );
}

PyDoc_STRVAR( GemRB_Button_SetActionIcon__doc,
"===== Button_SetActionIcon =====\n\
\n\
**Prototype:** GemRB.SetActionIcon (Window, Button, Dict, ActionIndex[, Function])\n\
\n\
**Metaclass Prototype:** SetActionIcon (ActionIndex[, Function])\n\
\n\
**Description:** Sets up an action button based on the guibtact table. \n\
The ActionIndex should be less than 34. This action will set the button's \n\
image, the tooltip and the push button event handler.\n\
\n\
**Parameters:**\n\
  * WindowIndex, ControlIndex - the button's reference\n\
  * ActionIndex - the row number in the guibtact.2da file\n\
  * Function - function key to assign\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetSpellIcon]], [[guiscript:Button_SetItemIcon]], [[guiscript:Window_SetupControls]]"
);

static PyObject* SetActionIcon(int WindowIndex, int ControlIndex, PyObject *dict, int Index, int Function)
{
	if (ControlIndex>99) {
		return AttributeError( GemRB_Button_SetActionIcon__doc );
	}
	if (Index>=MAX_ACT_COUNT) {
		return AttributeError( GemRB_Button_SetActionIcon__doc );
	}
	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	if (Index<0) {
		btn->SetImage( BUTTON_IMAGE_NONE, NULL );
		btn->SetEvent( IE_GUI_BUTTON_ON_PRESS, NULL );
		btn->SetEvent( IE_GUI_BUTTON_ON_RIGHT_PRESS, NULL );
		core->SetTooltip( (ieWord) WindowIndex, (ieWord) ControlIndex, "" );
		//no incref
		return Py_None;
	}

	if (GUIAction[0]==0xcccccccc) {
		ReadActionButtons();
	}

	//FIXME: this is a hardcoded resource (pst has no such one)
	AnimationFactory* bam = ( AnimationFactory* )
		gamedata->GetFactoryResource( GUIResRef[Index],
				IE_BAM_CLASS_ID, IE_NORMAL );
	if (!bam) {
		char tmpstr[24];

		snprintf(tmpstr,sizeof(tmpstr),"%s BAM not found", GUIResRef[Index]);
		return RuntimeError( tmpstr );
	}
	packtype row;

	row.data = GUIAction[Index];
	SetButtonCycle(bam, btn, (char) row.bytes[0], IE_GUI_BUTTON_UNPRESSED);
	SetButtonCycle(bam, btn, (char) row.bytes[1], IE_GUI_BUTTON_PRESSED);
	SetButtonCycle(bam, btn, (char) row.bytes[2], IE_GUI_BUTTON_SELECTED);
	SetButtonCycle(bam, btn, (char) row.bytes[3], IE_GUI_BUTTON_DISABLED);
	btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, OP_NAND);
	PyObject *Event = PyString_FromFormat("Action%sPressed", GUIEvent[Index]);
	PyObject *func = PyDict_GetItem(dict, Event);
	btn->SetEvent( IE_GUI_BUTTON_ON_PRESS, new PythonControlCallback(func) );

	PyObject *Event2 = PyString_FromFormat("Action%sRightPressed", GUIEvent[Index]);
	PyObject *func2 = PyDict_GetItem(dict, Event2);
	btn->SetEvent( IE_GUI_BUTTON_ON_RIGHT_PRESS, new PythonControlCallback(func2) );

	//cannot make this const, because it will be freed
	char *txt = NULL;
	if (GUITooltip[Index] != (ieDword) -1) {
		txt = core->GetCString( GUITooltip[Index] );
	}
	//will free txt
	SetFunctionTooltip(WindowIndex, ControlIndex, txt, Function);

	//no incref
	return Py_None;
}

static PyObject* GemRB_Button_SetActionIcon(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Index;
	int Function = 0;
	PyObject *dict;

	if (!PyArg_ParseTuple( args, "iiOi|i", &WindowIndex, &ControlIndex, &dict, &Index, &Function )) {
		return AttributeError( GemRB_Button_SetActionIcon__doc );
	}

	PyObject* ret = SetActionIcon(WindowIndex, ControlIndex, dict, Index, Function);
	if (ret) {
		Py_INCREF(ret);
	}
	return ret;
}

PyDoc_STRVAR( GemRB_HasResource__doc,
"===== HasResource =====\n\
\n\
**Prototype:** GemRB.HasResource (ResRef, ResType[, silent])\n\
\n\
**Description:** Returns true if the resource is accessible.\n\
\n\
**Parameters:**\n\
  * ResRef - the resource reference (8 characters filename)\n\
  * ResType - the class ID of the resource\n\
  * silent - if nonzero, don't give any output\n\
\n\
**Return value:** boolean"
);

static PyObject* GemRB_HasResource(PyObject * /*self*/, PyObject* args)
{
	const char *ResRef;
	int ResType;
	int silent = 0;

	if (!PyArg_ParseTuple( args, "si|i", &ResRef, &ResType, &silent )) {
		return AttributeError( GemRB_HasResource__doc );
	}
	if (gamedata->Exists(ResRef, ResType, silent)) {
		Py_INCREF( Py_True );
		return Py_True;
	} else {
		Py_INCREF( Py_False );
		return Py_False;
	}
}

PyDoc_STRVAR( GemRB_Window_SetupEquipmentIcons__doc,
"===== Window_SetupEquipmentIcons =====\n\
\n\
**Prototype:** GemRB.SetupEquipmentIcons (WindowIndex, dict, slot[, Start, Offset])\n\
\n\
**Metaclass Prototype:** SetupEquipmentIcons (Slot[, Start, Offset])\n\
\n\
**Description:** Sets up all 12 action buttons for a player character \n\
with the usable equipment functions. \n\
It also sets up the scroll buttons left and right if needed. \n\
If Start is supplied, it will skip the first few items.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the buttons' window index\n\
  * Slot        - the player character's index in the party\n\
  * Start       - start the equipment list from this value\n\
  * Offset      - control ID offset to the first usable button\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_SetupControls]], [[guiscript:UseItem]]"
);

static PyObject* GemRB_Window_SetupEquipmentIcons(PyObject * /*self*/, PyObject* args)
{
	int wi, globalID;
	int Start = 0;
	int Offset = 0; //control offset (iwd2 has the action buttons starting at 6)
	PyObject *dict;

	if (!PyArg_ParseTuple( args, "iOi|ii", &wi, &dict, &globalID, &Start, &Offset)) {
		return AttributeError( GemRB_Window_SetupEquipmentIcons__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	//-2 because of the left/right scroll icons
	if (!ItemArray) {
		ItemArray = (ItemExtHeader *) malloc((GUIBT_COUNT) * sizeof (ItemExtHeader) );
	}
	bool more = actor->inventory.GetEquipmentInfo(ItemArray, Start, GUIBT_COUNT-(Start?1:0));
	int i;
	if (Start||more) {
		PyObject *ret = SetActionIcon(wi,GetControlIndex(wi, Offset),dict, ACT_LEFT,0);
		if (!ret) {
			return RuntimeError("Cannot set action button!\n");
		}
	}
	//FIXME: this is a hardcoded resource (pst has no such one)
	AnimationFactory* bam = ( AnimationFactory* )
		gamedata->GetFactoryResource( "guibtbut",
				IE_BAM_CLASS_ID, IE_NORMAL );
	if (!bam) {
		return RuntimeError("guibtbut BAM not found");
	}

	for (i=0;i<GUIBT_COUNT-(more?1:0);i++) {
		int ci = GetControlIndex(wi, i+Offset+(Start?1:0) );
		Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
		if (!btn) {
			Log(ERROR, "GUIScript", "Button %d in window %d not found!", ci, wi);
			continue;
		}
		PyObject *Function = PyDict_GetItemString(dict, "EquipmentPressed");
		btn->SetEvent(IE_GUI_BUTTON_ON_PRESS, new PythonControlCallback(Function));
		strcpy(btn->VarName,"Equipment");
		btn->Value = Start+i;

		ItemExtHeader *item = ItemArray+i;
		Sprite2D *Picture = NULL;

		if (item->UseIcon[0]) {
			Picture = gamedata->GetBAMSprite(item->UseIcon, 1, 0, true);
			// try cycle 0 if cycle 1 doesn't exist
			// (needed for e.g. sppr707b which is used by Daystar's Sunray)
			if (!Picture)
				Picture = gamedata->GetBAMSprite(item->UseIcon, 0, 0, true);
		}

		if (!Picture) {
			btn->SetState(IE_GUI_BUTTON_DISABLED);
			btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE, OP_SET);
			btn->SetTooltip(NULL);
		} else {
			SetButtonCycle(bam, btn, 0, IE_GUI_BUTTON_UNPRESSED);
			SetButtonCycle(bam, btn, 1, IE_GUI_BUTTON_PRESSED);
			SetButtonCycle(bam, btn, 2, IE_GUI_BUTTON_SELECTED);
			SetButtonCycle(bam, btn, 3, IE_GUI_BUTTON_DISABLED);
			btn->SetPicture( Picture );
			btn->SetState(IE_GUI_BUTTON_UNPRESSED);
			btn->SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_RIGHT, OP_SET);
			char* tip = core->GetCString(item->Tooltip, 0);
			btn->SetTooltip(tip);
			delete tip;

			if (item->Charges && (item->Charges!=0xffff) ) {
				SetItemText(btn, item->Charges, false);
			} else if (!item->Charges && (item->ChargeDepletion==CHG_NONE) ) {
				btn->SetState(IE_GUI_BUTTON_DISABLED);
			}
		}
	}

	if (more) {
		PyObject *ret = SetActionIcon(wi,GetControlIndex(wi, i+Offset+1),dict,ACT_RIGHT,i+1);
		if (!ret) {
			return RuntimeError("Cannot set action button!\n");
		}
	}

	Py_RETURN_NONE;
}

static bool CanUseActionButton(Actor *pcc, int type)
{
	int capability = -1;
	if (core->HasFeature(GF_3ED_RULES)) {
		switch (type) {
		case ACT_STEALTH:
			capability = pcc->GetSkill(IE_STEALTH) + pcc->GetSkill(IE_HIDEINSHADOWS);
			break;
		case ACT_THIEVING:
			capability = pcc->GetSkill(IE_LOCKPICKING) + pcc->GetSkill(IE_PICKPOCKET);
			break;
		case ACT_SEARCH:
			capability = 1; // everyone can try to search
			break;
		default:
			Log(WARNING, "GUIScript", "Unknown action (button) type: %d", type);
		}
	} else {
		// use levels instead, so inactive dualclasses work as expected
		switch (type) {
		case ACT_STEALTH:
			capability = pcc->GetThiefLevel() + pcc->GetMonkLevel() + pcc->GetRangerLevel();
			break;
		case ACT_THIEVING:
			capability = pcc->GetThiefLevel() + pcc->GetBardLevel();
			break;
		case ACT_SEARCH:
			capability = pcc->GetThiefLevel();
			break;
		default:
			Log(WARNING, "GUIScript", "Unknown action (button) type: %d", type);
		}
	}
	return capability > 0;
}

PyDoc_STRVAR( GemRB_Window_SetupControls__doc,
"===== Window_SetupControls =====\n\
\n\
**Prototype:** GemRB.SetupControls (WindowIndex, dict, slot[, Start])\n\
\n\
**Metaclass Prototype:** SetupControls (Slot[, Start])\n\
\n\
**Description:** Sets up all 12 action buttons for a player character, \n\
based on preset preferences and the character's class.\n\
\n\
**Parameters:**\n\
  * WindowIndex - the buttons' window index\n\
  * Slot        - the player character's index in the party\n\
  * Start       - action offset\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Button_SetActionIcon]], [[guiscript:SetDefaultActions]], [[guiscript:Window_SetupEquipmentIcons]]"
);

static PyObject* GemRB_Window_SetupControls(PyObject * /*self*/, PyObject* args)
{
	int wi, globalID;
	int Start = 0;
	PyObject *dict;
	PyObject *Tuple = NULL;

	if (!PyArg_ParseTuple( args, "iOi|iO", &wi, &dict, &globalID, &Start, &Tuple)) {
		return AttributeError( GemRB_Window_SetupControls__doc );
	}

	GET_GAME();

	GET_GAMECONTROL();

	Actor* actor = NULL;

	if (globalID) {
		if (globalID > 1000) {
			actor = game->GetActorByGlobalID( globalID );
		} else {
			actor = game->FindPC( globalID );
		}
	} else {
		if (game->selected.size()==1) {
			actor = game->selected[0];
		}
	}

	if (!actor) {
		return RuntimeError( "Actor not found!\n" );
	}

	ActionButtonRow myrow;
	if (Tuple) {
		if (!PyObject_TypeCheck( Tuple, &PyTuple_Type )) {
			return AttributeError( GemRB_Window_SetupControls__doc );
		}
		if (PyTuple_Size(Tuple)!=GUIBT_COUNT) {
			return AttributeError( GemRB_Window_SetupControls__doc );
		}
		for(int i=0;i<GUIBT_COUNT;i++) {
			PyObject *x = PyTuple_GetItem(Tuple, i);
			if (!PyObject_TypeCheck( x, &PyInt_Type )) {
				return AttributeError( GemRB_Window_SetupControls__doc );
			}
			myrow[i] = PyInt_AsLong(x);
		}
	} else {
		actor->GetActionButtonRow(myrow);
	}
	bool fistdrawn = true;
	ieDword magicweapon = actor->inventory.GetMagicSlot();
	if (!actor->inventory.HasItemInSlot("",magicweapon) ) {
		magicweapon = 0xffff;
	}
	ieDword fistweapon = actor->inventory.GetFistSlot();
	ieDword usedslot = actor->inventory.GetEquippedSlot();
	int tmp;
	for (int i=0;i<GUIBT_COUNT;i++) {
		int ci = GetControlIndex(wi, i+Start);
		if (ci<0) {
			print("Couldn't find button #%d on Window #%d", i+Start, wi);
			return RuntimeError ("No such control!\n");
		}
		int action = myrow[i];
		if (action==100) {
			action = -1;
		} else {
			if (action < ACT_IWDQSPELL) {
				action %= 100;
			}
		}
		Button * btn = (Button *) GetControl(wi,ci,IE_GUI_BUTTON);
		if (!btn) {
			return NULL;
		}
		btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_RIGHT, OP_SET);
		SetItemText(btn, 0, false);
		PyObject *ret = SetActionIcon(wi,ci,dict, action,i+1);

		if (action!=-1) {
			// reset it to the first one, so we can handle them more easily below
			if ( (action>=ACT_IWDQSPELL) && (action<=ACT_IWDQSPELL+9) ) action = ACT_IWDQSPELL;
			else if ( (action>=ACT_IWDQITEM) && (action<=ACT_IWDQITEM+9) ) action = ACT_IWDQITEM;
			else if ( (action>=ACT_IWDQSPEC) && (action<=ACT_IWDQSPEC+9) ) action = ACT_IWDQSPEC;
			else if ( (action>=ACT_IWDQSONG) && (action<=ACT_IWDQSONG+9) ) action = ACT_IWDQSONG;
		}

		int state = IE_GUI_BUTTON_UNPRESSED;
		ieDword modalstate = actor->Modal.State;
		int type;
		switch (action) {
		case ACT_INNATE:
			if (actor->spellbook.IsIWDSpellBook()) {
				type = (1<<IE_IWD2_SPELL_INNATE) | (1<<IE_IWD2_SPELL_SHAPE);
			} else {
				type = 1<<IE_SPELL_TYPE_INNATE;
			}
			if (!actor->spellbook.GetSpellInfoSize(type)) {
				state = IE_GUI_BUTTON_DISABLED;
			}
			break;
		case ACT_CAST:
			//luckily the castable spells in IWD2 are all bits below INNATE, so we can do this trick
			if (actor->spellbook.IsIWDSpellBook()) {
				type = (1<<IE_IWD2_SPELL_INNATE)-1;
			} else {
				type = (1<<IE_SPELL_TYPE_INNATE)-1;
			}
			//returns true if there are ANY spells to cast
			if (!actor->spellbook.GetSpellInfoSize(type) || !actor->GetAnyActiveCasterLevel()) {
				state = IE_GUI_BUTTON_DISABLED;
			}
			break;
		case ACT_BARD:
		case ACT_CLERIC:
		case ACT_DRUID:
		case ACT_PALADIN:
		case ACT_RANGER:
		case ACT_SORCERER:
		case ACT_WIZARD:
		case ACT_DOMAIN:
			if (actor->spellbook.IsIWDSpellBook()) {
				type = 1<<(action-ACT_BARD);
			} else {
				//only cleric or wizard switch exists in the bg engine
				if (action==ACT_WIZARD) type = 1<<IE_SPELL_TYPE_WIZARD;
				else type = 1<<IE_SPELL_TYPE_PRIEST;
			}
			//returns true if there is ANY shape
			if (!actor->spellbook.GetSpellInfoSize(type)) {
				state = IE_GUI_BUTTON_DISABLED;
			}
			break;
		case ACT_WILDSHAPE:
		case ACT_SHAPE:
			if (actor->spellbook.IsIWDSpellBook()) {
				type = 1<<IE_IWD2_SPELL_SHAPE;
			} else {
				type = 0; //no separate shapes in old spellbook
			}
			//returns true if there is ANY shape
			if (!actor->spellbook.GetSpellInfoSize(type)) {
				state = IE_GUI_BUTTON_DISABLED;
			}
			break;
		case ACT_USE:
			//returns true if there is ANY equipment
			if (!actor->inventory.GetEquipmentInfo(NULL, 0, 0)) {
				state = IE_GUI_BUTTON_DISABLED;
			}
			break;
		case ACT_BARDSONG:
			if (actor->spellbook.IsIWDSpellBook()) {
				type = 1<<IE_IWD2_SPELL_SONG;
				if (!actor->spellbook.GetSpellInfoSize(type)) {
					state = IE_GUI_BUTTON_DISABLED;
				} else if (modalstate == MS_BATTLESONG) {
					state = IE_GUI_BUTTON_SELECTED;
				}
			} else {
				if (modalstate==MS_BATTLESONG) {
					state = IE_GUI_BUTTON_SELECTED;
				}
			}
			break;
		case ACT_TURN:
			if (actor->GetStat(IE_TURNUNDEADLEVEL)<1) {
				state = IE_GUI_BUTTON_DISABLED;
			} else {
				if (modalstate==MS_TURNUNDEAD) {
					state = IE_GUI_BUTTON_SELECTED;
				}
			}
			break;
		case ACT_STEALTH:
			if (!CanUseActionButton(actor, action)) {
				state = IE_GUI_BUTTON_DISABLED;
			} else {
				if (modalstate==MS_STEALTH) {
					state = IE_GUI_BUTTON_SELECTED;
				}
			}
			break;
		case ACT_SEARCH:
			//in IWD2 everyone can try to search, in bg2 only thieves get the icon
			if (!CanUseActionButton(actor, action)) {
				state = IE_GUI_BUTTON_DISABLED;
			} else {
				if (modalstate == MS_DETECTTRAPS) {
					state = IE_GUI_BUTTON_SELECTED;
				}
			}
			break;
		case ACT_THIEVING:
			if (!CanUseActionButton(actor, action)) {
				state = IE_GUI_BUTTON_DISABLED;
			}
			break;
		case ACT_TAMING:
			if (actor->GetStat(IE_ANIMALS)<=0 ) {
				state = IE_GUI_BUTTON_DISABLED;
			}
			break;
		case ACT_WEAPON1:
		case ACT_WEAPON2:
		case ACT_WEAPON3:
		case ACT_WEAPON4:
		{
			SetButtonBAM(wi, ci, "stonweap",0,0,-1);
			ieDword slot;
			if (magicweapon!=0xffff) {
				slot = magicweapon;
			} else {
					slot = actor->GetQuickSlot(action-ACT_WEAPON1);
			}
			if (slot!=0xffff) {
				CREItem *item = actor->inventory.GetSlotItem(slot);
				//no slot translation required
				int launcherslot = actor->inventory.FindSlotRangedWeapon(slot);
				const char* Item2ResRef = 0;
				if (launcherslot != actor->inventory.GetFistSlot()) {
					// launcher/projectile in this slot
					CREItem* item2;
					item2 = actor->inventory.GetSlotItem(launcherslot);
					Item2ResRef = item2->ItemResRef;
				}

				if (item) {
					int mode = 4;
					if (slot == fistweapon) {
						if (fistdrawn) {
							fistdrawn = false;
						} else {
							//empty weapon slot, already drawn
							break;
						}
					}
					SetItemIcon(wi, ci, item->ItemResRef,mode,(item->Flags&IE_INV_ITEM_IDENTIFIED)?2:1, i+1, Item2ResRef);
					SetItemText(btn, item->Usages[actor->PCStats->QuickWeaponHeaders[action-ACT_WEAPON1]], true);
					if (usedslot == slot) {
						btn->EnableBorder(0, true);
						if (gc->GetTargetMode() == TARGET_MODE_ATTACK) {
							state = IE_GUI_BUTTON_SELECTED;
						} else {
							state = IE_GUI_BUTTON_FAKEDISABLED;
						}
					} else {
						btn->EnableBorder(0, false);
					}
				}
			}
		}
		break;
		case ACT_IWDQSPELL:
			SetButtonBAM(wi, ci, "stonspel",0,0,-1);
			if (actor->version==22 && i>3) {
				tmp = i-3;
			} else {
				tmp = 0;
			}
			goto jump_label2;
		case ACT_IWDQSONG:
			SetButtonBAM(wi, ci, "stonsong",0,0,-1);
			if (actor->version==22 && i>3) {
				tmp = i-3;
			} else {
				tmp = 0;
			}
			goto jump_label2;
		case ACT_IWDQSPEC:
			SetButtonBAM(wi, ci, "stonspec",0,0,-1);
			if (actor->version==22 && i>3) {
				tmp = i-3;
			} else {
				tmp = 0;
			}
			goto jump_label2;
		case ACT_QSPELL1:
		case ACT_QSPELL2:
		case ACT_QSPELL3:
			SetButtonBAM(wi, ci, "stonspel",0,0,-1);
			tmp = action-ACT_QSPELL1;
jump_label2:
		{
			ieResRef *poi = &actor->PCStats->QuickSpells[tmp];
			if ((*poi)[0]) {
				SetSpellIcon(wi, ci, *poi, 1, 1, i+1);
				int mem = actor->spellbook.GetMemorizedSpellsCount(*poi, -1, true);
				if (!mem) {
					state = IE_GUI_BUTTON_FAKEDISABLED;
				}
				SetItemText(btn, mem, true);
			}
		}
		break;
		case ACT_IWDQITEM:
			if (i>3) {
				tmp = (i+1)%3;
				goto jump_label;
			}
			// fall through as a synonym
			// should eventually get replaced with proper +0-9 recognition
			// intentional fallthrough
		case ACT_QSLOT1:
			tmp=0;
			goto jump_label;
		case ACT_QSLOT2:
			tmp=1;
			goto jump_label;
		case ACT_QSLOT3:
			tmp=2;
			goto jump_label;
		case ACT_QSLOT4:
			tmp=3;
			goto jump_label;
		case ACT_QSLOT5:
			tmp=4;
jump_label:
		{
			SetButtonBAM(wi, ci, "stonitem",0,0,-1);
			ieDword slot = actor->PCStats->QuickItemSlots[tmp];
			if (slot!=0xffff) {
				//no slot translation required
				CREItem *item = actor->inventory.GetSlotItem(slot);
				if (item) {
					//MISC3H (horn of blasting) is not displayed when it is out of usages
					int header = actor->PCStats->QuickItemHeaders[tmp];
					int usages = item->Usages[header];
					//I don't like this feature, if the goal is full IE compatibility
					//uncomment the next line.
					//if (usages)
					{
						//SetItemIcon parameter needs header+6 to display extended header icons
						SetItemIcon(wi, ci, item->ItemResRef,header+6,(item->Flags&IE_INV_ITEM_IDENTIFIED)?2:1, i+1, NULL);
						SetItemText(btn, usages, false);
					}
				} else {
					if (action == ACT_IWDQITEM) {
						action = -1; // so it gets marked as disabled below
					}
				}
			} else {
				if (action == ACT_IWDQITEM) {
					action = -1; // so it gets marked as disabled below
				}
			}
		}
			break;
		default:
			break;
		}
		if (!ret) {
			return RuntimeError("Cannot set action button!\n");
		}
		ieDword disabledbutton = actor->GetStat(IE_DISABLEDBUTTON);
		if (action<0 || (action <= ACT_SKILLS && (disabledbutton & (1<<action) ))) {
			state = IE_GUI_BUTTON_DISABLED;
		} else if (action >= ACT_QSPELL1 && action <= ACT_QSPELL3 && (disabledbutton & (1<<ACT_CAST))) {
			state = IE_GUI_BUTTON_DISABLED;
		}
		btn->SetState(state);
		//you have to set this overlay up
		// this state check looks bizzare, but without it most buttons get misrendered
		btn->EnableBorder(1, state==IE_GUI_BUTTON_DISABLED);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_ClearActions__doc,
"===== ClearActions =====\n\
\n\
**Prototype:** GemRB.ClearActions (globalID)\n\
\n\
**Description:** Stops an actor's movement and any pending action.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_ClearActions(PyObject * /*self*/, PyObject* args)
{
	int globalID;

	if (!PyArg_ParseTuple( args, "i", &globalID)) {
		return AttributeError( GemRB_ClearActions__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (actor->GetInternalFlag()&IF_NOINT) {
		Log(MESSAGE, "GuiScript","Cannot break action!");
		Py_INCREF( Py_None );
		return Py_None;
	}
	if (!(actor->GetStep()) && !actor->Modal.State && !actor->LastTarget && actor->LastTargetPos.isempty() && !actor->LastSpellTarget) {
		Log(MESSAGE, "GuiScript","No breakable action!");
		Py_INCREF( Py_None );
		return Py_None;
	}
	actor->Stop(); //stop pending action involved walking
	actor->SetModal(MS_NONE);//stop modal actions
	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_SetDefaultActions__doc,
"===== SetDefaultActions =====\n\
\n\
**Prototype:** GemRB.SetDefaultActions (qslot, action1, action2, action3)\n\
\n\
**Description:** Sets whether quick slots need an additional translation \n\
like in iwd2. Also sets up the first three default action types.\n\
\n\
**Parameters:**\n\
  * qslot     - bool\n\
  * action1-3 - button codes (slots)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Window_SetupControls]]\n\
"
);

static PyObject* GemRB_SetDefaultActions(PyObject * /*self*/, PyObject* args)
{
	int qslot;
	int slot1, slot2, slot3;

	if (!PyArg_ParseTuple( args, "iiii", &qslot, &slot1, &slot2, &slot3 )) {
		return AttributeError( GemRB_SetDefaultActions__doc );
	}
	Actor::SetDefaultActions((bool) qslot, (ieByte) slot1, (ieByte) slot2, (ieByte) slot3);
	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_SetupQuickSpell__doc,
"===== SetupQuickSpell =====\n\
\n\
**Prototype:** GemRB.SetupQuickSpell (globalID, spellslot, spellindex, type)\n\
\n\
**Description:** Set up a quick spell slot of a PC. It also returns the \n\
target type of the selected spell.\n\
\n\
**Parameters:**\n\
  * globalID - global ID of the actor to use\n\
  * spellslot - quickspell slot to use\n\
  * spellindex - spell to assign\n\
  * type - spell(book) type (255 means any)\n\
\n\
**Return value:** integer, target type constant\n\
"
);

static PyObject* GemRB_SetupQuickSpell(PyObject * /*self*/, PyObject* args)
{
	SpellExtHeader spelldata;
	int globalID, which, slot, type;

	if (!PyArg_ParseTuple( args, "iiii", &globalID, &slot, &which, &type)) {
		return AttributeError( GemRB_SetupQuickSpell__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (!actor->PCStats) {
		//no quick slots for this actor, is this an error?
		//return RuntimeError( "Actor has no quickslots!\n" );
		Py_INCREF( Py_None );
		return Py_None;
	}

	actor->spellbook.GetSpellInfo(&spelldata, type, which, 1);
	if (!spelldata.spellname[0]) {
		return RuntimeError( "Invalid parameter! Spell not found!\n" );
	}

	memcpy(actor->PCStats->QuickSpells[slot], spelldata.spellname, sizeof(ieResRef) );
	actor->PCStats->QuickSpellClass[slot] = type;

	return PyInt_FromLong( spelldata.Target );
}

PyDoc_STRVAR( GemRB_SetupQuickSlot__doc,
"===== SetupQuickSlot =====\n\
\n\
**Prototype:** GemRB.SetupQuickSlot (PartyID, QuickSlotID, InventorySlot[, AbilityIndex])\n\
\n\
**Description:** Sets up a quickslot or weapon slot to point to a particular \n\
inventory slot. Also sets the used ability for that given quickslot. \n\
If the abilityindex is omitted, it will be assumed as 0. \n\
If the InventorySlot is -1, then it won't be assigned to the quickslot \n\
(this way you can alter the used Ability index only). \n\
If the QuickSlotID is 0, then it will try to find the quickslot/weaponslot \n\
by the InventorySlot, and assign the AbilityIndex to it. \n\
(Use this if you don't know the exact quick slot, or don't care to find it).\n\
\n\
**Parameters:**\n\
  * PartyID       - the PC's position in the party (1 based)\n\
  * QuickSlotID   - the quickslot to set up\n\
  * InventorySlot - the inventory slot assigned to this quickslot, this is\n\
usually constant and taken care by the core\n\
  * AbilityIndex  - the number of the item extended header to use with this quickslot\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetEquippedQuickSlot]], [[guiscript:SetEquippedQuickSlot]]\n\
"
);

static PyObject* GemRB_SetupQuickSlot(PyObject * /*self*/, PyObject* args)
{
	int globalID, which, slot, headerindex = 0;

	if (!PyArg_ParseTuple( args, "iii|i", &globalID, &which, &slot, &headerindex)) {
		return AttributeError( GemRB_SetupQuickSlot__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	slot = core->QuerySlot(slot);
	actor->SetupQuickSlot(which, slot, headerindex);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetEquippedQuickSlot__doc,
"===== SetEquippedQuickSlot =====\n\
\n\
**Prototype:** GemRB.SetEquippedQuickSlot (PartyID, QWeaponSlot[, ability])\n\
\n\
**Description:** Sets the specified weapon slot as equipped weapon slot. \n\
Optionally sets the used ability.\n\
\n\
**Parameters:**\n\
  * PartyID     - the PC's position in the party (1 based)\n\
  * QWeaponSlot - the quickslot to equip\n\
  * ability     - optional integer, sets the used extended header\n\
\n\
**Return value:** 0 success, -1 silent failure\n\
\n\
**See also:** [[guiscript:GetEquippedQuickSlot]], [[guiscript:SetupQuickSlot]]\n\
"
);

static PyObject* GemRB_SetEquippedQuickSlot(PyObject * /*self*/, PyObject* args)
{
	int ret;
	int slot;
	int dummy;
	int globalID;
	int ability = -1;

	if (!PyArg_ParseTuple( args, "ii|i", &globalID, &slot, &ability)) {
		return AttributeError( GemRB_SetEquippedQuickSlot__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	CREItem *item = actor->inventory.GetUsedWeapon(false, dummy);
	if (item && (item->Flags & IE_INV_ITEM_CURSED)) {
		displaymsg->DisplayConstantString(STR_CURSED, DMC_WHITE);
	} else {
		ret = actor->SetEquippedQuickSlot(slot, ability);
		if (ret) {
			displaymsg->DisplayConstantString(ret, DMC_WHITE);
		}
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetEquippedQuickSlot__doc,
"===== GetEquippedQuickSlot =====\n\
\n\
**Prototype:** GemRB.GetEquippedQuickSlot (PartyID[, NoTrans])\n\
\n\
**Description:** Returns the quickweapon slot index or the inventory slot.\n\
\n\
**Parameters:**\n\
  * PartyID - the PC's position in the party (1 based)\n\
  * NoTrans - which equipped slot to return?\n\
    * 0 - return the inventory slot\n\
    * 1 - return the quickweapon slot index\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [[guiscript:SetEquippedQuickSlot]], [[guiscript:GetEquippedAmmunition]]\n\
"
);

static PyObject* GemRB_GetEquippedQuickSlot(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	int NoTrans = 0;

	if (!PyArg_ParseTuple( args, "i|i", &globalID, &NoTrans)) {
		return AttributeError( GemRB_GetEquippedQuickSlot__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int ret = actor->inventory.GetEquippedSlot();
	if (actor->PCStats) {
		for(int i=0;i<4;i++) {
			if (ret == actor->PCStats->QuickWeaponSlots[i]) {
				if (NoTrans) {
					return PyInt_FromLong(i);
				}
				ret = i+actor->inventory.GetWeaponSlot();
				break;
			}
		}
	}
	return PyInt_FromLong( core->FindSlot(ret) );
}

PyDoc_STRVAR( GemRB_GetEquippedAmmunition__doc,
"===== GetEquippedAmmunition =====\n\
\n\
**Prototype:** GemRB.GetEquippedAmmunition (globalID)\n\
\n\
**Description:** Returns the equipped ammunition slot, if any\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** If ammunition is equipped, the inventory slot, otherwise -1.\n\
\n\
**See also:** [[guiscript:GetEquippedQuickSlot]]\n\
"
);

static PyObject* GemRB_GetEquippedAmmunition(PyObject * /*self*/, PyObject* args)
{
	int globalID;

	if (!PyArg_ParseTuple( args, "i", &globalID)) {
		return AttributeError( GemRB_GetEquippedQuickSlot__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int ret = actor->inventory.GetEquippedSlot();
	int effect = core->QuerySlotEffects(ret);
	if (effect == SLOT_EFFECT_MISSILE) {
		return PyInt_FromLong( core->FindSlot(ret) );
	} else {
		return PyInt_FromLong( -1 );
	}
}

PyDoc_STRVAR( GemRB_SetModalState__doc,
"===== SetModalState =====\n\
\n\
**Prototype:** GemRB.SetModalState (globalID, Value[, spell])\n\
\n\
**Description:** Sets an actor's modal state. The modal states are listed \n\
in ie_modal.py. If 'spell' is not given, it will set a default spell \n\
resource associated with the state.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * Value - new modal state\n\
  * Spell - the spell resource associated with the state\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:** \n\
  GemRB.SetModalState (pc, MS_TURNUNDEAD)\n\
The above example makes the player start the turn undead action.\n\
\n\
**See also:** [[guiscript:SetPlayerStat]], [[guiscript:SetPlayerName]]\n\
"
);

static PyObject* GemRB_SetModalState(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	int state;
	const char *spell=NULL;

	if (!PyArg_ParseTuple( args, "ii|s", &globalID, &state, &spell )) {
		return AttributeError( GemRB_SetModalState__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetModal( (ieDword) state, 0);
	actor->SetModalSpell(state, spell);
	if (actor->ModalSpellSkillCheck()) {
		actor->ApplyModal(actor->Modal.Spell); // force immediate effect
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_PrepareSpontaneousCast__doc,
"===== PrepareSpontaneousCast =====\n\
\n\
**Prototype:** GemRB.PrepareSpontaneousCast (globalID, spellIndex, type, level, spellResRef)\n\
\n\
**Description:** Depletes the memorised spell and replaces it with another \n\
(in memory). WARNING: useful only immediately before casting.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * spellIndex - current spell's index\n\
  * type - spell's booktype\n\
  * level - spell's level\n\
  * spellResRef - replacement spell's resource reference\n\
\n\
**Return value:** new spell's spellinfo index"
);

static PyObject* GemRB_PrepareSpontaneousCast(PyObject * /*self*/, PyObject* args)
{
	int globalID, type, level;
	const char *spell = NULL;
	const char *spell2 = NULL;
	ieResRef replacementSpell;

	if (!PyArg_ParseTuple( args, "isiis", &globalID, &spell, &type, &level, &spell2)) {
		return AttributeError( GemRB_PrepareSpontaneousCast__doc );
	}
	strnlwrcpy(replacementSpell, spell2, 8);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	// deplete original memorisation
	actor->spellbook.UnmemorizeSpell(spell, true);
	// set spellinfo to all known spells of desired type
	actor->spellbook.SetCustomSpellInfo(NULL, NULL, 1<<type);
	SpellExtHeader spelldata;
	int idx = actor->spellbook.FindSpellInfo(&spelldata, replacementSpell, 1<<type);

	return PyInt_FromLong(idx-1);
}

PyDoc_STRVAR( GemRB_SpellCast__doc,
"===== SpellCast =====\n\
\n\
**Prototype:** GemRB.SpellCast (PartyID, Type, Spell)\n\
\n\
**Description:** Makes PartyID cast a spell of Type. This handles targeting \n\
and executes the appropriate scripting command. If type is -1, then the \n\
castable spell list will be deleted and no spell will be cast.\n\
\n\
**Parameters:**\n\
  * PartyID - player character's index in the party\n\
  * Type    - spell type bitfield (1-mage, 2-priest, 4-innate)\n\
  * Spell   - spell's index in the memorised list\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:UseItem]]\n\
"
);

static PyObject* GemRB_SpellCast(PyObject * /*self*/, PyObject* args)
{
	unsigned int globalID;
	int type;
	unsigned int spell;

	if (!PyArg_ParseTuple( args, "iii", &globalID, &type, &spell)) {
		return AttributeError( GemRB_SpellCast__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	//don't cast anything, just reinit the spell list
	if (type==-1) {
		actor->spellbook.ClearSpellInfo();
		Py_INCREF( Py_None );
		return Py_None;
	}

	SpellExtHeader spelldata; // = SpellArray[spell];

	if (type==-2) {
		//resolve quick spell slot
		if (!actor->PCStats) {
			//no quick slots for this actor, is this an error?
			//return RuntimeError( "Actor has no quickslots!\n" );
			Py_INCREF( Py_None );
			return Py_None;
		}
		actor->spellbook.FindSpellInfo(&spelldata, actor->PCStats->QuickSpells[spell], actor->PCStats->QuickSpellClass[spell]);
	} else {
		ieDword ActionLevel = 0;
		core->GetDictionary()->Lookup("ActionLevel", ActionLevel);
		if (ActionLevel == 5) {
			// get the right spell, since the lookup below only checks the memorized list
			actor->spellbook.SetCustomSpellInfo(NULL, NULL, type);
		}
		actor->spellbook.GetSpellInfo(&spelldata, type, spell, 1);
	}

	print("Cast spell: %s", spelldata.spellname);
	print("Slot: %d", spelldata.slot);
	print("Type: %d (%d vs %d)", spelldata.type, 1<<spelldata.type, type);
	//cannot make this const, because it will be freed
	char *tmp = core->GetCString(spelldata.strref);
	print("Spellname: %s", tmp);
	core->FreeString(tmp);
	print("Target: %d", spelldata.Target);
	print("Range: %d", spelldata.Range);
	if (type > 0 && !((1<<spelldata.type) & type)) {
		return RuntimeError( "Wrong type of spell!");
	}

	GET_GAMECONTROL();

	switch (spelldata.Target) {
		case TARGET_SELF:
			// FIXME: GA_NO_DEAD and such are not actually used by SetupCasting
			gc->SetupCasting(spelldata.spellname, spelldata.type, spelldata.level, spelldata.slot, actor, GA_NO_DEAD, spelldata.TargetNumber);
			gc->TryToCast(actor, actor);
			break;
		case TARGET_NONE:
			//reset the cursor
			gc->ResetTargetMode();
			//this is always instant casting without spending the spell
			core->ApplySpell(spelldata.spellname, actor, actor, 0);
			break;
		case TARGET_AREA:
			gc->SetupCasting(spelldata.spellname, spelldata.type, spelldata.level, spelldata.slot, actor, GA_POINT, spelldata.TargetNumber);
			break;
		case TARGET_CREA:
			gc->SetupCasting(spelldata.spellname, spelldata.type, spelldata.level, spelldata.slot, actor, GA_NO_DEAD, spelldata.TargetNumber);
			break;
		case TARGET_DEAD:
			gc->SetupCasting(spelldata.spellname, spelldata.type, spelldata.level, spelldata.slot, actor, 0, spelldata.TargetNumber);
			break;
		case TARGET_INV:
			//bring up inventory in the end???
			//break;
		default:
			print("Unhandled target type: %d", spelldata.Target);
			break;
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_ApplySpell__doc,
"===== ApplySpell =====\n\
\n\
**Prototype:** GemRB.ApplySpell (globalID, resref[, casterID])\n\
\n\
**Description:** Applies a spell on the actor. \n\
This function can be used to add abilities that are stored as spells \n\
(eg. innates).\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * resref   - spell resource reference\n\
  * casterID - global id of the desired caster\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:SpellCast]], [[guiscript:ApplyEffect]], [[guiscript:CountEffects]]\n\
"
);

static PyObject* GemRB_ApplySpell(PyObject * /*self*/, PyObject* args)
{
	int globalID, casterID = 0;
	const char *spell;

	if (!PyArg_ParseTuple( args, "is|i", &globalID, &spell, &casterID )) {
		return AttributeError( GemRB_ApplySpell__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	Actor *caster = NULL;
	Map *map = game->GetCurrentArea();
	if (map) caster = map->GetActorByGlobalID(casterID);
	if (!caster) caster = game->GetActorByGlobalID(casterID);
	if (!caster) caster = actor;

	core->ApplySpell(spell, actor, caster, 0);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_UseItem__doc,
"===== UseItem =====\n\
\n\
**Prototype:** GemRB.UseItem (globalID, Slot, header[,forcetarget])\n\
\n\
**Description:** Makes the actor try to use an item. \n\
If slot is non-negative, then header is the header of the item in the 'slot'. \n\
If slot is -1, then header is the index of the item functionality in the use item list. \n\
If slot is -2, then header is the quickslot index. \n\
This handles targeting and executes the appropriate scripting command.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * Slot     - item's inventory slot\n\
  * header   - item index from the SetupEquipmentIcons list, an item may have multiple entries, because of multiple features\n\
  * forcetarget - overrides Target number if set\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:CanUseItemType]], [[guiscript:SpellCast]], [[guiscript:Window_SetupEquipmentIcons]]\n\
"
);

static PyObject* GemRB_UseItem(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	int slot;
	int header;
	int forcetarget=-1; //some crappy scrolls don't target self correctly!

	if (!PyArg_ParseTuple( args, "iii|i", &globalID, &slot, &header, &forcetarget)) {
		return AttributeError( GemRB_UseItem__doc );
	}

	GET_GAME();

	GET_GAMECONTROL();
	GET_ACTOR_GLOBAL();

	ItemExtHeader itemdata;
	int flags = 0;

	switch (slot) {
		case -1:
			//some equipment
			actor->inventory.GetEquipmentInfo(&itemdata, header, 1);
			break;
		case -2:
			//quickslot
			actor->GetItemSlotInfo(&itemdata, header, -1);
			if (!itemdata.Charges) {
				Log(MESSAGE, "GUIScript", "QuickItem has no charges.");
				Py_INCREF( Py_None );
				return Py_None;
			}
			break;
		default:
			//any normal slot
			actor->GetItemSlotInfo(&itemdata, core->QuerySlot(slot), header);
			flags = UI_SILENT;
			break;
	}

	if(forcetarget==-1) {
		forcetarget = itemdata.Target;
	}

	//is there any better check for a non existent item?
	if (!itemdata.itemname[0]) {
		Log(WARNING, "GUIScript", "Empty slot used?");
		Py_INCREF( Py_None );
		return Py_None;
	}

	/// remove this after projectile is done
	print("Use item: %s", itemdata.itemname);
	print("Extended header: %d", itemdata.headerindex);
	print("Attacktype: %d", itemdata.AttackType);
	print("Range: %d", itemdata.Range);
	print("Target: %d", forcetarget);
	print("Projectile: %d", itemdata.ProjectileAnimation);
	int count = 1;
	switch (forcetarget) {
		case TARGET_SELF:
			if (core->HasFeature(GF_TEAM_MOVEMENT)) count += 1000; // pst inventory workaround to avoid another parameter
			gc->SetupItemUse(itemdata.slot, itemdata.headerindex, actor, GA_NO_DEAD, count);
			gc->TryToCast(actor, actor);
			break;
		case TARGET_NONE:
			gc->ResetTargetMode();
			actor->UseItem(itemdata.slot, itemdata.headerindex, NULL, flags);
			break;
		case TARGET_AREA:
			gc->SetupItemUse(itemdata.slot, itemdata.headerindex, actor, GA_POINT, itemdata.TargetNumber);
			break;
		case TARGET_CREA:
			gc->SetupItemUse(itemdata.slot, itemdata.headerindex, actor, GA_NO_DEAD, itemdata.TargetNumber);
			break;
		case TARGET_DEAD:
			gc->SetupItemUse(itemdata.slot, itemdata.headerindex, actor, 0, itemdata.TargetNumber);
			break;
		default:
			Log(ERROR, "GUIScript", "Unhandled target type!");
			break;
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetGamma__doc,
"===== SetGamma =====\n\
\n\
**Prototype:** GemRB.SetGamma (brightness, contrast)\n\
\n\
**Description:** Adjusts brightness and contrast.\n\
\n\
**Parameters:**\n\
  * brightness - value must be 0 ... 40\n\
  * contrast   - value must be 0 ... 5\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:SetFullScreen]]\n\
"
);

static PyObject* GemRB_SetGamma(PyObject * /*self*/, PyObject* args)
{
	int brightness, contrast;

	if (!PyArg_ParseTuple( args, "ii", &brightness, &contrast )) {
		return AttributeError( GemRB_SetGamma__doc );
	}
	if (brightness<0 || brightness>40) {
		return RuntimeError( "Brightness must be 0-40" );
	}
	if (contrast<0 || contrast>5) {
		return RuntimeError( "Contrast must be 0-5" );
	}
	core->GetVideoDriver()->SetGamma(brightness, contrast);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetMouseScrollSpeed__doc,
"===== SetMouseScrollSpeed =====\n\
\n\
**Prototype:** GemRB.SetMouseScrollSpeed (speed)\n\
\n\
**Description:** Adjusts the mouse scroll speed.\n\
\n\
**Parameters:**\n\
  * speed - defaults between 30 and 50\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:SetTooltipDelay]]"
);

static PyObject* GemRB_SetMouseScrollSpeed(PyObject * /*self*/, PyObject* args)
{
	int mouseSpeed;

	if (!PyArg_ParseTuple( args, "i", &mouseSpeed)) {
		return AttributeError( GemRB_SetMouseScrollSpeed__doc );
	}

	core->SetMouseScrollSpeed(mouseSpeed);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetTooltipDelay__doc,
"===== SetTooltipDelay =====\n\
\n\
**Prototype:** GemRB.SetTooltipDelay (time)\n\
\n\
**Description:** Sets the tooltip delay.\n\
\n\
**Parameters:**\n\
  * time - 0-10\n\
\n\
**See also:** [[guiscript:Control_SetTooltip]], [[guiscript:SetMouseScrollSpeed]]\n\
"
);

static PyObject* GemRB_SetTooltipDelay(PyObject * /*self*/, PyObject* args)
{
	int tooltipDelay;

	if (!PyArg_ParseTuple( args, "i", &tooltipDelay)) {
		return AttributeError( GemRB_SetTooltipDelay__doc );
	}

	core->TooltipDelay = tooltipDelay;

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetFullScreen__doc,
"===== SetFullScreen =====\n\
\n\
**Prototype:** GemRB.SetFullScreen (flag)\n\
\n\
**Description:** Adjusts fullscreen mode.\n\
\n\
**Parameters:**\n\
  * flag:\n\
    * -1 -  toggle fullscreen mode\n\
    * 0 - set windowed mode\n\
    * 1 - set fullscreen mode\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:SetGamma]]\n\
"
);

static PyObject* GemRB_SetFullScreen(PyObject * /*self*/, PyObject* args)
{
	int fullscreen;

	if (!PyArg_ParseTuple( args, "i", &fullscreen )) {
		return AttributeError( GemRB_SetFullScreen__doc );
	}
	core->GetVideoDriver()->SetFullscreenMode(fullscreen);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_RunRestScripts__doc,
"===== RunRestScripts =====\n\
\n\
**Prototype:** GemRB.RunRestScripts ()\n\
\n\
**Description:** Executes the party pre-rest scripts if any.\n\
\n\
**Return value:** bool, true if a dream script ran Rest or RestParty.\n\
"
);

static PyObject* GemRB_RunRestScripts(PyObject * /*self*/, PyObject* /*args*/)
{
	int dreamed = 0;
	GET_GAME();

	// check if anyone wants to banter first (bg2)
	static int dreamer = -2;
	if (dreamer == -2) {
		AutoTable pdtable("pdialog");
		dreamer = pdtable->GetColumnIndex("DREAM_SCRIPT_FILE");
	}
	if (dreamer >= 0) {
		AutoTable pdtable("pdialog");
		int ii = game->GetPartySize(true); // party size, only alive
		bool bg2expansion = core->GetGame()->Expansion == 5;
		while (ii--) {
			Actor *tar = game->GetPC(ii, true);
			const char* scriptname = tar->GetScriptName();
			if (pdtable->GetRowIndex(scriptname) != -1) {
				ieResRef resref;
				if (bg2expansion) {
					strnlwrcpy(resref, pdtable->QueryField(scriptname, "25DREAM_SCRIPT_FILE"), sizeof(ieResRef)-1);
				} else {
					strnlwrcpy(resref, pdtable->QueryField(scriptname, "DREAM_SCRIPT_FILE"), sizeof(ieResRef)-1);
				}
				GameScript* restscript = new GameScript(resref, tar, 0, 0);
				if (restscript->Update()) {
					// there could be several steps involved, so we can't reliably check tar->GetLastRested()
					dreamed = 1;
				}
				delete restscript;
			}
		}
	}

	return PyInt_FromLong(dreamed);
}

PyDoc_STRVAR( GemRB_RestParty__doc,
"===== RestParty =====\n\
\n\
**Prototype:** GemRB.RestParty (flags, movie, hp)\n\
\n\
**Description:** Makes the party rest. It is possible to check various \n\
things that may forbid resting (hostile creatures, area flags, party \n\
scattered). It is possible to play a movie or dream too.\n\
\n\
**Parameters:**\n\
  * flags - which checks to run?\n\
  * movie - a number, see restmov.2da\n\
  * hp    - hit points healed, 0 means full healing\n\
\n\
**Return value:** dict\n\
  * Error: True if resting was prevented by one of the checks\n\
  * ErrorMsg: a strref with a reason for the error\n\
  * Cutscene: True if a cutscene needs to run\n\
\n\
**See also:** StartStore(gamescript)"
);

static PyObject* GemRB_RestParty(PyObject * /*self*/, PyObject* args)
{
	int noareacheck;
	int dream, hp;

	if (!PyArg_ParseTuple( args, "iii", &noareacheck, &dream, &hp)) {
		return AttributeError( GemRB_RestParty__doc );
	}
	GET_GAME();

	// check if resting is possible and if not, return the reason, otherwise rest
	// Error feedback is eventually handled this way:
	// - resting outside: popup an error window with the reason in pst, print it to message window elsewhere
	// - resting in inns: popup a GUISTORE error window with the reason
	PyObject* dict = PyDict_New();
	int cannotRest = game->CanPartyRest(noareacheck);
	// fall back to the generic: you may not rest at this time
	if (cannotRest == -1) {
		if (core->HasFeature(GF_AREA_OVERRIDE)) {
			cannotRest = displaymsg->GetStringReference(STR_MAYNOTREST);
		} else {
			cannotRest = 10309;
		}
	}
	PyDict_SetItemString(dict, "Error", PyBool_FromLong(cannotRest != 0));
	if (cannotRest) {
		PyDict_SetItemString(dict, "ErrorMsg", PyInt_FromLong(cannotRest));
		PyDict_SetItemString(dict, "Cutscene", PyBool_FromLong(0));
	} else {
		PyDict_SetItemString(dict, "ErrorMsg", PyInt_FromLong(-1));
		// all is well, so do the actual resting
		PyDict_SetItemString(dict, "Cutscene", PyBool_FromLong(game->RestParty(0, dream, hp)));
	}

	return dict;
}

PyDoc_STRVAR( GemRB_ChargeSpells__doc,
"===== ChargeSpells =====\n\
\n\
**Prototype:** GemRB.ChargeSpells (globalID)\n\
\n\
**Description:** Recharges the actor's spells.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** N/A\n\
"
);
static PyObject* GemRB_ChargeSpells(PyObject * /*self*/, PyObject* args)
{
	int globalID;

	if (!PyArg_ParseTuple( args, "i", &globalID)) {
		return AttributeError( GemRB_ChargeSpells__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->spellbook.ChargeAllSpells();

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_HasSpecialItem__doc,
"===== HasSpecialItem =====\n\
\n\
**Prototype:** GemRB.HasSpecialItem (globalID, itemtype, useup)\n\
\n\
**Description:** Checks if the actor has an item, optionally depletes it.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * itemtype - see itemspec.2da (usually 1)\n\
  * useup - destroy/remove a charge after use\n\
\n\
**Return value:** bool"
);

//itemtype 1 - identify
static PyObject* GemRB_HasSpecialItem(PyObject * /*self*/, PyObject* args)
{
	int globalID, itemtype, useup;

	if (!PyArg_ParseTuple( args, "iii", &globalID, &itemtype, &useup)) {
		return AttributeError( GemRB_HasSpecialItem__doc );
	}
	if (SpecialItemsCount==-1) {
		ReadSpecialItems();
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int i = SpecialItemsCount;
	int slot = -1;
	while(i--) {
		if (itemtype&SpecialItems[i].value) {
			slot = actor->inventory.FindItem(SpecialItems[i].resref,0);
			if (slot>=0) {
				break;
			}
		}
	}

	if (slot<0) {
		return PyInt_FromLong( 0 );
	}

	if (useup) {
		//use the found item's first usage
		useup = actor->UseItem((ieDword) slot, 0, actor, UI_SILENT|UI_FAKE|UI_NOAURA);
	} else {
		CREItem *si = actor->inventory.GetSlotItem( slot );
		if (si->Usages[0]) useup = 1;
	}
	return PyInt_FromLong( useup );
}

PyDoc_STRVAR( GemRB_HasSpecialSpell__doc,
"===== HasSpecialSpell =====\n\
\n\
**Prototype:** GemRB.HasSpecialSpell (globalID, itemtype, useup)\n\
\n\
**Description:** Checks if the actor has a spell, optionally depletes it.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * specialtype - see splspec.2da\n\
  * useup - destroy/remove a charge after use\n\
\n\
**Return value:** bool"
);

//specialtype 1 - identify
//            2 - can use in silence
//            4 - cannot use in wildsurge
static PyObject* GemRB_HasSpecialSpell(PyObject * /*self*/, PyObject* args)
{
	int globalID, specialtype, useup;

	if (!PyArg_ParseTuple( args, "iii", &globalID, &specialtype, &useup)) {
		return AttributeError( GemRB_HasSpecialSpell__doc );
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int i = core->GetSpecialSpellsCount();
	if (i == -1) {
		return RuntimeError( "Game has no splspec.2da table!" );
	}
	SpecialSpellType *special_spells = core->GetSpecialSpells();
	while(i--) {
		if (specialtype & special_spells[i].flags) {
			if (actor->spellbook.HaveSpell(special_spells[i].resref,useup)) {
				if (useup) {
					//actor->SpellCast(SpecialSpells[i].resref, actor);
				}
				break;
			}
		}
	}

	if (i<0) {
		return PyInt_FromLong( 0 );
	}
	return PyInt_FromLong( 1 );
}

PyDoc_STRVAR( GemRB_ApplyEffect__doc,
"===== ApplyEffect =====\n\
\n\
**Prototype:** GemRB.ApplyEffect (globalID, opcode, param1, param2[, resref, resref2, resref3, source, timing])\n\
\n\
**Description:** Creates a basic effect and applies it on the actor marked \n\
by PartyID. \n\
This function cam be used to add stats that are stored in effect blocks.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * opcode   - the effect opcode (for values see effects.ids)\n\
  * param1   - parameter 1 for the opcode\n\
  * param2   - parameter 2 for the opcode\n\
  * resref   - optional resource reference to set in effect\n\
  * resref2  - (optional) resource reference to set in the effect\n\
  * resref3  - (optional) resource reference to set in the effect\n\
  * resref4  - (optional) resource reference to set in the effect\n\
  * source   - (optional) source to set in the effect\n\
  * timing   - (optional) timing mode to set in the effect\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    for i in range(ProfCount-8):\n\
        StatID = GemRB.GetTableValue (TmpTable, i+8, 0)\n\
        Value = GemRB.GetVar ('Prof '+str(i))\n\
        if Value:\n\
            GemRB.ApplyEffect (MyChar, 'Proficiency', Value, StatID)\n\
\n\
The above example sets the weapon proficiencies in a bg2's CharGen9.py script.\n\
\n\
**See also:** [[guiscript:SpellCast]], [[guiscript:SetPlayerStat]], [[guiscript:GetPlayerStat]], [[guiscript:CountEffects]]"
);

static PyObject* GemRB_ApplyEffect(PyObject * /*self*/, PyObject* args)
{
	int timing = FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES;
	int globalID;
	const char *opcodename;
	int param1, param2;
	const char *resref1 = NULL;
	const char *resref2 = NULL;
	const char *resref3 = NULL;
	const char *source = NULL;

	if (!PyArg_ParseTuple( args, "isii|ssssi", &globalID, &opcodename, &param1, &param2, &resref1, &resref2, &resref3, &source, &timing)) {
		return AttributeError( GemRB_ApplyEffect__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	work_ref.Name=opcodename;
	work_ref.opcode=-1;
	Effect *fx = EffectQueue::CreateEffect(work_ref, param1, param2, timing);
	if (!fx) {
		//invalid effect name didn't resolve to opcode
		return RuntimeError( "Invalid effect name!\n" );
	}
	if (resref1) {
		strnlwrcpy(fx->Resource, resref1, 8);
	}
	if (resref2) {
		strnlwrcpy(fx->Resource2, resref2, 8);
	}
	if (resref3) {
		strnlwrcpy(fx->Resource3, resref3, 8);
	}
	if (source) {
		strnlwrcpy(fx->Source, source, 8);
	}
	//This is a hack...
	fx->Parameter3=1;

	//fx is not freed by this function
	core->ApplyEffect(fx, actor, actor);

	//lets kill it
	delete fx;

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_CountEffects__doc,
"===== CountEffects =====\n\
\n\
**Prototype:** GemRB.CountEffects (globalID, opcode, param1, param2[, resref])\n\
\n\
**Description:** Counts how many matching effects are applied on the actor. \n\
If a parameter is set to -1, it will be ignored.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * opcode   - the effect opcode (for values see effects.ids)\n\
  * param1   - parameter 1 for the opcode\n\
  * param2   - parameter 2 for the opcode\n\
  * resref   - optional resource reference to match the effect\n\
\n\
**Return value:** N/A\n\
\n\
**Example:**\n\
    res = GemRB.CountEffect (MyChar, 'HLA', -1, -1, AbilityName)\n\
\n\
The above example returns how many HLA effects were applied on the character.\n\
\n\
**See also:** [[guiscript:ApplyEffect]]"
);

static PyObject* GemRB_CountEffects(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	const char *opcodename;
	int param1, param2;
	const char *resref = NULL;

	if (!PyArg_ParseTuple( args, "isii|s", &globalID, &opcodename, &param1, &param2, &resref)) {
		return AttributeError( GemRB_CountEffects__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	work_ref.Name=opcodename;
	work_ref.opcode=-1;
	ieDword ret = actor->fxqueue.CountEffects(work_ref, param1, param2, resref);
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_ModifyEffect__doc,
"===== ModifyEffect =====\n\
\n\
**Prototype:** GemRB.ModifyEffects (PartyID, opcode, x, y)\n\
\n\
**Description:** Changes/sets the target coordinates of the specified effect. \n\
This command is used for the farsight spell.\n\
\n\
**Parameters:**\n\
  * PartyID - the player character's index in the party\n\
  * opcode  - the effect opcode (for values see effects.ids)\n\
  * x       - target x coordinate\n\
  * y       - target y coordinate\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:ApplyEffect]], [[guiscript:CountEffects]]\n\
"
);

static PyObject* GemRB_ModifyEffect(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	const char *opcodename;
	int px, py;

	if (!PyArg_ParseTuple( args, "isii", &globalID, &opcodename, &px, &py)) {
		return AttributeError( GemRB_ModifyEffect__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	work_ref.Name=opcodename;
	work_ref.opcode=-1;
	actor->fxqueue.ModifyEffectPoint(work_ref, px, py);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_StealFailed__doc,
"===== StealFailed =====\n\
\n\
**Prototype:** GemRB.StealFailed ()\n\
\n\
**Description:** Sends the steal failed trigger (attacked) to the owner \n\
of the current store — the Sender of the StartStore action.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:GetStore]], [[guiscript:EnterStore]], [[guiscript:LeaveStore]], [[guiscript:GameGetSelectedPCSingle]]\n\
"
);

static PyObject* GemRB_StealFailed(PyObject * /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError( "No store loaded!" );
	}
	GET_MAP();

	Actor* owner = map->GetActorByGlobalID( store->GetOwnerID() );
	if (!owner) owner = game->GetActorByGlobalID( store->GetOwnerID() );
	if (!owner) {
		Log(WARNING, "GUIScript", "No owner found!");
		Py_INCREF( Py_None );
		return Py_None;
	}
	Actor* attacker = game->FindPC((int) game->GetSelectedPCSingle() );
	if (!attacker) {
		Log(WARNING, "GUIScript", "No thief found!");
		Py_INCREF( Py_None );
		return Py_None;
	}

	// apply the reputation penalty
	int repmod = core->GetReputationMod(2);
	if (repmod) {
		game->SetReputation(game->Reputation + repmod);
	}

	//not sure if this is ok
	//owner->LastDisarmFailed = attacker->GetGlobalID();
	if (core->HasFeature(GF_STEAL_IS_ATTACK)) {
		owner->AttackedBy(attacker);
	}
	owner->AddTrigger(TriggerEntry(trigger_stealfailed, attacker->GetGlobalID()));
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SwapPCs__doc,
"===== SwapPCs =====\n\
\n\
**Prototype:** GemRB.SwapPCs (idx1, idx2)\n\
\n\
**Description:** Swaps the party order for two player characters.\n\
\n\
**Parameters:** \n\
  * idx1 - position in the party\n\
  * idx2 - position in the party\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_SwapPCs(PyObject * /*self*/, PyObject* args)
{
	int idx1, idx2;

	if (!PyArg_ParseTuple( args, "ii", &idx1, &idx2)) {
		return AttributeError( GemRB_SwapPCs__doc );
	}

	GET_GAME();

	game->SwapPCs(game->FindPlayer(idx1), game->FindPlayer(idx2));
	//leader changed
	if (idx1==1 || idx2==1) {
		DisplayStringCore( game->FindPC(1), VB_LEADER, DS_CONST);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetRepeatClickFlags__doc,
"===== SetRepeatClickFlags =====\n\
\n\
**Prototype:** GemRB.SetRepeatClickFlags (value, op)\n\
\n\
**Description:** Sets the mode repeat clicks are handled.\n\
\n\
**Parameters:** \n\
  * value - speed, see the GEM_RK* flags in GUIDefines.py\n\
  * op - bit operation to perform\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:bit_operation]]\n\
"
);

static PyObject* GemRB_SetRepeatClickFlags(PyObject * /*self*/, PyObject* args)
{
	int value, op;
	unsigned long ret;

	if (!PyArg_ParseTuple( args, "ii", &value, &op)) {
		return AttributeError( GemRB_SetRepeatClickFlags__doc );
	}
	ret = core->GetEventMgr()->SetRKFlags( (unsigned long) value, (unsigned long) op);
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_DisplayString__doc,
"===== DisplayString =====\n\
\n\
**Prototype:** GemRB.DisplayString (strref, color[, PartyID])\n\
\n\
**Description:** Displays a string in the messagewindow using methods \n\
supplied by the core engine. The optional actor is the party ID of the \n\
character whose name will be displayed (as saying the string).\n\
\n\
**Parameters:**\n\
  * strref  - the tlk reference\n\
  * color   - a hex packed RGB value\n\
  * PartyID - if supplied, then the PC's name will be displayed too\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [[guiscript:Control_SetText]]\n\
"
);

static PyObject* GemRB_DisplayString(PyObject * /*self*/, PyObject* args)
{
	int strref, color;
	int globalID = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &strref, &color, &globalID)) {
		return AttributeError( GemRB_DisplayString__doc );
	}
	if (globalID) {
		GET_GAME();
		GET_ACTOR_GLOBAL();

		displaymsg->DisplayStringName(strref, (unsigned int) color, actor, IE_STR_SOUND);
	} else {
		displaymsg->DisplayString(strref, (unsigned int) color, IE_STR_SOUND);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetCombatDetails__doc,
"===== GetCombatDetails =====\n\
\n\
**Prototype:** GemRB.GetCombatDetails (pc, leftorright)\n\
\n\
**Description:** Returns the current THAC0 and other data relating to the \n\
equipped weapon.\n\
\n\
**Parameters:** \n\
  * pc - position in the party\n\
  * leftorright - left or right hand weapon (main or offhand)\n\
\n\
**Return value:** dict: 'ToHit', 'Flags', 'DamageBonus', 'Speed', \n\
'CriticalBonus', 'Style', 'Proficiency', 'Range', 'Enchantment', 'Slot', \n\
'APR', 'CriticalMultiplier', 'CriticalRange', 'ProfDmgBon', \n\
'LauncherDmgBon', 'WeaponStrBonus', 'AC' (dict), 'ToHitStats' (dict)\n\
\n\
**See also:** [[guiscript:IsDualWielding]]"
);

static PyObject* GemRB_GetCombatDetails(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	int leftorright;

	if (!PyArg_ParseTuple( args, "ii", &globalID, &leftorright)) {
		return AttributeError( GemRB_GetCombatDetails__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	leftorright = leftorright&1;
	WeaponInfo wi;
	ITMExtHeader *header = NULL; // contains the weapon header
	ITMExtHeader *hittingheader = NULL; // same header, except for ranged weapons it is the ammo header
	int tohit=20;
	int DamageBonus=0;
	int CriticalBonus=0;
	int speed=0;
	int style=0;

	PyObject* dict = PyDict_New();
	if (!actor->GetCombatDetails(tohit, leftorright, wi, header, hittingheader, DamageBonus, speed, CriticalBonus, style, NULL)) {
		//TODO: handle error, so tohit will still be set correctly?
	}
	PyDict_SetItemString(dict, "Slot", PyInt_FromLong (wi.slot));
	PyDict_SetItemString(dict, "Flags", PyInt_FromLong (wi.wflags));
	PyDict_SetItemString(dict, "Enchantment", PyInt_FromLong (wi.enchantment));
	PyDict_SetItemString(dict, "Range", PyInt_FromLong (wi.range));
	PyDict_SetItemString(dict, "Proficiency", PyInt_FromLong (wi.prof));
	PyDict_SetItemString(dict, "DamageBonus", PyInt_FromLong (DamageBonus));
	PyDict_SetItemString(dict, "Speed", PyInt_FromLong (speed));
	PyDict_SetItemString(dict, "CriticalBonus", PyInt_FromLong (CriticalBonus));
	PyDict_SetItemString(dict, "Style", PyInt_FromLong (style));
	PyDict_SetItemString(dict, "APR", PyInt_FromLong (actor->GetNumberOfAttacks() ));
	PyDict_SetItemString(dict, "CriticalMultiplier", PyInt_FromLong (wi.critmulti));
	PyDict_SetItemString(dict, "CriticalRange", PyInt_FromLong (wi.critrange));
	PyDict_SetItemString(dict, "ProfDmgBon", PyInt_FromLong (wi.profdmgbon));
	PyDict_SetItemString(dict, "LauncherDmgBon", PyInt_FromLong (wi.launcherdmgbon));
	PyDict_SetItemString(dict, "WeaponStrBonus", PyInt_FromLong (actor->WeaponDamageBonus(wi)));
	if (hittingheader) {
		PyDict_SetItemString(dict, "HitHeaderNumDice", PyInt_FromLong (hittingheader->DiceThrown));
		PyDict_SetItemString(dict, "HitHeaderDiceSides", PyInt_FromLong (hittingheader->DiceSides));
		PyDict_SetItemString(dict, "HitHeaderDiceBonus", PyInt_FromLong (hittingheader->DamageBonus));
	} else {
		return RuntimeError("Serious problem in GetCombatDetails: could not find the hitting header!");
	}

	PyObject *ac = PyDict_New();
	PyDict_SetItemString(ac, "Total", PyInt_FromLong (actor->AC.GetTotal()));
	PyDict_SetItemString(ac, "Natural", PyInt_FromLong (actor->AC.GetNatural()));
	PyDict_SetItemString(ac, "Armor", PyInt_FromLong (actor->AC.GetArmorBonus()));
	PyDict_SetItemString(ac, "Shield", PyInt_FromLong (actor->AC.GetShieldBonus()));
	PyDict_SetItemString(ac, "Deflection", PyInt_FromLong (actor->AC.GetDeflectionBonus()));
	PyDict_SetItemString(ac, "Generic", PyInt_FromLong (actor->AC.GetGenericBonus()));
	PyDict_SetItemString(ac, "Dexterity", PyInt_FromLong (actor->AC.GetDexterityBonus()));
	PyDict_SetItemString(ac, "Wisdom", PyInt_FromLong (actor->AC.GetWisdomBonus()));
	PyDict_SetItemString(dict, "AC", ac);

	PyObject *tohits = PyDict_New();
	PyDict_SetItemString(tohits, "Total", PyInt_FromLong (actor->ToHit.GetTotal()));
	PyDict_SetItemString(tohits, "Base", PyInt_FromLong (actor->ToHit.GetBase()));
	PyDict_SetItemString(tohits, "Armor", PyInt_FromLong (actor->ToHit.GetArmorBonus()));
	PyDict_SetItemString(tohits, "Shield", PyInt_FromLong (actor->ToHit.GetShieldBonus()));
	PyDict_SetItemString(tohits, "Proficiency", PyInt_FromLong (actor->ToHit.GetProficiencyBonus()));
	PyDict_SetItemString(tohits, "Generic", PyInt_FromLong (actor->ToHit.GetGenericBonus() + actor->ToHit.GetFxBonus()));
	PyDict_SetItemString(tohits, "Ability", PyInt_FromLong (actor->ToHit.GetAbilityBonus()));
	PyDict_SetItemString(tohits, "Weapon", PyInt_FromLong (actor->ToHit.GetWeaponBonus()));
	PyDict_SetItemString(dict, "ToHitStats", tohits);

	const CREItem *wield;
	// wi.slot has the launcher, so look up the ammo
	//FIXME: remove the need to look it up again
	if (hittingheader && (hittingheader->AttackType&ITEM_AT_PROJECTILE)) {
		wield = actor->inventory.GetSlotItem(actor->inventory.GetEquippedSlot());
		header = hittingheader;
	} else {
		wield = actor->inventory.GetUsedWeapon(leftorright, wi.slot);
	}
	if (!wield) {
		Log(WARNING, "Actor", "Invalid weapon wielded by %s!", actor->GetName(1));
		return dict;
	}
	Item *item = gamedata->GetItem(wield->ItemResRef, true);
	if (!item) {
		Log(WARNING, "Actor", "Missing or invalid weapon item: %s!", wield->ItemResRef);
		return dict;
	}

	// create a tuple with all the 100% probable damage opcodes' stats
	std::vector<DMGOpcodeInfo> damage_opcodes = item->GetDamageOpcodesDetails(header) ;
	PyObject *alldos = PyTuple_New(damage_opcodes.size());
	unsigned int i;
	for (i = 0; i < damage_opcodes.size(); i++) {
		PyObject *dos = PyDict_New();
		PyDict_SetItemString(dos, "TypeName", PyString_FromString (damage_opcodes[i].TypeName));
		PyDict_SetItemString(dos, "NumDice", PyInt_FromLong (damage_opcodes[i].DiceThrown));
		PyDict_SetItemString(dos, "DiceSides", PyInt_FromLong (damage_opcodes[i].DiceSides));
		PyDict_SetItemString(dos, "DiceBonus", PyInt_FromLong (damage_opcodes[i].DiceBonus));
		PyDict_SetItemString(dos, "Chance", PyInt_FromLong (damage_opcodes[i].Chance));
		PyTuple_SetItem(alldos, i, dos);
	}
	PyDict_SetItemString(dict, "DamageOpcodes", alldos);

	return dict;
}

PyDoc_STRVAR( GemRB_GetDamageReduction__doc,
"===== GetDamageReduction =====\n\
\n\
**Prototype:** GemRB.GetDamageReduction (globalID, enchantment[, missile])\n\
\n\
**Description:** returns the actor's damage reduction for the specified \n\
enchantment level and type. Used in iwd2. Can be cancelled by high \n\
weapon enchantment.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * enchantment - enchantment level (usually 0-5)\n\
  * missile - look at missile reduction, not melee\n\
\n\
**Return value:** integer, the amount of resisted damage\n\
\n\
**See also:** [[guiscript:GetCombatDetails]]\n\
"
);
static PyObject* GemRB_GetDamageReduction(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	unsigned int enchantment = 0;
	int missile = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &globalID, &enchantment, &missile)) {
		return AttributeError( GemRB_GetDamageReduction__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	int total = 0;
	if (missile) {
		total = actor->GetDamageReduction(IE_RESISTMISSILE, enchantment);
	} else {
		total = actor->GetDamageReduction(IE_RESISTCRUSHING, enchantment);
	}

	return PyInt_FromLong(total);
}

PyDoc_STRVAR( GemRB_GetSpellFailure__doc,
"===== GetSpellFailure =====\n\
\n\
**Prototype:** GemRB.GetSpellFailure (globalID[, divine])\n\
\n\
**Description:** returns the arcane/divine spell failure in percent.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * divine - return divine spell failure instead\n\
\n\
**Return value:** dict, spell failure (Total, Armor, Shield)\n\
"
);
static PyObject* GemRB_GetSpellFailure(PyObject * /*self*/, PyObject* args)
{
	int globalID;
	int cleric = 0;

	if (!PyArg_ParseTuple( args, "i|i", &globalID, &cleric)) {
		return AttributeError( GemRB_GetSpellFailure__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	PyObject *failure = PyDict_New();
	// true means arcane, so reverse the passed argument
	PyDict_SetItemString(failure, "Total", PyInt_FromLong (actor->GetSpellFailure(!cleric)));
	// set also the shield and armor penalty - we can't reuse the ones for to-hit boni, since they also considered armor proficiency
	int am = 0, sm = 0;
	actor->GetArmorFailure(am, sm);
	PyDict_SetItemString(failure, "Armor", PyInt_FromLong (am));
	PyDict_SetItemString(failure, "Shield", PyInt_FromLong (sm));

	return failure;
}

PyDoc_STRVAR( GemRB_IsDualWielding__doc,
"===== IsDualWielding =====\n\
\n\
**Prototype:** GemRB.IsDualWielding (globalID)\n\
\n\
**Description:** 1 if the actor is dual wielding; 0 otherwise.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** bool\n\
\n\
**See also:** [[guiscript:GetCombatDetails]]\n\
"
);

static PyObject* GemRB_IsDualWielding(PyObject * /*self*/, PyObject* args)
{
	int globalID;

	if (!PyArg_ParseTuple( args, "i", &globalID)) {
		return AttributeError( GemRB_IsDualWielding__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	int dualwield = actor->IsDualWielding();
	return PyInt_FromLong( dualwield );
}

PyDoc_STRVAR( GemRB_GetSelectedSize__doc,
"===== GetSelectedSize =====\n\
\n\
**Prototype:** GemRB.GetSelectedSize ()\n\
\n\
**Description:** Returns the number of actors selected in the party.\n\
\n\
**Return value:** int\n\
\n\
**See also:** [[guiscript:GetSelectedActors]], [[guiscript:GetSelectedPCSingle]]"
);

static PyObject* GemRB_GetSelectedSize(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyInt_FromLong(game->selected.size());
}

PyDoc_STRVAR( GemRB_GetSelectedActors__doc,
"===== GetSelectedActors =====\n\
\n\
**Prototype:** GemRB.GetSelectedActors ()\n\
\n\
**Description:** Returns the global ids of selected actors in a tuple.\n\
\n\
**Return value:** tuple of ints\n\
\n\
**See also:** [[guiscript:GetSelectedSize]], [[guiscript:GetSelectedPCSingle]]"
);

static PyObject* GemRB_GetSelectedActors(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	int count = game->selected.size();
	PyObject* actor_list = PyTuple_New(count);
	for (int i = 0; i < count; i++) {
		PyTuple_SetItem(actor_list, i, PyInt_FromLong( game->selected[i]->GetGlobalID() ) );
	}
	return actor_list;
}

PyDoc_STRVAR( GemRB_GetSpellCastOn__doc,
"===== GetSpellCastOn =====\n\
\n\
**Prototype:** GemRB.GetSpellCastOn (pc)\n\
\n\
**Description:** Returns the last spell cast on a party member.\n\
\n\
**Parameters:**\n\
  * pc - PartyID\n\
\n\
**Return value:** resref"
);

static PyObject* GemRB_GetSpellCastOn(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	ieResRef splname;

	if (!PyArg_ParseTuple( args, "i", &globalID )) {
		return AttributeError( GemRB_GetSpellCastOn__doc );
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();
/*
	Actor* actor = game->FindPC( globalID );
	if (!actor) {
		return RuntimeError( "Actor not found!\n" );
	}
*/
	ResolveSpellName(splname, actor->LastSpellOnMe);
	return PyString_FromString(splname);
}

PyDoc_STRVAR( GemRB_SetTickHook__doc,
"===== SetTickHook =====\n\
\n\
**Prototype:** GemRB.SetTickHook (callback)\n\
\n\
**Description:** Set callback to be called every main loop iteration. \n\
This is useful for things like running a twisted reactor.\n\
\n\
**Parameters:**\n\
  * callback - python function to run\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_SetTickHook(PyObject* /*self*/, PyObject* args)
{
	PyObject* function;

	if (!PyArg_ParseTuple(args, "O", &function)) {
		return AttributeError( GemRB_SetTickHook__doc );
	}

	EventHandler handler = NULL;
	if (function != Py_None && PyCallable_Check(function)) {
		handler = new PythonCallback(function);
	} else {
		char buf[256];
		snprintf(buf, sizeof(buf), "Can't set timed event handler %s!", PyEval_GetFuncName(function));
		return RuntimeError(buf);
	}

	core->SetTickHook(handler);

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetupMaze__doc,
"===== SetupMaze =====\n\
\n\
**Prototype:** GemRB.SetupMaze (x,y)\n\
\n\
**Description:** Initializes a maze of size XxY. The dimensions shouldn't \n\
exceed the maximum possible maze size (8x8).\n\
\n\
**Parameters:** \n\
  * x, y - dimensions\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_SetupMaze(PyObject* /*self*/, PyObject* args)
{
	int xsize, ysize;

	if (!PyArg_ParseTuple( args, "ii", &xsize, &ysize )) {
		return AttributeError( GemRB_SetupMaze__doc );
	}

	if ((unsigned) xsize>MAZE_MAX_DIM || (unsigned) ysize>MAZE_MAX_DIM) {
		return AttributeError( GemRB_SetupMaze__doc );
	}

	GET_GAME();

	maze_header *h = reinterpret_cast<maze_header *> (game->AllocateMazeData()+MAZE_ENTRY_COUNT*MAZE_ENTRY_SIZE);
	memset(h, 0, MAZE_HEADER_SIZE);
	h->maze_sizex = xsize;
	h->maze_sizey = ysize;
	for(int i=0;i<MAZE_ENTRY_COUNT;i++) {
		maze_entry *m = reinterpret_cast<maze_entry *> (game->mazedata+i*MAZE_ENTRY_SIZE);
		memset(m, 0, MAZE_ENTRY_SIZE);
		bool used = (i/MAZE_MAX_DIM<ysize) && (i%MAZE_MAX_DIM<xsize);
		m->valid = used;
		m->accessible = used;
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetMazeEntry__doc,
"===== SetMazeEntry =====\n\
\n\
**Prototype:** GemRB.SetMazeEntry (entry, type, value)\n\
\n\
**Description:** Sets a field in a maze entry. The entry index shouldn't \n\
exceed the maximum possible maze size (64).\n\
\n\
**Parameters:** \n\
  * entry - index of entry to change\n\
  * type - what sort of entry should it be?\n\
    * ME_ACCESSED, ME_WALLS, ME_TRAP, ME_SPECIAL\n\
  * value - what to set (meaning depends on type)\n\
\n\
**Return value:** N/A\n\
"
);

static PyObject* GemRB_SetMazeEntry(PyObject* /*self*/, PyObject* args)
{
	int entry;
	int index;
	int value;

	if (!PyArg_ParseTuple( args, "iii", &entry, &index, &value )) {
		return AttributeError( GemRB_SetMazeEntry__doc );
	}

	if (entry<0 || entry>63) {
		return AttributeError( GemRB_SetMazeEntry__doc );
	}

	GET_GAME();

	if (!game->mazedata) {
		return RuntimeError( "No maze set up!" );
	}

	maze_entry *m = reinterpret_cast<maze_entry *> (game->mazedata+entry*MAZE_ENTRY_SIZE);
	maze_entry *m2;
	switch(index) {
		case ME_OVERRIDE:
			m->me_override = value;
			break;
		default:
		case ME_VALID:
		case ME_ACCESSIBLE:
			return AttributeError( GemRB_SetMazeEntry__doc );
		case ME_TRAP: //trapped/traptype
			if (value==-1) {
				m->trapped = 0;
				m->traptype = 0;
			} else {
				m->trapped = 1;
				m->traptype = value;
			}
			break;
		case ME_WALLS:
			m->walls |= value;
			if (value & WALL_SOUTH) {
				if (entry%MAZE_MAX_DIM!=MAZE_MAX_DIM-1) {
					m2 = reinterpret_cast<maze_entry *> (game->mazedata+(entry+1)*MAZE_ENTRY_SIZE);
					m2->walls|=WALL_NORTH;
				}
			}

			if (value & WALL_NORTH) {
				if (entry%MAZE_MAX_DIM) {
					m2 = reinterpret_cast<maze_entry *> (game->mazedata+(entry-1)*MAZE_ENTRY_SIZE);
					m2->walls|=WALL_SOUTH;
				}
			}

			if (value & WALL_EAST) {
				if (entry+MAZE_MAX_DIM<MAZE_ENTRY_COUNT) {
					m2 = reinterpret_cast<maze_entry *> (game->mazedata+(entry+MAZE_MAX_DIM)*MAZE_ENTRY_SIZE);
					m2->walls|=WALL_WEST;
				}
			}

			if (value & WALL_WEST) {
				if (entry>=MAZE_MAX_DIM) {
					m2 = reinterpret_cast<maze_entry *> (game->mazedata+(entry-MAZE_MAX_DIM)*MAZE_ENTRY_SIZE);
					m2->walls|=WALL_EAST;
				}
			}

			break;
		case ME_VISITED:
			m->visited = value;
			break;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_SetMazeData__doc,
"===== SetMazeData =====\n\
\n\
**Prototype:** GemRB.SetMazeData (field, value)\n\
\n\
**Description:** Sets a field in the maze header.\n\
\n\
**Parameters:** \n\
  * field - look at MH_* constants in maze_defs.py\n\
  * value - target value\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_SetMazeData(PyObject* /*self*/, PyObject* args)
{
	int entry;
	int value;

	if (!PyArg_ParseTuple( args, "ii", &entry, &value )) {
		return AttributeError( GemRB_SetMazeData__doc );
	}

	GET_GAME();

	if (!game->mazedata) {
		return RuntimeError( "No maze set up!" );
	}

	maze_header *h = reinterpret_cast<maze_header *> (game->mazedata+MAZE_ENTRY_COUNT*MAZE_ENTRY_SIZE);
	switch(entry) {
		case MH_POS1X:
			h->pos1x = value;
			break;
		case MH_POS1Y:
			h->pos1y = value;
			break;
		case MH_POS2X:
			h->pos2x = value;
			break;
		case MH_POS2Y:
			h->pos2y = value;
			break;
		case MH_POS3X:
			h->pos3x = value;
			break;
		case MH_POS3Y:
			h->pos3y = value;
			break;
		case MH_POS4X:
			h->pos4x = value;
			break;
		case MH_POS4Y:
			h->pos4y = value;
			break;
		case MH_TRAPCOUNT:
			h->trapcount = value;
			break;
		case MH_INITED:
			h->initialized = value;
			break;
		case MH_UNKNOWN2C:
			h->unknown2c = value;
			break;
		case MH_UNKNOWN30:
			h->unknown30 = value;
			break;
		default:
			return AttributeError( GemRB_SetMazeData__doc );
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetMazeHeader__doc,
"===== GetMazeHeader =====\n\
\n\
**Prototype:** GemRB.GetMazeHeader ()\n\
\n\
**Description:** Returns the Maze header of Planescape Torment savegames.\n\
\n\
**Return value:** dict"
);

static PyObject* GemRB_GetMazeHeader(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	if (!game->mazedata) {
		Py_RETURN_NONE;
	}

	PyObject* dict = PyDict_New();
	maze_header *h = reinterpret_cast<maze_header *> (game->mazedata+MAZE_ENTRY_COUNT*MAZE_ENTRY_SIZE);
	PyDict_SetItemString(dict, "MazeX", PyInt_FromLong (h->maze_sizex));
	PyDict_SetItemString(dict, "MazeY", PyInt_FromLong (h->maze_sizey));
	PyDict_SetItemString(dict, "Pos1X", PyInt_FromLong (h->pos1x));
	PyDict_SetItemString(dict, "Pos1Y", PyInt_FromLong (h->pos1y));
	PyDict_SetItemString(dict, "Pos2X", PyInt_FromLong (h->pos2x));
	PyDict_SetItemString(dict, "Pos2Y", PyInt_FromLong (h->pos2y));
	PyDict_SetItemString(dict, "Pos3X", PyInt_FromLong (h->pos3x));
	PyDict_SetItemString(dict, "Pos3Y", PyInt_FromLong (h->pos3y));
	PyDict_SetItemString(dict, "Pos4X", PyInt_FromLong (h->pos4x));
	PyDict_SetItemString(dict, "Pos4Y", PyInt_FromLong (h->pos4y));
	PyDict_SetItemString(dict, "TrapCount", PyInt_FromLong (h->trapcount));
	PyDict_SetItemString(dict, "Inited", PyInt_FromLong (h->initialized));
	return dict;
}

PyDoc_STRVAR( GemRB_GetMazeEntry__doc,
"===== GetMazeEntry =====\n\
\n\
**Prototype:** GemRB.GetMazeEntry (entry)\n\
\n\
**Description:** Returns a Maze entry from Planescape Torment savegames.\n\
\n\
**Parameters:** \n\
  * entry - target entry (0-63; lesser than max maze size)\n\
\n\
**Return value:** dict\n\
"
);

static PyObject* GemRB_GetMazeEntry(PyObject* /*self*/, PyObject* args)
{
	int entry;

	if (!PyArg_ParseTuple( args, "i", &entry )) {
		return AttributeError( GemRB_GetMazeEntry__doc );
	}

	if (entry<0 || entry>=MAZE_ENTRY_COUNT) {
		return AttributeError( GemRB_GetMazeEntry__doc );
	}

	GET_GAME();

	if (!game->mazedata) {
		return RuntimeError( "No maze set up!" );
	}

	PyObject* dict = PyDict_New();
	maze_entry *m = reinterpret_cast<maze_entry *> (game->mazedata+entry*MAZE_ENTRY_SIZE);
	PyDict_SetItemString(dict, "Override", PyInt_FromLong (m->me_override));
	PyDict_SetItemString(dict, "Accessible", PyInt_FromLong (m->accessible));
	PyDict_SetItemString(dict, "Valid", PyInt_FromLong (m->valid));
	if (m->trapped) {
		PyDict_SetItemString(dict, "Trapped", PyInt_FromLong (m->traptype));
	} else {
		PyDict_SetItemString(dict, "Trapped", PyInt_FromLong (-1));
	}
	PyDict_SetItemString(dict, "Walls", PyInt_FromLong (m->walls));
	PyDict_SetItemString(dict, "Visited", PyInt_FromLong (m->visited));
	return dict;
}

char gametype_hint[100];
int gametype_hint_weight;

PyDoc_STRVAR( GemRB_AddGameTypeHint__doc,
"===== AddGameTypeHint =====\n\
\n\
**Prototype:** GemRB.AddGameTypeHint (type, weight, flags=0)\n\
\n\
**Description:** Asserts that GameType should be TYPE, with confidence WEIGHT. \n\
This is used by Autodetect.py scripts when GameType was set to 'auto'.\n\
\n\
**Parameters:**\n\
  * type - GameType (e.g. bg1, bg2, iwd, how, iwd2, pst and others)\n\
  * weight - numeric, confidence that TYPE is correct. Standard games should use values <= 100, (eventual) new games based on the standard ones should use values above 100.\n\
  * flags - numeric, not used now\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_AddGameTypeHint(PyObject* /*self*/, PyObject* args)
{
	char* type;
	int weight;
	int flags = 0;

	if (!PyArg_ParseTuple( args, "si|i", &type, &weight, &flags )) {
		return AttributeError( GemRB_AddGameTypeHint__doc );
	}

	if (weight > gametype_hint_weight) {
		gametype_hint_weight = weight;
		strncpy(gametype_hint, type, sizeof(gametype_hint)-1);
		// I assume the '\0' in the end of gametype_hint
	}

	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_GetAreaInfo__doc,
"===== GetAreaInfo =====\n\
\n\
**Prototype:** GemRB.GetAreaInfo ()\n\
\n\
**Description:** Returns important values about the current area.\n\
\n\
**Return value:** dict (area name and mouse position)\n\
"
);

static PyObject* GemRB_GetAreaInfo(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();
	GET_GAMECONTROL();

	PyObject* info = PyDict_New();
	PyDict_SetItemString(info, "CurrentArea", PyString_FromResRef( game->CurrentArea ) );
	PyDict_SetItemString(info, "PositionX", PyInt_FromLong (gc->lastMouseX));
	PyDict_SetItemString(info, "PositionY", PyInt_FromLong (gc->lastMouseY));

	return info;
}

PyDoc_STRVAR( GemRB_Log__doc,
"===== Log =====\n\
\n\
**Prototype:** GemRB.Log (log_level, owner, message)\n\
\n\
**Description:** Log a message to GemRB's logging system.\n\
\n\
**Parameters:**\n\
  * log_level - integer log level (from GUIDefines.py)\n\
    * LOG_NONE = -1\n\
    * LOG_FATAL = 0\n\
    * LOG_ERROR = 1\n\
    * LOG_WARNING = 2\n\
    * LOG_MESSAGE = 3\n\
    * LOG_COMBAT = 4\n\
    * LOG_DEBUG = 5\n\
  * owner - name of the context or owner of the message\n\
  * message - string to log\n\
\n\
**Return value:** N/A"
);

static PyObject* GemRB_Log(PyObject* /*self*/, PyObject* args)
{
	log_level level;
	char* owner;
	char* message;

	if (!PyArg_ParseTuple(args, "iss", &level, &owner, &message)) {
		return NULL;
	}

	Log(level, owner, "%s", message);
	Py_RETURN_NONE;
}


PyDoc_STRVAR( GemRB_SetFeature__doc,
"===== SetFeature =====\n\
\n\
**Prototype:** GemRB.SetFeature (feature, value)\n\
\n\
**Description:** Set GameType flag FEATURE to VALUE, either True or False.\n\
\n\
**Parameters:**\n\
  * FEATURE - GF_xxx constant defined in GUIDefines.py and globals.h\n\
  * VALUE - value to set the feature to. Either True or False\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
    GemRB.SetFeature(GF_ALL_STRINGS_TAGGED, True)\n\
"
);

static PyObject* GemRB_SetFeature(PyObject* /*self*/, PyObject* args)
{
	unsigned int feature;
	bool value;

	if (!PyArg_ParseTuple(args, "ib", &feature, &value)) {
		return NULL;
	}

	core->SetFeature(value, feature);
	Py_RETURN_NONE;
}

PyDoc_STRVAR( GemRB_GetMultiClassPenalty__doc,
"===== GetMultiClassPenalty =====\n\
\n\
**Prototype:** GemRB.GetMultiClassPenalty (globalID)\n\
\n\
**Description:** Returns the experience penalty from unsynced classes.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** integer"
);

static PyObject* GemRB_GetMultiClassPenalty(PyObject* /*self*/, PyObject* args)
{
	int globalID;

	if (!PyArg_ParseTuple(args, "i", &globalID)) {
		return AttributeError(GemRB_GetMultiClassPenalty__doc);
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	return  PyInt_FromLong(actor->GetFavoredPenalties());
}

static PyMethodDef GemRBMethods[] = {
	METHOD(ActOnPC, METH_VARARGS),
	METHOD(AddGameTypeHint, METH_VARARGS),
	METHOD(AddNewArea, METH_VARARGS),
	METHOD(ApplyEffect, METH_VARARGS),
	METHOD(ApplySpell, METH_VARARGS),
	METHOD(CanUseItemType, METH_VARARGS),
	METHOD(ChangeContainerItem, METH_VARARGS),
	METHOD(ChangeItemFlag, METH_VARARGS),
	METHOD(ChangeStoreItem, METH_VARARGS),
	METHOD(ChargeSpells, METH_VARARGS),
	METHOD(CheckFeatCondition, METH_VARARGS),
	METHOD(CheckSpecialSpell, METH_VARARGS),
	METHOD(CheckVar, METH_VARARGS),
	METHOD(ClearActions, METH_VARARGS),
	METHOD(CloseRighthandStore, METH_NOARGS),
	METHOD(CountEffects, METH_VARARGS),
	METHOD(CountSpells, METH_VARARGS),
	METHOD(CreateCreature, METH_VARARGS),
	METHOD(CreateItem, METH_VARARGS),
	METHOD(CreateMovement, METH_VARARGS),
	METHOD(CreatePlayer, METH_VARARGS),
	METHOD(CreateString, METH_VARARGS),
	METHOD(CreateWindow, METH_VARARGS),
	METHOD(DeleteSaveGame, METH_VARARGS),
	METHOD(DispelEffect, METH_VARARGS),
	METHOD(DisplayString, METH_VARARGS),
	METHOD(DragItem, METH_VARARGS),
	METHOD(DrawWindows, METH_NOARGS),
	METHOD(DropDraggedItem, METH_VARARGS),
	METHOD(DumpActor, METH_VARARGS),
	METHOD(EnableCheatKeys, METH_VARARGS),
	METHOD(EndCutSceneMode, METH_NOARGS),
	METHOD(EnterGame, METH_NOARGS),
	METHOD(EnterStore, METH_VARARGS),
	METHOD(EvaluateString, METH_VARARGS),
	METHOD(ExecuteString, METH_VARARGS),
	METHOD(ExploreArea, METH_VARARGS),
	METHOD(FillPlayerInfo, METH_VARARGS),
	METHOD(FindItem, METH_VARARGS),
	METHOD(FindStoreItem, METH_VARARGS),
	METHOD(GameControlGetTargetMode, METH_NOARGS),
	METHOD(GameControlToggleAlwaysRun, METH_NOARGS),
	METHOD(GameControlSetLastActor, METH_VARARGS),
	METHOD(GameControlSetScreenFlags, METH_VARARGS),
	METHOD(GameControlSetTargetMode, METH_VARARGS),
	METHOD(GameGetReputation, METH_NOARGS),
	METHOD(GameSetReputation, METH_VARARGS),
	METHOD(GameGetFirstSelectedActor, METH_NOARGS),
	METHOD(GameGetFirstSelectedPC, METH_NOARGS),
	METHOD(GameGetFormation, METH_VARARGS),
	METHOD(GameGetPartyGold, METH_NOARGS),
	METHOD(GameGetSelectedPCSingle, METH_VARARGS),
	METHOD(GameIsBeastKnown, METH_VARARGS),
	METHOD(GameIsPCSelected, METH_VARARGS),
	METHOD(GamePause, METH_VARARGS),
	METHOD(GameSelectPC, METH_VARARGS),
	METHOD(GameSelectPCSingle, METH_VARARGS),
	METHOD(GameSetExpansion, METH_VARARGS),
	METHOD(GameGetExpansion, METH_NOARGS),
	METHOD(GameSetFormation, METH_VARARGS),
	METHOD(GameSetPartyGold, METH_VARARGS),
	METHOD(GameSetPartySize, METH_VARARGS),
	METHOD(GameSetProtagonistMode, METH_VARARGS),
	METHOD(GameSetScreenFlags, METH_VARARGS),
	METHOD(GetAreaInfo, METH_NOARGS),
	METHOD(GetAvatarsValue, METH_VARARGS),
	METHOD(GetAbilityBonus, METH_VARARGS),
	METHOD(GetCombatDetails, METH_VARARGS),
	METHOD(GetContainer, METH_VARARGS),
	METHOD(GetContainerItem, METH_VARARGS),
	METHOD(GetCurrentArea, METH_NOARGS),
	METHOD(GetDamageReduction, METH_VARARGS),
	METHOD(GetEquippedAmmunition, METH_VARARGS),
	METHOD(GetEquippedQuickSlot, METH_VARARGS),
	METHOD(GetGamePortraitPreview, METH_VARARGS),
	METHOD(GetGamePreview, METH_VARARGS),
	METHOD(GetGameString, METH_VARARGS),
	METHOD(GetGameTime, METH_NOARGS),
	METHOD(GetGameVar, METH_VARARGS),
	METHOD(GetINIBeastsKey, METH_VARARGS),
	METHOD(GetINIPartyCount, METH_NOARGS),
	METHOD(GetINIPartyKey, METH_VARARGS),
	METHOD(GetINIQuestsKey, METH_VARARGS),
	METHOD(GetItem, METH_VARARGS),
	METHOD(GetJournalEntry, METH_VARARGS),
	METHOD(GetJournalSize, METH_VARARGS),
	METHOD(GetKnownSpell, METH_VARARGS),
	METHOD(GetKnownSpellsCount, METH_VARARGS),
	METHOD(GetMaxEncumbrance, METH_VARARGS),
	METHOD(GetMazeEntry, METH_VARARGS),
	METHOD(GetMazeHeader, METH_NOARGS),
	METHOD(GetMemorizableSpellsCount, METH_VARARGS),
	METHOD(GetMemorizedSpell, METH_VARARGS),
	METHOD(GetMemorizedSpellsCount, METH_VARARGS),
	METHOD(GetMultiClassPenalty, METH_VARARGS),
	METHOD(MessageWindowDebug, METH_VARARGS),
	METHOD(GetMessageWindowSize, METH_NOARGS),
	METHOD(GetPartySize, METH_NOARGS),
	METHOD(GetPCStats, METH_VARARGS),
	METHOD(GetPlayerName, METH_VARARGS),
	METHOD(GetPlayerPortrait, METH_VARARGS),
	METHOD(GetPlayerStat, METH_VARARGS),
	METHOD(GetPlayerStates, METH_VARARGS),
	METHOD(GetPlayerScript, METH_VARARGS),
	METHOD(GetPlayerSound, METH_VARARGS),
	METHOD(GetPlayerString, METH_VARARGS),
	METHOD(GetRumour, METH_VARARGS),
	METHOD(GetSaveGames, METH_VARARGS),
	METHOD(GetSelectedSize, METH_NOARGS),
	METHOD(GetSelectedActors, METH_NOARGS),
	METHOD(GetString, METH_VARARGS),
	METHOD(GetSpellFailure, METH_VARARGS),
	METHOD(GetSpellCastOn, METH_VARARGS),
	METHOD(GetSlotType, METH_VARARGS),
	METHOD(GetStore, METH_VARARGS),
	METHOD(GetStoreDrink, METH_VARARGS),
	METHOD(GetStoreCure, METH_VARARGS),
	METHOD(GetStoreItem, METH_VARARGS),
	METHOD(GetSpell, METH_VARARGS),
	METHOD(GetSpelldata, METH_VARARGS),
	METHOD(GetSpelldataIndex, METH_VARARGS),
	METHOD(GetSlotItem, METH_VARARGS),
	METHOD(GetSlots, METH_VARARGS),
	METHOD(GetSystemVariable, METH_VARARGS),
	METHOD(GetToken, METH_VARARGS),
	METHOD(GetVar, METH_VARARGS),
	METHOD(HardEndPL, METH_NOARGS),
	METHOD(HasFeat, METH_VARARGS),
	METHOD(HasResource, METH_VARARGS),
	METHOD(HasSpecialItem, METH_VARARGS),
	METHOD(HasSpecialSpell, METH_VARARGS),
	METHOD(HideGUI, METH_NOARGS),
	METHOD(IncreaseReputation, METH_VARARGS),
	METHOD(IsDraggingItem, METH_NOARGS),
	METHOD(IsDualWielding, METH_VARARGS),
	METHOD(IsValidStoreItem, METH_VARARGS),
	METHOD(LearnSpell, METH_VARARGS),
	METHOD(LeaveContainer, METH_VARARGS),
	METHOD(LeaveParty, METH_VARARGS),
	METHOD(LeaveStore, METH_VARARGS),
	METHOD(LoadGame, METH_VARARGS),
	METHOD(LoadMusicPL, METH_VARARGS),
	METHOD(LoadRighthandStore, METH_VARARGS),
	METHOD(LoadSymbol, METH_VARARGS),
	METHOD(LoadTable, METH_VARARGS),
	METHOD(LoadWindowPack, METH_VARARGS),
	METHOD(LoadWindow, METH_VARARGS),
	METHOD(LoadWindowFrame, METH_VARARGS),
	METHOD(Log, METH_VARARGS),
	METHOD(MemorizeSpell, METH_VARARGS),
	METHOD(ModifyEffect, METH_VARARGS),
	METHOD(MoveToArea, METH_VARARGS),
	METHOD(Quit, METH_NOARGS),
	METHOD(QuitGame, METH_NOARGS),
	METHOD(PlaySound, METH_VARARGS),
	METHOD(PlayMovie, METH_VARARGS),
	METHOD(PrepareSpontaneousCast, METH_VARARGS),
	METHOD(RemoveItem, METH_VARARGS),
	METHOD(RemoveSpell, METH_VARARGS),
	METHOD(RemoveEffects, METH_VARARGS),
	METHOD(RestParty, METH_VARARGS),
	METHOD(RevealArea, METH_VARARGS),
	METHOD(Roll, METH_VARARGS),
	METHOD(RunRestScripts, METH_NOARGS),
	METHOD(SaveCharacter, METH_VARARGS),
	METHOD(SaveGame, METH_VARARGS),
	METHOD(SaveConfig, METH_NOARGS),
	METHOD(SetDefaultActions, METH_VARARGS),
	METHOD(SetEquippedQuickSlot, METH_VARARGS),
	METHOD(SetFeat, METH_VARARGS),
	METHOD(SetFeature, METH_VARARGS),
	METHOD(SetFullScreen, METH_VARARGS),
	METHOD(SetGamma, METH_VARARGS),
	METHOD(SetGlobal, METH_VARARGS),
	METHOD(SetInfoTextColor, METH_VARARGS),
	METHOD(SetJournalEntry, METH_VARARGS),
	METHOD(SetMapAnimation, METH_VARARGS),
	METHOD(SetMapDoor, METH_VARARGS),
	METHOD(SetMapExit, METH_VARARGS),
	METHOD(SetMapnote, METH_VARARGS),
	METHOD(SetMapRegion, METH_VARARGS),
	METHOD(SetMasterScript, METH_VARARGS),
	METHOD(SetMazeEntry, METH_VARARGS),
	METHOD(SetMazeData, METH_VARARGS),
	METHOD(SetMemorizableSpellsCount, METH_VARARGS),
	METHOD(SetModalState, METH_VARARGS),
	METHOD(SetMouseScrollSpeed, METH_VARARGS),
	METHOD(SetNextScript, METH_VARARGS),
	METHOD(SetPlayerDialog, METH_VARARGS),
	METHOD(SetPlayerName, METH_VARARGS),
	METHOD(SetPlayerScript, METH_VARARGS),
	METHOD(SetPlayerStat, METH_VARARGS),
	METHOD(SetPlayerString, METH_VARARGS),
	METHOD(SetPlayerSound, METH_VARARGS),
	METHOD(SetPurchasedAmount, METH_VARARGS),
	METHOD(SetRepeatClickFlags, METH_VARARGS),
	METHOD(SetTickHook, METH_VARARGS),
	METHOD(SetTimedEvent, METH_VARARGS),
	METHOD(SetToken, METH_VARARGS),
	METHOD(SetTooltipDelay, METH_VARARGS),
	METHOD(SetupMaze, METH_VARARGS),
	METHOD(SetupQuickSlot, METH_VARARGS),
	METHOD(SetupQuickSpell, METH_VARARGS),
	METHOD(SetVar, METH_VARARGS),
	METHOD(SoftEndPL, METH_NOARGS),
	METHOD(SpellCast, METH_VARARGS),
	METHOD(StatComment, METH_VARARGS),
	METHOD(StealFailed, METH_NOARGS),
	METHOD(SwapPCs, METH_VARARGS),
	METHOD(UnhideGUI, METH_NOARGS),
	METHOD(UnmemorizeSpell, METH_VARARGS),
	METHOD(UpdateAmbientsVolume, METH_NOARGS),
	METHOD(UpdateMusicVolume, METH_NOARGS),
	METHOD(UpdateWorldMap, METH_VARARGS),
	METHOD(UseItem, METH_VARARGS),
	METHOD(ValidTarget, METH_VARARGS),
	METHOD(VerbalConstant, METH_VARARGS),
	// terminating entry
	{NULL, NULL, 0, NULL}
};

static PyMethodDef GemRBInternalMethods[] = {
	METHOD(Button_CreateLabelOnButton, METH_VARARGS),
	METHOD(Button_EnableBorder, METH_VARARGS),
	METHOD(Button_SetActionIcon, METH_VARARGS),
	METHOD(Button_SetBAM, METH_VARARGS),
	METHOD(Button_SetBorder, METH_VARARGS),
	METHOD(Button_SetFlags, METH_VARARGS),
	METHOD(Button_SetFont, METH_VARARGS),
	METHOD(Button_SetAnchor, METH_VARARGS),
	METHOD(Button_SetPushOffset, METH_VARARGS),
	METHOD(Button_SetItemIcon, METH_VARARGS),
	METHOD(Button_SetMOS, METH_VARARGS),
	METHOD(Button_SetOverlay, METH_VARARGS),
	METHOD(Button_SetPLT, METH_VARARGS),
	METHOD(Button_SetPicture, METH_VARARGS),
	METHOD(Button_SetPictureClipping, METH_VARARGS),
	METHOD(Button_SetSpellIcon, METH_VARARGS),
	METHOD(Button_SetSprite2D, METH_VARARGS),
	METHOD(Button_SetSprites, METH_VARARGS),
	METHOD(Button_SetState, METH_VARARGS),
	METHOD(Button_SetTextColor, METH_VARARGS),
	METHOD(Control_AttachScrollBar, METH_VARARGS),
	METHOD(Control_GetRect, METH_VARARGS),
	METHOD(Control_HasAnimation, METH_VARARGS),
	METHOD(Control_QueryText, METH_VARARGS),
	METHOD(Control_SetAnimation, METH_VARARGS),
	METHOD(Control_SetAnimationPalette, METH_VARARGS),
	METHOD(Control_SetEvent, METH_VARARGS),
	METHOD(Control_SetPos, METH_VARARGS),
	METHOD(Control_SetSize, METH_VARARGS),
	METHOD(Control_SetStatus, METH_VARARGS),
	METHOD(Control_SetText, METH_VARARGS),
	METHOD(Control_SetTooltip, METH_VARARGS),
	METHOD(Control_SetVarAssoc, METH_VARARGS),
	METHOD(Control_SubstituteForControl, METH_VARARGS),
	METHOD(TextArea_SetFlags, METH_VARARGS),
	METHOD(Label_SetFont, METH_VARARGS),
	METHOD(Label_SetTextColor, METH_VARARGS),
	METHOD(Label_SetUseRGB, METH_VARARGS),
	METHOD(SaveGame_GetDate, METH_VARARGS),
	METHOD(SaveGame_GetGameDate, METH_VARARGS),
	METHOD(SaveGame_GetName, METH_VARARGS),
	METHOD(SaveGame_GetPortrait, METH_VARARGS),
	METHOD(SaveGame_GetPreview, METH_VARARGS),
	METHOD(SaveGame_GetSaveID, METH_VARARGS),
	METHOD(ScrollBar_SetDefaultScrollBar, METH_VARARGS),
	METHOD(Symbol_GetValue, METH_VARARGS),
	METHOD(Symbol_Unload, METH_VARARGS),
	METHOD(Table_FindValue, METH_VARARGS),
	METHOD(Table_GetColumnCount, METH_VARARGS),
	METHOD(Table_GetColumnIndex, METH_VARARGS),
	METHOD(Table_GetColumnName, METH_VARARGS),
	METHOD(Table_GetRowCount, METH_VARARGS),
	METHOD(Table_GetRowIndex, METH_VARARGS),
	METHOD(Table_GetRowName, METH_VARARGS),
	METHOD(Table_GetValue, METH_VARARGS),
	METHOD(Table_Unload, METH_VARARGS),
	METHOD(TextArea_Append, METH_VARARGS),
	METHOD(TextArea_Clear, METH_VARARGS),
	METHOD(TextArea_ListResources, METH_VARARGS),
	METHOD(TextArea_SetOptions, METH_VARARGS),
	METHOD(TextArea_SetChapterText, METH_VARARGS),
	METHOD(TextEdit_SetBackground, METH_VARARGS),
	METHOD(TextEdit_SetBufferLength, METH_VARARGS),
	METHOD(Window_CreateButton, METH_VARARGS),
	METHOD(Window_CreateLabel, METH_VARARGS),
	METHOD(Window_CreateMapControl, METH_VARARGS),
	METHOD(Window_CreateScrollBar, METH_VARARGS),
	METHOD(Window_CreateTextArea, METH_VARARGS),
	METHOD(Window_CreateTextEdit, METH_VARARGS),
	METHOD(Window_CreateWorldMapControl, METH_VARARGS),
	METHOD(Window_DeleteControl, METH_VARARGS),
	METHOD(Window_GetControl, METH_VARARGS),
	METHOD(Window_GetRect, METH_VARARGS),
	METHOD(Window_HasControl, METH_VARARGS),
	METHOD(Window_Invalidate, METH_VARARGS),
	METHOD(Window_ReassignControls, METH_VARARGS),
	METHOD(Window_SetFrame, METH_VARARGS),
	METHOD(Window_SetKeyPressEvent, METH_VARARGS),
	METHOD(Window_SetPicture, METH_VARARGS),
	METHOD(Window_SetPos, METH_VARARGS),
	METHOD(Window_SetSize, METH_VARARGS),
	METHOD(Window_SetVisible, METH_VARARGS),
	METHOD(Window_SetupControls, METH_VARARGS),
	METHOD(Window_SetupEquipmentIcons, METH_VARARGS),
	METHOD(Window_ShowModal, METH_VARARGS),
	METHOD(Window_Unload, METH_VARARGS),
	METHOD(WorldMap_AdjustScrolling, METH_VARARGS),
	METHOD(WorldMap_GetDestinationArea, METH_VARARGS),
	METHOD(WorldMap_SetTextColor, METH_VARARGS),
	// terminating entry
	{NULL, NULL, 0, NULL}
};

GUIScript::GUIScript(void)
{
	gs = this;
	pDict = NULL; //borrowed, but used outside a function
	pModule = NULL; //should decref it
	pMainDic = NULL; //borrowed, but used outside a function
	pGUIClasses = NULL;
}

GUIScript::~GUIScript(void)
{
	if (Py_IsInitialized()) {
		if (pModule) {
			Py_DECREF( pModule );
		}
		Py_Finalize();
	}
	if (ItemArray) {
		free(ItemArray);
		ItemArray=NULL;
	}
	if (SpellArray) {
		free(SpellArray);
		SpellArray=NULL;
	}
	if (StoreSpells) {
		free(StoreSpells);
		StoreSpells=NULL;
	}
	if (SpecialItems) {
		free(SpecialItems);
		SpecialItems=NULL;
	}
	if (UsedItems) {
		free(UsedItems);
		UsedItems=NULL;
	}

	StoreSpellsCount = -1;
	SpecialItemsCount = -1;
	UsedItemsCount = -1;
	ReputationIncrease[0]=(int) UNINIT_IEDWORD;
	ReputationDonation[0]=(int) UNINIT_IEDWORD;
	GUIAction[0]=UNINIT_IEDWORD;
}

/**
 * Quote path for use in python strings.
 * On windows also convert backslashes to forward slashes.
 */
static char* QuotePath(char* tgt, const char* src)
{
	char *p = tgt;
	char c;

	do {
		c = *src++;
#ifdef WIN32
		if (c == '\\') c = '/';
#endif
		if (c == '"' || c == '\\') *p++ = '\\';
	} while (0 != (*p++ = c));
	return tgt;
}


PyDoc_STRVAR( GemRB__doc,
"Module exposing GemRB data and engine internals\n\n"
"This module exposes to python GUIScripts GemRB engine data and internals. "
"It's implemented in gemrb/plugins/GUIScript/GUIScript.cpp" );

PyDoc_STRVAR( GemRB_internal__doc,
"Internal module for GemRB metaclasses.\n\n"
"This module is only for implementing GUIClass.py."
"It's implemented in gemrb/plugins/GUIScript/GUIScript.cpp" );

/** Initialization Routine */

bool GUIScript::Init(void)
{
	Py_Initialize();
	if (!Py_IsInitialized()) {
		return false;
	}

	PyObject *pMainMod = PyImport_AddModule( "__main__" );
	/* pMainMod is a borrowed reference */
	pMainDic = PyModule_GetDict( pMainMod );
	/* pMainDic is a borrowed reference */

	PyObject* pGemRB = Py_InitModule3( "GemRB", GemRBMethods, GemRB__doc );
	if (!pGemRB) {
		return false;
	}

	PyObject* p_GemRB = Py_InitModule3( "_GemRB", GemRBInternalMethods, GemRB_internal__doc );
	if (!p_GemRB) {
		return false;
	}

	char string[_MAX_PATH+200];

	sprintf( string, "import sys" );
	if (PyRun_SimpleString( string ) == -1) {
		Log(ERROR, "GUIScript", "Error running: %s", string);
		return false;
	}

	// 2.6+ only, so we ignore failures
	sprintf( string, "sys.dont_write_bytecode = True");
	PyRun_SimpleString( string );

	char path[_MAX_PATH];
	char path2[_MAX_PATH];
	char quoted[_MAX_PATH];

	PathJoin(path, core->GUIScriptsPath, "GUIScripts", NULL);

	// Add generic script path early, so GameType detection works
	sprintf( string, "sys.path.append(\"%s\")", QuotePath( quoted, path ));
	if (PyRun_SimpleString( string ) == -1) {
		Log(ERROR, "GUIScript", "Error running: %s", string);
		return false;
	}

	sprintf( string, "import GemRB\n");
	if (PyRun_SimpleString( "import GemRB" ) == -1) {
		Log(ERROR, "GUIScript", "Error running: %s", string);
		return false;
	}

	sprintf(string, "GemRB.Version = '%s'", VERSION_GEMRB);
	PyRun_SimpleString(string);

	// Detect GameType if it was set to auto
	if (stricmp( core->GameType, "auto" ) == 0) {
		Autodetect();
	}

	// use the iwd guiscripts for how, but leave its override
	if (stricmp( core->GameType, "how" ) == 0) {
		PathJoin(path2, path, "iwd", NULL);
	} else {
		PathJoin(path2, path, core->GameType, NULL);
	}

	// GameType-specific import path must have a higher priority than
	// the generic one, so insert it before it
	sprintf( string, "sys.path.insert(-1, \"%s\")", QuotePath( quoted, path2 ));
	if (PyRun_SimpleString( string ) == -1) {
		Log(ERROR, "GUIScript", "Error running: %s", string );
		return false;
	}
	sprintf( string, "GemRB.GameType = \"%s\"", core->GameType);
	if (PyRun_SimpleString( string ) == -1) {
		Log(ERROR, "GUIScript", "Error running: %s", string );
		return false;
	}

#if PY_MAJOR_VERSION == 2
#if PY_MINOR_VERSION > 5
	// warn about python stuff that will go away in 3.0
	Py_Py3kWarningFlag = true;
#endif
#endif

	if (PyRun_SimpleString( "from GUIDefines import *" ) == -1) {
		Log(ERROR, "GUIScript", "Check if %s/GUIDefines.py exists!", path);
		return false;
	}

	if (PyRun_SimpleString( "from GUIClasses import *" ) == -1) {
		Log(ERROR, "GUIScript", "Check if %s/GUIClasses.py exists!", path);
		return false;
	}

	if (PyRun_SimpleString( "from GemRB import *" ) == -1) {
		Log(ERROR, "GUIScript", "builtin GemRB module failed to load!!!");
		return false;
	}

	// TODO: Put this file somewhere user editable
	// TODO: Search multiple places for this file
	char include[_MAX_PATH];
	PathJoin(include, core->GUIScriptsPath, "GUIScripts/include.py", NULL);
	ExecFile(include);

	PyObject *pClassesMod = PyImport_AddModule( "GUIClasses" );
	/* pClassesMod is a borrowed reference */
	pGUIClasses = PyModule_GetDict( pClassesMod );
	/* pGUIClasses is a borrowed reference */

	return true;
}

bool GUIScript::Autodetect(void)
{
	Log(MESSAGE, "GUIScript", "Detecting GameType.");

	char path[_MAX_PATH];
	PathJoin( path, core->GUIScriptsPath, "GUIScripts", NULL );
	DirectoryIterator iter( path );
	if (!iter)
		return false;

	gametype_hint[0] = '\0';
	gametype_hint_weight = 0;

	do {
		const char *dirent = iter.GetName();
		char module[_MAX_PATH];

		//print("DE: %s", dirent);
		if (iter.IsDirectory() && dirent[0] != '.') {
			// NOTE: these methods subtly differ in sys.path content, need for __init__.py files ...
			// Method1:
			PathJoin(module, core->GUIScriptsPath, "GUIScripts", dirent, "Autodetect.py", NULL);
			ExecFile(module);
			// Method2:
			//strcpy( module, dirent );
			//strcat( module, ".Autodetect");
			//LoadScript(module);
		}
	} while (++iter);

	if (gametype_hint[0]) {
		Log(MESSAGE, "GUIScript", "Detected GameType: %s", gametype_hint);
		strcpy(core->GameType, gametype_hint);
		return true;
	}
	else {
		Log(ERROR, "GUIScript", "Failed to detect game type.");
		return false;
	}
}

bool GUIScript::LoadScript(const char* filename)
{
	if (!Py_IsInitialized()) {
		return false;
	}
	Log(MESSAGE, "GUIScript", "Loading Script %s.", filename);

	PyObject *pName = PyString_FromString( filename );
	/* Error checking of pName left out */
	if (pName == NULL) {
		Log(ERROR, "GUIScript", "Failed to create filename for script \"%s\".", filename);
		return false;
	}

	if (pModule) {
		Py_DECREF( pModule );
	}

	pModule = PyImport_Import( pName );
	Py_DECREF( pName );

	if (pModule != NULL) {
		pDict = PyModule_GetDict( pModule );
		if (PyDict_Merge( pDict, pMainDic, false ) == -1)
			return false;
		/* pDict is a borrowed reference */
	} else {
		PyErr_Print();
		Log(ERROR, "GUIScript", "Failed to load script \"%s\".", filename);
		return false;
	}
	return true;
}

/* Similar to RunFunction, but with parameters, and doesn't necessarily fail */
PyObject *GUIScript::RunFunction(const char* moduleName, const char* functionName, PyObject* pArgs, bool report_error)
{
	if (!Py_IsInitialized()) {
		return NULL;
	}

	PyObject *module;
	if (moduleName) {
		module = PyImport_ImportModule(const_cast<char*>(moduleName));
	} else {
		module = pModule;
		Py_XINCREF(module);
	}
	if (module == NULL) {
		PyErr_Print();
		return NULL;
	}
	PyObject *dict = PyModule_GetDict(module);

	PyObject *pFunc = PyDict_GetItemString(dict, const_cast<char*>(functionName));
	/* pFunc: Borrowed reference */
	if (!pFunc || !PyCallable_Check(pFunc)) {
		if (report_error) {
			Log(ERROR, "GUIScript", "Missing function: %s from %s", functionName, moduleName);
		}
		Py_DECREF(module);
		return NULL;
	}
	PyObject *pValue = PyObject_CallObject( pFunc, pArgs );
	if (pValue == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
	}
	Py_DECREF(module);
	return pValue;
}

bool GUIScript::RunFunction(const char *moduleName, const char* functionName, bool report_error, int intparam)
{
	PyObject *pArgs;
	if (intparam == -1) {
		pArgs = NULL;
	} else {
		pArgs = Py_BuildValue("(i)", intparam);
	}
	PyObject *pValue = RunFunction(moduleName, functionName, pArgs, report_error);
	Py_XDECREF(pArgs);
	if (pValue == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
		return false;
	}
	Py_DECREF(pValue);
	return true;
}

bool GUIScript::RunFunction(const char *moduleName, const char* functionName, bool report_error, Point param)
{
	PyObject *pArgs = Py_BuildValue("(ii)", param.x, param.y);
	PyObject *pValue = RunFunction(moduleName, functionName, pArgs, report_error);
	Py_XDECREF(pArgs);
	if (pValue == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
		return false;
	}
	Py_DECREF(pValue);
	return true;
}

void GUIScript::ExecFile(const char* file)
{
	FileStream fs;
	if (!fs.Open(file))
		return;

	int len = fs.Remains();
	if (len <= 0)
		return;

	char* buffer = (char *) malloc(len+1);
	if (!buffer)
		return;

	if (fs.Read(buffer, len) == GEM_ERROR) {
		free(buffer);
		return;
	}
	buffer[len] = 0;

	ExecString(buffer);
	free(buffer);
}

/** Exec a single String */
void GUIScript::ExecString(const char* string, bool feedback)
{
	PyObject* run = PyRun_String(string, Py_file_input, pMainDic, pMainDic);

	if (run) {
		// success
		if (feedback) {
			PyObject* pyGUI = PyImport_ImportModule("GUICommon");
			if (pyGUI) {
				PyObject* catcher = PyObject_GetAttrString(pyGUI, "outputFunnel");
				if (catcher) {
					PyObject* output = PyObject_GetAttrString(catcher, "lastLine");
					String* msg = StringFromCString(PyString_AsString(output));
					displaymsg->DisplayString(*msg, DMC_WHITE, NULL);
					delete msg;
					Py_DECREF(catcher);
				}
				Py_DECREF(pyGUI);
			}
		}
		Py_DECREF(run);
	} else {
		// failure
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		//Get error message
		String* error = StringFromCString(PyString_AsString(pvalue));
		if (error) {
			displaymsg->DisplayString(L"Error: " + *error, DMC_RED, NULL);
		}
		PyErr_Print();
		Py_DECREF(ptype);
		Py_DECREF(pvalue);
		if (ptraceback) Py_DECREF(ptraceback);
		delete error;
	}
	PyErr_Clear();
}

PyObject* GUIScript::ConstructObject(const char* type, int arg)
{
	PyObject* tuple = PyTuple_New(1);
	PyTuple_SET_ITEM(tuple, 0, PyInt_FromLong(arg));
	PyObject* ret = gs->ConstructObject(type, tuple);
	Py_DECREF(tuple);
	return ret;
}

PyObject* GUIScript::ConstructObject(const char* type, PyObject* pArgs)
{
	char classname[_MAX_PATH] = "G";
	strncat(classname, type, _MAX_PATH - 2);
	if (!pGUIClasses) {
		char buf[256];
		snprintf(buf, sizeof(buf), "Tried to use an object (%.50s) before script compiled!", classname);
		return RuntimeError(buf);
	}

	PyObject* cobj = PyDict_GetItemString( pGUIClasses, classname );
	if (!cobj) {
		char buf[256];
		snprintf(buf, sizeof(buf), "Failed to lookup name '%.50s'", classname);
		return RuntimeError(buf);
	}
	PyObject* ret = PyObject_Call(cobj, pArgs, NULL);
	if (!ret) {
		return RuntimeError("Failed to call constructor");
	}
	return ret;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x1B01BE6B, "GUI Script Engine (Python)")
PLUGIN_CLASS(IE_GUI_SCRIPT_CLASS_ID, GUIScript)
END_PLUGIN()
