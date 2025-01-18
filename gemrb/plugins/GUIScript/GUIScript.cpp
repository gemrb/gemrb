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

#include "Audio.h"
#include "CharAnimations.h"
#include "DataFileMgr.h"
#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "EffectQueue.h"
#include "Game.h"
#include "GameData.h"
#include "ImageFactory.h"
#include "Interface.h"
#include "Item.h"
#include "KeyMap.h"
#include "Map.h"
#include "MusicMgr.h"
#include "Palette.h"
#include "PalettedImageMgr.h"
#include "PythonCallbacks.h"
#include "PythonConversions.h"
#include "PythonErrors.h"
#include "RNG.h"
#include "ResourceDesc.h"
#include "SaveGameIterator.h"
#include "Spell.h"
#include "TileMap.h"
#include "WorldMap.h"

#include "GUI/Button.h"
#include "GUI/Console.h"
#include "GUI/EventMgr.h"
#include "GUI/GUIScriptInterface.h"
#include "GUI/GameControl.h"
#include "GUI/Label.h"
#include "GUI/MapControl.h"
#include "GUI/ScrollBar.h"
#include "GUI/Slider.h"
#include "GUI/TextArea.h"
#include "GUI/TextEdit.h"
#include "GUI/WindowManager.h"
#include "GUI/WorldMapControl.h"
#include "GameScript/GSUtils.h" //checkvariable
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "Streams/FileStream.h"
#include "System/FileFilters.h"
#include "Video/Video.h"

#include <algorithm>
#include <cstdio>

using namespace GemRB;

GUIScript* GemRB::gs = NULL;

// a shorthand for declaring methods in method table
#define METHOD(name, args) \
	{ \
		#name, GemRB_##name, args, GemRB_##name##__doc \
	}

//Check removal/equip/swap of item based on item name and actor's scriptname
#define CRI_REMOVE        0
#define CRI_EQUIP         1
#define CRI_SWAP          2
#define CRI_REMOVEFORSWAP 3

//bit used in SetCreatureStat to access some fields
#define EXTRASETTINGS 0x1000

//maximum distance for passing items between two characters
#define MAX_DISTANCE 500

struct UsedItemType {
	ResRef itemname;
	ieVariable username; //death variable
	std::vector<ieStrRef> feedback;
	int flags;
	ieStrRef GetFeedback() const { return feedback[RAND<size_t>(0, feedback.size() - 1)]; }
};

using EventNameType = FixedSizeString<16>;
#define IS_DROP        0
#define IS_GET         1
#define IS_SWINGOFFSET 2 // offset to the swing sound columns

#define UNINIT_IEDWORD 0xcccccccc

static std::vector<SpellDescType> SpecialItems;
static std::vector<SpellDescType> StoreSpells;

static std::vector<UsedItemType> UsedItems;

//4 action button indices are packed on a single ieDword, there are 32 actions max.
//there are additional fake action buttons
static ieDword GUIAction[MAX_ACT_COUNT] = { UNINIT_IEDWORD };
static ieStrRef GUITooltip[MAX_ACT_COUNT] = { ieStrRef::INVALID };
static ResRef GUIResRef[MAX_ACT_COUNT];
static EventNameType GUIEvent[MAX_ACT_COUNT] {};
static Store* rhstore = NULL;

static EffectRef fx_learn_spell_ref = { "Spell:Learn", -1 };


#define GET_GAME() \
	Game* game = core->GetGame(); \
	if (!game) { \
		return RuntimeError("No game loaded!\n"); \
	}

#define GET_MAP() \
	Map* map = game->GetCurrentArea(); \
	if (!map) { \
		return RuntimeError("No current area!"); \
	}

#define GET_GAMECONTROL() \
	GameControl* gc = core->GetGameControl(); \
	if (!gc) { \
		return RuntimeError("Can't find GameControl!"); \
	}

#define GET_ACTOR_GLOBAL() \
	Actor* actor; \
	if (globalID > 1000) { \
		actor = game->GetActorByGlobalID(globalID); \
	} else { \
		actor = game->FindPC(globalID); \
	} \
	if (!actor) { \
		return RuntimeError("Actor not found!\n"); \
	}

#define PARSE_ARGS(args, fmt, ...) \
	if (!PyArg_ParseTuple(args, fmt, __VA_ARGS__)) { \
		return NULL; \
	}

#define ABORT_IF_NULL(thing) \
	if (thing == nullptr) return RuntimeError(#thing " cannot be null.")

#define RETURN_BOOL(boolean) \
	if (boolean) { \
		Py_RETURN_TRUE; \
	} else { \
		Py_RETURN_FALSE; \
	}

const ScriptingRefBase* GUIScript::GetScriptingRef(PyObject* obj) const
{
	if (!obj || obj == Py_None) {
		return nullptr;
	}

	PyObject* attr = PyObject_GetAttrString(obj, "ID");
	if (!attr) {
		RuntimeError("Invalid Scripting reference, must have ID attribute.");
		return nullptr;
	}
	ScriptingId id = (ScriptingId) PyLong_AsUnsignedLongLong(attr);
	Py_DecRef(attr);

	attr = PyObject_GetAttrString(obj, "SCRIPT_GROUP");
	if (!attr) {
		RuntimeError("Invalid Scripting reference, must have SCRIPT_GROUP attribute.");
		return nullptr;
	}

	ScriptingGroup_t group = ASCIIStringFromPy<ScriptingGroup_t>(attr);
	Py_DecRef(attr);

	return ScriptEngine::GetScriptingRef(group, id);
}

template<class RETURN = View>
static RETURN* GetView(PyObject* obj)
{
	auto ref = gs->GetScriptingRef(obj);
	if (!ref) {
		PyErr_Clear();
		return nullptr;
	}
	return ScriptingRefCast<RETURN>(ref);
}

static bool StatIsASkill(unsigned int StatID)
{
	// traps, lore, stealth, lockpicking, pickpocket
	if (StatID >= IE_LORE && StatID <= IE_PICKPOCKET) return true;

	// alchemy, animals, bluff, concentration, diplomacy, intimidate, search, spellcraft, magicdevice
	// NOTE: change if you want to use IE_PROFICIENCYCLUB or IE_EXTRAPROFICIENCY2 etc., as they use the same values
	if (StatID >= IE_ALCHEMY && StatID <= IE_MAGICDEVICE) return true;

	// Hide, Wilderness_Lore
	if (StatID == IE_HIDEINSHADOWS || StatID == IE_TRACKING) return true;

	return false;
}

static int GetCreatureStat(const Actor* actor, unsigned int StatID, int Mod)
{
	//this is a hack, if more PCStats fields are needed, improve it
	if (StatID & EXTRASETTINGS) {
		const auto& ps = actor->PCStats;
		if (!ps) {
			//the official invalid value in GetStat
			return 0xdadadada;
		}
		StatID &= 15;
		return ps->ExtraSettings[StatID];
	}
	if (Mod) {
		if (core->HasFeature(GFFlags::RULES_3ED) && StatIsASkill(StatID)) {
			return actor->GetSkill(StatID);
		} else {
			if (StatID != IE_HITPOINTS || actor->HasVisibleHP()) {
				return actor->GetStat(StatID);
			} else {
				return 0xdadadada;
			}
		}
	}
	return actor->GetBase(StatID);
}

static int SetCreatureStat(Actor* actor, unsigned int StatID, int StatValue, bool pcf)
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
	if (StatID & EXTRASETTINGS) {
		const auto& ps = actor->PCStats;
		if (!ps) {
			return 0;
		}
		StatID &= 15;
		ps->ExtraSettings[StatID] = StatValue;
		actor->ApplyExtraSettings();
		return 1;
	}

	if (pcf) {
		actor->SetBase(StatID, StatValue);
	} else {
		actor->SetBaseNoPCF(StatID, StatValue);
	}
	actor->CreateDerivedStats();
	return 1;
}

PyDoc_STRVAR(GemRB_GetGameString__doc,
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
**See also:** [GetSystemVariable](GetSystemVariable.md), [GetToken](GetToken.md)");

static PyObject* GemRB_GetGameString(PyObject*, PyObject* args)
{
	int Index = -1;
	PARSE_ARGS(args, "i", &Index);

	switch (Index & 0xf0) {
		case 0: //game strings
			const Game* game = core->GetGame();
			if (!game) {
				return PyString_FromString("");
			}
			switch (Index & 15) {
				case 0: // STR_LOADMOS
					return PyString_FromResRef(game->LoadMos);
				case 1: // STR_AREANAME
					return PyString_FromResRef(game->CurrentArea);
				case 2: // STR_TEXTSCREEN
					return PyString_FromResRef(game->TextScreen);
			}
	}

	return NULL;
}

PyDoc_STRVAR(GemRB_LoadGame__doc,
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
**Examples:**\n\
\n\
    GemRB.SetVar ('PlayMode', 0)\n\
    GemRB.LoadGame (-1, 22)\n\
\n\
**See also:** [EnterGame](EnterGame.md), [CreatePlayer](CreatePlayer.md), [SetVar](SetVar.md), [SaveGame](SaveGame.md)\n\
");

static PyObject* GemRB_LoadGame(PyObject*, PyObject* args)
{
	PyObject* obj = NULL;
	int VersionOverride = 0;
	if (PyArg_ParseTuple(args, "O|i", &obj, &VersionOverride)) {
		CObject<SaveGame> save(obj);
		core->SetupLoadGame(save, VersionOverride);
		Py_RETURN_NONE;
	}
	return NULL;
}

PyDoc_STRVAR(GemRB_EnterGame__doc,
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
**See also:** [QuitGame](QuitGame.md), [LoadGame](LoadGame.md), [SetNextScript](SetNextScript.md)");

static PyObject* GemRB_EnterGame(PyObject*, PyObject* /*args*/)
{
	core->QuitFlag |= QF_ENTERGAME;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_QuitGame__doc,
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
**See also:** [EnterGame](EnterGame.md), [Quit](Quit.md), [SetNextScript](SetNextScript.md), [HideGUI](HideGUI.md)\n\
");
static PyObject* GemRB_QuitGame(PyObject*, PyObject* /*args*/)
{
	core->QuitFlag = QF_QUITGAME;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_TextArea_SetChapterText__doc,
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
**Examples:**\n\
\n\
    TextArea = ChapterWindow.GetControl (5)\n\
    TextArea.SetChapterText (text)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:**");

static PyObject* GemRB_TextArea_SetChapterText(PyObject* self, PyObject* args)
{
	PyObject* text = nullptr;
	PARSE_ARGS(args, "OO", &self, &text);

	TextArea* ta = GetView<TextArea>(self);
	ABORT_IF_NULL(ta);

	ta->ClearText();

	// insert enough newlines to push the text offscreen
	auto margins = ta->GetMargins();
	int rowHeight = ta->LineHeight();
	int h = ta->Frame().h - (margins.top + margins.bottom);
	int w = ta->Frame().w - (margins.left + margins.right);
	int newlines = CeilDiv(h, rowHeight);
	ta->AppendText(String(newlines - 1, L'\n'));
	ta->AppendText(PyString_AsStringObj(text));
	// append again (+1 since there may not be a trailing newline) after the chtext so it will scroll out of view
	ta->AppendText(String(newlines + 1, L'\n'));

	ta->SetFlags(View::IgnoreEvents, BitOp::OR);
	int lines = ta->ContentHeight() / rowHeight;
	float heightScale = 12.0f / float(rowHeight); // scale based on text size so smaller text scrolls more slowly
	float widthScale = 640.0f / float(w); // scale based on width to become more slow as we get wider
	float textSpeed = static_cast<float>(gamedata->GetTextSpeed());
	int ticksPerLine = int(11.0f * heightScale * widthScale * textSpeed);
	ta->ScrollToY(-ta->ContentHeight(), lines * ticksPerLine);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetString__doc,
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
**Return value:** A string with resolved tokens.\
\n\
**Examples:**\n\
\n\
     Level = GemRB.GetPlayerStat (pc, IE_LEVEL) # 1 at character generation\n\
     Label.SetText (GemRB.GetString(12137) + str(Level)) \n\
\n\
The above example will display 'Level: 1' in the addressed label.\n\
\n\
**See also:** [Control_SetText](Control_SetText.md)\n\
");

static PyObject* GemRB_GetString(PyObject* /*self*/, PyObject* args)
{
	PyObject* strref = nullptr;
	int flags = 0;
	PARSE_ARGS(args, "O|i", &strref, &flags);

	String text = core->GetString(StrRefFromPy(strref), STRING_FLAGS(flags));
	return PyString_FromStringObj(text);
}

PyDoc_STRVAR(GemRB_EndCutSceneMode__doc,
	     "===== EndCutSceneMode =====\n\
\n\
**Prototype:** GemRB.EndCutSceneMode ()\n\
\n\
**Description:** Exits the CutScene Mode. It is similar to the gamescript \n\
command of the same name. It gives back the cursor and shows the game GUI \n\
windows hidden by the CutSceneMode() gamescript action. \n\
This is mainly a debugging command.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A\n\
");

static PyObject* GemRB_EndCutSceneMode(PyObject* /*self*/, PyObject* /*args*/)
{
	core->SetCutSceneMode(false);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_LoadWindow__doc,
	     "===== LoadWindow =====\n\
\n\
**Prototype:** GemRB.LoadWindow (WindowID, windowPack[, position])\n\
\n\
**Description:** Returns a Window. You must call LoadWindowPack before using \n\
this command. The window won't be displayed. If LoadWindowPack() set nonzero \n\
natural screen size with Width and Height parameters, the loaded window is \n\
then moved by (screen size - winpack size) / 2\n\
\n\
**Parameters:**\n\
  * a window ID, see the .chu file specification\n\
  * windowPack: which window pack (.chu) to take the window from\n\
  * position: where to place it on screen (defaults to WINDOW_CENTER, see GUIDefines.py)\n\
\n\
**Return value:** GWindow\n\
\n\
**See also:** [LoadWindowPack](LoadWindowPack.md), [Window_GetControl](Window_GetControl.md), [Window_ShowModal](Window_ShowModal.md)");

static PyObject* GemRB_LoadWindow(PyObject* /*self*/, PyObject* args)
{
	int WindowID = -1;
	Window::WindowPosition pos = Window::PosCentered;
	char* ref = NULL;
	PARSE_ARGS(args, "is|i", &WindowID, &ref, &pos);

	Window* win = core->GetWindowManager()->LoadWindow(WindowID, ScriptingGroup_t(ref), pos);
	ABORT_IF_NULL(win);
	win->SetFlags(Window::AlphaChannel, BitOp::OR);
	return gs->ConstructObjectForScriptable(win->GetScriptingRef());
}

PyDoc_STRVAR(GemRB_EnableCheatKeys__doc,
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
**See also:** GameControl.cpp for actual cheat key functions");

static PyObject* GemRB_EnableCheatKeys(PyObject* /*self*/, PyObject* args)
{
	int Flag = core->CheatEnabled();
	PARSE_ARGS(args, "i", &Flag);
	core->EnableCheatKeys(Flag);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_LoadTable__doc,
	     "===== LoadTable =====\n\
\n\
**Prototype:** GemRB.LoadTable (2DAResRef[, ignore_error=0, silent=0])\n\
\n\
**Description:** Loads a 2DA Table. In case it was already loaded, it \n\
will return the table's existing reference (won't load it again).\n\
\n\
**Parameters:** \n\
  * 2DAResRef    - the table's name (.2da resref)\n\
  * ignore_error - boolean, if set, handle missing files gracefully\n\
  * silent - boolean, if set, don't print lookup messages\n\
\n\
**Return value:** GTable\n\
\n\
**See also:** [UnloadTable](UnloadTable.md), [LoadSymbol](LoadSymbol.md)");

static PyObject* GemRB_LoadTable(PyObject* /*self*/, PyObject* args)
{
	PyObject* tablename = nullptr;
	int noerror = 0;
	int silent = 0;
	PARSE_ARGS(args, "O|ii", &tablename, &noerror, &silent);

	auto tab = gamedata->LoadTable(ResRefFromPy(tablename), silent > 0);
	if (tab == nullptr) {
		if (noerror) {
			Py_RETURN_NONE;
		} else {
			return RuntimeError("Can't find resource");
		}
	}

	return PyObject_FromHolder<TableMgr>(std::move(tab));
}

PyDoc_STRVAR(GemRB_Table_GetValue__doc,
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
**See also:** [GetSymbolValue](GetSymbolValue.md), [Table_FindValue](Table_FindValue.md), [LoadTable](LoadTable.md)");

static PyObject* GemRB_Table_GetValue(PyObject* self, PyObject* args)
{
	PyObject* row = nullptr;
	PyObject* col = nullptr;
	int type = -1;
	PARSE_ARGS(args, "OOO|i", &self, &row, &col, &type);

	AutoTable tm = CObject<TableMgr, std::shared_ptr>(self);
	ABORT_IF_NULL(tm);

	if (row != Py_None && col != Py_None && !PyObject_TypeCheck(row, Py_TYPE(col))) {
		return AttributeError("RowIndex/RowString and ColIndex/ColString must be the same type.");
	}

	auto GetIndex = [&tm](PyObject* obj, bool isRow) -> TableMgr::index_t {
		if (PyUnicode_Check(obj)) {
			if (isRow) {
				return tm->GetRowIndex(PyString_AsStringView(obj));
			}
			return tm->GetColumnIndex(PyString_AsStringView(obj));
		} else if (PyLong_Check(obj)) {
			return static_cast<TableMgr::index_t>(PyLong_AsLong(obj));
		}
		return -1; // will read default value
	};

	TableMgr::index_t rowIdx = GetIndex(row, true);
	TableMgr::index_t colIdx = GetIndex(col, false);

	std::string ret = tm->QueryField(rowIdx, colIdx);

	switch (type) {
		case 0: // string
			return PyString_FromStringObj(ret);
		case 2:
			return PyLong_FromLong(core->TranslateStat(ret));
		default:
			long val;
			bool valid = valid_signednumber(ret.c_str(), val);
			if (type == 3) {
				String str = core->GetString(ieStrRef(val));
				return PyString_FromStringObj(str);
			}
			if (valid || type == 1) {
				return PyLong_FromLong(val);
			}
			// else return string
			return PyString_FromStringObj(ret);
	}
}

PyDoc_STRVAR(GemRB_Table_FindValue__doc,
	     "===== Table_FindValue =====\n\
\n\
**Prototype:** GemRB.FindTableValue (TableIndex, ColumnIndex, Value[, StartRow])\n\
\n\
**Metaclass Prototype:** FindValue (ColumnIndex, Value[, StartRow])\n\
\n\
**Description:** Returns the first row index of a field value in a 2DA \n\
Table. If StartRow is omitted, the search starts from the beginning.\n\
\n\
**Parameters:**\n\
  * TableIndex - integer, returned by a previous LoadTable command.\n\
  * Column - index or name of the column in which to look for value.\n\
  * Value - value to find in the table\n\
  * StartRow - integer, starting row (offset)\n\
\n\
**Return value:** numeric, None if the value isn't to be found\n\
\n\
**See also:** [LoadTable](LoadTable.md), [Table_GetValue](Table_GetValue.md)");

static PyObject* GemRB_Table_FindValue(PyObject* self, PyObject* args)
{
	int col;
	int start = 0;
	long Value;
	PyObject* colname = nullptr;
	PyObject* strvalue = nullptr;

	if (!PyArg_ParseTuple(args, "Oil|i", &self, &col, &Value, &start)) {
		col = -1;
		if (!PyArg_ParseTuple(args, "OOl|i", &self, &colname, &Value, &start)) {
			col = -2;
			PARSE_ARGS(args, "OOO|i", &self, &colname, &strvalue, &start);
		}
		PyErr_Clear(); //clearing the exception
	}

	AutoTable tm = CObject<TableMgr, std::shared_ptr>(self);
	ABORT_IF_NULL(tm);

	TableMgr::index_t val = TableMgr::npos;
	if (col == -1) {
		val = tm->FindTableValue(PyString_AsStringView(colname), Value, start);
	} else if (col == -2) {
		val = tm->FindTableValue(PyString_AsStringView(colname), PyString_AsStringView(strvalue), start);
	} else {
		val = tm->FindTableValue(col, Value, start);
	}

	if (val == TableMgr::npos) {
		Py_RETURN_NONE;
	}
	return PyLong_FromLong(val);
}

PyDoc_STRVAR(GemRB_Table_GetRowIndex__doc,
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
**Return value:** numeric, None if row doesn't exist\n\
\n\
**See also:** [LoadTable](LoadTable.md)");

static PyObject* GemRB_Table_GetRowIndex(PyObject* self, PyObject* args)
{
	PyObject* rowname;
	PARSE_ARGS(args, "OO", &self, &rowname);

	AutoTable tm = CObject<TableMgr, std::shared_ptr>(self);
	ABORT_IF_NULL(tm);

	TableMgr::index_t row = tm->GetRowIndex(PyString_AsStringView(rowname));
	if (row == TableMgr::npos) {
		Py_RETURN_NONE;
	}
	return PyLong_FromLong(row);
}

PyDoc_STRVAR(GemRB_Table_GetRowName__doc,
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
**See also:** [LoadTable](LoadTable.md), [Table_GetColumnName](Table_GetColumnName.md)");

static PyObject* GemRB_Table_GetRowName(PyObject* self, PyObject* args)
{
	int row;
	PARSE_ARGS(args, "Oi", &self, &row);

	AutoTable tm = CObject<TableMgr, std::shared_ptr>(self);
	ABORT_IF_NULL(tm);

	const std::string& str = tm->GetRowName(row);
	return PyString_FromStringObj(str);
}

PyDoc_STRVAR(GemRB_Table_GetColumnIndex__doc,
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
**Return value:** numeric, None if column doesn't exist\n\
\n\
**See also:** [LoadTable](LoadTable.md), [Table_GetRowIndex](Table_GetRowIndex.md)");

static PyObject* GemRB_Table_GetColumnIndex(PyObject* self, PyObject* args)
{
	PyObject* colname;
	PARSE_ARGS(args, "OO", &self, &colname);

	AutoTable tm = CObject<TableMgr, std::shared_ptr>(self);
	ABORT_IF_NULL(tm);

	TableMgr::index_t col = tm->GetColumnIndex(PyString_AsStringView(colname));
	if (col == TableMgr::npos) {
		Py_RETURN_NONE;
	}
	return PyLong_FromLong(col);
}

PyDoc_STRVAR(GemRB_Table_GetColumnName__doc,
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
**See also:** [LoadTable](LoadTable.md), [Table_GetRowName](Table_GetRowName.md)");

static PyObject* GemRB_Table_GetColumnName(PyObject* self, PyObject* args)
{
	int col;
	PARSE_ARGS(args, "Oi", &self, &col);

	AutoTable tm = CObject<TableMgr, std::shared_ptr>(self);
	ABORT_IF_NULL(tm);

	const char* str = tm->GetColumnName(col).c_str();
	ABORT_IF_NULL(str);

	return PyString_FromString(str);
}

PyDoc_STRVAR(GemRB_Table_GetRowCount__doc,
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
**See also:** [LoadTable](LoadTable.md), [Table_GetColumnCount](Table_GetColumnCount.md)");

static PyObject* GemRB_Table_GetRowCount(PyObject* self, PyObject* args)
{
	PARSE_ARGS(args, "O", &self);

	AutoTable tm = CObject<TableMgr, std::shared_ptr>(self);
	ABORT_IF_NULL(tm);

	return PyLong_FromLong(tm->GetRowCount());
}

PyDoc_STRVAR(GemRB_Table_GetColumnCount__doc,
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
**See also:** [LoadTable](LoadTable.md), [Table_GetRowCount](Table_GetRowCount.md)");

static PyObject* GemRB_Table_GetColumnCount(PyObject* self, PyObject* args)
{
	int row = 0;
	PARSE_ARGS(args, "O|i", &self, &row);

	AutoTable tm = CObject<TableMgr, std::shared_ptr>(self);
	ABORT_IF_NULL(tm);

	return PyLong_FromLong(tm->GetColumnCount(row));
}

PyDoc_STRVAR(GemRB_LoadSymbol__doc,
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
**Return value:** GSymbol object, None if loading failed\n\
\n\
**See also:** [UnloadSymbol](UnloadSymbol.md)\n\
");

static PyObject* GemRB_LoadSymbol(PyObject* /*self*/, PyObject* args)
{
	PyObject* string = nullptr;
	PARSE_ARGS(args, "O", &string);

	int ind = core->LoadSymbol(ResRefFromPy(string));
	if (ind == -1) {
		Py_RETURN_NONE;
	}

	return gs->ConstructObject("Symbol", ind);
}

PyDoc_STRVAR(GemRB_Symbol_GetValue__doc,
	     "===== GetSymbolValue =====\n\
\n\
**Prototype:** GemRB.GetSymbolValue (GSymbol, StringVal|IntVal)\n\
\n\
**Metaclass Prototype:** GetValue (StringVal|IntVal)\n\
\n\
**Description:** Returns a field of a IDS Symbol Table.\n\
\n\
**Parameters:**\n\
  * GSymbol - returned by a previous LoadSymbol command\n\
  * StringVal - name of the symbol to resolve (first column of .ids file)\n\
  * IntVal - value of the symbol to find (second column of .ids file)\n\
\n\
**Return value:**\n\
  * numeric, if the symbol's name was given (the value of the symbol)\n\
  * string, if the value of the symbol was given (the symbol's name)\n\
\n\
**Examples:**\n\
\n\
    align = GemRB.GetPlayerStat (pc, IE_ALIGNMENT)\n\
    ss = GemRB.LoadSymbol ('ALIGN')\n\
    sym = GemRB.GetSymbolValue (ss, align)\n\
\n\
The above example will find the symbolic name of the player's alignment.\n\
\n\
**See also:** [LoadSymbol](LoadSymbol.md), [Table_GetValue](Table_GetValue.md)");

static PyObject* GemRB_Symbol_GetValue(PyObject* self, PyObject* args)
{
	PyObject* sym;

	PARSE_ARGS(args, "OO", &self, &sym);

	auto sm = GetSymbols(self);

	if (!sm) {
		return AttributeError("No such symbols");
	}

	if (PyObject_TypeCheck(sym, &PyUnicode_Type)) {
		long val = sm->GetValue(PyString_AsStringView(sym));
		return PyLong_FromLong(val);
	}
	if (PyObject_TypeCheck(sym, &PyLong_Type)) {
		int symi = static_cast<int>(PyLong_AsLong(sym));
		return PyString_FromStringView(sm->GetValue(symi));
	}

	return RuntimeError("Invalid ags");
}

PyDoc_STRVAR(GemRB_View_AddSubview__doc,
	     "===== View_AddSubview =====\n\
\n\
**Prototype:** View_AddSubview (GView, subview [,siblingView=None, id=-1])\n\
\n\
**Metaclass Prototype:** AddSubview (subview[, siblingView=None, id=-1])\n\
\n\
**Description:** Adds a view to a new superview in front of siblingView (or the back if None), removing it from its previous superview (if any).\n\
\n\
**Parameters:**\n\
  * GView - the control's (superview's) reference\n\
  * subview - View to add\n\
  * siblingView - View to add\n\
  * id - assign this numeric ID to the new view (useful if it's already taken)\n\
\n\
**Examples:**\n\
\n\
    RaceWindow.AddSubview (ScrollBarControl)\n\
\n\
**Return value:** the new View");

static PyObject* GemRB_View_AddSubview(PyObject* self, PyObject* args)
{
	PyObject* pySubview = NULL;
	PyObject* pySiblingView = Py_None;
	PyObject* pyid = NULL; // if we were moving a view from one window to another we may need to pass this to avoid conflicts
	PARSE_ARGS(args, "OO|OO", &self, &pySubview, &pySiblingView, &pyid);

	ScriptingId id = pyid ? (ScriptingId) PyLong_AsUnsignedLongLong(pyid) : ScriptingId(-1);

	const ViewScriptingRef* ref = dynamic_cast<const ViewScriptingRef*>(gs->GetScriptingRef(pySubview));
	assert(ref);

	View* superView = GetView<View>(self);
	View* subView = ref->GetObject();
	const View* siblingView = GetView<View>(pySiblingView);
	if (superView && subView) {
		PyObject* attr = PyObject_GetAttrString(pySubview, "SCRIPT_GROUP");

		const Window* oldwin = subView->GetWindow();
		superView->AddSubviewInFrontOfView(subView, siblingView);

		const ControlScriptingRef* cref = dynamic_cast<const ControlScriptingRef*>(ref);

		ScriptingGroup_t grp = ASCIIStringFromPy<ScriptingGroup_t>(attr);
		if (cref == nullptr) {
			// plain old view
			if (id != ScriptingId(-1)) {
				const ViewScriptingRef* newref = subView->AssignScriptingRef(id, "VIEW");
				return gs->ConstructObjectForScriptable(newref);
			}
			// return the ref we already have
			Py_IncRef(pySubview);
			return pySubview;
		} else if (grp == "__DEL__") {
			if (id == ScriptingId(-1)) {
				return RuntimeError("Cannot add deleted view without a valid id parameter.");
			}
			// replace the ref with a new one and return it
			const ControlScriptingRef* newref = RegisterScriptableControl(static_cast<Control*>(subView), id, cref);
			return gs->ConstructObjectForScriptable(newref);
		} else if (!oldwin || id != ScriptingId(-1)) {
			// create a new reference and return it
			ScriptingId sid = (id == ScriptingId(-1)) ? cref->Id : id;
			const ControlScriptingRef* newref = RegisterScriptableControl(static_cast<Control*>(subView), sid);
			return gs->ConstructObjectForScriptable(newref);
		} else {
			// return the ref we already have
			Py_IncRef(pySubview);
			return pySubview;
		}
	}

	return AttributeError("Invalid view parameters.");
}

PyDoc_STRVAR(GemRB_View_AddAlias__doc,
	     "===== View_AddAlias =====\n\
\n\
**Prototype:** View_AddAlias (GView, AliasGroup[, AliasID, Overwrite])\n\
\n\
**Metaclass Prototype:** AddAlias (AliasGroup[, AliasID, Overwrite])\n\
\n\
**Description:** Adds an additional entry to the Scripting engine under\n\
AliasGroup with AliasID and binds it to the view, optionally\n\
overwriting an existing entry.\n\
\n\
**Parameters:**\n\
  * GView - the control's reference\n\
  * AliasGroup - View group\n\
  * AliasID - force to this alias numeric ID\n\
  * Overwrite - overwrite any existing alias\n\
\n\
**Examples:**\n\
\n\
    PortraitWindow.AddAlias ('HIDE_CUT', 3)\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_View_AddAlias(PyObject* self, PyObject* args)
{
	char* cstr = NULL;
	// default to the lowest valid value (since its optional and we often only want 1 control per alias)
	// the exception being for creating groups such as the GAMEGUI windows for quickly hiding/showing the entire group
	ScriptingId controlId = 0;
	int overwrite = false;
	PARSE_ARGS(args, "Os|li", &self, &cstr, &controlId, &overwrite);

	const ScriptingGroup_t group = ScriptingGroup_t(cstr);
	View* view = GetView<View>(self);
	ABORT_IF_NULL(view);
	if (overwrite) {
		const ViewScriptingRef* delref = static_cast<const ViewScriptingRef*>(ScriptEngine::GetScriptingRef(group, controlId));
		if (delref) {
			delref = delref->GetObject()->RemoveScriptingRef(delref);
			ABORT_IF_NULL(delref);
		}
	}
	const ViewScriptingRef* ref = view->AssignScriptingRef(controlId, group);
	ABORT_IF_NULL(ref);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetView__doc,
	     "===== GetView =====\n\
\n\
**Prototype:** GetView (GView[, ID])\n\
\n\
**Metaclass Prototype:** /\n\
\n\
**Description:** Lookup view from either a window or from an alias.\n\
**Parameters:**\n\
  * GView - the window's reference or a string with the alias\n\
  * ID - look for a view with a specific numeric ID\n\
\n\
**Examples:**\n\
\n\
    GemRB.GetView ('ACTWIN')\n\
\n\
**Return value:** View");

static PyObject* GemRB_GetView(PyObject* /*self*/, PyObject* args)
{
	PyObject* pyid = nullptr;
	PyObject* lookup = nullptr;
	PARSE_ARGS(args, "O|O", &lookup, &pyid);

	// for convenience we allow an alias to default to the lowest valid id
	// the typical use case is typically wanting to specify a string name for a single control
	ScriptingId id = 0;
	if (pyid && pyid != Py_None) {
		id = PyLong_AsUnsignedLong(pyid);
		if (PyErr_Occurred()) {
			return nullptr;
		}
	}

	const View* view = nullptr;
	if (PyUnicode_Check(lookup)) {
		const ScriptingGroup_t group = ASCIIStringFromPy<ScriptingGroup_t>(lookup);
		view = ScriptingRefCast<View>(ScriptEngine::GetScriptingRef(group, id));
	} else {
		const Window* win = GetView<Window>(lookup);
		if (win) {
			view = GetControl(id, win);
		}
	}

	if (view) {
		// return retView->GetScriptingRef() so that Python objects compare correctly (instead of returning the alias ref)
		return gs->ConstructObjectForScriptable(view->GetScriptingRef());
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Scrollable_Scroll__doc,
	     "===== Scrollable_Scroll =====\n\
\n\
**Prototype:** Scrollable_Scroll (GView, x, y[, relative])\n\
\n\
**Metaclass Prototype:** Scroll (x, y[, relative])\n\
\n\
**Description:** Scrolls a scrollable View.\n\
\n\
**Parameters:**\n\
  * GView - the control's reference\n\
  * x - x coordinate of point to scroll to\n\
  * y - y coordinate of point to scroll to\n\
  * relative - optional, set if you don't want an absolute scroll\n\
\n\
**Examples:**\n\
\n\
    WorldMapControl.Scroll (0, 0, False)\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_Scrollable_Scroll(PyObject* self, PyObject* args)
{
	int relative = 0;
	Point p;
	PARSE_ARGS(args, "Oii|i", &self, &p.x, &p.y, &relative);

	View* view = GetView(self);
	View::Scrollable* scroller = dynamic_cast<View::Scrollable*>(view);
	ABORT_IF_NULL(scroller);

	if (relative) {
		scroller->ScrollDelta(p);
	} else {
		scroller->ScrollTo(p);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Control_SetColor__doc,
	     "===== Control_SetColor =====\n\
\n\
**Metaclass Prototype:** SetColor (Color[, Index])\n\
\n\
**Description:** Set the control's desired text color as a rgb dict.\n\
Specifics depend on control type:\n\
  * text area: set the text color corresponding to the index, which is from GUIDefines:\n\
    * TA_COLOR_NORMAL: text color\n\
    * TA_COLOR_INITIALS: color of the artful initial\n\
    * TA_COLOR_BACKGROUND: text background color\n\
    * TA_COLOR_OPTIONS: color of pick-one selection options\n\
    * TA_COLOR_HOVER: color of options on hover\n\
    * TA_COLOR_SELECTED: color of the selected option\n\
  * button: set the text color (same as passing TA_COLOR_NORMAL)\n\
    * TA_COLOR_BACKGROUND: text background color (only for fonts that have background enabled in fonts.2da)\n\
  * label: set the text color and enable color mode (black background by default)\n\
\n\
**Parameters:**\n\
  * Color - Python dictionary of r,g,b,a color values\n\
  * Index - the COLOR_TYPE\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_Control_SetColor(PyObject* self, PyObject* args)
{
	PyObject* pyColor;
	TextArea::COLOR_TYPE colorType = TextArea::COLOR_NORMAL;
	PARSE_ARGS(args, "OO|i", &self, &pyColor, &colorType);

	const Control* ctrl = GetView<Control>(self);
	ABORT_IF_NULL(ctrl);
	const Color color = ColorFromPy(pyColor);

	if (ctrl->ControlType == IE_GUI_BUTTON) {
		Button* button = GetView<Button>(self);
		button->SetTextColor(color, colorType == TextArea::COLOR_BACKGROUND);
	} else if (ctrl->ControlType == IE_GUI_LABEL) {
		Label* label = GetView<Label>(self);
		label->SetColors(color, ColorBlack);
		label->SetFlags(Label::UseColor, BitOp::OR);
	} else if (ctrl->ControlType == IE_GUI_TEXTAREA) {
		TextArea* textArea = GetView<TextArea>(self);
		textArea->SetColor(color, colorType);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Control_QueryText__doc,
	     "===== Control_QueryText =====\n\
 \n\
**Metaclass Prototype:** QueryText ()\n\
 \n\
 **Description:** Returns the Text of a TextEdit/TextArea/Label control. \n\
 In case of a TextArea, it will return the selected row, not the entire \n\
 textarea.\n\
 \n\
 **Parameters:** N/A\n\
 \n\
 **Return value:** string, may be empty\n\
 \n\
**Examples:**\n\
\n\
    Name = NameField.QueryText ()\n\
    GemRB.SetToken ('CHARNAME', Name)\n\
\n\
The above example retrieves the character's name typed into the TextEdit control and stores it in a Token (a string variable accessible to gamescripts, the engine core and to the guiscripts too).\n\
\n\
    GemRB.SetToken ('VoiceSet', TextAreaControl.QueryText ())\n\
\n\
The above example sets the VoiceSet token to the value of the selected string in a TextArea control. Later this voiceset could be stored in the character sheet.\n\
\n\
 **See also:** [Control_SetText](Control_SetText.md), [SetToken](SetToken.md)");

static PyObject* GemRB_Control_QueryText(PyObject* self, PyObject* args)
{
	PARSE_ARGS(args, "O", &self);
	const Control* ctrl = GetView<Control>(self);
	ABORT_IF_NULL(ctrl);

	return PyString_FromStringObj(ctrl->QueryText());
}

PyDoc_STRVAR(GemRB_TextEdit_SetBufferLength__doc,
	     "===== TextEdit_SetBufferLength =====\n\
\n\
**Metaclass Prototype:** SetBufferLength (Length)\n\
\n\
**Description:**  Sets the maximum text length of a TextEdit control. It \n\
cannot be more than 65535.\n\
\n\
**Parameters:**\n\
  * Length - the maximum text length\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_TextEdit_SetBufferLength(PyObject* self, PyObject* args)
{
	int length;
	PARSE_ARGS(args, "Oi", &self, &length);

	TextEdit* te = GetView<TextEdit>(self);
	ABORT_IF_NULL(te);

	te->SetBufferLength(length);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Control_SetText__doc,
	     "===== Control_SetText =====\n\
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
  * String - an arbitrary string\n\
  * Strref - a string index from the dialog.tlk table.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Control_QueryText](Control_QueryText.md), [DisplayString](DisplayString.md), [Window_GetControl](Window_GetControl.md)");

static PyObject* GemRB_Control_SetText(PyObject* self, PyObject* args)
{
	PyObject* str;
	PARSE_ARGS(args, "OO", &self, &str);

	Control* ctrl = GetView<Control>(self);
	if (!ctrl) {
		return RuntimeError("Invalid Control");
	}

	if (PyObject_TypeCheck(str, &PyLong_Type)) { // strref
		ieStrRef StrRef = StrRefFromPy(str);
		ctrl->SetText(core->GetString(StrRef));
	} else if (str == Py_None) {
		// clear the text
		ctrl->SetText(u"");
	} else if (PyObject_TypeCheck(str, &PyByteArray_Type)) { // state font
		static const EncodingStruct enc { "ISO-8859-1", false, false, false }; // ISO-8859-1 is extended ASCII, which we need for state fonts
		const char* tmp = PyByteArray_AS_STRING(str);
		ctrl->SetText(StringFromEncodedView(StringView(tmp), enc));
	} else { // string value of the object
		ctrl->SetText(PyString_AsStringObj(str));
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_TextArea_Append__doc,
	     "===== TextArea_Append =====\n\
\n\
**Metaclass Prototype:** Append (String|Strref [, Row[, Flag]])\n\
\n\
**Description:** Appends the Text to the TextArea Control in the Window. \n\
If row is specified, it can also append text to existing rows.\n\
\n\
**Parameters:**\n\
  * String - literal text, it could have embedded colour codes\n\
  * Strref - a string index from the dialog.tlk table.\n\
  * Row - the row of text to add the text to, if omitted, the text will be added (in a new row) after the last row.\n\
  * Flag - the flags for QueryText (if strref resolution is used)\n\
    * 1 - strrefs displayed (even if not enabled by default)\n\
    * 2 - sound (plays the associated sound)\n\
    * 4 - speech (works only with if sound was set)\n\
    * 256 - strrefs not displayed (even if allowed by default)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Control_SetText](Control_SetText.md), [Control_QueryText](Control_QueryText.md)");

static PyObject* GemRB_TextArea_Append(PyObject* self, PyObject* args)
{
	PyObject* pystr;
	ieDword flags = 0;
	PARSE_ARGS(args, "OO|i", &self, &pystr, &flags);

	TextArea* ta = GetView<TextArea>(self);
	ABORT_IF_NULL(ta);

	if (PyObject_TypeCheck(pystr, &PyUnicode_Type)) {
		ta->AppendText(PyString_AsStringObj(pystr));
	} else if (PyObject_TypeCheck(pystr, &PyLong_Type)) {
		ta->AppendText(core->GetString(StrRefFromPy(pystr), STRING_FLAGS(flags)));
	}

	Py_RETURN_NONE;
}

static inline void SetViewTooltipFromRef(View* view, ieStrRef ref)
{
	String string = core->GetString(ref);
	if (view) {
		view->SetTooltip(string);
	}
}

PyDoc_STRVAR(GemRB_View_SetTooltip__doc,
	     "===== Control_SetTooltip =====\n\
\n\
**Prototype:** GemRB.SetTooltip (GView, String|Strref[, Function])\n\
\n\
**Metaclass Prototype:** SetTooltip (String|Strref[, Function])\n\
\n\
**Description:** Sets view's tooltip. Any view may have a tooltip.\n\
\n\
The tooltip's visual properties must be set in the gemrb.ini file:\n\
  * TooltipFont - Font used to display tooltips\n\
  * TooltipBack - Sprite displayed behind the tooltip text, if any\n\
  * TooltipMargin - Space between tooltip text and sides of TooltipBack (x2)\n\
\n\
**Parameters:**\n\
  * GView - the view's reference\n\
  * String - an arbitrary string\n\
  * Strref - a string index from the dialog.tlk table.\n\
  * Function - (optional) function key to prepend\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Control_SetText](Control_SetText.md)");

static PyObject* GemRB_View_SetTooltip(PyObject* self, PyObject* args)
{
	PyObject* str;
	PARSE_ARGS(args, "OO", &self, &str);

	View* view = GetView<View>(self);
	if (!view) {
		return RuntimeError("Cannot find view!");
	}

	if (PyObject_TypeCheck(str, &PyUnicode_Type)) {
		view->SetTooltip(PyString_AsStringObj(str));
	} else {
		ieStrRef StrRef = StrRefFromPy(str);
		SetViewTooltipFromRef(view, StrRef);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Window_Focus__doc,
	     "===== Window_Focus =====\n\
\n\
**Metaclass Prototype:** Focus ([GView])\n\
\n\
**Description:** Brings window to front and makes it visible if it is not already.\n\
Optionally pass a subview to also make that the focused view of the window..\n\
\n\
**Parameters:**\n\
  * GView - the view to focus\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_Window_Focus(PyObject* self, PyObject* args)
{
	PyObject* pyview = nullptr;
	PARSE_ARGS(args, "O|O", &self, &pyview);

	Window* win = GetView<Window>(self);
	ABORT_IF_NULL(win);

	if (pyview) {
		View* view = GetView<View>(pyview);
		if (view && view->GetWindow() != win) {
			return RuntimeError("View must be a subview of the window!");
		}
		win->SetFocused(view);
	} else {
		win->Focus();
	}

	Py_RETURN_NONE;
}

//useful only for ToB and HoW, sets masterscript/worldmap name
PyDoc_STRVAR(GemRB_SetMasterScript__doc,
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
**See also:** [LoadGame](LoadGame.md)\n\
");

static PyObject* GemRB_SetMasterScript(PyObject* /*self*/, PyObject* args)
{
	PyObject* script = NULL;
	PyObject* worldmap1 = NULL;
	PyObject* worldmap2 = NULL;
	PARSE_ARGS(args, "OO|O", &script, &worldmap1, &worldmap2);

	core->GlobalScript = ResRefFromPy(script);
	core->WorldMapName[0] = ResRefFromPy(worldmap1);
	core->WorldMapName[1] = ResRefFromPy(worldmap2);
	core->UpdateMasterScript();
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Window_ShowModal__doc,
	     "===== Window_ShowModal =====\n\
\n\
**Metaclass Prototype:** ShowModal ([Shadow=MODAL_SHADOW_NONE])\n\
\n\
**Description:** Show a Window on Screen setting the Modal Status. If \n\
Shadow is MODAL_SHADOW_GRAY, other windows are grayed. If Shadow is \n\
MODAL_SHADOW_BLACK, they are blacked out.\n\
\n\
**Parameters:**\n\
  * Shadow:\n\
    * MODAL_SHADOW_NONE = 0\n\
    * MODAL_SHADOW_GRAY = 1 (translucent)\n\
    * MODAL_SHADOW_BLACK = 2\n\
\n\
**Return value:** N/A\n\
");

static PyObject* GemRB_Window_ShowModal(PyObject* self, PyObject* args)
{
	Window::ModalShadow Shadow = Window::ModalShadow::None;
	PARSE_ARGS(args, "O|i", &self, &Shadow);

	Window* win = GetView<Window>(self);
	ABORT_IF_NULL(win);
	if (!win->DisplayModal(Shadow)) {
		return RuntimeError("Couldn't display modal window. Window already closed.");
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetTimedEvent__doc,
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
**See also:** [Control_SetEvent](Control_SetEvent.md)\n\
");

static PyObject* GemRB_SetTimedEvent(PyObject* /*self*/, PyObject* args)
{
	PyObject* function;
	int rounds;
	PARSE_ARGS(args, "Oi", &function, &rounds);

	EventHandler handler = NULL;
	if (PyCallable_Check(function)) {
		handler = PythonCallback(function);
	} else {
		return RuntimeError(fmt::format("Can't set timed event handler {}!", PyEval_GetFuncName(function)));
	}
	Game* game = core->GetGame();
	if (game) {
		game->SetTimedEvent(std::move(handler), rounds);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Window_SetAction__doc,
	     "===== Window_SetAction =====\n\
\n\
**Metaclass Prototype:** SetAction (PythonFunction, EventType)\n\
\n\
**Description:** Ties an event of a control to a python function\
\n\
**Parameters:** \n\
  * PythonFunction - a callback for when the event occurs.\n\
  * EventType - the event type to bind to.\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
\n\
    Bar.SetAction (OnWindowClose, ACTION_WINDOW_CLOSED)\n\
    ...\n\
    def OnWindowClose (Window):\n\
      ...\n\
\n\
**See also:** [Control_SetAction](Control_SetAction.md)");

static PyObject* GemRB_Window_SetAction(PyObject* self, PyObject* args)
{
	int key = -1;
	PyObject* func = nullptr;
	PARSE_ARGS(args, "OOi", &self, &func, &key);

	Window* win = GetView<Window>(self);
	ABORT_IF_NULL(win);

	Window::WindowEventHandler handler = nullptr;
	if (PyCallable_Check(func)) {
		handler = PythonWindowCallback(func);
	}
	win->SetAction(std::move(handler), static_cast<Window::Action>(key));
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Control_SetAction__doc,
	     "===== Control_SetAction =====\n\
\n\
**Metaclass Prototype:** SetAction (PythonFunction, EventType[, Button, Mod, Count])\n\
\n\
**Description:** Ties an event of a control to a python function\
\n\
**Parameters:** \n\
  * PythonFunction - a callback for when the event occurs.\n\
  * EventType - the event type to bind to.\n\
  * Button - the button of the EventType.\n\
  * Mod - the modifier keys (flags).\n\
  * Count - the repeat count of the event.\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
\n\
    Bar.OnEndReached (EndLoadScreen)\n\
    ...\n\
    def EndLoadScreen ():\n\
      Skull = LoadScreen.GetControl (1)\n\
      Skull.SetPicture ('GSKULON')\n\
\n\
The above example changes the image on the loadscreen when the progressbar reaches the end.\n\
\n\
  Button.SetAction (Buttons.YesButton, IE_GUI_MOUSE_PRESS, 1, 0, 1)\n\
The above example sets up the 'YesButton' function from the Buttons module to be called when the button is pressed with the left mouse button one time.\n\
\n\
**See also:** [Window_GetControl](Window_GetControl.md), [Control_SetVarAssoc](Control_SetVarAssoc.md), [SetTimedEvent](SetTimedEvent.md)");

static PyObject* GemRB_Control_SetAction(PyObject* self, PyObject* args)
{
	Control::Action type = Control::Click;
	EventButton button = 0;
	Event::EventMods mod = 0;
	short count = 0;
	PyObject* func = NULL;
	PARSE_ARGS(args, "OOi|bhh", &self, &func, &type, &button, &mod, &count);

	Control* ctrl = GetView<Control>(self);
	if (ctrl) {
		ControlEventHandler handler = nullptr;
		if (PyCallable_Check(func)) {
			handler = PythonControlCallback(func);
		}
		ctrl->SetAction(std::move(handler), type, button, mod, count);

		Py_RETURN_NONE;
	}

	return AttributeError("Invalid Control");
}

PyDoc_STRVAR(GemRB_Control_SetActionInterval__doc,
	     "===== Control_SetActionInterval =====\n\
\n\
**Metaclass Prototype:** SetActionInterval ([interval])\n\
\n\
**Description:** Sets the tick interval between repeating actions such as a held down mouse button\
\n\
**Parameters:** \n\
* interval - the number of ticks between firing actions, default is Control::ActionRepeatDelay\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Control_SetEvent](Control_SetEvent.md)");

static PyObject* GemRB_Control_SetActionInterval(PyObject* self, PyObject* args)
{
	int interval = Control::ActionRepeatDelay;
	PARSE_ARGS(args, "O|i", &self, &interval);

	Control* ctrl = GetView<Control>(self);
	assert(ctrl);

	ctrl->SetActionInterval(interval);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetNextScript__doc,
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
**Examples:**\n\
\n\
    GemRB.SetNextScript ('CharGen')\n\
    return\n\
\n\
**See also:** [QuitGame](QuitGame.md)\n\
");

static PyObject* GemRB_SetNextScript(PyObject* /*self*/, PyObject* args)
{
	const char* funcName;
	PARSE_ARGS(args, "s", &funcName);

	if (!strcmp(funcName, "")) {
		return NULL;
	}

	core->SetNextScript(funcName);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Control_SetStatus__doc,
	     "===== SetControlStatus =====\n\
\n\
**Metaclass Prototype:** SetStatus (State)\n\
\n\
**Description:** Sets the state of a Control. For buttons, this is the \n\
same as Button_SetState.\n\
For other controls, this command will set the common value of the \n\
control, which has various uses.\n\
\n\
**Parameters:**\n\
  * Button States:**\n\
    * IE_GUI_BUTTON_ENABLED    = 0x00000000, default state\n\
    * IE_GUI_BUTTON_UNPRESSED  = 0x00000000, same as above\n\
    * IE_GUI_BUTTON_PRESSED    = 0x00000001, the button is pressed\n\
    * IE_GUI_BUTTON_SELECTED   = 0x00000002, the button stuck in pressed state\n\
    * IE_GUI_BUTTON_DISABLED   = 0x00000003, the button is disabled \n\
    * IE_GUI_BUTTON_LOCKED     = 0x00000004, the button is inactive (like DISABLED, but processes MouseOver events and draws UNPRESSED bitmap)\n\
    * IE_GUI_BUTTON_FAKEDISABLED = 0x00000005, draws DISABLED bitmap, but it isn't disabled\n\
    * IE_GUI_BUTTON_FAKEPRESSED = 0x00000006, draws PRESSED bitmap, but it isn't shifted\n\
  * Map Control States (add 0x09000000 to these):\n\
    * IE_GUI_MAP_NO_NOTES   =  0, no mapnotes visible\n\
    * IE_GUI_MAP_VIEW_NOTES =  1, view notes (no setting)\n\
    * IE_GUI_MAP_SET_NOTE   =  2, allow setting notes\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetState](Button_SetState.md)");

static PyObject* GemRB_Control_SetStatus(PyObject* self, PyObject* args)
{
	Button::State status;
	PARSE_ARGS(args, "OB", &self, &status);

	Control* ctrl = GetView<Control>(self);
	if (!ctrl) {
		return RuntimeError("Control is not found.");
	}

	switch (ctrl->ControlType) {
		case IE_GUI_BUTTON:
			//Button
			{
				Button* btn = (Button*) ctrl;
				btn->SetState(status);
			}
			break;
		case IE_GUI_WORLDMAP:
			break;
		default:
			ctrl->SetValue(status);
			break;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Control_SetValue__doc,
	     "===== Control_SetValue =====\n\
\n\
**Metaclass Prototype:** SetValue (LongValue)\n\
\n\
**Description:** Set the value of a control. \n\
\n\
**Parameters:**\n\
  * LongValue - numeric, a value associated with the control\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Control_SetVarAssoc](Control_SetVarAssoc.md)");

static PyObject* GemRB_Control_SetValue(PyObject* self, PyObject* args)
{
	PyObject* Value;
	PARSE_ARGS(args, "OO", &self, &Value);

	Control* ctrl = GetView<Control>(self);
	ABORT_IF_NULL(ctrl);

	Control::value_t val = Control::INVALID_VALUE;
	if (PyNumber_Check(Value)) {
		val = static_cast<Control::value_t>(PyLong_AsUnsignedLongMask(Value));
	}
	val = ctrl->SetValue(val);

	gs->AssignViewAttributes(self, ctrl);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Control_SetVarAssoc__doc,
	     "===== Control_SetVarAssoc =====\n\
\n\
**Metaclass Prototype:** SetVarAssoc (VariableName, LongValue)\n\
\n\
**Description:** It associates a variable name and an optionally bounded \n\
value with a control. \n\
The control uses this associated value differently, depending on the \n\
control. See more about this in 'data_exchange'.\n\
\n\
**Parameters:**\n\
  * Variablename - string, a Global Dictionary Name associated with the control\n\
  * LongValue - numeric, a value associated with the control\n\
  * min - numeric, minimum range value (optional)\n\
  * max - numeric, maximum range value (optional)\n\
\n\
**Return value:** N/A\n\
\n\
**Special:** If the 'DialogChoose' variable was set to -1 or 0 during a dialog session, it will terminate (-1) or pick the first available option (0) from the dialog automatically. (0 is used for 'continue', -1 is used for 'end dialogue').\n\
\n\
**See also:** [Button_SetFlags](Button_SetFlags.md), [SetVar](SetVar.md), [GetVar](GetVar.md)");

static PyObject* GemRB_Control_SetVarAssoc(PyObject* self, PyObject* args)
{
	PyObject* Value;
	PyObject* pyVar = nullptr;
	unsigned int min = Control::INVALID_VALUE;
	unsigned int max = Control::INVALID_VALUE;
	PARSE_ARGS(args, "OOO|II", &self, &pyVar, &Value, &min, &max);

	Control* ctrl = GetView<Control>(self);
	ABORT_IF_NULL(ctrl);

	Control::value_t val = Control::INVALID_VALUE;
	if (PyNumber_Check(Value)) {
		val = static_cast<Control::value_t>(PyLong_AsUnsignedLongMask(Value));
	}

	auto VarName = PyString_AsStringView(pyVar);

	Control::value_t realVal = core->GetDictionary().Get(VarName, 0);
	Control::varname_t varname = Control::varname_t(StringView(VarName));

	ctrl->BindDictVariable(varname, val, Control::ValueRange(min, max));
	// restore variable for sliders, since it's only a multiplier for them
	if (ctrl->ControlType == IE_GUI_SLIDER) {
		ctrl->UpdateState(realVal);
		core->GetDictionary().Set(VarName, val * static_cast<Slider*>(ctrl)->GetPosition());
	}

	gs->AssignViewAttributes(self, ctrl);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_RemoveScriptingRef__doc,
	     "===== View_RemoveScriptingRef =====\n\
\n\
**Prototype:** View_RemoveScriptingRef (GView)\n\
\n\
**Metaclass Prototype:** RemoveScriptingRef ()\n\
\n\
**Description:** Remove the decoded view reference from the scripting engine. You can use this to remove an alias or any other reference.\n\
\n\
**Parameters:**\n\
  * GView - the control's reference\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_RemoveScriptingRef(PyObject* self, PyObject* args)
{
	PARSE_ARGS(args, "O", &self);

	const ViewScriptingRef* ref = dynamic_cast<const ViewScriptingRef*>(gs->GetScriptingRef(self));
	ABORT_IF_NULL(ref);
	auto delref = ref->GetObject()->RemoveScriptingRef(ref);
	ABORT_IF_NULL(delref);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_RemoveView__doc,
	     "===== View_RemoveView =====\n\
\n\
**Prototype:** View_RemoveView (GView[, delete])\n\
\n\
**Metaclass Prototype:** RemoveView ([delete])\n\
\n\
**Description:** Remove a View from its superview and optionally delete it.\n\
\n\
**Parameters:**\n\
  * GView - the control's reference\n\
  * delete - set to non-zero to also delete the View\n\
\n\
**Return value:** None or a new reference to the View");

static PyObject* GemRB_RemoveView(PyObject* /*self*/, PyObject* args)
{
	int del = true;
	PyObject* pyView = NULL;
	PARSE_ARGS(args, "O|i", &pyView, &del);

	View* view = GetView(pyView);
	if (view) {
		Window* win = dynamic_cast<Window*>(view);
		if (win) {
			win->Close();
			if (win->Flags() & Window::DestroyOnClose) {
				// invalidate the reference
				PyObject_SetAttrString(pyView, "ID", DecRef(PyLong_FromLong, -1));
			}
			Py_RETURN_NONE;
		}

		if (del) {
			// invalidate the reference
			PyObject_SetAttrString(pyView, "ID", DecRef(PyLong_FromLong, -1));
			view->RemoveFromSuperview();
			delete view;
			Py_RETURN_NONE;
		} else {
			// return a new ref for a deleted group
			const ViewScriptingRef* ref = dynamic_cast<const ViewScriptingRef*>(gs->GetScriptingRef(pyView));
			auto delref = view->RemoveScriptingRef(ref);
			assert(delref);
			view->RemoveFromSuperview();

			return gs->ConstructObjectForScriptable(delref);
		}
	}
	return AttributeError("Invalid view");
}

PyDoc_STRVAR(GemRB_CreateView__doc,
	     "===== CreateView =====\n\
\n\
**Prototype:** GemRB.CreateView (ControlID, Type, FrameRect[, OtherArgs])\n\
\n\
**Description:** Creates a new empty view and returns it.\n\
\n\
**Parameters:** \n\
  * ControlID - the desired control ID\n\
  * Type - the View's type\n\
  * FrameRect - a dict with the View's frame (origin and size)\n\
  * OtherArgs - further arguments depending on the View type\n\
\n\
**Examples:**\n\
\n\
    view = CreateView (control, IE_GUI_SCROLLBAR, frame, CreateScrollbarARGs(bam))\n\
\n\
**See also:** [RemoveView](RemoveView.md), [AddSubview](AddSubview.md), [GetFrame](GetFrame.md)\n\
\n\
**Return value:** GView");

static PyObject* GemRB_CreateView(PyObject* /*self*/, PyObject* args)
{
	int type = -1;
	int id = -1;
	PyObject* pyRect;
	PyObject* constructArgs = nullptr;
	PARSE_ARGS(args, "iiO|O",
		   &id, &type,
		   &pyRect, &constructArgs);

	if (type > IE_GUI_INVALID) {
		return AttributeError("type is out of range.");
	}

	Region rgn = RectFromPy(pyRect);
	View* view = NULL;
	switch (type) {
		case IE_GUI_TEXTAREA:
			{
				PyObject* font;
				PARSE_ARGS(constructArgs, "O", &font);
				view = new TextArea(rgn, core->GetFont(ResRefFromPy(font)));
			}
			break;
		case IE_GUI_LABEL:
			{
				unsigned char alignment;
				PyObject* font;
				PyObject* text;
				PARSE_ARGS(constructArgs, "OOb", &font, &text, &alignment);

				Label* lbl = new Label(rgn, core->GetFont(ResRefFromPy(font)), PyString_AsStringObj(text));

				lbl->SetAlignment(alignment);
				view = lbl;
			}
			break;
		case IE_GUI_SCROLLBAR:
			{
				PyObject* pyRef;
				PyObject* pyImgList = NULL;
				PARSE_ARGS(constructArgs, "OO", &pyRef, &pyImgList);
				ResRef resRef = ResRefFromPy(pyRef);

				auto af = gamedata->GetFactoryResourceAs<const AnimationFactory>(resRef, IE_BAM_CLASS_ID);
				if (!af) {
					return RuntimeError(fmt::format("{} BAM not found!", resRef));
				}

				Holder<Sprite2D> images[ScrollBar::IMAGE_COUNT];
				for (int i = 0; i < ScrollBar::IMAGE_COUNT; i++) {
					PyErr_Clear();
					AnimationFactory::index_t frame = static_cast<AnimationFactory::index_t>(PyLong_AsLong(PyList_GetItem(pyImgList, i)));
					if (PyErr_Occurred()) {
						return AttributeError("Error retrieving image from list");
					}
					images[i] = af->GetFrame(frame);
				}

				view = new ScrollBar(rgn, images);
			}
			break;
		case IE_GUI_MAP:
			{
				PyObject* pylabel = NULL;
				if (!PyArg_ParseTuple(constructArgs, "O", &pylabel)) {
					PyErr_Clear(); //clearing the exception
				}

				auto flags = gamedata->GetFactoryResourceAs<const AnimationFactory>("FLAG1", IE_BAM_CLASS_ID);
				MapControl* map = new MapControl(rgn, std::move(flags));
				map->LinkedLabel = GetView<Control>(pylabel);

				view = map;
			}
			break;
		case IE_GUI_WORLDMAP:
			{
				PyObject* fontname = nullptr;
				PyObject* anim = nullptr;
				PyObject* pyColorNormal = nullptr;
				PyObject* pyColorSelected = nullptr;
				PyObject* pyColorNotVisited = nullptr;
				PARSE_ARGS(constructArgs, "|OOOOO", &fontname, &anim, &pyColorNormal, &pyColorSelected, &pyColorNotVisited);

				auto font = fontname ? core->GetFont(ResRefFromPy(fontname)) : nullptr;
				WorldMapControl* wmap = nullptr;
				if (pyColorNormal) {
					wmap = new WorldMapControl(rgn, std::move(font), ColorFromPy(pyColorNormal), ColorFromPy(pyColorSelected), ColorFromPy(pyColorNotVisited));
				} else {
					wmap = new WorldMapControl(rgn, std::move(font));
				}

				auto bam = gamedata->GetFactoryResourceAs<AnimationFactory>(ResRefFromPy(anim), IE_BAM_CLASS_ID);
				if (bam) {
					wmap->areaIndicator = bam->GetFrame(0, 0);
				}

				view = wmap;
			}
			break;
		case IE_GUI_BUTTON:
			view = new Button(rgn);
			break;
		case IE_GUI_CONSOLE:
			{
				PyObject* pyta = NULL;
				if (!PyArg_ParseTuple(constructArgs, "O", &pyta)) {
					PyErr_Clear(); //clearing the exception
				}
				view = new Console(rgn, GetView<TextArea>(pyta));
			}
			break;
		case IE_GUI_INVALID:
			view = core->GetWindowManager()->CreateWindow((unsigned short) id, rgn);
			break;
		default:
			view = new View(rgn);
			break;
	}

	assert(view);

	if (type < IE_GUI_VIEW) {
		// this is a control
		RegisterScriptableControl(static_cast<Control*>(view), id);
	} else {
		// these are getting stuck in a global group instead of under the window group...
		view->AssignScriptingRef(id, "VIEW");
	}

	return gs->ConstructObjectForScriptable(view->GetScriptingRef());
}

PyDoc_STRVAR(GemRB_View_SetEventProxy__doc,
	     "===== View_SetEventProxy =====\n\
\n\
**Prototype:** View_SetEventProxy (GView, ProxyView)\n\
\n\
**Metaclass Prototype:** SetEventProxy (ProxyView)\n\
\n\
**Description:** Set a proxy View that will receive events on behalf of the target View.\n\
\n\
**Parameters:**\n\
  * GView - the control's reference\n\
  * ProxyView - another View\n\
\n\
**Examples:**\n\
\n\
    RaceWindow.SetEventProxy (ScrollBarControl)\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_View_SetEventProxy(PyObject* self, PyObject* args)
{
	PyObject* pyView = NULL;
	PARSE_ARGS(args, "OO", &self, &pyView);

	View* target = GetView<View>(self);
	ABORT_IF_NULL(target);
	View* proxy = NULL;
	if (pyView != Py_None) {
		proxy = GetView<View>(pyView);
		ABORT_IF_NULL(proxy);
	}
	target->SetEventProxy(proxy);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_View_GetFrame__doc,
	     "===== View_GetFrame =====\n\
\n\
**Prototype:** View_GetFrame (GView)\n\
\n\
**Metaclass Prototype:** GetFrame ()\n\
\n\
**Description:** Return the View's frame rect.\n\
\n\
**Parameters:** \n\
  * GView - the control's reference\n\
\n\
**Examples:**\n\
\n\
    StartWindow.GetFrame ()\n\
\n\
**See also:** [SetFrame](SetFrame.md)\n\
\n\
**Return value:** a dictionary with members 'x', 'y', 'w', and 'h' representing the View's frame rect");

static PyObject* GemRB_View_GetFrame(PyObject* self, PyObject* args)
{
	PARSE_ARGS(args, "O", &self);
	const View* view = GetView<View>(self);
	ABORT_IF_NULL(view);

	const Region& frame = view->Frame();
	return Py_BuildValue("{s:i,s:i,s:i,s:i}", "x", frame.x, "y", frame.y, "w", frame.w, "h", frame.h);
}

PyDoc_STRVAR(GemRB_View_SetFrame__doc,
	     "===== View_SetFrame =====\n\
\n\
**Prototype:** View_SetFrame (GView, frameRect)\n\
\n\
**Metaclass Prototype:** SetFrame (frameRect)\n\
\n\
**Description:** Set the View's frame to the specified region.\n\
\n\
**Parameters:**\n\
  * GView - the control's reference\n\
  * frameRect - a dict with x, y, w, h keys denoting the frame origin and size\n\
\n\
**Examples:**\n\
\n\
    StartWindow.SetFrame ({ 'x': 0, 'y': 0, 'h': 300, 'w': 300 })\n\
\n\
**See also:** [GetFrame](GetFrame.md)\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_View_SetFrame(PyObject* self, PyObject* args)
{
	PyObject* pyRect = NULL;
	PARSE_ARGS(args, "OO", &self, &pyRect);

	View* view = GetView<View>(self);
	if (view) {
		view->SetFrame(RectFromPy(pyRect));
		Py_RETURN_NONE;
	}

	return AttributeError("Invalid view");
}

PyDoc_STRVAR(GemRB_View_SetBackground__doc,
	     "===== View_SetBackground =====\n\
\n\
**Prototype:** View_SetBackground (GView, ResRef|Color|None)\n\
\n\
**Metaclass Prototype:** SetBackground (ResRef|Color|None)\n\
\n\
**Description:** Set the background image or color for the View.\n\
\n\
**Parameters:**\n\
  * GView - the control's reference\n\
  * ResRef - name of the image to use\n\
  * Color - a dict with colors to use for a plain fill\n\
  * None - if the None object is passed, the background is cleared\n\
\n\
**Examples:**\n\
\n\
    StartWindow.SetBackground ('STARTOLD')\n\
    consoleOut.SetBackground ({'r' : 0, 'g' : 0, 'b' : 0, 'a' : 128})\n\
    NoteLabel.SetBackground (None)\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_View_SetBackground(PyObject* self, PyObject* args)
{
	PyObject* pybg;
	if (!PyArg_ParseTuple(args, "OO", &self, &pybg)) {
		return NULL;
	}

	View* view = GetView<View>(self);
	ABORT_IF_NULL(view);

	if (pybg == Py_None) {
		view->SetBackground(nullptr);
	} else if (PyDict_Check(pybg)) {
		const Color color = ColorFromPy(pybg);
		view->SetBackground(nullptr, &color);
	} else {
		Holder<Sprite2D> pic = SpriteFromPy(pybg);

		if (!pic) {
			return RuntimeError("Failed to acquire the picture!\n");
		}
		view->SetBackground(std::move(pic));
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_View_SetFlags__doc,
	     "===== View_SetFlags =====\n\
\n\
**Prototype:** View_SetFlags (GView, Flags[, Operation])\n\
\n\
**Metaclass Prototype:** SetFlags (Flags[, Operation])\n\
\n\
**Description:** Sets the general flags of a View.\n\
\n\
**Parameters:**\n\
  * GView - the control's reference\n\
  * Flags - the bits to enable\n\
  * Operation - the bit operation to use, defaults to SET\n\
\n\
**Examples:**\n\
\n\
    Button.SetFlags (IE_GUI_BUTTON_ALIGN_RIGHT | IE_GUI_BUTTON_ALIGN_BOTTOM, OP_OR)\n\
    Window.SetFlags (WF_ALPHA_CHANNEL, OP_NAND)\n\
\n\
**Return value:** boolean marking success or failure");

static PyObject* GemRB_View_SetFlags(PyObject* self, PyObject* args)
{
	unsigned int Flags;
	BitOp Operation = BitOp::SET;
	PARSE_ARGS(args, "OI|i", &self, &Flags, &Operation);

	// check if we were called by a button, so we can ensure the
	// Disabled state is preserved — also set by SetState
	Button* btn = GetView<Button>(self);
	if (btn && Operation == BitOp::SET) {
		bool wasDisabled = btn->Flags() & View::Disabled;
		bool set = btn->SetFlags(Flags, Operation);
		if (wasDisabled) btn->SetFlags(View::Disabled, BitOp::OR);
		RETURN_BOOL(set);
	}
	View* view = GetView<View>(self);
	ABORT_IF_NULL(view);
	RETURN_BOOL(view->SetFlags(Flags, Operation));
}

PyDoc_STRVAR(GemRB_View_SetResizeFlags__doc,
	     "===== View_SetResizeFlags =====\n\
\n\
**Prototype:** View_SetResizeFlags (GView, Flags[, Operation])\n\
\n\
**Metaclass Prototype:** SetResizeFlags (Flags[, Operation])\n\
\n\
**Description:** Sets the resize flags of a View.\n\
\n\
**Parameters:**\n\
  * GView - the control's reference\n\
  * Flags - the bits to enable\n\
  * Operation - the bit operation to use, defaults to SET\n\
\n\
**Examples:**\n\
\n\
    TextArea.SetResizeFlags (IE_GUI_VIEW_RESIZE_ALL, OR)\n\
\n\
**Return value:** boolean marking success or failure");

static PyObject* GemRB_View_SetResizeFlags(PyObject* self, PyObject* args)
{
	unsigned int flags;
	BitOp op = BitOp::SET;
	PARSE_ARGS(args, "OI|i", &self, &flags, &op);

	View* view = GetView<View>(self);
	ABORT_IF_NULL(view);
	RETURN_BOOL(view->SetAutoResizeFlags(flags, op));
}

PyDoc_STRVAR(GemRB_View_Focus__doc,
	     "===== View_Focus =====\n\
\n\
**Metaclass Prototype:** Focus ()\n\
\n\
**Description:** Focuses the view in its window to direct keyboard and certain other events to the view.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_View_Focus(PyObject* self, PyObject* args)
{
	PARSE_ARGS(args, "O", &self);

	View* view = GetView<View>(self);
	ABORT_IF_NULL(view);
	Window* win = view->GetWindow();
	ABORT_IF_NULL(win);

	win->SetFocused(view);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_SetSprites__doc,
	     "===== Button_SetSprites =====\n\
\n\
**Metaclass Prototype:** SetSprites (ResRef, Cycle, UnpressedFrame, PressedFrame, SelectedFrame, DisabledFrame)\n\
\n\
**Description:** Sets the Button's images. You can disable the images by \n\
setting the IE_GUI_BUTTON_NO_IMAGE flag on the control.\n\
\n\
**Parameters:**\n\
  * ResRef - a .bam animation resource (.bam resref)\n\
  * Cycle - the cycle of the .bam from which all frames of this button will come\n\
  * UnpressedFrame - the frame which will be displayed by default\n\
  * PressedFrame - the frame which will be displayed when the button is pressed \n\
  * SelectedFrame - this is for selected checkboxes\n\
  * DisabledFrame - this is for inactivated buttons\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetFlags](Button_SetFlags.md), [Button_SetPicture](Button_SetPicture.md), [Button_SetPicture](Button_SetPicture.md)");

static PyObject* GemRB_Button_SetSprites(PyObject* self, PyObject* args)
{
	int cycle, unpressed, pressed, selected, disabled;
	PyObject* pyref;
	if (PyArg_ParseTuple(args, "OOiiiii", &self,
			     &pyref, &cycle, &unpressed,
			     &pressed, &selected, &disabled)) {
		Button* btn = GetView<Button>(self);
		ABORT_IF_NULL(btn);

		ResRef ResRef = ResRefFromPy(pyref);
		if (ResRef[0] == 0) {
			btn->SetImage(ButtonImage::None, nullptr);
			Py_RETURN_NONE;
		}

		auto af = gamedata->GetFactoryResourceAs<AnimationFactory>(ResRef, IE_BAM_CLASS_ID);
		if (!af) {
			return RuntimeError(fmt::format("{} BAM not found!", ResRef));
		}
		Holder<Sprite2D> tspr;
		tspr = af->GetFrame((AnimationFactory::index_t) unpressed, (unsigned char) cycle);
		btn->SetImage(ButtonImage::Unpressed, std::move(tspr));
		tspr = af->GetFrame((AnimationFactory::index_t) pressed, (unsigned char) cycle);
		btn->SetImage(ButtonImage::Pressed, std::move(tspr));
		tspr = af->GetFrame((AnimationFactory::index_t) selected, (unsigned char) cycle);
		btn->SetImage(ButtonImage::Selected, std::move(tspr));
		tspr = af->GetFrame((AnimationFactory::index_t) disabled, (unsigned char) cycle);
		btn->SetImage(ButtonImage::Disabled, std::move(tspr));

		Py_RETURN_NONE;
	}
	return AttributeError("Unable to parse arguments.");
}

PyDoc_STRVAR(GemRB_Button_SetOverlay__doc,
	     "===== Button_SetOverlay =====\n\
\n\
**Metaclass Prototype:** SetOverlay (ratio, r1,g1,b1,a1, r2,g2,b2,a2)\n\
\n\
**Description:** Sets ratio (0-1.0) of height to which button picture will \n\
be overlaid in a different colour. The colour will fade from the first rgba \n\
values to the second.\n\
\n\
**Parameters:** \n\
  * ClippingRatio  - a floating point value from the 0-1 interval\n\
  * rgba1          - source colour\n\
  * rgba2          - target colour\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetPictureClipping](Button_SetPictureClipping.md)");

static PyObject* GemRB_Button_SetOverlay(PyObject* self, PyObject* args)
{
	double Clipping;
	PyObject *pyColorSrc, *pyColorDest;
	PARSE_ARGS(args, "OdOO", &self,
		   &Clipping, &pyColorSrc, &pyColorDest);

	Button* btn = GetView<Button>(self);
	ABORT_IF_NULL(btn);

	const Color src = ColorFromPy(pyColorSrc);
	const Color dest = ColorFromPy(pyColorDest);

	if (Clipping < 0.0)
		Clipping = 0.0;
	else if (Clipping > 1.0)
		Clipping = 1.0;
	//can't call clipping, because the change of ratio triggers color change
	btn->SetHorizontalOverlay(Clipping, src, dest);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_SetBorder__doc,
	     "===== Button_SetBorder =====\n\
\n\
**Prototype:** GemRB.SetButtonBorder (GButton, BorderIndex, Color, [enabled=0, filled=0, Rect=None])\n\
\n\
**Metaclass Prototype:** SetBorder (BorderIndex, Color, [enabled=0, filled=0, Rect=None])\n\
\n\
**Description:** Sets border/frame/overlay parameters for a button. This \n\
command can be used for drawing a border around a button, or to overlay \n\
it with a tint (like with unusable or unidentified item's icons).\n\
\n\
**Parameters:** \n\
  * GButton - the control's reference\n\
  * BorderIndex - 0, 1 or 2\n\
  * RGBA - red,green,blue,opacity components of the border colour\n\
  * enabled - 1 means enable it immediately\n\
  * filled - 1 means draw it filled (overlays)\n\
  * Rect - the border rectangle (the button frame if None)\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
\n\
    color = {'r' : 200, 'g' : 0, 'b' : 0, 'a' : 64}\n\
    IconButton.SetBorder (0, color, 0, 1)\n\
\n\
Not known spells are drawn darkened (the whole button will be overlaid).\n\
\n\
**See also:** [Button_EnableBorder](Button_EnableBorder.md)");

static PyObject* GemRB_Button_SetBorder(PyObject* self, PyObject* args)
{
	int BorderIndex, enabled = 0, filled = 0;
	PyObject* pyColor;
	PyObject* pyRect = Py_None;
	PARSE_ARGS(args, "OiO|iiO", &self,
		   &BorderIndex, &pyColor, &enabled, &filled, &pyRect);

	Button* btn = GetView<Button>(self);
	ABORT_IF_NULL(btn);

	Color color = ColorFromPy(pyColor);
	Region rgn = (pyRect == Py_None) ? Region(Point(), btn->Dimensions()) : RectFromPy(pyRect);
	btn->SetBorder(BorderIndex, rgn, color, enabled, filled);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_EnableBorder__doc,
	     "===== Button_EnableBorder =====\n\
\n\
**Metaclass Prototype:** EnableBorder (BorderIndex, enabled)\n\
\n\
**Description:** Enable or disable specified button border/frame/overlay.\n\
\n\
**Parameters:** \n\
  * BorderIndex - 0, 1 or 2\n\
  * enabled - boolean, true enables the border\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetPicture](Button_SetPicture.md), [Button_SetFlags](Button_SetFlags.md), [Button_SetBorder](Button_SetBorder.md)");

static PyObject* GemRB_Button_EnableBorder(PyObject* self, PyObject* args)
{
	int BorderIndex, enabled;
	PARSE_ARGS(args, "Oii", &self, &BorderIndex, &enabled);

	Button* btn = GetView<Button>(self);
	ABORT_IF_NULL(btn);

	btn->EnableBorder(BorderIndex, (bool) enabled);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Control_SetFont__doc,
	     "===== Control_SetFont =====\n\
\n\
**Metaclass Prototype:** SetFont (FontResRef[, which = 0])\n\
\n\
**Description:** Sets font used for drawing button, label or textarea text.\n\
\n\
**Parameters:**\n\
  * FontResref - a .bam resref which must be listed in fonts.2da\n\
  * which - for textareas set to 1 if you want to set the initials font\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Window_CreateLabel](Window_CreateLabel.md)");

static PyObject* GemRB_Control_SetFont(PyObject* self, PyObject* args)
{
	PyObject* FontResRef = nullptr;
	int which = 0;
	PARSE_ARGS(args, "OO|i", &self, &FontResRef, &which);

	const Control* ctrl = GetView<Control>(self);
	ABORT_IF_NULL(ctrl);
	ResRef fontRef = ResRefFromPy(FontResRef);
	if (ctrl->ControlType == IE_GUI_BUTTON) {
		Button* button = GetView<Button>(self);
		button->SetFont(core->GetFont(fontRef));
	} else if (ctrl->ControlType == IE_GUI_LABEL) {
		Label* label = GetView<Label>(self);
		label->SetFont(core->GetFont(fontRef));
	} else if (ctrl->ControlType == IE_GUI_TEXTAREA) {
		TextArea* textArea = GetView<TextArea>(self);
		textArea->SetFont(core->GetFont(fontRef), which);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_SetAnchor__doc,
	     "===== Button_SetAnchor =====\n\
\n\
**Metaclass Prototype:** SetAnchor (x, y)\n\
\n\
**Description:** Sets explicit anchor point used for drawing button label.\n\
\n\
**Parameters:** \n\
  * x, y - anchor position \n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetPushOffset](Button_SetPushOffset.md)");

static PyObject* GemRB_Button_SetAnchor(PyObject* self, PyObject* args)
{
	int x, y;
	PARSE_ARGS(args, "Oii", &self, &x, &y);

	Button* btn = GetView<Button>(self);
	ABORT_IF_NULL(btn);

	btn->SetAnchor(x, y);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_SetPushOffset__doc,
	     "===== Button_SetPushOffset =====\n\
\n\
**Metaclass Prototype:** SetPushOffset (x, y)\n\
\n\
**Description:** Sets the amount pictures and label move on button press.\n\
\n\
**Parameters:** \n\
  * x, y - anchor position \n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetAnchor](Button_SetAnchor.md)");

static PyObject* GemRB_Button_SetPushOffset(PyObject* self, PyObject* args)
{
	int x, y;
	PARSE_ARGS(args, "Oii", &self, &x, &y);

	Button* btn = GetView<Button>(self);
	ABORT_IF_NULL(btn);

	btn->SetPushOffset(x, y);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_AddNewArea__doc,
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
**Return value:** N/A");

static PyObject* GemRB_AddNewArea(PyObject* /*self*/, PyObject* args)
{
	PyObject* pystr = nullptr;
	PARSE_ARGS(args, "O", &pystr);

	ResRef resref = ResRefFromPy(pystr);
	AutoTable newarea = gamedata->LoadTable(resref);
	if (!newarea) {
		return RuntimeError("2da not found!\n");
	}

	WorldMap* wmap = core->GetWorldMap();
	if (!wmap) {
		return RuntimeError("no worldmap loaded!");
	}

	ResRef enc[5];
	int k;
	TableMgr::index_t rows = newarea->GetRowCount();
	for (TableMgr::index_t i = 0; i < rows; ++i) {
		const ResRef area = newarea->QueryField(i, 0);
		const ieVariable script = newarea->QueryField(i, 1);
		int flags = newarea->QueryFieldSigned<int>(i, 2);
		int icon = newarea->QueryFieldSigned<int>(i, 3);
		int locx = newarea->QueryFieldSigned<int>(i, 4);
		int locy = newarea->QueryFieldSigned<int>(i, 5);
		int label = newarea->QueryFieldSigned<int>(i, 6);
		int name = newarea->QueryFieldSigned<int>(i, 7);
		ResRef ltab = newarea->QueryField(i, 8);
		EnumArray<WMPDirection, ieDword> links;
		links[WMPDirection::NORTH] = newarea->QueryFieldUnsigned<ieDword>(i, 9);
		links[WMPDirection::EAST] = newarea->QueryFieldUnsigned<ieDword>(i, 10);
		links[WMPDirection::SOUTH] = newarea->QueryFieldUnsigned<ieDword>(i, 11);
		links[WMPDirection::WEST] = newarea->QueryFieldUnsigned<ieDword>(i, 12);
		//this is the number of links in the 2da, we don't need it
		int linksto = newarea->QueryFieldSigned<int>(i, 13);

		unsigned int local = 0;
		int linkcnt = wmap->GetLinkCount();
		EnumArray<WMPDirection, ieDword> indices;
		for (WMPDirection dir : EnumIterator<WMPDirection>()) {
			indices[dir] = linkcnt;
			linkcnt += links[dir];
			local += links[dir];
		}
		unsigned int total = linksto + local;

		AutoTable newlinks = gamedata->LoadTable(ltab);
		if (!newlinks || total != newlinks->GetRowCount()) {
			return RuntimeError("invalid links 2da!");
		}

		WMPAreaEntry entry;
		entry.AreaName = area;
		entry.AreaResRef = area;
		entry.ScriptName = script;
		entry.SetAreaStatus(flags, BitOp::SET);
		entry.IconSeq = icon;
		entry.pos.x = locx;
		entry.pos.y = locy;
		entry.LocCaptionName = ieStrRef(label);
		entry.LocTooltipName = ieStrRef(name);
		entry.LoadScreenResRef.Reset();
		entry.AreaLinksIndex = std::move(indices);
		entry.AreaLinksCount = std::move(links);

		int newAreaIdx = wmap->GetEntryCount(); // once we add it in the next line
		wmap->AddAreaEntry(std::move(entry));
		for (unsigned int j = 0; j < total; j++) {
			const ResRef larea = newlinks->QueryField(j, 0);
			int lflags = newlinks->QueryFieldSigned<int>(j, 1);
			const ieVariable ename = newlinks->QueryField(j, 2);
			int distance = newlinks->QueryFieldSigned<int>(j, 3);
			int encprob = newlinks->QueryFieldSigned<int>(j, 4);
			for (k = 0; k < 5; k++) {
				enc[k] = newlinks->QueryField(i, 5 + k);
			}
			WMPDirection linktodir = EnumIndex<WMPDirection>(newlinks->QueryFieldUnsigned<under_t<WMPDirection>>(j, 10));

			size_t areaIndex;
			const WMPAreaEntry* oarea = wmap->GetArea(larea, areaIndex);
			if (!oarea) {
				//blabla
				return RuntimeError("cannot establish area link!");
			}
			WMPAreaLink link;
			link.DestEntryPoint = ename;
			link.DistanceScale = distance;
			link.DirectionFlags = lflags;
			link.EncounterChance = encprob;
			for (k = 0; k < 5; k++) {
				if (IsStar(enc[k])) {
					link.EncounterAreaResRef[k].Reset();
				} else {
					link.EncounterAreaResRef[k] = enc[k];
				}
			}

			//first come the local links, then 'links to' this area
			// if the LINKTO_DIR column has a *, ignore it
			if (j < local && linktodir != WMPDirection::NONE) {
				link.AreaIndex = newAreaIdx;
				//linktodir may need translation
				wmap->InsertAreaLink(areaIndex, linktodir, std::move(link));
			} else {
				link.AreaIndex = static_cast<ieDword>(areaIndex);
				wmap->AddAreaLink(std::move(link));
			}
		}
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_CreateMovement__doc,
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
**See also:** [WorldMap_GetDestinationArea](WorldMap_GetDestinationArea.md)\n\
");

static PyObject* GemRB_CreateMovement(PyObject* /*self*/, PyObject* args)
{
	int everyone;
	PyObject* area = nullptr;
	char* entrance;
	int direction = 0;
	PARSE_ARGS(args, "Os|i", &area, &entrance, &direction);
	if (core->HasFeature(GFFlags::TEAM_MOVEMENT)) {
		everyone = CT_WHOLE;
	} else {
		everyone = CT_GO_CLOSER;
	}
	GET_GAME();

	GET_MAP();

	map->MoveToNewArea(ResRefFromPy(area), ieVariable(entrance), (unsigned int) direction, everyone, NULL);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_UpdateWorldMap__doc,
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
**See also:** [GetWorldMap](GetWorldMap.md)\n\
");

static PyObject* GemRB_UpdateWorldMap(PyObject* /*self*/, PyObject* args)
{
	PyObject* wmResRef = nullptr;
	PyObject* areaResRef = nullptr;
	bool update = true;
	PARSE_ARGS(args, "O|O", &wmResRef, &areaResRef);

	if (areaResRef != nullptr) {
		update = core->GetWorldMap()->GetArea(ResRefFromPy(areaResRef)) == nullptr;
	}

	if (update)
		core->UpdateWorldMap(ResRefFromPy(wmResRef));

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_WorldMap_GetDestinationArea__doc,
	     "===== WorldMap_GetDestinationArea =====\n\
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
  * RndEncounter - check for random encounters?\n\
\n\
**Return value:** Dictionary\n\
  * Target      - The target area selected by the player\n\
  * Distance    - The traveling distance, if it is negative, the way is blocked\n\
  * Destination - The area resource reference where the player arrives (if there was a random encounter, it differs from Target)\n\
  * Entrance    - The area entrance in the Destination area, it could be empty, in this casethe player should appear in middle of the area\n\
\n\
**See also:** [Window_CreateWorldMapControl](Window_CreateWorldMapControl.md), [CreateMovement](CreateMovement.md)");

static PyObject* GemRB_WorldMap_GetDestinationArea(PyObject* self, PyObject* args)
{
	int eval = 0;
	PARSE_ARGS(args, "O|i", &self, &eval);

	const WorldMapControl* wmc = GetView<WorldMapControl>(self);
	ABORT_IF_NULL(wmc);
	//no area was pointed on
	if (!wmc->Area) {
		Py_RETURN_NONE;
	}
	WorldMap* wm = core->GetWorldMap();
	//the area the user clicked on
	PyObject* dict = Py_BuildValue("{s:s,s:s}", "Target", wmc->Area->AreaName.c_str(), "Destination", wmc->Area->AreaName.c_str());

	if (wmc->Area->AreaName == core->GetGame()->CurrentArea) {
		PyDict_SetItemString(dict, "Distance", PyLong_FromLong(-1));
		return dict;
	}

	bool encounter;
	int distance;
	WMPAreaLink* wal = wm->GetEncounterLink(wmc->Area->AreaName, encounter);
	if (!wal) {
		PyDict_SetItemString(dict, "Distance", PyLong_FromLong(-1));
		return dict;
	}
	PyDict_SetItemString(dict, "Entrance", DecRef(PyString_FromStringView, wal->DestEntryPoint));
	PyDict_SetItemString(dict, "Direction", DecRef(PyLong_FromLong, wal->DirectionFlags));
	distance = wm->GetDistance(wmc->Area->AreaName);

	if (!eval) {
		PyDict_SetItemString(dict, "Distance", DecRef(PyLong_FromLong, distance));
		return dict;
	}

	wm->ClearEncounterArea();

	// evaluate the area the user will fall on in a random encounter
	if (!encounter) {
		PyDict_SetItemString(dict, "Distance", DecRef(PyLong_FromLong, distance));
		return dict;
	}

	if (wal->EncounterChance >= 100) {
		wal->EncounterChance -= 100;
	}

	// bounty encounter
	const WMPAreaEntry* linkdest = wm->GetEntry(wal->AreaIndex);
	ResRef tmpresref = linkdest->AreaResRef;
	if (core->GetGame()->RandomEncounter(tmpresref)) {
		displaymsg->DisplayConstantString(HCStrings::Ambush, GUIColors::XPCHANGE);
		PyDict_SetItemString(dict, "Destination", DecRef(PyString_FromStringView, tmpresref));
		PyDict_SetItemString(dict, "Entrance", DecRef(PyString_FromString, ""));
		distance = wm->GetDistance(linkdest->AreaResRef) - (wal->DistanceScale * 4 / 2);
		wm->SetEncounterArea(tmpresref, wal);
	} else {
		// regular random encounter, find a valid encounter area
		int i = RAND(0, 4);

		for (int j = 0; j < 5; j++) {
			ResRef& area = wal->EncounterAreaResRef[(i + j) % 5];

			if (!area.IsEmpty() && area != core->GetGame()->CurrentArea) {
				displaymsg->DisplayConstantString(HCStrings::Ambush, GUIColors::XPCHANGE);
				PyDict_SetItemString(dict, "Destination", DecRef(PyString_FromResRef, area));
				//drop player in the middle of the map
				PyDict_SetItemString(dict, "Entrance", DecRef(PyString_FromString, ""));
				PyDict_SetItemString(dict, "Direction", DecRef(PyLong_FromLong, ADIRF_CENTER));
				//only count half the distance of the final link
				distance = wm->GetDistance(linkdest->AreaResRef) - (wal->DistanceScale * 4 / 2);
				wm->SetEncounterArea(area, wal);
				break;
			}
		}
	}

	PyDict_SetItemString(dict, "Distance", DecRef(PyLong_FromLong, distance));
	return dict;
}

PyDoc_STRVAR(GemRB_Button_SetHotKey__doc,
	     "===== Button_SetHotKey =====\n\
\n\
**Metaclass Prototype:** SetHotKey(char or keymaping[, modifiers=0, global=False])\n\
\n\
**Description:** Binds a keyboard key to trigger the control event when its window is focused.\n\
If global is set, the hot key works even if the window does not have focus.\n\
If None is passed as the key, any existing hotkey binding is cleared.\n\
\n\
**Parameters:**\n\
 * char - key to bind\n\
 * modifiers - bitfield denoting if modifier keys need to be pressed\n\
   * GEM_MOD_SHIFT (1) - shift\n\
   * GEM_MOD_CTRL (2) - control\n\
   * GEM_MOD_ALT (4) - alt\n\
 * global - boolean toggling focus requirement\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_Button_SetHotKey(PyObject* self, PyObject* args)
{
	unsigned char hotkey = 0;
	short mods = 0;
	int global = false;
	Button* btn = NULL;
	PyObject* arg1 = PyTuple_GetItem(args, 1);

	if (arg1 == Py_None) {
		btn = GetView<Button>(PyTuple_GetItem(args, 0));
		// work around a bug in cpython where PyArg_ParseTuple doesn't return as expected when 'c' format doesn't match
	} else if (PyObject_TypeCheck(arg1, &PyUnicode_Type) && PyUnicode_GetLength(arg1) == 1) {
		int ch = 0;
		PARSE_ARGS(args, "OC|hi", &self, &ch, &mods, &global);
		hotkey = static_cast<unsigned char>(ch);
		btn = GetView<Button>(self);
		assert(btn);
	} else {
		PyObject* pyStr = nullptr;
		PARSE_ARGS(args, "OO|i", &self, &pyStr, &global);

		auto keymap = PyString_AsStringView(pyStr);
		auto func = core->GetKeyMap()->LookupFunction(keymap);
		if (!func) {
			Log(DEBUG, "GUIScript", "Couldn't find keymap entry for {}", StringView(keymap));
			Py_RETURN_NONE;
		}

		btn = GetView<Button>(self);
		assert(btn);

		PyObject* moduleName = PyImport_ImportModule(func->moduleName.c_str());
		if (moduleName == nullptr) {
			return RuntimeError("Hot key map referenced a moduleName that doesn't exist.");
		}
		PyObject* dict = PyModule_GetDict(moduleName);

		PyObject* pFunc = PyDict_GetItemString(dict, func->function.c_str());
		/* pFunc: Borrowed reference */
		if (!PyCallable_Check(pFunc)) {
			Py_DECREF(moduleName);
			return RuntimeError("Hot key map referenced a function that doesn't exist.");
		}

		btn->SetAction(PythonControlCallback(pFunc));
		Py_DECREF(moduleName);

		hotkey = static_cast<unsigned char>(func->key);
	}

	btn->SetHotKey(hotkey, mods, global);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GameSetPartySize__doc,
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
**See also:** [GetPartySize](GetPartySize.md)\n\
");

static PyObject* GemRB_GameSetPartySize(PyObject* /*self*/, PyObject* args)
{
	int Flags;
	PARSE_ARGS(args, "i", &Flags);

	GET_GAME();

	game->SetPartySize(Flags);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GameSetProtagonistMode__doc,
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
**See also:** [LoadGame](LoadGame.md), [GameSetPartySize](GameSetPartySize.md)\n\
");

static PyObject* GemRB_GameSetProtagonistMode(PyObject* /*self*/, PyObject* args)
{
	int Flags;
	PARSE_ARGS(args, "i", &Flags);

	GET_GAME();

	game->SetProtagonistMode(Flags);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GameGetExpansion__doc,
	     "===== GameGetExpansion =====\n\
\n\
**Prototype:** GemRB.GameGetExpansion ()\n\
\n\
**Description:** Gets the expansion mode.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** integer version of the current expansion\n\
\n\
**See also:** [GameSetExpansion](GameSetExpansion.md)");
static PyObject* GemRB_GameGetExpansion(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyLong_FromLong(game->Expansion);
}

PyDoc_STRVAR(GemRB_GameSetExpansion__doc,
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
**Return value:** false if already set, true otherwise\n\
\n\
**See also:** [LoadGame](LoadGame.md), [GameGetExpansion](GameGetExpansion.md), GameType(variable)\n\
");

static PyObject* GemRB_GameSetExpansion(PyObject* /*self*/, PyObject* args)
{
	int value;
	PARSE_ARGS(args, "i", &value);

	GET_GAME();

	if ((unsigned int) value <= game->Expansion) {
		Py_RETURN_FALSE;
	}
	game->SetExpansion(value);
	Py_RETURN_TRUE;
}

PyDoc_STRVAR(GemRB_GameSetScreenFlags__doc,
	     "===== GameSetScreenFlags =====\n\
\n\
**Prototype:** GemRB.GameSetScreenFlags (Bits, Operation)\n\
\n\
**Description:** Sets the Display Flags of the main game screen (pane \n\
status, dialog textarea size).\n\
\n\
**Parameters:**\n\
  * Bits (partly game dependent due to different GUI layouts)\n\
    * 0 - default, small message window size\n\
    * 1 - enable party AI\n\
    * 2 - medium message window size\n\
    * 4 - large message window size (use with 2, as 6, GS_LARGEDIALOG)\n\
    * 8 - in dialog mode\n\
    * 16 - hide the whole GUI\n\
    * 32 - hide left menu (options) window\n\
    * 64 - hide portrait window\n\
  * Operation - The usual bit operations\n\
\n\
**Return value:** boolean denoting success");

static PyObject* GemRB_GameSetScreenFlags(PyObject* /*self*/, PyObject* args)
{
	int Flags;
	BitOp Operation;
	PARSE_ARGS(args, "ii", &Flags, &Operation);
	GET_GAME();
	RETURN_BOOL(game->SetControlStatus(Flags, Operation));
}

PyDoc_STRVAR(GemRB_GameControlSetScreenFlags__doc,
	     "===== GameControlSetScreenFlags =====\n\
\n\
**Prototype:** GemRB.GameControlSetScreenFlags (Mode, Operation)\n\
\n\
**Description:** Sets screen flags, like cutscene mode, disable mouse, etc. \n\
Don't confuse it with the saved screen flags set by GameSetScreenFlags.\n\
\n\
**Parameters:**\n\
  * Mode:\n\
    * 0 - center on actor (one time)\n\
    * 1 - center on actor (always)\n\
    * 2 - cutscene mode (rather use ai scripts for this)\n\
  * Operation - bit operation to use\n\
\n\
**Return value:** boolean denoting success\n\
\n\
**See also:** [GameSetScreenFlags](GameSetScreenFlags.md)");

static PyObject* GemRB_GameControlSetScreenFlags(PyObject* /*self*/, PyObject* args)
{
	ScreenFlags flag;
	BitOp Operation;
	PARSE_ARGS(args, "Ii", &flag, &Operation);
	GET_GAMECONTROL();
	RETURN_BOOL(gc->SetScreenFlags(flag, Operation));
}


PyDoc_STRVAR(GemRB_GameControlSetTargetMode__doc,
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
**See also:** [GameControlSetScreenFlags](GameControlSetScreenFlags.md), [GameControlGetTargetMode](GameControlGetTargetMode.md)\n\
");

static PyObject* GemRB_GameControlSetTargetMode(PyObject* /*self*/, PyObject* args)
{
	int Mode;
	int Types = GA_SELECT | GA_NO_DEAD | GA_NO_HIDDEN | GA_NO_UNSCHEDULED;
	PARSE_ARGS(args, "i|i", &Mode, &Types);

	GET_GAMECONTROL();

	//target mode is only the low bits (which is a number)
	gc->SetTargetMode(TargetMode(Mode & GA_ACTION));
	//target type is all the bits
	gc->target_types = (Mode & GA_ACTION) | Types;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GameControlGetTargetMode__doc,
	     "===== GameControlGetTargetMode =====\n\
\n\
**Prototype:** GemRB.GameControlGetTargetMode ()\n\
\n\
**Description:** Returns the current target mode.\n\
\n\
**Return value:** numeric (see GameControlSetTargetMode)\n\
\n\
**See also:** [GameControlSetTargetMode](GameControlSetTargetMode.md), [GameControlSetScreenFlags](GameControlSetScreenFlags.md)");

static PyObject* GemRB_GameControlGetTargetMode(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAMECONTROL();

	return PyLong_FromLong(int(gc->GetTargetMode()));
}

PyDoc_STRVAR(GemRB_GameControlLocateActor__doc,
	     "===== GameControlLocateActor =====\n\
\n\
**Prototype:** GemRB.GameControlLocateActor ()\n\
\n\
**Description:** Activates location indicators such as arrow markers or pulsating draw circles for the actor.\n\
Only one actor can be located at a time.\n\
\n\
**Return value:** N/A\n\
");

static PyObject* GemRB_GameControlLocateActor(PyObject* /*self*/, PyObject* args)
{
	int globalID = -1;
	PARSE_ARGS(args, "i", &globalID);

	GET_GAMECONTROL();

	if (globalID == -1) {
		gc->SetLastActor(nullptr);
		Py_RETURN_NONE;
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	gc->SetLastActor(actor);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GameControlToggleAlwaysRun__doc,
	     "===== GameControlToggleAlwaysRun =====\n\
\n\
**Prototype:** GemRB.GameControlToggleAlwaysRun ()\n\
\n\
**Description:** Toggles using running instead of walking by default.\n\
\n\
**Return value:** N/A\n\
");

static PyObject* GemRB_GameControlToggleAlwaysRun(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAMECONTROL();

	gc->ToggleAlwaysRun();

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_SetState__doc,
	     "===== Button_SetState =====\n\
\n\
**Metaclass Prototype:** SetState (State)\n\
\n\
**Description:** Sets the state of a Button Control. Doesn't work if the button \n\
is a checkbox or a radio button though, their states are handled internally.\n\
\n\
**Parameters:**\n\
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
**See also:** [Button_SetFlags](Button_SetFlags.md)");

static PyObject* GemRB_Button_SetState(PyObject* self, PyObject* args)
{
	Button::State state;
	PARSE_ARGS(args, "OB", &self, &state);

	Button* btn = GetView<Button>(self);
	ABORT_IF_NULL(btn);

	btn->SetState(state);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_SetPictureClipping__doc,
	     "===== Button_SetPictureClipping =====\n\
\n\
**Metaclass Prototype:** SetPictureClipping (ClippingRatio)\n\
\n\
**Description:** Sets percent (0-1.0) of width to which button picture \n\
will be clipped. This clipping cannot be used simultaneously with \n\
SetButtonOverlay().\n\
\n\
**Parameters:** \n\
  * ClippingRatio  - a floating point value from the 0-1 interval\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetPicture](Button_SetPicture.md), [Button_SetOverlay](Button_SetOverlay.md)");

static PyObject* GemRB_Button_SetPictureClipping(PyObject* self, PyObject* args)
{
	double Clipping;
	PARSE_ARGS(args, "Od", &self, &Clipping);

	Button* btn = GetView<Button>(self);
	ABORT_IF_NULL(btn);

	if (Clipping < 0.0)
		Clipping = 0.0;
	else if (Clipping > 1.0)
		Clipping = 1.0;
	btn->SetPictureClipping(Clipping);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_SetPicture__doc,
	     "===== Button_SetPicture =====\n\
\n\
**Metaclass Prototype:** SetPicture (PictureResRef[, DefaultResRef])\n\
\n\
**Description:** Sets the Picture of a Button Control from a BMP file or a Sprite2D.\n\
\n\
**Parameters:**\n\
  * PictureResRef - the name of the picture (a .bmp resref) or a Sprite2D object\n\
  * DefaultResRef - an alternate bmp should the picture be nonexistent\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetPicture](Button_SetPicture.md), [Button_SetPLT](Button_SetPLT.md), [Button_SetSprites](Button_SetSprites.md), [Button_SetPictureClipping](Button_SetPictureClipping.md), [Window_SetPicture](Window_SetPicture.md)");

static PyObject* GemRB_Button_SetPicture(PyObject* self, PyObject* args)
{
	PyObject *pypic, *pydefaultPic = NULL;
	PARSE_ARGS(args, "OO|O", &self, &pypic, &pydefaultPic);

	Button* btn = GetView<Button>(self);
	if (!btn) {
		return RuntimeError("Cannot find the button!\n");
	}

	if (pypic == Py_None) {
		// clear the picture by passing None
		btn->SetPicture(NULL);
	} else {
		Holder<Sprite2D> pic = SpriteFromPy(pypic);

		if (!pic && pydefaultPic) {
			// use default pic
			pic = SpriteFromPy(pydefaultPic);
		}
		if (!pic) {
			return RuntimeError("Picture resource not found!\n");
		}

		btn->SetPicture(std::move(pic));
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_SetPLT__doc,
	     "===== Button_SetPLT =====\n\
\n\
**Metaclass Prototype:** SetPLT (PLTResRef, col1, col2, col3, col4, col5, col6, col7, col8[, type])\n\
\n\
**Description:** Sets the Picture of a Button Control from a PLT file. \n\
Sets up the palette based on the eight given gradient colors.\n\
\n\
**Parameters:**\n\
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
**See also:** [Button_SetPicture](Button_SetPicture.md)");

static PyObject* GemRB_Button_SetPLT(PyObject* self, PyObject* args)
{
	ieDword col[8];
	int type = 0;
	PyObject* pyref;

	memset(col, -1, sizeof(col));
	if (!PyArg_ParseTuple(args, "OOiiiiiiii|i", &self,
			      &pyref, &(col[0]), &(col[1]), &(col[2]), &(col[3]),
			      &(col[4]), &(col[5]), &(col[6]), &(col[7]), &type)) {
		return NULL;
	}

	Button* btn = GetView<Button>(self);
	ABORT_IF_NULL(btn);

	ResRef ResRef = ResRefFromPy(pyref);

	//empty image
	if (ResRef.IsEmpty() || IsStar(ResRef)) {
		btn->SetPicture(NULL);
		Py_RETURN_NONE;
	}

	Holder<Sprite2D> Picture;
	Holder<Sprite2D> Picture2;

	// NOTE: it seems nobody actually wants to use external palettes!
	// the only users with external plts are in bg2, but they don't match the bam:
	//   lvl9 shapeshift targets: troll, golem, fire elemental, illithid, wolfwere
	// 1pp deliberately breaks palettes for the bam to be used (so the original did support)
	// ... but also not all are identical and we'd be missing half-orcs
	// so we need to prefer PLTs to BAMs, but avoid bad ones
	ResourceHolder<PalettedImageMgr> im = gamedata->GetResourceHolder<PalettedImageMgr>(ResRef, true, IE_PLT_CLASS_ID);
	if (!im) {
		// the PLT doesn't exist or is bad, so try BAM
		Picture = GetPaperdollImage(ResRef, col[0] == 0xFFFFFFFF ? 0 : col, Picture2, (unsigned int) type);
		if (!Picture) {
			Log(ERROR, "Button_SetPLT", "Paperdoll picture is null ({})", ResRef);
			Py_RETURN_NONE;
		}
	} else {
		// use PLT
		Picture = im->GetSprite2D(type, col);
		if (!Picture) {
			Log(ERROR, "Button_SetPLT", "Picture is null ({})", ResRef);
		}
	}

	if (type == 0)
		btn->ClearPictureList();
	btn->StackPicture(Picture);
	if (Picture2) {
		btn->SetFlags(IE_GUI_BUTTON_BG1_PAPERDOLL, BitOp::OR);
		btn->StackPicture(Picture2);
	} else if (type == 0) {
		btn->SetFlags(IE_GUI_BUTTON_BG1_PAPERDOLL, BitOp::NAND);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetSprite__doc,
	     "===== GetSprite =====\n\
\n\
**Prototype:** GemRB.GetSprite (resref[, grad, cycle, frame])\n\
\n\
**Description:** Return a Sprite2D for a given resref. If \n\
the supplied color gradient value is the default -1, then no palette change, \n\
if it is >=0, then it changes the 4-16 palette entries of the Sprite palette. Since it \n\
uses 12 colors palette, it has issues in PST.\n\
\n\
**Parameters:**\n\
  * resref - the name of the BAM animation (a .bam resref)\n\
  * grad - the gradient number, (-1 is no gradient)\n\
  * cycle, frame - the cycle and frame index of the picture in the bam\n\
\n\
**Return value:** N/A\n");

static PyObject* GemRB_GetSprite(PyObject* /*self*/, PyObject* args)
{
	int cycle = 0;
	int frame = 0;
	int palidx = -1;
	PyObject* pyObj;
	PARSE_ARGS(args, "O|iii", &pyObj, &palidx, &cycle, &frame);

	Holder<Sprite2D> spr;
	if (PyUnicode_Check(pyObj)) {
		const ResRef& resref = ResRefFromPy(pyObj);
		auto af = gamedata->GetFactoryResourceAs<const AnimationFactory>(resref, IE_BAM_CLASS_ID);
		if (af) {
			spr = af->GetFrame(frame, cycle);
		}
	}

	if (spr == nullptr) {
		spr = SpriteFromPy(pyObj);
	}

	if (spr == nullptr) {
		Py_RETURN_NONE;
	}

	if (palidx >= 0) {
		spr = spr->copy();
		auto pal = spr->GetPalette();
		ABORT_IF_NULL(pal);
		Holder<Palette> newpal = MakeHolder<Palette>(*pal);
		const auto& pal16 = core->GetPalette16(static_cast<uint8_t>(palidx));
		newpal->CopyColors(4, &pal16[0], &pal16[12]);
		spr->SetPalette(newpal);
	}

	return PyObject_FromHolder<Sprite2D>(std::move(spr));
}

PyDoc_STRVAR(GemRB_Button_SetAnimation__doc,
	     "===== Button_SetAnimation =====\n\
\n\
**Metaclass Prototype:** SetAnimation (Animation[, Cycle, Flags, Cols])\n\
\n\
**Description:**  Sets the animation of a Button from\n\
a BAM file with optional cycle or a list of Sprite2D objects and a duration.\n\
Optionally Flags can be used to specify the animation flags.\n\
\n\
**Parameters:** \n\
  * GButton - the button\n\
  * Animation - resref of the animation, or a list of Sprite2D\
  * Cycle - (optional) number of the cycle to use if using a BAM, otherwise the duration (in ms) required for the animation.\n\
  * Flags - (optional) set the animation flags \n\
  * Cols - (optional) a list of Colors to apply as the palette\n\
\n\
**Return value:** N/A\n");

static PyObject* GemRB_Button_SetAnimation(PyObject* self, PyObject* args)
{
	PyObject* pyAnim = nullptr;
	int cycle = 0;
	int flags = 0;
	PyObject* cols = nullptr;
	PARSE_ARGS(args, "OO|iiO", &self, &pyAnim, &cycle, &flags, &cols);

	Button* btn = GetView<Button>(self);
	ABORT_IF_NULL(btn);

	if (pyAnim == Py_None) {
		btn->SetAnimation(nullptr);
		Py_RETURN_NONE;
	}

	if (cols && !PyList_Check(cols)) {
		return RuntimeError("Invalid argument for 'cols'");
	}

	float fps = ANI_DEFAULT_FRAMERATE;
	std::shared_ptr<Animation> anim;
	if (PyUnicode_Check(pyAnim)) {
		const ResRef& ref = ResRefFromPy(pyAnim);
		auto af = gamedata->GetFactoryResourceAs<AnimationFactory>(ref, IE_BAM_CLASS_ID);
		ABORT_IF_NULL(af);
		anim.reset(af->GetCycle(cycle));
	} else if (PyList_Check(pyAnim)) {
		std::vector<Holder<Sprite2D>> frames;
		for (Py_ssize_t i = 0; i < PyList_Size(pyAnim); ++i) {
			PyObject* item = PyList_GetItem(pyAnim, i);
			frames.push_back(SpriteFromPy(item));
		}

		fps = frames.size() / (cycle / 1000.0f);
		anim = std::make_shared<Animation>(std::move(frames), fps);
	}

	ABORT_IF_NULL(anim);

	if (cols) {
		ieDword indices[8] {};
		Py_ssize_t min = std::min<Py_ssize_t>(8, PyList_Size(cols));
		for (Py_ssize_t i = 0; i < min; i++) {
			PyObject* item = PyList_GetItem(cols, i);
			indices[i] = static_cast<ieDword>(PyLong_AsLong(item));
		}
		// assumes all sprites share a palette
		auto spr = anim->GetFrame(0);
		auto pal = spr->GetPalette();
		*pal = SetupPaperdollColours(indices, 0);
	}

	constexpr auto GAMEANIM = Animation::Flags::Unused; // repurpose the unused bit
	Animation::Flags animFlags = Animation::Flags(flags);
	anim->fps = fps;
	anim->flags = (animFlags | Animation::Flags::Active) & ~GAMEANIM;
	anim->gameAnimation = bool(animFlags & GAMEANIM);

	btn->SetAnimation(new SpriteAnimation(std::move(anim)));

	Py_RETURN_NONE;
}


PyDoc_STRVAR(GemRB_ValidTarget__doc,
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
**See also:** [GameControlSetTargetMode](GameControlSetTargetMode.md)\n\
\n\
**Return value:** boolean");

static PyObject* GemRB_ValidTarget(PyObject* /*self*/, PyObject* args)
{
	int globalID, flags;
	PARSE_ARGS(args, "ii", &globalID, &flags);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	RETURN_BOOL(actor->ValidTarget(flags, actor));
}


PyDoc_STRVAR(GemRB_VerbalConstant__doc,
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
**Return value:** N/A");

static PyObject* GemRB_VerbalConstant(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	Verbal str;

	if (!PyArg_ParseTuple(args, "iI", &globalID, &str)) {
		return AttributeError(GemRB_VerbalConstant__doc);
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (str >= Verbal::count) {
		return AttributeError("SoundSet Entry is too large");
	}

	//get soundset based string constant
	std::string sound = fmt::format("{}{}{}{:02d}", fmt::WideToChar { actor->PCStats->SoundFolder }, PathDelimiter, actor->PCStats->SoundSet, str);
	SFXChannel channel = actor->InParty ? SFXChannel(ieByte(SFXChannel::Char0) + actor->InParty - 1) : SFXChannel::Dialog;
	core->GetAudioDrv()->Play(sound, channel, Point(), GEM_SND_SPEECH | GEM_SND_EFX);
	Py_RETURN_NONE;
}


PyDoc_STRVAR(GemRB_PlaySound__doc,
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
  * DefSoundIndex - the sound index into defsound.2da\n\
  * channel - the name of the channel the sound should be played on (optional, defaults to 'GUI'\n\
  * xpos - x coordinate of the position where the sound should be played (optional)\n\
  * ypos - y coordinate of the position where the sound should be played (optional)\n\
  * type - defaults to 1, use 4 for speeches or other sounds that should stop the previous sounds (optional)\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [LoadMusicPL](LoadMusicPL.md)");

static PyObject* GemRB_PlaySound(PyObject* /*self*/, PyObject* args)
{
	char* channel_name = NULL;
	Point pos;
	unsigned int flags = 0;
	SFXChannel channel = SFXChannel::GUI;
	int index;

	if (PyArg_ParseTuple(args, "i|z", &index, &channel_name)) {
		if (channel_name) {
			channel = core->GetAudioDrv()->GetChannel(channel_name);
		}
		core->PlaySound(index, channel);
	} else {
		PyErr_Clear(); //clearing the exception
		PyObject* pyref = nullptr;
		if (!PyArg_ParseTuple(args, "O|ziii", &pyref, &channel_name, &pos.x, &pos.y, &flags)) {
			return AttributeError(GemRB_PlaySound__doc);
		}

		if (channel_name) {
			channel = core->GetAudioDrv()->GetChannel(channel_name);
		}

		if (pyref == Py_None) {
			core->GetAudioDrv()->Play("", channel, pos, flags);
		} else if (PyUnicode_Check(pyref)) {
			core->GetAudioDrv()->PlayMB(PyString_AsStringObj(pyref), channel, pos, flags);
		} else {
			core->GetAudioDrv()->Play(PyString_AsStringView(pyref), channel, pos, flags);
		}
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Quit__doc,
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
**See also:** [QuitGame](QuitGame.md)\n\
");

static PyObject* GemRB_Quit(PyObject* /*self*/, PyObject* /*args*/)
{
	core->QuitFlag |= QF_EXITGAME;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_LoadMusicPL__doc,
	     "===== LoadMusicPL =====\n\
\n\
**Prototype:** GemRB.LoadMusicPL (MusicPlayListResource[, HardEnd])\n\
\n\
**Description:** Loads and starts a Music PlayList.\n\
\n\
**Parameters:**\n\
  * MusicPlayListResource - a .mus resref\n\
  * HardEnd - off by default, set to 1 to disable the fading at the end\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [SoftEndPL](SoftEndPL.md), [HardEndPL](HardEndPL.md)");

static PyObject* GemRB_LoadMusicPL(PyObject* /*self*/, PyObject* args)
{
	const char* pl = nullptr;
	int HardEnd = 0;
	PARSE_ARGS(args, "s|i", &pl, &HardEnd);

	core->GetMusicMgr()->SwitchPlayList(ieVariable(pl), HardEnd);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SoftEndPL__doc,
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
**See also:** [HardEndPL](HardEndPL.md)");

static PyObject* GemRB_SoftEndPL(PyObject* /*self*/, PyObject* /*args*/)
{
	core->GetMusicMgr()->End();

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_HardEndPL__doc,
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
**See also:** [SoftEndPL](SoftEndPL.md)\n\
");

static PyObject* GemRB_HardEndPL(PyObject* /*self*/, PyObject* /*args*/)
{
	core->GetMusicMgr()->HardEnd();

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetToken__doc,
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
  *  Value        - string or None, the value of the token\n\
\n\
**Examples:**\n\
\n\
    ClassTitle = CommonTables.Classes.GetValue (Class, 'CAP_REF', GTV_REF)\n\
    GemRB.SetToken ('CLASS', ClassTitle)\n\
    # force an update of the string by refetching it\n\
    TextArea.SetText (GemRB.GetString (16480))\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [GetToken](GetToken.md), [Control_QueryText](Control_QueryText.md), [Control_SetText](Control_SetText.md)\n\
");

static PyObject* GemRB_SetToken(PyObject* /*self*/, PyObject* args)
{
	PyObject* Variable;
	PyObject* value;
	PARSE_ARGS(args, "OO", &Variable, &value);

	if (value == Py_None) {
		core->GetTokenDictionary().erase(ieVariableFromPy(Variable));
	} else {
		core->GetTokenDictionary()[ieVariableFromPy(Variable)] = PyString_AsStringObj(value);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetVar__doc,
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
\n\
    GemRB.SetVar ('ActWinID', ActionsWindow.ID)\n\
    GemRB.SetVar ('ActionsPosition', 4)\n\
\n\
**See also:** [Control_SetVarAssoc](Control_SetVarAssoc.md), [SetToken](SetToken.md), [LoadGame](LoadGame.md), [HideGUI](HideGUI.md)");

static PyObject* GemRB_SetVar(PyObject* /*self*/, PyObject* args)
{
	PyObject* Variable;
	PyObject* pynum = nullptr;
	PARSE_ARGS(args, "OO", &Variable, &pynum);

	Control::value_t val = Control::INVALID_VALUE;
	if (PyLong_Check(pynum)) {
		val = Control::value_t(PyLong_AsUnsignedLongMask(pynum));
	} else if (pynum != Py_None) {
		return RuntimeError("Expected a numeric or None type.");
	}

	core->GetDictionary().Set(PyString_AsStringView(Variable), val);

	//this is a hack to update the settings deeper in the core
	UpdateActorConfig();
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetGUIFlags__doc,
	     "===== GetGUIFlags =====\n\
\n\
**Prototype:** GemRB.GetGUIFlags ()\n\
\n\
**Description:** Returns current GUI flags. Works only when a game is loaded.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** integer (GS_ flag bits)");

static PyObject* GemRB_GetGUIFlags(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyLong_FromLong(game->ControlStatus);
}

PyDoc_STRVAR(GemRB_GetToken__doc,
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
**Return value:** string value of the token or None if it doesn't exist\n\
\n\
**Examples:**\n\
\n\
    TextArea.Append (GemRB.GetToken ('CHARNAME'))\n\
\n\
The above example will add the protagonist's name to the TextArea (if the token was set correctly).\n\
\n\
**See also:** [SetToken](SetToken.md), [Control_QueryText](Control_QueryText.md)\n\
");

static PyObject* GemRB_GetToken(PyObject* /*self*/, PyObject* args)
{
	PyObject* Variable;
	PARSE_ARGS(args, "O", &Variable);

	auto& tokens = core->GetTokenDictionary();
	auto lookup = tokens.find(ieVariableFromPy(Variable));
	if (lookup != tokens.cend()) {
		return PyString_FromStringObj(lookup->second);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetVar__doc,
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
\n\
    selected = GemRB.GetVar ('SelectedMovie')\n\
\n\
**See also:** [SetVar](SetVar.md), [Control_SetVarAssoc](Control_SetVarAssoc.md)\n\
");

static PyObject* GemRB_GetVar(PyObject* /*self*/, PyObject* args)
{
	PyObject* Variable;
	PARSE_ARGS(args, "O", &Variable);

	int32_t value = core->GetDictionary().Get(PyString_AsStringView(Variable), -1);
	if (value == -1) {
		Py_RETURN_NONE;
	}

	return PyLong_FromLong(value);
}

PyDoc_STRVAR(GemRB_CheckVar__doc,
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
**See also:** [GetGameVar](GetGameVar.md)");

static PyObject* GemRB_CheckVar(PyObject* /*self*/, PyObject* args)
{
	char* Variable;
	PyObject* pyctx = nullptr;
	PARSE_ARGS(args, "sO", &Variable, &pyctx);
	GET_GAMECONTROL();

	const Scriptable* Sender = (Scriptable*) gc->GetLastActor();
	if (!Sender) {
		GET_GAME();

		Sender = (Scriptable*) game->GetCurrentArea();
	}

	if (!Sender) {
		Log(ERROR, "GUIScript", "No Sender!");
		return NULL;
	}
	ResRef context = ResRefFromPy(pyctx);
	long value = CheckVariable(Sender, StringParam(Variable), context);
	Log(DEBUG, "GUISCript", "{} {}={}", context, Variable, value);
	return PyLong_FromLong(value);
}

PyDoc_STRVAR(GemRB_SetGlobal__doc,
	     "===== SetGlobal =====\n\
\n\
**Prototype:** GemRB.SetGlobal (VariableName, Context, Value)\n\
\n\
**Description:** Sets a gamescript variable to the specified numeric value.\n\
\n\
**Parameters:** \n\
  * VariableName - name of the variable\n\
  * Context - LOCALS, GLOBALS, MYAREA or area specific\n\
  * Value - value to set\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [SetVar](SetVar.md), [Control_SetVarAssoc](Control_SetVarAssoc.md), [SetToken](SetToken.md)");

static PyObject* GemRB_SetGlobal(PyObject* /*self*/, PyObject* args)
{
	char* Variable;
	PyObject* pyctx;
	int Value;
	PARSE_ARGS(args, "sOi", &Variable, &pyctx, &Value);

	Scriptable* Sender = NULL;

	GET_GAME();

	ResRef context = ResRefFromPy(pyctx);
	if (context == "MYAREA" || context == "LOCALS") {
		GET_GAMECONTROL();

		Sender = (Scriptable*) gc->GetLastActor();
		if (!Sender) {
			Sender = (Scriptable*) game->GetCurrentArea();
		}
		if (!Sender) {
			Log(ERROR, "GUIScript", "No Sender!");
			return NULL;
		}
	} // else GLOBAL, area name or KAPUTZ

	SetVariable(Sender, StringParam(Variable), Value, context);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetGameVar__doc,
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
**Return value:** numeric\n\
\n\
**Examples:**\n\
\n\
    Chapter = GemRB.GetGameVar ('chapter')\n\
\n\
**See also:** [GetVar](GetVar.md), [GetToken](GetToken.md), [CheckVar](CheckVar.md)");

static PyObject* GemRB_GetGameVar(PyObject* /*self*/, PyObject* args)
{
	PyObject* variable;
	PARSE_ARGS(args, "O", &variable);

	GET_GAME();

	return PyLong_FromLong((unsigned long) game->GetGlobal(ieVariableFromPy(variable), 0));
}

PyDoc_STRVAR(GemRB_PlayMovie__doc,
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
**See also:** [SetVar](SetVar.md), [GetVar](GetVar.md)\n\
");

static PyObject* GemRB_PlayMovie(PyObject* /*self*/, PyObject* args)
{
	PyObject* string;
	int flag = 0;
	PARSE_ARGS(args, "O|i", &string, &flag);

	ResRef resref = ResRefFromPy(string);
	//Lookup will leave the flag untouched if it doesn't exist yet
	ieDword ind = core->GetDictionary().Get(resref, 0);
	if (flag)
		ind = 0;
	if (!ind) {
		ind = core->PlayMovie(resref);
	}
	return PyLong_FromLong(ind);
}

PyDoc_STRVAR(GemRB_DumpActor__doc,
	     "===== DumpActor =====\n\
\n\
**Prototype:** GemRB.DumpActor (globalID)\n\
\n\
**Description:** Prints the character's debug dump\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** N/A");
static PyObject* GemRB_DumpActor(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PARSE_ARGS(args, "i", &globalID);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->dump();
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SaveCharacter__doc,
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
**Examples:** \n\
\n\
    pc = GemRB.GameGetSelectedPCSingle ()\n\
    GemRB.SaveCharacter (pc, ExportFileName)\n\
\n\
The above example exports the currently selected character.\n\
\n\
**See also:** [CreatePlayer](CreatePlayer.md)");

static PyObject* GemRB_SaveCharacter(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PyObject* name = nullptr;
	PARSE_ARGS(args, "iO", &globalID, &name);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyLong_FromLong(core->WriteCharacter(PyString_AsStringView(name), actor));
}


PyDoc_STRVAR(GemRB_SaveConfig__doc,
	     "===== SaveConfig =====\n\
\n\
**Prototype:** GemRB.SaveConfig ()\n\
\n\
**Description:** Exports the game configuration to a file.\n\
\n\
**Return value:** bool denoting success");

static PyObject* GemRB_SaveConfig(PyObject* /*self*/, PyObject* /*args*/)
{
	UpdateActorConfig(); // Button doesn't trigger this in its OnMouseUp handler where it calls SetVar
	return PyBool_FromLong(core->SaveConfig());
}

PyDoc_STRVAR(GemRB_SaveGame__doc,
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
**Return value:** 0 on success\n\
\n\
**Examples:** \n\
\n\
    GemRB.SaveGame (10, 'After meeting Dhall')\n\
\n\
**See also:** [LoadGame](LoadGame.md), [SaveCharacter](SaveCharacter.md)");

static PyObject* GemRB_SaveGame(PyObject* /*self*/, PyObject* args)
{
	PyObject* obj;
	int slot = -1;
	int Version = -1;
	PyObject* folder = nullptr;

	if (!PyArg_ParseTuple(args, "OO|i", &obj, &folder, &Version)) {
		PyErr_Clear();
		PARSE_ARGS(args, "i|i", &slot, &Version);
	}

	GET_GAME();

	const SaveGameIterator* sgip = core->GetSaveGameIterator();
	if (!sgip) {
		return RuntimeError("No savegame iterator");
	}

	if (Version > 0) {
		game->version = Version;
	}
	if (slot == -1) {
		CObject<SaveGame> save(obj);

		return PyLong_FromLong(sgip->CreateSaveGame(save, PyString_AsStringObj(folder)));
	} else {
		return PyLong_FromLong(sgip->CreateSaveGame(slot, core->config.MultipleQuickSaves));
	}
}

PyDoc_STRVAR(GemRB_GetSaveGames__doc,
	     "===== GetSaveGames =====\n\
\n\
**Prototype:** GemRB.GetSaveGameCount ()\n\
\n\
**Description:** Returns a list of saved games.\n\
\n\
**Return value:** python list");

static PyObject* GemRB_GetSaveGames(PyObject* /*self*/, PyObject* /*args*/)
{
	return MakePyList<SaveGame>(core->GetSaveGameIterator()->GetSaveGames());
}

PyDoc_STRVAR(GemRB_DeleteSaveGame__doc,
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
**See also:** [GetSaveGames](GetSaveGames.md)");

static PyObject* GemRB_DeleteSaveGame(PyObject* /*self*/, PyObject* args)
{
	PyObject* Slot;
	PARSE_ARGS(args, "O", &Slot);

	CObject<SaveGame> game(Slot);
	core->GetSaveGameIterator()->DeleteSaveGame(game);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SaveGame_GetName__doc,
	     "===== SaveGame_GetName =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetName ()\n\
\n\
**Metaclass Prototype:** GetName ()\n\
\n\
**Description:**  Returns name of the saved game.\n\
\n\
**Return value:** string/int");

static PyObject* GemRB_SaveGame_GetName(PyObject* /*self*/, PyObject* args)
{
	PyObject* Slot;
	PARSE_ARGS(args, "O", &Slot);

	Holder<SaveGame> save = CObject<SaveGame>(Slot);
	const auto& name = save->GetName();

	return PyString_FromStringObj(name);
}

PyDoc_STRVAR(GemRB_SaveGame_GetDate__doc,
	     "===== SaveGame_GetDate =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetDate ()\n\
\n\
**Metaclass Prototype:** GetDate ()\n\
\n\
**Description:** Returns date of the saved game.\n\
\n\
**Return value:** string/int");

static PyObject* GemRB_SaveGame_GetDate(PyObject* /*self*/, PyObject* args)
{
	PyObject* Slot;
	PARSE_ARGS(args, "O", &Slot);

	Holder<SaveGame> save = CObject<SaveGame>(Slot);
	const std::string& date = save->GetDate();
	return PyUnicode_Decode(date.c_str(), date.length(), core->config.SystemEncoding.c_str(), "strict");
}

PyDoc_STRVAR(GemRB_SaveGame_GetGameDate__doc,
	     "===== SaveGame_GetGameDate =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetGameDate ()\n\
\n\
**Metaclass Prototype:** GetGameDate ()\n\
\n\
**Description:** Returns game date of the saved game.\n\
\n\
**Return value:** string/int");

static PyObject* GemRB_SaveGame_GetGameDate(PyObject* /*self*/, PyObject* args)
{
	PyObject* Slot;
	PARSE_ARGS(args, "O", &Slot);

	Holder<SaveGame> save = CObject<SaveGame>(Slot);
	const std::string& date = save->GetGameDate();
	return PyUnicode_Decode(date.c_str(), date.length(), core->config.SystemEncoding.c_str(), "strict");
}

PyDoc_STRVAR(GemRB_SaveGame_GetSaveID__doc,
	     "===== SaveGame_GetSaveID =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetSaveID ()\n\
\n\
**Metaclass Prototype:** GetSaveID ()\n\
\n\
**Description:** Returns ID of the saved game.\n\
\n\
**Return value:** string/int");

static PyObject* GemRB_SaveGame_GetSaveID(PyObject* /*self*/, PyObject* args)
{
	PyObject* Slot;
	PARSE_ARGS(args, "O", &Slot);

	Holder<SaveGame> save = CObject<SaveGame>(Slot);
	return PyLong_FromLong(save->GetSaveID());
}

PyDoc_STRVAR(GemRB_SaveGame_GetPreview__doc,
	     "===== SaveGame_GetPreview =====\n\
\n\
**Prototype:** GemRB.SaveGame_GetPreview ()\n\
\n\
**Metaclass Prototype:** GetPreview ()\n\
\n\
**Description:** Returns preview of the saved game.\n\
\n\
**Return value:** string/int");

static PyObject* GemRB_SaveGame_GetPreview(PyObject* /*self*/, PyObject* args)
{
	PyObject* Slot;
	PARSE_ARGS(args, "O", &Slot);

	Holder<SaveGame> save = CObject<SaveGame>(Slot);
	return PyObject_FromHolder<Sprite2D>(save->GetPreview());
}

PyDoc_STRVAR(GemRB_SaveGame_GetPortrait__doc,
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
**Return value:** string/int");

static PyObject* GemRB_SaveGame_GetPortrait(PyObject* /*self*/, PyObject* args)
{
	PyObject* Slot;
	int index;
	PARSE_ARGS(args, "Oi", &Slot, &index);

	Holder<SaveGame> save = CObject<SaveGame>(Slot);
	return PyObject_FromHolder<Sprite2D>(save->GetPortrait(index));
}

PyDoc_STRVAR(GemRB_GetGamePreview__doc,
	     "===== GetGamePreview =====\n\
\n\
**Prototype:** GemRB.GetGamePreview ()\n\
\n\
**Description:** Gets current game area preview.\n\
\n\
**Return value:** python image object");

static PyObject* GemRB_GetGamePreview(PyObject* /*self*/, PyObject* /*args*/)
{
	// FIXME: this method should be removed
	// A SaveGame object should be created prior to this (not the *actual* save files)
	// from that SaveGame we have methods for SaveGame_GetPortrait and SaveGame_GetPreview
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Roll__doc,
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
**Examples:** \n\
\n\
    dice = 3\n\
    size = 5\n\
    v = GemRB.Roll (dice, size, 3)\n\
\n\
The above example generates a 3d5+3 number.");

static PyObject* GemRB_Roll(PyObject* /*self*/, PyObject* args)
{
	int Dice, Size, Add;
	PARSE_ARGS(args, "iii", &Dice, &Size, &Add);
	return PyLong_FromLong(core->Roll(Dice, Size, Add));
}

PyDoc_STRVAR(GemRB_TextArea_ListResources__doc,
	     "===== TextArea_ListResources =====\n\
\n\
**Metaclass Prototype:** ListResources (type [, flags])\n\
\n\
**Description:** Lists the resources of 'type' as selectable options in the TextArea.\n\
\n\
**Parameters:** \n\
  * type - one of CHR_PORTRAITS, CHR_SOUNDS, CHR_EXPORTS or CHR_SCRIPTS\n\
  * flags:\n\
    - for CHR_PORTRAITS chooses suffix: 0 'M', 1 'S', 2 'L'\n\
\n\
**Return value:** list - the list of options added to the TextArea");

static PyObject* GemRB_TextArea_ListResources(PyObject* self, PyObject* args)
{
	RESOURCE_DIRECTORY type;
	int flags = 0;
	PARSE_ARGS(args, "Oi|i", &self, &type, &flags);
	TextArea* ta = GetView<TextArea>(self);
	ABORT_IF_NULL(ta);

	DirectoryIterator dirit = core->GetResourceDirectory(type);
	bool dirs = false;
	auto suffix = "S";
	switch (type) {
		case DIRECTORY_CHR_PORTRAITS:
			if (flags & 1) suffix = "M";
			if (flags & 2) suffix = "L";
			dirit.SetFilterPredicate(std::make_shared<EndsWithFilter>(suffix), true);
			break;
		case DIRECTORY_CHR_SOUNDS:
			if (core->HasFeature(GFFlags::SOUNDFOLDERS)) {
				dirs = true;
			} else {
				dirit.SetFilterPredicate(std::make_shared<EndsWithFilter>("A"), true);
			}
			break;
		case DIRECTORY_CHR_EXPORTS:
		case DIRECTORY_CHR_SCRIPTS:
		default:
			break;
	}

	int itflags = DirectoryIterator::Files;
	itflags |= dirs ? DirectoryIterator::Directories : 0;
	dirit.SetFlags(itflags, true);

	std::vector<String> strings;
	if (dirit) {
		do {
			const path_t& name = dirit.GetName();
			if (name[0] == '.' || dirit.IsDirectory() != dirs)
				continue;

			String string = StringFromUtf8(name.c_str());

			if (dirs == false) {
				size_t pos = string.find_last_of(u'.');
				if (pos == String::npos || (type == DIRECTORY_CHR_SOUNDS && pos-- == 0)) {
					continue;
				}
				string.resize(pos);
			}
			strings.emplace_back(std::move(string));
		} while (++dirit);
	}

	std::vector<SelectOption> TAOptions;
	std::sort(strings.begin(), strings.end());
	for (size_t i = 0; i < strings.size(); i++) {
		TAOptions.emplace_back(i, strings[i]);
	}
	ta->SetSelectOptions(TAOptions, false);

	return MakePyList<const String&, PyString_FromStringObj>(strings);
}

PyDoc_STRVAR(GemRB_TextArea_SetOptions__doc,
	     "===== TextArea_SetOptions =====\n\
\n\
**Metaclass Prototype:** SetOptions (Options)\n\
\n\
**Description:** Set the selectable options for the TextArea\n\
\n\
**Parameters:** \n\
  * Options - python list of options\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_TextArea_SetOptions(PyObject* self, PyObject* args)
{
	PyObject* list;
	PARSE_ARGS(args, "OO", &self, &list);

	if (!PyList_Check(list)) {
		return NULL;
	}

	TextArea* ta = GetView<TextArea>(self);
	ABORT_IF_NULL(ta);

	// FIXME: should we have an option to sort the list?
	// PyList_Sort(list);
	std::vector<SelectOption> TAOptions;
	PyObject* item = NULL;
	for (int i = 0; i < PyList_Size(list); i++) {
		item = PyList_GetItem(list, i);
		String string;
		if (!PyUnicode_Check(item)) {
			if (PyLong_Check(item)) {
				string = String(core->GetString(StrRefFromPy(item)));
			} else {
				return NULL;
			}
		} else {
			string = PyString_AsStringObj(item);
		}
		TAOptions.emplace_back(i, std::move(string));
	}
	ta->SetSelectOptions(TAOptions, false);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetPartySize__doc,
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
**See also:** [LoadGame](LoadGame.md), [QuitGame](QuitGame.md), [GameSetPartySize](GameSetPartySize.md)");

static PyObject* GemRB_GetPartySize(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyLong_FromLong(game->GetPartySize(false));
}

PyDoc_STRVAR(GemRB_GetGameTime__doc,
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
**See also:** [GameGetPartyGold](GameGetPartyGold.md), [GetPartySize](GetPartySize.md)\n\
");

static PyObject* GemRB_GetGameTime(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	unsigned long GameTime = game->GameTime / core->Time.defaultTicksPerSec;
	return PyLong_FromLong(GameTime);
}

PyDoc_STRVAR(GemRB_GameGetReputation__doc,
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
**See also:** [GetPlayerStat](GetPlayerStat.md), [GameSetReputation](GameSetReputation.md)\n\
");

static PyObject* GemRB_GameGetReputation(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyLong_FromLong((int) game->Reputation);
}

PyDoc_STRVAR(GemRB_GameSetReputation__doc,
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
**See also:** [GameGetReputation](GameGetReputation.md), [IncreaseReputation](IncreaseReputation.md)");

static PyObject* GemRB_GameSetReputation(PyObject* /*self*/, PyObject* args)
{
	int Reputation;
	PARSE_ARGS(args, "i", &Reputation);
	GET_GAME();

	game->SetReputation((unsigned int) Reputation);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_IncreaseReputation__doc,
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
**See also:** [GameGetReputation](GameGetReputation.md), [GameGetPartyGold](GameGetPartyGold.md), [GameSetPartyGold](GameSetPartyGold.md)");

static PyObject* GemRB_IncreaseReputation(PyObject* /*self*/, PyObject* args)
{
	int Donation;
	int Increase = 0;
	PARSE_ARGS(args, "i", &Donation);

	GET_GAME();

	int Limit = gamedata->GetReputationMod(8);
	if (Limit > Donation) {
		return PyLong_FromLong(0);
	}
	Increase = gamedata->GetReputationMod(4);
	if (Increase) {
		game->SetReputation(game->Reputation + Increase);
	}
	return PyLong_FromLong(Increase);
}

PyDoc_STRVAR(GemRB_GameGetPartyGold__doc,
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
**See also:** [GetPlayerStat](GetPlayerStat.md), [GameSetPartyGold](GameSetPartyGold.md)");

static PyObject* GemRB_GameGetPartyGold(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	int Gold = game->PartyGold;
	return PyLong_FromLong(Gold);
}

PyDoc_STRVAR(GemRB_GameSetPartyGold__doc,
	     "===== GameSetPartyGold =====\n\
\n\
**Prototype:** GemRB.GameSetPartyGold (Gold)\n\
\n\
**Description:** Sets current party gold.\n\
\n\
**Parameters:**\n\
  * Gold - the target party gold amount\n\
  * feedback - optional, off by default\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [GameGetPartyGold](GameGetPartyGold.md)");

static PyObject* GemRB_GameSetPartyGold(PyObject* /*self*/, PyObject* args)
{
	int Gold, flag = 0;
	PARSE_ARGS(args, "i|i", &Gold, &flag);
	GET_GAME();

	if (flag) {
		game->AddGold(Gold);
	} else {
		game->PartyGold = Gold;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GameGetFormation__doc,
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
**See also:** [GameSetFormation](GameSetFormation.md)");

static PyObject* GemRB_GameGetFormation(PyObject* /*self*/, PyObject* args)
{
	int Which = -1;
	int Formation;
	PARSE_ARGS(args, "|i", &Which);
	GET_GAME();

	if (Which < 0) {
		Formation = game->WhichFormation; // an index, not actual formation
	} else {
		if (Which > 4) {
			return NULL;
		}
		Formation = game->Formations[Which];
	}
	return PyLong_FromLong(Formation);
}

PyDoc_STRVAR(GemRB_GameSetFormation__doc,
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
**See also:** [GameGetFormation](GameGetFormation.md)\n\
");

static PyObject* GemRB_GameSetFormation(PyObject* /*self*/, PyObject* args)
{
	ieWord Formation;
	int Which = -1;
	PARSE_ARGS(args, "H|i", &Formation, &Which);
	GET_GAME();

	if (Which < 0) {
		game->WhichFormation = Formation; // an index, not actual formation
	} else {
		if (Which > 4) {
			return NULL;
		}
		game->Formations[Which] = Formation;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetJournalSize__doc,
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
**See also:** [GetJournalEntry](GetJournalEntry.md), [SetJournalEntry](SetJournalEntry.md)\n\
");

static PyObject* GemRB_GetJournalSize(PyObject* /*self*/, PyObject* args)
{
	int chapter;
	int section = -1;
	PARSE_ARGS(args, "i|i", &chapter, &section);

	GET_GAME();

	int count = 0;
	for (unsigned int i = 0; i < game->GetJournalCount(); i++) {
		const GAMJournalEntry* je = game->GetJournalEntry(i);
		if ((section == -1 || section == je->Section) && (chapter == je->Chapter))
			count++;
	}

	return PyLong_FromLong(count);
}

PyDoc_STRVAR(GemRB_GetJournalEntry__doc,
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
**See also:** [GetJournalSize](GetJournalSize.md), [SetJournalEntry](SetJournalEntry.md)");

static PyObject* GemRB_GetJournalEntry(PyObject* /*self*/, PyObject* args)
{
	int section = -1, index, chapter;
	PARSE_ARGS(args, "ii|i", &chapter, &index, &section);

	GET_GAME();

	int count = 0;
	for (unsigned int i = 0; i < game->GetJournalCount(); i++) {
		const GAMJournalEntry* je = game->GetJournalEntry(i);
		if ((section == -1 || section == je->Section) && (chapter == je->Chapter)) {
			if (index == count) {
				return Py_BuildValue("{s:i,s:i,s:i,s:i}", "Text", (signed) je->Text,
						     "GameTime", je->GameTime,
						     "Section", je->Section, "Chapter", je->Chapter);
			}
			count++;
		}
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetJournalEntry__doc,
	     "===== SetJournalEntry =====\n\
\n\
**Prototype:** GemRB.SetJournalEntry (strref[, section, chapter, feedback])\n\
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
  * feedback - strref, optional different second half of the feedback message\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [GetJournalEntry](GetJournalEntry.md)\n\
");

static PyObject* GemRB_SetJournalEntry(PyObject* /*self*/, PyObject* args)
{
	PyObject* pyref = nullptr;
	ieDword chapter = -1;
	int section = -1;
	PyObject* feedback = nullptr;
	PARSE_ARGS(args, "O|iiO", &pyref, &section, &chapter, &feedback);

	GET_GAME();

	ieStrRef strref;
	if (PyObject_TypeCheck(pyref, &PyLong_Type)) {
		strref = StrRefFromPy(pyref);
		if (strref == ieStrRef::INVALID) {
			// delete the whole journal
			section = -1;
		}
	} else {
		// create new ref and entry
		String text = PyString_AsStringObj(pyref);
		strref = core->strings->GetNextStrRef();
		strref = core->UpdateString(strref, text);
	}

	ieStrRef msg2 = ieStrRef::INVALID;
	if (feedback) msg2 = StrRefFromPy(feedback);

	if (section == -1) {
		//delete one or all entries
		game->DeleteJournalEntry(strref);
	} else {
		if (chapter == ieDword(-1)) {
			chapter = game->GetGlobal("CHAPTER", -1);
		}
		game->AddJournalEntry(strref, (JournalSection) section, (ieByte) chapter, msg2);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GameIsBeastKnown__doc,
	     "===== GameIsBeastKnown =====\n\
\n\
**Prototype:** GemRB.GameIsBeastKnown (index)\n\
\n\
**Description:** Returns whether beast with given index is known to PCs. \n\
Works only in PST.\n\
\n\
**Parameters:**\n\
  * index - the beast's index as of beast.ini\n\
\n\
**Return value:** boolean, 1 means beast is known.\n\
\n\
**See also:** [GetINIBeastsKey](GetINIBeastsKey.md)\n\
");

static PyObject* GemRB_GameIsBeastKnown(PyObject* /*self*/, PyObject* args)
{
	unsigned int index;
	PARSE_ARGS(args, "I", &index);

	GET_GAME();
	return PyLong_FromLong(game->IsBeastKnown(index));
}

PyDoc_STRVAR(GemRB_GetINIPartyCount__doc,
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
**See also:** [GetINIPartyKey](GetINIPartyKey.md)\n\
");

static PyObject* GemRB_GetINIPartyCount(PyObject* /*self*/,
					PyObject* /*args*/)
{
	if (!core->GetPartyINI()) {
		return RuntimeError("INI resource not found!\n");
	}
	return PyLong_FromSize_t(core->GetPartyINI()->GetTagsCount());
}

PyDoc_STRVAR(GemRB_GetINIQuestsKey__doc,
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
**See also:** [GetINIBeastsKey](GetINIBeastsKey.md)\n\
");

static PyObject* GemRB_GetINIQuestsKey(PyObject* /*self*/, PyObject* args)
{
	PyObject* Tag = nullptr;
	PyObject* Key = nullptr;
	PyObject* Default = nullptr;
	PARSE_ARGS(args, "OOO", &Tag, &Key, &Default);
	if (!core->GetQuestsINI()) {
		return RuntimeError("INI resource not found!\n");
	}
	return PyString_FromStringView(core->GetQuestsINI()->GetKeyAsString(PyString_AsStringView(Tag),
									    PyString_AsStringView(Key),
									    PyString_AsStringView(Default)));
}

PyDoc_STRVAR(GemRB_GetINIBeastsKey__doc,
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
**See also:** [GetINIQuestsKey](GetINIQuestsKey.md)");
static PyObject* GemRB_GetINIBeastsKey(PyObject* /*self*/, PyObject* args)
{
	PyObject* Tag = nullptr;
	PyObject* Key = nullptr;
	PyObject* Default = nullptr;
	PARSE_ARGS(args, "OOO", &Tag, &Key, &Default);
	if (!core->GetBeastsINI()) {
		return NULL;
	}
	return PyString_FromStringView(core->GetBeastsINI()->GetKeyAsString(PyString_AsStringView(Tag),
									    PyString_AsStringView(Key),
									    PyString_AsStringView(Default)));
}

PyDoc_STRVAR(GemRB_GetINIPartyKey__doc,
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
**See also:** [GetINIPartyCount](GetINIPartyCount.md)");

static PyObject* GemRB_GetINIPartyKey(PyObject* /*self*/, PyObject* args)
{
	PyObject* Tag = nullptr;
	PyObject* Key = nullptr;
	PyObject* Default = nullptr;
	PARSE_ARGS(args, "OOO", &Tag, &Key, &Default);
	if (!core->GetPartyINI()) {
		return RuntimeError("INI resource not found!\n");
	}
	const StringView desc = core->GetPartyINI()->GetKeyAsString(PyString_AsStringView(Tag), PyString_AsStringView(Key), PyString_AsStringView(Default));
	return PyString_FromStringView(desc);
}

PyDoc_STRVAR(GemRB_CreatePlayer__doc,
	     "===== CreatePlayer =====\n\
\n\
**Prototype:** GemRB.CreatePlayer (CREResRef, Slot [,Import, VersionOverride])\n\
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
\n\
    MyChar = GemRB.GetVar ('Slot')\n\
    GemRB.CreatePlayer ('charbase', MyChar)\n\
\n\
The above example will create a new player character in the slot selected\n\
by the Slot variable.\n\
\n\
    MyChar = GemRB.GetVar ('Slot')\n\
    ImportName = 'avenger'\n\
    GemRB.CreatePlayer (ImportName, MyChar, 1)\n\
\n\
The above example would import avenger.chr into the slot selected by the \n\
Slot Variable. If it exists in the Characters directory of the game.\n\
\n\
**See also:** [LoadGame](LoadGame.md), [EnterGame](EnterGame.md), [QuitGame](QuitGame.md), [FillPlayerInfo](FillPlayerInfo.md), [SetPlayerStat](SetPlayerStat.md)");

static PyObject* GemRB_CreatePlayer(PyObject* /*self*/, PyObject* args)
{
	PyObject* pystr = nullptr;
	int PlayerSlot, Slot;
	int Import = 0;
	int VersionOverride = -1;
	PARSE_ARGS(args, "Oi|ii", &pystr, &PlayerSlot, &Import, &VersionOverride);
	//PlayerSlot is zero based
	Slot = (PlayerSlot & 0x7fff);
	GET_GAME();

	// overwriting original slot?
	if (PlayerSlot & 0x8000) {
		PlayerSlot = game->FindPlayer(Slot);
		if (PlayerSlot >= 0) {
			Map* map = game->GetCurrentArea();
			if (map) {
				Actor* actor = game->GetPC(PlayerSlot, false);
				map->RemoveActor(actor);
			}
			game->DelPC(PlayerSlot, true);
		}
	} else {
		PlayerSlot = game->FindPlayer(PlayerSlot);
		if (PlayerSlot >= 0) {
			return RuntimeError("Slot is already filled!\n");
		}
	}

	ResRef CreResRef = ResRefFromPy(pystr);
	if (!CreResRef.IsEmpty()) {
		PlayerSlot = gamedata->LoadCreature(CreResRef, Slot, (bool) Import, VersionOverride);
	} else {
		//just destroyed the previous actor, not going to create one
		PlayerSlot = 0;
	}
	if (PlayerSlot < 0) {
		return RuntimeError("File not found!\n");
	}
	return PyLong_FromLong(PlayerSlot);
}

PyDoc_STRVAR(GemRB_GetPlayerStates__doc,
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
**See also:** [GetPlayerName](GetPlayerName.md), [GetPlayerStat](GetPlayerStat.md)\n\
");

static PyObject* GemRB_GetPlayerStates(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PARSE_ARGS(args, "i", &globalID);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	auto stats = actor->PCStats->GetStateString();
	return PyByteArray_FromStringAndSize(stats.c_str(), stats.length());
}

PyDoc_STRVAR(GemRB_GetPlayerActionRow__doc,
	     "===== GetPlayerActionRow =====\n\
\n\
**Prototype:** GemRB.GetPlayerActionRow (globalID)\n\
\n\
**Description:** Returns the actor's action bar\n\
\n\
**Parameters:**\n\
  * globalID - the PC's position in the party (1 based)\n\
\n\
**Return value:** tuple with individual action button data\n");

static PyObject* GemRB_GetPlayerActionRow(PyObject* /*self*/, PyObject* args)
{
	int globalID;

	PARSE_ARGS(args, "i", &globalID);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	ActionButtonRow myRow;
	actor->GetActionButtonRow(myRow);

	PyObject* bar = PyTuple_New(GUIBT_COUNT);
	for (int i = 0; i < GUIBT_COUNT; i++) {
		PyTuple_SetItem(bar, i, PyLong_FromLong(myRow[i]));
	}
	return bar;
}

PyDoc_STRVAR(GemRB_GetPlayerLevel__doc,
	     "===== GetPlayerLevel =====\n\
\n\
**Prototype:** GemRB.GetPlayerLevel (globalID, class)\n\
\n\
**Description:** Returns the actor's level in the specified class. Takes\n\
dual-classing into account (inactive duals).\n\
\n\
**Parameters:**\n\
  * globalID - the PC's position in the party (1 based)\n\
  * class - which level to query, see ie_stats.py\n\
\n\
**Return value:** number\n");

static PyObject* GemRB_GetPlayerLevel(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int classis;

	PARSE_ARGS(args, "ii", &globalID, &classis);
	GET_GAME();
	GET_ACTOR_GLOBAL();
	return PyLong_FromLong(actor->GetClassLevel(classis));
}

PyDoc_STRVAR(GemRB_GetPlayerName__doc,
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
**See also:** [SetPlayerName](SetPlayerName.md), [GetPlayerStat](GetPlayerStat.md)");

static PyObject* GemRB_GetPlayerName(PyObject* /*self*/, PyObject* args)
{
	int globalID, Which;

	Which = 0;
	PARSE_ARGS(args, "i|i", &globalID, &Which);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	switch (Which) {
		case 0:
			return PyString_FromStringObj(actor->GetShortName());
		case 1:
			return PyString_FromStringObj(actor->GetLongName());
		case 2:
			return PyString_FromStringView(actor->GetScriptName());
		case -1:
		default:
			return PyString_FromStringObj(actor->GetDefaultName());
	}
}

PyDoc_STRVAR(GemRB_SetPlayerName__doc,
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
**Examples:**\n\
\n\
    GemRB.SetPlayerName (MyChar, GemRB.GetToken('CHARNAME'), 0)\n\
\n\
In the above example we set the player's name to a previously set Token (global string).\n\
\n\
**See also:** [Control_QueryText](Control_QueryText.md), [GetToken](GetToken.md)");

static PyObject* GemRB_SetPlayerName(PyObject* /*self*/, PyObject* args)
{
	PyObject* pyName = nullptr;
	int globalID;
	unsigned char whichName = 0;

	PARSE_ARGS(args, "iO|b", &globalID, &pyName, &whichName);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetName(PyString_AsStringObj(pyName), whichName);
	actor->SetMCFlag(MC_EXPORTABLE, BitOp::OR);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_CreateString__doc,
	     "===== CreateString =====\n\
\n\
**Prototype:** GemRB.CreateString (Strref, Text)\n\
\n\
**Description:** Creates or updates a custom string.\n\
\n\
**Parameters:**\n\
  * Strref - string index to use\n\
  * Text - string contents\n\
\n\
**Return value:** the actual strref of the new string");

static PyObject* GemRB_CreateString(PyObject* /*self*/, PyObject* args)
{
	PyObject* Text = nullptr;
	PyObject* pyref = nullptr;
	PARSE_ARGS(args, "OO", &pyref, &Text);
	GET_GAME();

	ieStrRef strref = core->UpdateString(StrRefFromPy(pyref), PyString_AsStringObj(Text));
	return PyLong_FromStrRef(strref);
}

PyDoc_STRVAR(GemRB_SetPlayerString__doc,
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
**Return value:** N/A");

static PyObject* GemRB_SetPlayerString(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	Verbal StringSlot;
	PyObject* pyref = nullptr;
	PARSE_ARGS(args, "iIO", &globalID, &StringSlot, &pyref);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (StringSlot >= Verbal::count) {
		return AttributeError("StringSlot is out of range!\n");
	}

	actor->StrRefs[StringSlot] = StrRefFromPy(pyref);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetPlayerSound__doc,
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
**See also:** [GetPlayerSound](GetPlayerSound.md), [FillPlayerInfo](FillPlayerInfo.md), [SetPlayerString](SetPlayerString.md)");

static PyObject* GemRB_SetPlayerSound(PyObject* /*self*/, PyObject* args)
{
	PyObject* Sound = nullptr;
	int globalID;
	PARSE_ARGS(args, "iO", &globalID, &Sound);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetSoundFolder(PyString_AsStringObj(Sound));
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetPlayerSound__doc,
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
**See also:** [SetPlayerSound](SetPlayerSound.md)");

static PyObject* GemRB_GetPlayerSound(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int flag = 0;
	PARSE_ARGS(args, "i|i", &globalID, &flag);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	ResRef ignore;
	auto sound = actor->GetSoundFolder(flag, ignore);

	return PyString_FromStringObj(sound);
}

PyDoc_STRVAR(GemRB_GetSlotType__doc,
	     "===== GetSlotType =====\n\
\n\
**Prototype:** GemRB.GetSlotType (idx[, PartyID])\n\
\n\
**Description:** Returns dictionary of an itemslot type (slottype.2da).\n\
Alternatively if idx is -1, returns the 'Count' of inventory items.\n\
\n\
**Parameters:**\n\
  * idx - a row number of slottype.2da\n\
  * PartyID - optional actor ID for a richer dictionary\n\
\n\
**Return value:** dictionary\n\
  * 'Type'   - bitfield, The inventory slot's type.\n\
  * 'ID'     - the gui button's controlID which belongs to this slot.\n\
  * 'Tip'    - the tooltip resref for this slot.\n\
  * 'ResRef' - the background .bam of the slot.\n\
  * 'Flags'  - the slot flags.\n\
  * 'Effects'- the slot effects.\n\
\n\
**See also:** [Button_SetItemIcon](Button_SetItemIcon.md)\n\
");

static PyObject* GemRB_GetSlotType(PyObject* /*self*/, PyObject* args)
{
	int idx;
	int PartyID = 0;
	const Actor* actor = nullptr;
	PARSE_ARGS(args, "i|i", &idx, &PartyID);

	if (PartyID) {
		GET_GAME();

		actor = game->FindPC(PartyID);
	}

	PyObject* dict = PyDict_New();
	if (idx == -1) {
		PyDict_SetItemString(dict, "Count", DecRef(PyLong_FromLong, core->GetInventorySize()));
		return dict;
	}
	int tmp = core->QuerySlot(idx);
	if (core->QuerySlotEffects(idx) == SLOT_EFFECT_ALIAS) {
		tmp = idx;
	}

	PyDict_SetItemString(dict, "Slot", DecRef(PyLong_FromLong, tmp));
	PyDict_SetItemString(dict, "Type", DecRef(PyLong_FromLong, (int) core->QuerySlotType(tmp)));
	PyDict_SetItemString(dict, "ID", DecRef(PyLong_FromLong, (int) core->QuerySlotID(tmp)));
	PyDict_SetItemString(dict, "Tip", DecRef(PyLong_FromLong, (int) core->QuerySlottip(tmp)));
	PyDict_SetItemString(dict, "Flags", PyLong_FromLong((int) core->QuerySlotFlags(tmp)));
	//see if the actor shouldn't have some slots displayed
	int weaponSlot;
	if (!actor || !actor->PCStats) {
		goto has_slot;
	}

	weaponSlot = Inventory::GetWeaponSlot();
	if (tmp < weaponSlot || tmp > weaponSlot + 3) {
		goto has_slot;
	}
	if (actor->GetQuickSlot(tmp - weaponSlot) == 0xffff) {
		PyDict_SetItemString(dict, "ResRef", DecRef(PyString_FromString, ""));
		goto continue_quest;
	}
has_slot:
	PyDict_SetItemString(dict, "ResRef", DecRef(PyString_FromStringView, core->QuerySlotResRef(tmp)));
continue_quest:
	PyDict_SetItemString(dict, "Effects", DecRef(PyLong_FromLong, core->QuerySlotEffects(tmp)));
	return dict;
}

PyDoc_STRVAR(GemRB_GetPCStats__doc,
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
**See also:** [GetPlayerStat](GetPlayerStat.md)");

static PyObject* GemRB_GetPCStats(PyObject* /*self*/, PyObject* args)
{
	int PartyID;
	PARSE_ARGS(args, "i", &PartyID);
	GET_GAME();

	const Actor* MyActor = game->FindPC(PartyID);
	if (!MyActor || !MyActor->PCStats) {
		Py_RETURN_NONE;
	}

	const auto& ps = MyActor->PCStats;

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "BestKilledName", DecRef(PyLong_FromStrRef, ps->BestKilledName));
	PyDict_SetItemString(dict, "BestKilledXP", DecRef(PyLong_FromLong, ps->BestKilledXP));
	PyDict_SetItemString(dict, "AwayTime", DecRef(PyLong_FromLong, ps->AwayTime));
	PyDict_SetItemString(dict, "JoinDate", DecRef(PyLong_FromLong, ps->JoinDate));
	PyDict_SetItemString(dict, "KillsChapterXP", DecRef(PyLong_FromLong, ps->KillsChapterXP));
	PyDict_SetItemString(dict, "KillsChapterCount", DecRef(PyLong_FromLong, ps->KillsChapterCount));
	PyDict_SetItemString(dict, "KillsTotalXP", DecRef(PyLong_FromLong, ps->KillsTotalXP));
	PyDict_SetItemString(dict, "KillsTotalCount", DecRef(PyLong_FromLong, ps->KillsTotalCount));

	static auto GetFav = [](const auto& favs) {
		auto fav = std::max_element(favs.begin(), favs.end(), [](const auto& lhs, const auto& rhs) {
			return lhs.second < rhs.second;
		});
		return fav->first;
	};

	auto favorite = GetFav(ps->FavouriteSpells);
	const Spell* favspell = gamedata->GetSpell(favorite);
	if (favspell) {
		PyDict_SetItemString(dict, "FavouriteSpell", DecRef(PyLong_FromStrRef, favspell->SpellName));
		gamedata->FreeSpell(favspell, favorite, false);
	} else {
		PyDict_SetItemString(dict, "FavouriteSpell", DecRef(PyLong_FromLong, -1));
	}

	favorite = GetFav(ps->FavouriteWeapons);
	const Item* favweap = gamedata->GetItem(favorite);
	if (favweap) {
		PyDict_SetItemString(dict, "FavouriteWeapon", DecRef(PyLong_FromStrRef, favweap->GetItemName(true)));
		gamedata->FreeItem(favweap, favorite, false);
	} else {
		PyDict_SetItemString(dict, "FavouriteWeapon", DecRef(PyLong_FromLong, -1));
	}

	// fill it also with the quickslot info
	PyObject* qss = PyTuple_New(MAX_QSLOTS);
	PyObject* qsb = PyTuple_New(MAX_QSLOTS);
	PyObject* qis = PyTuple_New(MAX_QUICKITEMSLOT);
	PyObject* qih = PyTuple_New(MAX_QUICKITEMSLOT);
	PyObject* qws = PyTuple_New(MAX_QUICKWEAPONSLOT);
	PyObject* qwh = PyTuple_New(MAX_QUICKWEAPONSLOT);
	for (int i = 0; i < MAX_QSLOTS; i++) {
		PyTuple_SetItem(qss, i, PyString_FromResRef(ps->QuickSpells[i]));
		PyTuple_SetItem(qsb, i, PyLong_FromLong(ps->QuickSpellBookType[i]));
	}
	for (int i = 0; i < MAX_QUICKITEMSLOT; i++) {
		PyTuple_SetItem(qis, i, PyLong_FromLong(ps->QuickItemSlots[i]));
		PyTuple_SetItem(qih, i, PyLong_FromLong(ps->QuickItemHeaders[i]));
	}
	for (int i = 0; i < MAX_QUICKWEAPONSLOT; i++) {
		PyTuple_SetItem(qws, i, PyLong_FromLong(ps->QuickWeaponSlots[i]));
		PyTuple_SetItem(qwh, i, PyLong_FromLong(ps->QuickWeaponHeaders[i]));
	}
	PyDict_SetItemString(dict, "QuickSpells", qss);
	PyDict_SetItemString(dict, "QuickSpellsBookType", qsb);
	PyDict_SetItemString(dict, "QuickItemSlots", qis);
	PyDict_SetItemString(dict, "QuickItemHeaders", qih);
	PyDict_SetItemString(dict, "QuickWeaponSlots", qws);
	PyDict_SetItemString(dict, "QuickWeaponHeaders", qwh);

	return dict;
}


PyDoc_STRVAR(GemRB_GameSelectPC__doc,
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
**Examples:**\n\
\n\
    def SelectAllOnPress ():\n\
      GemRB.GameSelectPC (0, 1)\n\
      return\n\
\n\
The above function is associated to the 'select all' button of the GUI screen.\n\
\n\
**See also:** [GameIsPCSelected](GameIsPCSelected.md), [GameSelectPCSingle](GameSelectPCSingle.md), [GameGetSelectedPCSingle](GameGetSelectedPCSingle.md), [GameGetFirstSelectedPC](GameGetFirstSelectedPC.md)");

static PyObject* GemRB_GameSelectPC(PyObject* /*self*/, PyObject* args)
{
	int PartyID, Select;
	int Flags = SELECT_NORMAL;
	PARSE_ARGS(args, "ii|i", &PartyID, &Select, &Flags);
	GET_GAME();

	Actor* actor = nullptr;
	if (PartyID > 0) {
		actor = game->FindPC(PartyID);
		if (!actor) {
			Py_RETURN_NONE;
		}
	}

	game->SelectActor(actor, (bool) Select, Flags);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GameIsPCSelected__doc,
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
**See also:** [GameSelectPC](GameSelectPC.md), [GameGetFirstSelectedPC](GameGetFirstSelectedPC.md)\n\
");

static PyObject* GemRB_GameIsPCSelected(PyObject* /*self*/, PyObject* args)
{
	int PlayerSlot;
	PARSE_ARGS(args, "i", &PlayerSlot);
	GET_GAME();

	const Actor* MyActor = game->FindPC(PlayerSlot);
	if (!MyActor) {
		Py_RETURN_FALSE;
	}
	return PyBool_FromLong(MyActor->IsSelected());
}


PyDoc_STRVAR(GemRB_GameSelectPCSingle__doc,
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
**See also:** [GameSelectPC](GameSelectPC.md), [GameGetSelectedPCSingle](GameGetSelectedPCSingle.md)");

static PyObject* GemRB_GameSelectPCSingle(PyObject* /*self*/, PyObject* args)
{
	int index;
	PARSE_ARGS(args, "i", &index);

	GET_GAME();
	return PyBool_FromLong(game->SelectPCSingle(index));
}

PyDoc_STRVAR(GemRB_GameGetSelectedPCSingle__doc,
	     "===== GameGetSelectedPCSingle =====\n\
\n\
**Prototype:** GemRB.GameGetSelectedPCSingle ()\n\
\n\
**Description:** Returns currently active pc \n\
in non-walk environment (i.e. in shops, inventory, ...).\n\
\n\
**Return value:** PartyID (1-10) or 0 if there is no such PC\n\
\n\
**See also:** [GameSelectPC](GameSelectPC.md), [GameSelectPCSingle](GameSelectPCSingle.md)");

static PyObject* GemRB_GameGetSelectedPCSingle(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyLong_FromLong(game->GetSelectedPCSingle());
}

PyDoc_STRVAR(GemRB_GameGetFirstSelectedPC__doc,
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
**See also:** [GameSelectPC](GameSelectPC.md), [GameIsPCSelected](GameIsPCSelected.md), [GameGetFirstSelectedActor](GameGetFirstSelectedActor.md)\n\
");

static PyObject* GemRB_GameGetFirstSelectedPC(PyObject* /*self*/, PyObject* /*args*/)
{
	const Actor* actor = core->GetFirstSelectedPC(false);
	if (actor) {
		return PyLong_FromLong(actor->InParty);
	}

	return PyLong_FromLong(0);
}

PyDoc_STRVAR(GemRB_GameGetFirstSelectedActor__doc,
	     "===== GameGetFirstSelectedActor =====\n\
\n\
**Prototype:** GemRB.GameGetFirstSelectedActor ()\n\
\n\
**Description:** Returns the global ID of the first selected actor, party ID if a pc or 0 if none.\n\
\n\
**Return value:** int\n\
\n\
**See also:** [GameGetFirstSelectedPC](GameGetFirstSelectedPC.md)");

static PyObject* GemRB_GameGetFirstSelectedActor(PyObject* /*self*/, PyObject* /*args*/)
{
	const Actor* actor = core->GetFirstSelectedActor();
	if (actor) {
		if (actor->InParty) {
			return PyLong_FromLong(actor->InParty);
		} else {
			return PyLong_FromLong(actor->GetGlobalID());
		}
	}

	return PyLong_FromLong(0);
}

PyDoc_STRVAR(GemRB_GameSwapPCs__doc,
	     "===== GameSwapPCs =====\n\
\n\
**Prototype:** GemRB.GameSwapPCs (PC1, PC2)\n\
\n\
**Parameters:**\n\
  * PC1 - party ID of the first PC\n\
  * PC2 - party ID of the second PC\n\
\n\
**Description:**  Swap the party order of PC1 with PC2.\n\
\n\
**Return value:** None");

static PyObject* GemRB_GameSwapPCs(PyObject* /*self*/, PyObject* args)
{
	uint32_t PC1, PC2;
	PARSE_ARGS(args, "II", &PC1, &PC2);
	GET_GAME();

	game->SwapPCs(PC1, PC2);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_ActOnPC__doc,
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
**See also:** [ClearActions](ClearActions.md), [SetModalState](SetModalState.md), [SpellCast](SpellCast.md)");

static PyObject* GemRB_ActOnPC(PyObject* /*self*/, PyObject* args)
{
	int PartyID;
	PARSE_ARGS(args, "i", &PartyID);
	GET_GAME();

	Actor* MyActor = game->FindPC(PartyID);
	if (MyActor) {
		GameControl* gc = core->GetGameControl();
		if (gc) {
			gc->PerformActionOn(MyActor);
		}
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetPlayerPortrait__doc,
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
**Return value:** dict or None\n\
  * Sprite - the player's portrait (image)\n\
  * ResRef - the portrait's name (image resref)\n\
\n\
**See also:** [FillPlayerInfo](FillPlayerInfo.md)\n\
");

static PyObject* GemRB_GetPlayerPortrait(PyObject* /*self*/, PyObject* args)
{
	int PartyID;
	int which = 0;
	PARSE_ARGS(args, "i|i", &PartyID, &which);

	GET_GAME();
	const Actor* actor = game->FindPC(PartyID);
	if (actor) {
		Holder<Sprite2D> portrait = actor->CopyPortrait(which);
		PyObject* dict = PyDict_New();
		PyDict_SetItemString(dict, "Sprite", PyObject_FromHolder(std::move(portrait)));
		PyObject* pystr = PyString_FromResRef(which ? actor->SmallPortrait : actor->LargePortrait);
		PyDict_SetItemString(dict, "ResRef", pystr);
		Py_DecRef(pystr);
		return dict;
	} else {
		Py_RETURN_NONE;
	}
}

PyDoc_STRVAR(GemRB_GetPlayerString__doc,
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
**See also:** [GetPlayerName](GetPlayerName.md), [GetPlayerStat](GetPlayerStat.md), [GetPlayerScript](GetPlayerScript.md)\n\
\n\
**See also:** sndslot.ids, soundoff.ids (it is a bit unclear which one is it)\n\
");

static PyObject* GemRB_GetPlayerString(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	Verbal Index;
	PARSE_ARGS(args, "iI", &globalID, &Index);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Index >= Verbal::count) {
		return RuntimeError("String reference is too high!\n");
	}

	return PyLong_FromStrRef(actor->StrRefs[Index]);
}

PyDoc_STRVAR(GemRB_GetPlayerStat__doc,
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
**See also:** [SetPlayerStat](SetPlayerStat.md), [GetPlayerName](GetPlayerName.md), [GetPlayerStates](GetPlayerStates.md)");

static PyObject* GemRB_GetPlayerStat(PyObject* /*self*/, PyObject* args)
{
	int globalID, StatID, StatValue, BaseStat;

	BaseStat = 0;
	PARSE_ARGS(args, "ii|i", &globalID, &StatID, &BaseStat);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	//returning the modified stat if BaseStat was 0 (default)
	StatValue = GetCreatureStat(actor, StatID, !BaseStat);

	// special handling for the hidden hp
	if ((unsigned) StatValue == 0xdadadada) {
		return PyString_FromString("?");
	} else {
		return PyLong_FromLong(StatValue);
	}
}

PyDoc_STRVAR(GemRB_SetPlayerStat__doc,
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
**Examples:** \n\
\n\
    PickedColor = ColorTable.GetValue (ColorIndex, GemRB.GetVar('Selected'))\n\
    GemRB.SetPlayerStat (pc, IE_MAJOR_COLOR, PickedColor)\n\
\n\
The above example sets the player's color just picked via the color customisation dialog. ColorTable holds the available colors.\n\
\n\
**See also:** [GetPlayerStat](GetPlayerStat.md), [SetPlayerName](SetPlayerName.md), [ApplyEffect](ApplyEffect.md)");

static PyObject* GemRB_SetPlayerStat(PyObject* /*self*/, PyObject* args)
{
	int globalID, StatID, StatValue;
	int pcf = 1;
	PARSE_ARGS(args, "iii|i", &globalID, &StatID, &StatValue, &pcf);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	//Setting the creature's base stat
	SetCreatureStat(actor, StatID, StatValue, pcf);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetPlayerScript__doc,
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
**See also:** [SetPlayerScript](SetPlayerScript.md)\n\
");

static PyObject* GemRB_GetPlayerScript(PyObject* /*self*/, PyObject* args)
{
	//class script is the custom slot for player scripts
	int globalID, Index = SCR_CLASS;
	PARSE_ARGS(args, "i|i", &globalID, &Index);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	ResRef scr = actor->GetScript(Index);
	if (scr.IsEmpty()) {
		Py_RETURN_NONE;
	}
	return PyString_FromResRef(scr);
}

PyDoc_STRVAR(GemRB_SetPlayerScript__doc,
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
**See also:** [GetPlayerScript](GetPlayerScript.md)");

static PyObject* GemRB_SetPlayerScript(PyObject* /*self*/, PyObject* args)
{
	PyObject* ScriptName = nullptr;
	int globalID, Index = SCR_CLASS;
	PARSE_ARGS(args, "iO|i", &globalID, &ScriptName, &Index);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetScript(ResRefFromPy(ScriptName), Index, true);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetPlayerDialog__doc,
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
");

static PyObject* GemRB_SetPlayerDialog(PyObject* /*self*/, PyObject* args)
{
	PyObject* DialogName = nullptr;
	int globalID;
	PARSE_ARGS(args, "iO", &globalID, &DialogName);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetDialog(ResRefFromPy(DialogName));
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_FillPlayerInfo__doc,
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
\n\
    GemRB.FillPlayerInfo (MyChar, PortraitName + 'M', PortraitName + 'S')\n\
\n\
**See also:** [LoadGame](LoadGame.md), [CreatePlayer](CreatePlayer.md), [SetPlayerStat](SetPlayerStat.md), [EnterGame](EnterGame.md)\n\
");

static PyObject* GemRB_FillPlayerInfo(PyObject* /*self*/, PyObject* args)
{
	int globalID, clear = 0;
	PyObject* Portrait1 = nullptr;
	PyObject* Portrait2 = nullptr;
	PARSE_ARGS(args, "i|OOi", &globalID, &Portrait1, &Portrait2, &clear);

	// here comes some code to transfer icon/name to the PC sheet
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Portrait1) {
		actor->SetPortrait(ResRefFromPy(Portrait1), 1);
	}
	if (Portrait2) {
		actor->SetPortrait(ResRefFromPy(Portrait2), 2);
	}

	//set up animation ID
	switch (actor->UpdateAnimationID(false)) {
		case -1:
			return RuntimeError("avprefix table contains no entries.");
		case -2:
			return RuntimeError("Couldn't load avprefix table.");
		case -3:
			return RuntimeError("Couldn't load an avprefix subtable.");
		default:
			break;
	}

	// clear several fields (only useful for cg; currently needed only in iwd2, but that will change if its system is ported to the rest)
	// fixes random action bar mess, kill stats, join time ...
	if (clear) {
		PCStatsStruct& oldstats = *actor->PCStats;
		PCStatsStruct newstats;
		newstats.AwayTime = oldstats.AwayTime;
		newstats.unknown10 = oldstats.unknown10;
		newstats.Happiness = oldstats.Happiness;
		newstats.SoundFolder = oldstats.SoundFolder;
		newstats.States = oldstats.States;
		newstats.SoundSet = oldstats.SoundSet;

		oldstats = std::move(newstats);
	}

	actor->SetOver(false);
	actor->InitButtons(actor->GetActiveClass(), true); // force re-init of actor's action bar

	//what about multiplayer?
	if ((globalID == 1) && core->HasFeature(GFFlags::HAS_DPLAYER)) {
		actor->SetScript("DPLAYER3", SCR_DEFAULT, false);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_Button_SetSpellIcon__doc,
	     "===== Button_SetSpellIcon =====\n\
\n\
**Metaclass Prototype:** SetSpellIcon (SPLResRef[, Type, Tooltip, Function])\n\
\n\
**Description:** Sets Spell icon image on a Button control. Type determines \n\
the icon type, if set to 1 it will use the Memorised Icon instead of the \n\
Spellbook Icon\
\n\
**Parameters:**\n\
  * SPLResRef - the name of the spell (.spl resref)\n\
  * Type - 0 (default, use parchment background) or 1 (use stone background)\n\
  * Tooltip - 0 (default); if 1, set the tooltip 'F<n> <spell_name>'\n\
  * Function - F-key number to be used in the tooltip above\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetItemIcon](Button_SetItemIcon.md)");

static PyObject* SetSpellIcon(Button* btn, const ResRef& SpellResRef, int type, int tooltip, int Function)
{
	ABORT_IF_NULL(btn);

	if (SpellResRef.IsEmpty()) {
		btn->SetPicture(NULL);
		//no incref here!
		return Py_None;
	}

	const Spell* spell = gamedata->GetSpell(SpellResRef, true);
	if (!spell) {
		btn->SetPicture(nullptr);
		Log(ERROR, "GUIScript", "Spell not found: {}", SpellResRef);
		//no incref here!
		return Py_None;
	}

	ResRef iconResRef;
	if (type) {
		iconResRef = spell->ext_headers[0].memorisedIcon;
	} else {
		iconResRef = spell->SpellbookIcon;
	}
	auto af = gamedata->GetFactoryResourceAs<const AnimationFactory>(iconResRef, IE_BAM_CLASS_ID, true);
	if (!af) {
		return RuntimeError(fmt::format("{} BAM not found", iconResRef));
	}
	//small difference between pst and others
	if (af->GetCycleSize(0) != 4) { //non-pst
		btn->SetPicture(af->GetFrame(0, 0));
	} else { //pst
		btn->SetImage(ButtonImage::Unpressed, af->GetFrame(0, 0));
		btn->SetImage(ButtonImage::Pressed, af->GetFrame(1, 0));
		btn->SetImage(ButtonImage::Selected, af->GetFrame(2, 0));
		btn->SetImage(ButtonImage::Disabled, af->GetFrame(3, 0));
	}
	if (tooltip) {
		SetViewTooltipFromRef(btn, spell->SpellName);
		btn->SetHotKey(GEM_FUNCTIONX(Function), 0, true);
	}
	gamedata->FreeSpell(spell, SpellResRef, false);
	//no incref here!
	return Py_None;
}

static PyObject* GemRB_Button_SetSpellIcon(PyObject* self, PyObject* args)
{
	PyObject* SpellResRef = nullptr;
	int type = 0;
	int tooltip = 0;
	int Function = 0;
	PARSE_ARGS(args, "OO|iii", &self, &SpellResRef, &type, &tooltip, &Function);

	Button* btn = GetView<Button>(self);
	PyObject* ret = SetSpellIcon(btn, ResRefFromPy(SpellResRef), type, tooltip, Function);
	if (ret) {
		Py_INCREF(ret);
	}
	return ret;
}

static Holder<Sprite2D> GetUsedWeaponIcon(const Item* item, int which)
{
	const ITMExtHeader* ieh = item->GetWeaponHeader(false);
	if (!ieh) {
		ieh = item->GetWeaponHeader(true);
	}
	if (ieh) {
		return gamedata->GetAnySprite(ieh->UseIcon, -1, which);
	}
	return gamedata->GetAnySprite(item->ItemIcon, -1, which);
}

static void SetItemText(Button* btn, int charges, bool oneisnone)
{
	if (!btn) return;

	if (charges && (charges > 1 || !oneisnone)) {
		btn->SetText(fmt::format(u"{}", charges));
	} else {
		btn->SetText(u"");
	}
}

PyDoc_STRVAR(GemRB_Button_SetItemIcon__doc,
	     "===== Button_SetItemIcon =====\n\
\n\
**Metaclass Prototype:** SetItemIcon (ITMResRef[, Type, Tooltip, FunctionKey, ITM2ResRef, BAM3ResRef]])\n\
\n\
**Description:** Sets Item icon image on a Button control.\n\
\n\
**Parameters:**\n\
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
  * Tooltip  - if set to 1 or 2 (identified), the tooltip for the item will also be set\n\
  * FunctionKey  - F-key to map to [1-12]\n\
  * ITM2ResRef - if set, a second item to display in the icon. ITM2 is drawn first. The tooltip of ITM is used. Only valid for Type 4 and 5\n\
  * BAM3ResRef - if set, a third image will be stacked on top of the others\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetSpellIcon](Button_SetSpellIcon.md), [Button_SetActionIcon](Button_SetActionIcon.md)");

static PyObject* SetItemIcon(Button* btn, const ResRef& ItemResRef, int Which, int tooltip, int Function, const ResRef& Item2ResRef, const ResRef& bam3ResRef)
{
	ABORT_IF_NULL(btn);

	if (ItemResRef.IsEmpty()) {
		btn->SetPicture(nullptr);
		//no incref here!
		return Py_None;
	}
	const Item* item = gamedata->GetItem(ItemResRef, true);
	if (item == nullptr) {
		btn->SetPicture(nullptr);
		//no incref here!
		return Py_None;
	}

	btn->SetFlags(IE_GUI_BUTTON_PICTURE, BitOp::OR);
	Holder<Sprite2D> Picture = nullptr;
	bool setpicture = true;
	const Item* item2;
	switch (Which) {
		case 0:
		case 1:
			Picture = gamedata->GetAnySprite(item->ItemIcon, -1, Which);
			break;
		case 2:
			btn->SetPicture(nullptr); // also calls ClearPictureList
			for (int i = 0; i < 4; i++) {
				Picture = gamedata->GetAnySprite(item->DescriptionIcon, -1, i);
				if (Picture)
					btn->StackPicture(Picture);
			}
			//fallthrough
		case 3:
			setpicture = false;
			Picture = nullptr;
			break;
		case 4:
		case 5:
			Picture = GetUsedWeaponIcon(item, Which - 4);
			if (!Item2ResRef) {
				break;
			}

			btn->SetPicture(nullptr); // also calls ClearPictureList
			item2 = gamedata->GetItem(Item2ResRef, true);
			if (item2) {
				Holder<Sprite2D> Picture2;
				Picture2 = gamedata->GetAnySprite(item2->ItemIcon, -1, Which - 4);
				if (Picture2) btn->StackPicture(Picture2);
				gamedata->FreeItem(item2, Item2ResRef, false);
			}
			if (Picture) btn->StackPicture(Picture);
			setpicture = false;
			break;
		default:
			const ITMExtHeader* eh = item->GetExtHeader(Which - 6);
			if (eh) {
				Picture = gamedata->GetAnySprite(eh->UseIcon, -1, 0);
			}
	}

	if (setpicture) btn->SetPicture(std::move(Picture));
	if (tooltip) {
		//later getitemname could also return tooltip stuff
		SetViewTooltipFromRef(btn, item->GetItemName(tooltip == 2));
		btn->SetHotKey(GEM_FUNCTIONX(Function), 0, true);
	}

	if (!bam3ResRef.IsEmpty()) {
		Holder<Sprite2D> Picture3 = gamedata->GetAnySprite(bam3ResRef, -1, 0);
		if (Picture3) btn->StackPicture(Picture3);
	}

	gamedata->FreeItem(item, ItemResRef, false);
	//no incref here!
	return Py_None;
}

static PyObject* GemRB_Button_SetItemIcon(PyObject* self, PyObject* args)
{
	PyObject* ItemResRef = nullptr;
	int Which = 0;
	int tooltip = 0;
	int Function = 0;
	PyObject* Item2ResRef = nullptr;
	PyObject* bam3ResRef = nullptr;
	PARSE_ARGS(args, "OO|iiiOO", &self, &ItemResRef, &Which, &tooltip, &Function, &Item2ResRef, &bam3ResRef);

	Button* btn = GetView<Button>(self);
	PyObject* ret = SetItemIcon(btn, ResRefFromPy(ItemResRef), Which, tooltip, Function, ResRefFromPy(Item2ResRef), ResRefFromPy(bam3ResRef));
	if (ret) {
		Py_INCREF(ret);
	}
	return ret;
}

PyDoc_STRVAR(GemRB_EnterStore__doc,
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
**See also:** [GetStore](GetStore.md), [GetStoreCure](GetStoreCure.md), [GetStoreDrink](GetStoreDrink.md), [LeaveStore](LeaveStore.md), [SetPurchasedAmount](SetPurchasedAmount.md)\n\
");

static PyObject* GemRB_EnterStore(PyObject* /*self*/, PyObject* args)
{
	PyObject* StoreResRef = nullptr;
	PARSE_ARGS(args, "O", &StoreResRef);

	//stores are cached, bags could be opened while in shops
	//so better just switch to the requested store silently
	//the core will be intelligent enough to not do excess work
	core->SetCurrentStore(ResRefFromPy(StoreResRef), 0);

	core->SetEventFlag(EF_OPENSTORE);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_LeaveStore__doc,
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
**See also:** [EnterStore](EnterStore.md), [GetStore](GetStore.md)\n\
");

static PyObject* GemRB_LeaveStore(PyObject* /*self*/, PyObject* /*args*/)
{
	core->CloseCurrentStore();
	core->ResetEventFlag(EF_OPENSTORE);
	core->SetEventFlag(EF_PORTRAIT);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_LoadRighthandStore__doc,
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
**See also:** [CloseRighthandStore](CloseRighthandStore.md), [GetStore](GetStore.md), [GetStoreItem](GetStoreItem.md), [SetPurchasedAmount](SetPurchasedAmount.md)\n\
");

static PyObject* GemRB_LoadRighthandStore(PyObject* /*self*/, PyObject* args)
{
	PyObject* StoreResRef = nullptr;
	if (!PyArg_ParseTuple(args, "O", &StoreResRef)) {
		return AttributeError(GemRB_LoadRighthandStore__doc);
	}

	Store* newrhstore = gamedata->GetStore(ResRefFromPy(StoreResRef));
	if (rhstore && rhstore != newrhstore) {
		gamedata->SaveStore(rhstore);
	}
	rhstore = newrhstore;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_CloseRighthandStore__doc,
	     "===== CloseRighthandStore =====\n\
\n\
**Prototype:** GemRB.CloseRighthandStore ()\n\
\n\
**Description:** Unloads the current right-hand store and saves it to cache.\n\
If there was no right-hand store opened, the function does nothing.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [LoadRighthandStore](LoadRighthandStore.md)\n\
");

static PyObject* GemRB_CloseRighthandStore(PyObject* /*self*/, PyObject* /*args*/)
{
	gamedata->SaveStore(rhstore);
	rhstore = NULL;
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_LeaveContainer__doc,
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
**See also:** [GetContainer](GetContainer.md), [GetContainerItem](GetContainerItem.md), [LeaveStore](LeaveStore.md)");

static PyObject* GemRB_LeaveContainer(PyObject* /*self*/, PyObject* /*args*/)
{
	core->CloseCurrentContainer();
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetContainer__doc,
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
**See also:** [GetStore](GetStore.md), [GameGetFirstSelectedPC](GameGetFirstSelectedPC.md), [GetContainerItem](GetContainerItem.md)");

static PyObject* GemRB_GetContainer(PyObject* /*self*/, PyObject* args)
{
	int PartyID;
	int autoselect = 0;
	PARSE_ARGS(args, "i|i", &PartyID, &autoselect);

	const Actor* actor;

	GET_GAME();

	if (PartyID) {
		actor = game->FindPC(PartyID);
	} else {
		actor = core->GetFirstSelectedPC(false);
	}
	if (!actor) {
		return RuntimeError("Actor not found!\n");
	}
	const Container* container = nullptr;
	if (autoselect) { //autoselect works only with piles
		Map* map = actor->GetCurrentArea();
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

	return Py_BuildValue("{s:i,s:i}", "Type", container->containerType, "ItemCount", container->inventory.GetSlotCount());
}

PyDoc_STRVAR(GemRB_GetContainerItem__doc,
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
  * 'ItemDesc'   - the StrRef of the item's description (identified or not)\n\
  * 'Usages0'    - The primary charges of the item (or the item's stack amount if the item is stackable).\n\
  * 'Usages1'    - The secondary charges of the item.\n\
  * 'Usages2'    - The tertiary charges of the item.\n\
  * 'Flags'      - Item flags.\n\
\n\
**See also:** [GetContainer](GetContainer.md), [GameGetFirstSelectedPC](GameGetFirstSelectedPC.md), [GetStoreItem](GetStoreItem.md)\n\
");

static PyObject* GemRB_GetContainerItem(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int index;
	PARSE_ARGS(args, "ii", &globalID, &index);
	const Container* container;

	if (globalID) {
		GET_GAME();
		GET_ACTOR_GLOBAL();

		Map* map = actor->GetCurrentArea();
		if (!map) {
			return RuntimeError("No current area!");
		}
		container = map->GetPile(actor->Pos);
	} else {
		container = core->GetCurrentContainer();
	}
	if (!container) {
		return RuntimeError("No current container!");
	}
	if (index >= container->inventory.GetSlotCount()) {
		Py_RETURN_NONE;
	}

	const CREItem* ci = container->inventory.GetSlotItem(index);

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "ItemResRef", DecRef(PyString_FromResRef, ci->ItemResRef));
	PyDict_SetItemString(dict, "Usages0", DecRef(PyLong_FromLong, ci->Usages[0]));
	PyDict_SetItemString(dict, "Usages1", DecRef(PyLong_FromLong, ci->Usages[1]));
	PyDict_SetItemString(dict, "Usages2", DecRef(PyLong_FromLong, ci->Usages[2]));
	PyDict_SetItemString(dict, "Flags", DecRef(PyLong_FromLong, ci->Flags));

	const Item* item = gamedata->GetItem(ci->ItemResRef, true);
	if (!item) {
		Log(MESSAGE, "GUIScript", "Cannot find container ({}) item {}!", container->GetScriptName(), ci->ItemResRef);
		Py_RETURN_NONE;
	}

	bool identified = ci->Flags & IE_INV_ITEM_IDENTIFIED;
	PyDict_SetItemString(dict, "ItemName", DecRef(PyLong_FromStrRef, item->GetItemName(identified)));
	PyDict_SetItemString(dict, "ItemDesc", DecRef(PyLong_FromStrRef, item->GetItemDesc(identified)));
	gamedata->FreeItem(item, ci->ItemResRef, false);
	return dict;
}

static void OverrideSound(const ResRef& itemRef, ResRef& soundRef, ieDword col)
{
	const Item* item = gamedata->GetItem(itemRef);
	if (!item) return;

	ResRef candidate;
	if (col == IS_DROP) {
		candidate = item->ReplacementItem;
	} else { // IS_GET
		candidate = item->DescriptionIcon;
	}

	if (core->HasFeature(GFFlags::HAS_PICK_SOUND) && !candidate.IsEmpty()) {
		soundRef = candidate;
	} else {
		gamedata->GetItemSound(soundRef, item->ItemType, item->AnimationType, col);
	}
	gamedata->FreeItem(item, itemRef, false);
}

PyDoc_STRVAR(GemRB_ChangeContainerItem__doc,
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
**See also:** [GetContainer](GetContainer.md), [GetSlotItem](GetSlotItem.md)\n\
");

static PyObject* GemRB_ChangeContainerItem(PyObject* /*self*/, PyObject* args)
{
	int globalID, Slot;
	int action;
	PARSE_ARGS(args, "iii", &globalID, &Slot, &action);
	GET_GAME();

	Container* container;
	Actor* actor = NULL;

	if (globalID) {
		if (globalID > 1000) {
			actor = game->GetActorByGlobalID(globalID);
		} else {
			actor = game->FindPC(globalID);
		}
		if (!actor) {
			return RuntimeError("Actor not found!\n");
		}
		const Map* map = actor->GetCurrentArea();
		if (!map) {
			return RuntimeError("No current area!");
		}
		container = map->TMap->GetContainer(actor->Pos, IE_CONTAINER_PILE);
	} else {
		actor = core->GetFirstSelectedPC(false);
		container = core->GetCurrentContainer();
	}
	if (!actor) {
		return RuntimeError("Actor not found!\n");
	}
	if (!container) {
		return RuntimeError("No current container!");
	}

	ResRef Sound;
	CREItem* si;
	int res;

	if (action) { //get stuff from container
		if (Slot < 0 || Slot >= container->inventory.GetSlotCount()) {
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
		si = container->RemoveItem(Slot, 0);
		if (!si) {
			Log(WARNING, "GUIScript", "Cannot move item, there is something weird!");
			Py_RETURN_NONE;
		}
		OverrideSound(si->ItemResRef, Sound, IS_DROP);
		if (res != -1) { //it is gold!
			game->PartyGold += res;
			delete si;
		} else {
			res = actor->inventory.AddSlotItem(si, SLOT_ONLYINVENTORY);
			if (res != ASI_SUCCESS) { //putting it back
				container->AddItem(si);
			}
		}
	} else { //put stuff in container, simple!
		res = core->CanMoveItem(actor->inventory.GetSlotItem(core->QuerySlot(Slot)));
		if (!res) { //cannot move
			Log(MESSAGE, "GUIScript", "Cannot move item, it is undroppable!");
			Py_RETURN_NONE;
		}

		si = actor->inventory.RemoveItem(core->QuerySlot(Slot));
		if (!si) {
			Log(WARNING, "GUIScript", "Cannot move item, there is something weird!");
			Py_RETURN_NONE;
		}
		OverrideSound(si->ItemResRef, Sound, IS_GET);
		actor->ReinitQuickSlots();

		if (res != -1) { //it is gold!
			game->PartyGold += res;
			delete si;
		} else {
			container->AddItem(si);
		}
	}

	if (Sound && Sound[0]) {
		core->GetAudioDrv()->Play(Sound, SFXChannel::GUI);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetStore__doc,
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
  * 'StoreOwner   '   - the ID of the owner of the store\n\
  * 'StoreRoomPrices' - a four elements tuple, negative if the room type is unavailable\n\
  * 'StoreButtons'    - a four elements tuple, possible actions\n\
  * 'StoreFlags'      - the store flags if you ever need them, StoreButtons is a digested information, but you might have something else in mind based on these\n\
  * 'TavernRumour'    - ResRef of tavern rumour dialog\n\
  * 'TempleRumour'    - ResRef of temple rumour dialog\n\
  * 'IDPrice'    - price for identification\n\
  * 'Lore'    - lore requirement\n\
  * 'Depreciation'    - price depreciation\n\
  * 'SellMarkup'    - markup for selling\n\
  * 'BuyMarkup'    - markup for buying\n\
  * 'StealFailure'    - chance to succeed at stealing\n\
\n\
**See also:** [EnterStore](EnterStore.md), [GetStoreCure](GetStoreCure.md), [GetStoreDrink](GetStoreDrink.md), [GetRumour](GetRumour.md)\n\
");

constexpr Py_ssize_t STORE_BUTTON_COUNT = 7;
using StoreButtons = std::array<StoreActionType, STORE_BUTTON_COUNT>;
static EnumArray<StoreType, StoreButtons> storebuttons {
	StoreButtons { //store
		       StoreActionType::BuySell,
		       StoreActionType::Identify | StoreActionType::Optional,
		       StoreActionType::Steal | StoreActionType::Optional,
		       StoreActionType::Donate | StoreActionType::Optional,
		       StoreActionType::Cure | StoreActionType::Optional,
		       StoreActionType::Drink | StoreActionType::Optional,
		       StoreActionType::RoomRent | StoreActionType::Optional },
	StoreButtons { // tavern
		       StoreActionType::Drink,
		       StoreActionType::BuySell | StoreActionType::Optional,
		       StoreActionType::Identify | StoreActionType::Optional,
		       StoreActionType::Steal | StoreActionType::Optional,
		       StoreActionType::Donate | StoreActionType::Optional,
		       StoreActionType::Cure | StoreActionType::Optional,
		       StoreActionType::RoomRent | StoreActionType::Optional },
	StoreButtons { // inn
		       StoreActionType::RoomRent,
		       StoreActionType::BuySell | StoreActionType::Optional,
		       StoreActionType::Drink | StoreActionType::Optional,
		       StoreActionType::Steal | StoreActionType::Optional,
		       StoreActionType::Identify | StoreActionType::Optional,
		       StoreActionType::Donate | StoreActionType::Optional,
		       StoreActionType::Cure | StoreActionType::Optional },
	StoreButtons { // temple
		       StoreActionType::Cure,
		       StoreActionType::Donate | StoreActionType::Optional,
		       StoreActionType::BuySell | StoreActionType::Optional,
		       StoreActionType::Identify | StoreActionType::Optional,
		       StoreActionType::Steal | StoreActionType::Optional,
		       StoreActionType::Drink | StoreActionType::Optional,
		       StoreActionType::RoomRent | StoreActionType::Optional },
	StoreButtons { // iwd container
		       StoreActionType::BuySell,
		       StoreActionType::None, StoreActionType::None, StoreActionType::None, StoreActionType::None, StoreActionType::None, StoreActionType::None },
	StoreButtons { // no need to steal from your own container (original engine had STEAL instead of DRINK)
		       StoreActionType::BuySell,
		       StoreActionType::Identify | StoreActionType::Optional,
		       StoreActionType::Drink | StoreActionType::Optional,
		       StoreActionType::Cure | StoreActionType::Optional,
		       StoreActionType::None, StoreActionType::None, StoreActionType::None },
	StoreButtons { // gemrb specific store type: (temple 2), added steal, removed identify
		       StoreActionType::BuySell,
		       StoreActionType::Steal | StoreActionType::Optional,
		       StoreActionType::Donate | StoreActionType::Optional,
		       StoreActionType::Cure | StoreActionType::Optional,
		       StoreActionType::None, StoreActionType::None, StoreActionType::None }
};

//buy/sell, identify, steal, cure, donate, drink, rent
static const EnumArray<StoreActionType, StoreActionFlags> storeBits { StoreActionFlags::Buy | StoreActionFlags::Sell, StoreActionFlags::ID, StoreActionFlags::Steal,
								      StoreActionFlags::Cure, StoreActionFlags::Donate, StoreActionFlags::Drink, StoreActionFlags::Rent };

static PyObject* GemRB_GetStore(PyObject* /*self*/, PyObject* args)
{
	int rh = 0;
	if (!PyArg_ParseTuple(args, "|i", &rh)) {
		return AttributeError(GemRB_GetStore__doc);
	}

	Store* store;
	if (rh) {
		store = rhstore;
	} else {
		store = core->GetCurrentStore();
	}
	if (!store) {
		Py_RETURN_NONE;
	}
	if (store->Type > StoreType::Bag) {
		store->Type = StoreType::Bag;
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "StoreType", DecRef(PyLong_FromLong, static_cast<int>(store->Type)));
	PyDict_SetItemString(dict, "StoreName", DecRef(PyLong_FromStrRef, store->StoreName));
	PyDict_SetItemString(dict, "StoreDrinkCount", DecRef(PyLong_FromLong, store->DrinksCount));
	PyDict_SetItemString(dict, "StoreCureCount", DecRef(PyLong_FromLong, store->CuresCount));
	PyDict_SetItemString(dict, "StoreItemCount", DecRef(PyLong_FromLong, store->GetRealStockSize()));
	PyDict_SetItemString(dict, "StoreCapacity", DecRef(PyLong_FromLong, store->Capacity));
	PyDict_SetItemString(dict, "StoreOwner", DecRef(PyLong_FromLong, store->GetOwnerID()));
	PyObject* p = PyTuple_New(store->RoomPrices.size());

	for (Py_ssize_t i = 0; i < 4; i++) {
		ieDword bit = 1 << i;
		if (store->AvailableRooms & bit) {
			ieDword k = store->RoomPrices[i];
			PyTuple_SetItem(p, i, PyLong_FromLong(k));
		} else {
			Py_INCREF(Py_None);
			PyTuple_SetItem(p, i, Py_None);
		}
	}
	PyDict_SetItemString(dict, "StoreRoomPrices", p);

	p = PyTuple_New(STORE_BUTTON_COUNT);
	Py_ssize_t i = 0;
	for (auto bit : storebuttons[store->Type]) {
		if (bool(bit & StoreActionType::Optional)) {
			bit &= ~StoreActionType::Optional;
			//check if the type was disabled
			if (!(store->Flags & storeBits[bit])) {
				continue;
			}
		} else if (bit == StoreActionType::None) {
			continue;
		}
		PyTuple_SetItem(p, i++, PyLong_FromLong(under_t<StoreActionType>(bit)));
	}

	for (; i < STORE_BUTTON_COUNT; ++i) {
		Py_INCREF(Py_None);
		PyTuple_SetItem(p, i, Py_None);
	}

	PyDict_SetItemString(dict, "StoreButtons", p);
	PyDict_SetItemString(dict, "StoreFlags", DecRef(PyLong_FromLong, under_t<StoreActionFlags>(store->Flags)));
	PyDict_SetItemString(dict, "TavernRumour", DecRef(PyString_FromResRef, store->RumoursTavern));
	PyDict_SetItemString(dict, "TempleRumour", DecRef(PyString_FromResRef, store->RumoursTemple));
	PyDict_SetItemString(dict, "IDPrice", DecRef(PyLong_FromLong, store->IDPrice));
	PyDict_SetItemString(dict, "Lore", DecRef(PyLong_FromLong, store->Lore));
	PyDict_SetItemString(dict, "Depreciation", DecRef(PyLong_FromLong, store->DepreciationRate));
	PyDict_SetItemString(dict, "SellMarkup", DecRef(PyLong_FromLong, store->SellMarkup));
	PyDict_SetItemString(dict, "BuyMarkup", DecRef(PyLong_FromLong, store->BuyMarkup));
	PyDict_SetItemString(dict, "StealFailure", DecRef(PyLong_FromLong, store->StealFailureChance));

	return dict;
}


PyDoc_STRVAR(GemRB_IsValidStoreItem__doc,
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
**See also:** [EnterStore](EnterStore.md), [GetSlotItem](GetSlotItem.md), [GetStoreItem](GetStoreItem.md), [ChangeStoreItem](ChangeStoreItem.md)");

static PyObject* GemRB_IsValidStoreItem(PyObject* /*self*/, PyObject* args)
{
	int globalID, Slot;
	int type = 0;
	PARSE_ARGS(args, "ii|i", &globalID, &Slot, &type);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	const Store* store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}

	ResRef ItemResRef;
	ieDword Flags;

	if (type) {
		const STOItem* si = nullptr;
		if (type != 2) {
			si = store->GetItem(Slot, true);
		} else if (rhstore) {
			si = rhstore->GetItem(Slot, true);
		}
		if (!si) {
			return PyLong_FromLong(0);
		}
		ItemResRef = si->ItemResRef;
		Flags = si->Flags;
	} else {
		const CREItem* si = actor->inventory.GetSlotItem(core->QuerySlot(Slot));
		if (!si) {
			return PyLong_FromLong(0);
		}
		ItemResRef = si->ItemResRef;
		Flags = si->Flags;
	}
	const Item* item = gamedata->GetItem(ItemResRef, true);
	if (!item) {
		Log(ERROR, "GUIScript", "Invalid resource reference: {}", ItemResRef);
		return PyLong_FromLong(0);
	}

	StoreActionFlags ret = store->AcceptableItemType(item->ItemType, Flags, type == 0 || type == 2);
	if (actor->GetBase(IE_PICKPOCKET) <= 0) {
		ret &= ~StoreActionFlags::Steal;
	}

	//don't allow putting a bag into itself
	if (ItemResRef == store->Name) {
		ret &= ~StoreActionFlags::Sell;
	}
	//this is a hack to report on selected items
	if (Flags & IE_INV_ITEM_SELECTED) {
		ret |= StoreActionFlags::Select;
	}

	//don't allow overstuffing bags
	if (store->Capacity && store->Capacity <= store->GetRealStockSize()) {
		ret = (ret | StoreActionFlags::Capacity) & ~StoreActionFlags::Sell;
	}

	//buying into bags respects bags' limitations
	if (rhstore && type != 0) {
		StoreActionFlags accept = rhstore->AcceptableItemType(item->ItemType, Flags, true);
		if (!(accept & StoreActionFlags::Sell)) {
			ret &= ~StoreActionFlags::Buy;
		}
		//probably won't happen in sane games, but doesn't hurt to check
		if (!(accept & StoreActionFlags::Buy)) {
			ret &= ~StoreActionFlags::Sell;
		}

		if (rhstore->Capacity && rhstore->Capacity <= rhstore->GetRealStockSize()) {
			ret = (ret | StoreActionFlags::Capacity) & ~StoreActionFlags::Buy;
		}
	}

	gamedata->FreeItem(item, ItemResRef, false);
	return PyLong_FromLong(under_t<StoreActionFlags>(ret));
}

PyDoc_STRVAR(GemRB_FindStoreItem__doc,
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
");

static PyObject* GemRB_FindStoreItem(PyObject* /*self*/, PyObject* args)
{
	PyObject* resref = nullptr;
	PARSE_ARGS(args, "O", &resref);

	const Store* store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}

	int Slot = store->FindItem(ResRefFromPy(resref), false);
	if (Slot == -1) {
		return PyLong_FromLong(0);
	}
	const STOItem* si = store->GetItem(Slot, true);
	if (!si) {
		// shouldn't be possible, item vanished
		return PyLong_FromLong(0);
	}

	if (si->InfiniteSupply == -1) {
		// change this if it is ever needed for something else than depreciation calculation
		return PyLong_FromLong(0);
	} else {
		return PyLong_FromLong(si->AmountInStock);
	}
}

PyDoc_STRVAR(GemRB_SetPurchasedAmount__doc,
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
**See also:** [EnterStore](EnterStore.md), [LeaveStore](LeaveStore.md), [SetPurchasedAmount](SetPurchasedAmount.md)\n\
");

static PyObject* GemRB_SetPurchasedAmount(PyObject* /*self*/, PyObject* args)
{
	int Slot, tmp;
	ieDword amount;
	int type = 0;

	if (!PyArg_ParseTuple(args, "ii|i", &Slot, &tmp, &type)) {
		return AttributeError(GemRB_SetPurchasedAmount__doc);
	}
	amount = (ieDword) tmp;
	const Store* store;
	if (type) {
		store = rhstore;
	} else {
		store = core->GetCurrentStore();
	}
	if (!store) {
		return RuntimeError("No current store!");
	}
	STOItem* si = store->GetItem(Slot, true);
	if (!si) {
		return RuntimeError("Store item not found!");
	}

	if (si->InfiniteSupply != -1) {
		if (si->AmountInStock < amount) {
			amount = si->AmountInStock;
		}
	}
	si->PurchasedAmount = static_cast<ieWord>(amount);
	if (amount) {
		si->Flags |= IE_INV_ITEM_SELECTED;
	} else {
		si->Flags &= ~IE_INV_ITEM_SELECTED;
	}

	Py_RETURN_NONE;
}

// a bunch of duplicated code moved from GemRB_ChangeStoreItem()
static int SellBetweenStores(STOItem* si, StoreActionFlags action, Store* store)
{
	CREItem ci(si);
	ci.Flags &= ~IE_INV_ITEM_SELECTED;
	if (action == StoreActionFlags::Steal) {
		ci.Flags |= IE_INV_ITEM_STOLEN;
	}

	while (si->PurchasedAmount) {
		//store/bag is at full capacity
		if (store->Capacity && (store->Capacity <= store->GetRealStockSize())) {
			Log(MESSAGE, "GUIScript", "Store is full.");
			return ASI_FAILED;
		}
		if (si->InfiniteSupply != -1) {
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

PyDoc_STRVAR(GemRB_ChangeStoreItem__doc,
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
    * Add 0x20000 for selection (in case of buy/sell only)\n\
\n\
**Return value:**\n\
  * 0 - failure\n\
  * 2 - success\n\
\n\
**See also:** [EnterStore](EnterStore.md), [GetSlotItem](GetSlotItem.md), [GetStoreItem](GetStoreItem.md), [IsValidStoreItem](IsValidStoreItem.md)\n\
");

static PyObject* ChangeSelectedStoreItem(Store* store, int slot, Actor* actor, StoreActionFlags action)
{
	int res = ASI_FAILED;

	switch (action) {
		case StoreActionFlags::Buy:
		case StoreActionFlags::Steal:
			{
				STOItem* si = store->GetItem(slot, true);
				if (!si) {
					return RuntimeError("Store item not found!");
				}
				si->Flags ^= IE_INV_ITEM_SELECTED;
				if (si->Flags & IE_INV_ITEM_SELECTED) {
					si->PurchasedAmount = 1;
				} else {
					si->PurchasedAmount = 0;
				}
				res = ASI_SUCCESS;
				break;
			}
		case StoreActionFlags::Sell:
		case StoreActionFlags::ID:
			{
				if (!rhstore) {
					//this is not removeitem, because the item is just marked
					CREItem* si = actor->inventory.GetSlotItem(core->QuerySlot(slot));
					if (!si) {
						return RuntimeError("Item not found!");
					}
					si->Flags ^= IE_INV_ITEM_SELECTED;
				} else {
					STOItem* si = rhstore->GetItem(slot, true);
					if (!si) {
						return RuntimeError("Bag item not found!");
					}
					si->Flags ^= IE_INV_ITEM_SELECTED;
					if (si->Flags & IE_INV_ITEM_SELECTED) {
						si->PurchasedAmount = 1;
					} else {
						si->PurchasedAmount = 0;
					}
				}
				res = ASI_SUCCESS;
				break;
			}
		default:
			break;
	}
	return PyLong_FromLong(res);
}

static PyObject* ChangeStoreItem(Store* store, int slot, Actor* actor, StoreActionFlags action)
{
	int res = ASI_FAILED;

	switch (action) {
		case StoreActionFlags::Steal:
		case StoreActionFlags::Buy:
			{
				STOItem* si = store->GetItem(slot, true);
				if (!si) {
					return RuntimeError("Store item not found!");
				}
				//always stealing only one item
				if (action == StoreActionFlags::Steal) {
					si->PurchasedAmount = 1;
				}
				if (!rhstore) {
					//the amount of items is stored in si->PurchasedAmount
					//it will adjust AmountInStock/PurchasedAmount
					actor->inventory.AddStoreItem(si, action == StoreActionFlags::Steal ? StoreActionType::Steal : StoreActionType::BuySell);
				} else {
					SellBetweenStores(si, action, rhstore);
				}
				if (si->PurchasedAmount) {
					//was not able to buy it due to lack of space
					res = ASI_FAILED;
					break;
				}
				// save the resref, since the pointer may get freed
				ResRef itemResRef = si->ItemResRef;
				//if no item remained, remove it
				if (si->AmountInStock) {
					si->Flags &= ~IE_INV_ITEM_SELECTED;
				} else {
					store->RemoveItem(si);
					delete si;
				}

				// play the item's inventory sound
				ResRef SoundItem;
				OverrideSound(itemResRef, SoundItem, IS_DROP);
				if (!SoundItem.IsEmpty()) {
					// speech means we'll only play the last sound if multiple items were bought
					core->GetAudioDrv()->Play(SoundItem, SFXChannel::GUI, Point(), GEM_SND_SPEECH);
				}
				res = ASI_SUCCESS;
				break;
			}
		case StoreActionFlags::ID:
			{
				if (!rhstore) {
					CREItem* si = actor->inventory.GetSlotItem(core->QuerySlot(slot));
					if (!si) {
						return RuntimeError("Item not found!");
					}
					si->Flags |= IE_INV_ITEM_IDENTIFIED;
				} else {
					STOItem* si = rhstore->GetItem(slot, true);
					if (!si) {
						return RuntimeError("Bag item not found!");
					}
					si->Flags |= IE_INV_ITEM_IDENTIFIED;
				}
				res = ASI_SUCCESS;
				break;
			}
		case StoreActionFlags::Sell:
			{
				//store/bag is at full capacity
				if (store->Capacity && (store->Capacity <= store->GetRealStockSize())) {
					Log(MESSAGE, "GUIScript", "Store is full.");
					res = ASI_FAILED;
					break;
				}

				if (rhstore) {
					STOItem* si = rhstore->GetItem(slot, true);
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
					CREItem* si = actor->inventory.RemoveItem(core->QuerySlot(slot));
					if (!si) {
						return RuntimeError("Item not found!");
					}
					//well, it shouldn't be sold at all, but if it is here
					//it will vanish!!!
					if (!si->Expired && (si->Flags & IE_INV_ITEM_RESELLABLE)) {
						si->Flags &= ~IE_INV_ITEM_SELECTED;
						store->AddItem(si);
					}
					delete si;
					res = ASI_SUCCESS;
				}
				break;
			}
		default:
			break;
	}
	return PyLong_FromLong(res);
}

static PyObject* GemRB_ChangeStoreItem(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int slot;
	int actint;
	PARSE_ARGS(args, "iii", &globalID, &slot, &actint);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	Store* store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}

	StoreActionFlags action = static_cast<StoreActionFlags>(actint);
	if (bool(action & StoreActionFlags::Select)) {
		return ChangeSelectedStoreItem(store, slot, actor, action ^ StoreActionFlags::Select);
	}

	return ChangeStoreItem(store, slot, actor, action);
}

PyDoc_STRVAR(GemRB_GetStoreItem__doc,
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
**See also:** [EnterStore](EnterStore.md), [GetStoreDrink](GetStoreDrink.md), [GetStoreCure](GetStoreCure.md), [GetStore](GetStore.md), [GetSlotItem](GetSlotItem.md)\n\
");

static PyObject* GemRB_GetStoreItem(PyObject* /*self*/, PyObject* args)
{
	int index;
	int rh = 0;

	if (!PyArg_ParseTuple(args, "i|i", &index, &rh)) {
		return AttributeError(GemRB_GetStoreItem__doc);
	}
	const Store* store;
	if (rh) {
		store = rhstore;
	} else {
		store = core->GetCurrentStore();
	}
	if (!store) {
		return RuntimeError("No current store!");
	}
	if (index >= store->GetRealStockSize()) {
		Log(WARNING, "GUIScript", "Item is not available???");
		Py_RETURN_NONE;
	}

	STOItem* si = store->GetItem(index, true);
	if (!si) {
		Log(WARNING, "GUIScript", "Item is not available???");
		Py_RETURN_NONE;
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "ItemResRef", DecRef(PyString_FromResRef, si->ItemResRef));
	PyDict_SetItemString(dict, "Usages0", DecRef(PyLong_FromLong, si->Usages[0]));
	PyDict_SetItemString(dict, "Usages1", DecRef(PyLong_FromLong, si->Usages[1]));
	PyDict_SetItemString(dict, "Usages2", DecRef(PyLong_FromLong, si->Usages[2]));
	PyDict_SetItemString(dict, "Flags", DecRef(PyLong_FromLong, si->Flags));
	PyDict_SetItemString(dict, "Purchased", DecRef(PyLong_FromLong, si->PurchasedAmount));

	if (si->InfiniteSupply == -1) {
		PyDict_SetItemString(dict, "Amount", DecRef(PyLong_FromLong, -1));
	} else {
		PyDict_SetItemString(dict, "Amount", DecRef(PyLong_FromLong, si->AmountInStock));
	}

	const Item* item = gamedata->GetItem(si->ItemResRef, true);
	if (!item) {
		Log(WARNING, "GUIScript", "Item is not available???");
		Py_RETURN_NONE;
	}

	int identified = !!(si->Flags & IE_INV_ITEM_IDENTIFIED);
	PyDict_SetItemString(dict, "ItemName", DecRef(PyLong_FromStrRef, item->GetItemName(identified)));
	PyDict_SetItemString(dict, "ItemDesc", DecRef(PyLong_FromStrRef, item->GetItemDesc(identified)));

	int price = item->Price * store->SellMarkup / 100;
	//calculate depreciation too
	//store->DepreciationRate, mount

	price *= si->Usages[0];

	//is this correct?
	if (price < 1) {
		price = 1;
	}
	PyDict_SetItemString(dict, "Price", DecRef(PyLong_FromLong, price));

	gamedata->FreeItem(item, si->ItemResRef, false);
	return dict;
}

PyDoc_STRVAR(GemRB_GetStoreDrink__doc,
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
**See also:** [EnterStore](EnterStore.md), [GetStoreCure](GetStoreCure.md), [GetStore](GetStore.md)\n\
");

static PyObject* GemRB_GetStoreDrink(PyObject* /*self*/, PyObject* args)
{
	int index;
	PARSE_ARGS(args, "i", &index);

	const Store* store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}
	if (index >= (int) store->DrinksCount) {
		Py_RETURN_NONE;
	}

	const STODrink* drink = store->GetDrink(index);
	return Py_BuildValue("{s:i,s:i,s:i}", "DrinkName", (signed) drink->DrinkName, "Price", drink->Price, "Strength", drink->Strength);
}

static void ReadUsedItems()
{
	AutoTable table = gamedata->LoadTable("item_use");
	if (table) {
		TableMgr::index_t UsedItemsCount = table->GetRowCount();
		UsedItems.resize(UsedItemsCount);
		for (TableMgr::index_t i = 0; i < UsedItemsCount; i++) {
			UsedItems[i].itemname = table->GetRowName(i);
			UsedItems[i].username = table->QueryField(i, 0);
			if (IsStar(UsedItems[i].username)) {
				UsedItems[i].username.Reset();
			}
			// this is an strref, potentially more than one
			auto refs = Explode<StringView, std::string>(table->QueryField(i, 1));
			for (const auto& ref : refs) {
				ieStrRef complaint = static_cast<ieStrRef>(strtounsigned<ieDword>(ref.c_str()));
				UsedItems[i].feedback.push_back(complaint);
			}
			//1 - named actor cannot remove it
			//2 - anyone else cannot equip it
			//4 - can only swap it for something else
			//8 - (pst) can only be equipped in eye slots
			//16 - (pst) can only be equipped in ear slots
			UsedItems[i].flags = table->QueryFieldSigned<int>(i, 2);
		}
	}
}

static void ReadSpecialItems()
{
	AutoTable tab = gamedata->LoadTable("itemspec");
	if (tab) {
		TableMgr::index_t SpecialItemsCount = tab->GetRowCount();
		SpecialItems.resize(SpecialItemsCount);
		for (TableMgr::index_t i = 0; i < SpecialItemsCount; i++) {
			SpecialItems[i].resref = tab->GetRowName(i);
			//if there are more flags, compose this value into a bitfield
			SpecialItems[i].value = tab->QueryFieldAsStrRef(i, 0);
		}
	}
}

static ieStrRef GetSpellDesc(const ResRef& CureResRef)
{
	if (StoreSpells.empty()) {
		AutoTable tab = gamedata->LoadTable("speldesc");
		if (tab) {
			TableMgr::index_t StoreSpellsCount = tab->GetRowCount();
			StoreSpells.resize(StoreSpellsCount);
			for (TableMgr::index_t i = 0; i < StoreSpellsCount; i++) {
				StoreSpells[i].resref = tab->GetRowName(i);
				StoreSpells[i].value = tab->QueryFieldAsStrRef(i, 0);
			}
		}
	}

	for (const auto& spell : StoreSpells) {
		if (spell.resref == CureResRef) {
			return spell.value;
		}
	}
	return ieStrRef::INVALID;
}

PyDoc_STRVAR(GemRB_GetStoreCure__doc,
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
**See also:** [EnterStore](EnterStore.md), [GetStoreDrink](GetStoreDrink.md), [GetStore](GetStore.md)");

static PyObject* GemRB_GetStoreCure(PyObject* /*self*/, PyObject* args)
{
	int index;
	PARSE_ARGS(args, "i", &index);
	const Store* store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}
	if (index >= (int) store->CuresCount) {
		Py_RETURN_NONE;
	}
	const STOCure* cure = store->GetCure(index);
	return Py_BuildValue("{s:s,s:i,s:i}", "CureResRef", cure->CureResRef.c_str(), "Price",
			     cure->Price, "Description", (signed) GetSpellDesc(cure->CureResRef));
}

PyDoc_STRVAR(GemRB_ExecuteString__doc,
	     "===== ExecuteString =====\n\
\n\
**Prototype:** GemRB.ExecuteString (String[, Slot])\n\
\n\
**Description:** Executes an in-game script action. If a valid Slot (greater than 0) \n\
is specified, the actor in that Slot is used as the script context. Slot numbers \n\
[1-1000] are treated as portrait indices, while Slot numbers greater than 1000 are \n\
treated as global actor IDs. If no Slot is specified, the scriptable object (actor, \n\
container, door, etc.) currently under the cursor (if available) becomes the script \n\
context. If no valid object exists under the cursor, the current area script is used \n\
as the script context instead.\n\
\n\
The script context determines which scriptable executes the action and its \n\
associated behavior. For example, LOCALS used by the action will be those of the \n\
object acting as the script context.\n\
\n\
**Parameters:**\n\
  * String - a gamescript action\n\
  * Slot   - a player slot or global ID\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
\n\
    GemRB.ExecuteString('ActionOverride([PC], Attack(NearestEnemyOf(Myself)) )')\n\
\n\
The above example will force a player (most likely Player1) to attack an enemy, issuing the command as it would come from the current area's script. The current gametype must support the scripting action.\n\
\n\
\n\
    GemRB.ExecuteString('Attack(NearestEnemyOf(Myself))', 2)\n\
\n\
The above example will force Player2 to attack an enemy, as the example will run in that actor's script context.\n\
\n\
**See also:** [EvaluateString](EvaluateString.md), gamescripts\n\
");

static PyObject* GemRB_ExecuteString(PyObject* /*self*/, PyObject* args)
{
	char* String;
	int Slot = 0;

	PARSE_ARGS(args, "s|i", &String, &Slot);
	GET_GAME()
	GET_GAMECONTROL()
	GET_MAP()

	Scriptable* scriptable = nullptr;

	if (Slot != 0) {
		scriptable = Slot > 1000 ? game->GetActorByGlobalID(Slot) : game->FindPC(Slot);
		if (scriptable == nullptr) {
			return RuntimeError("Actor not found!\n");
		}
	} else {
		scriptable = gc->GetHoverObject();
		if (scriptable == nullptr) {
			scriptable = map;
		}
	}

	GameScript::ExecuteString(scriptable, String);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_EvaluateString__doc,
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
**See also:** [ExecuteString](ExecuteString.md)\n\
");

static PyObject* GemRB_EvaluateString(PyObject* /*self*/, PyObject* args)
{
	const char* String;
	PARSE_ARGS(args, "s", &String);
	GET_GAME();

	if (GameScript::EvaluateString(game->GetCurrentArea(), String)) {
		Log(DEBUG, "GUIScript", "{} returned True", String);
	} else {
		Log(DEBUG, "GUIScript", "{} returned False", String);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_UpdateVolume__doc,
	     "===== UpdateVolume =====\n\
\n\
**Prototype:** GemRB.UpdateVolume ([type])\n\
\n\
**Description:** Updates volume on-the-fly.\n\
\n\
**Parameters**:\n\
  * type:\n\
    * 1 - music\n\
    * 2 - ambients\n\
    * 3 - both, default\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_UpdateVolume(PyObject* /*self*/, PyObject* args)
{
	int type = 3;
	PARSE_ARGS(args, "i", &type);
	core->GetAudioDrv()->UpdateVolume(type);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_ConsoleWindowLog__doc,
	     "ConsoleWindowLog(log_level)\n\n"
	     "Enable/Disable debug messages of log_level in the Console Window.");

static PyObject* GemRB_ConsoleWindowLog(PyObject* /*self*/, PyObject* args)
{
	LogLevel logLevel;
	PARSE_ARGS(args, "b", &logLevel);
	if (logLevel >= LogLevel::count) logLevel = LogLevel::DEBUG;

	SetConsoleWindowLogLevel(logLevel);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetCurrentArea__doc,
	     "===== GetCurrentArea =====\n\
\n\
**Prototype:** GemRB.GetCurrentArea ()\n\
\n\
**Description:** Returns the resref of the current area. It is the same as \n\
GetGameString(1). It works only after a LoadGame() was issued.\n\
\n\
**Return value:** string, (ARE resref)\n\
\n\
**See also:** [GetGameString](GetGameString.md)\n\
");

static PyObject* GemRB_GetCurrentArea(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyString_FromResRef(game->CurrentArea);
}

PyDoc_STRVAR(GemRB_MoveToArea__doc,
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
**See also:** [GetCurrentArea](GetCurrentArea.md)\n\
");

static PyObject* GemRB_MoveToArea(PyObject* /*self*/, PyObject* args)
{
	PyObject* String = nullptr;
	PARSE_ARGS(args, "O", &String);
	GET_GAME();

	Map* map2 = game->GetMap(ResRefFromPy(String), true);
	if (!map2) {
		return RuntimeError("Map not found!");
	}
	int i = game->GetPartySize(false);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (!actor->Selected) {
			continue;
		}
		Map* map1 = actor->GetCurrentArea();
		if (map1) {
			map1->RemoveActor(actor);
		}
		map2->AddActor(actor, true);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetMemorizableSpellsCount__doc,
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
**See also:** [SetMemorizableSpellsCount](SetMemorizableSpellsCount.md)\n\
");

static PyObject* GemRB_GetMemorizableSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Bonus = 1;
	PARSE_ARGS(args, "iii|i", &globalID, &SpellType, &Level, &Bonus);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	//this isn't in the actor's spellbook, handles Wisdom
	return PyLong_FromLong(actor->spellbook.GetMemorizableSpellsCount((ieSpellType) SpellType, Level, (bool) Bonus));
}

PyDoc_STRVAR(GemRB_SetMemorizableSpellsCount__doc,
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
**See also:** [GetMemorizableSpellsCount](GetMemorizableSpellsCount.md)\n\
");

static PyObject* GemRB_SetMemorizableSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int globalID, Value, SpellType, Level;
	PARSE_ARGS(args, "iiii", &globalID, &Value, &SpellType, &Level);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	//the bonus increased value (with wisdom too) is handled by the core
	actor->spellbook.SetMemorizableSpellsCount(Value, (ieSpellType) SpellType, Level, false);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_CountSpells__doc,
	     "===== CountSpells =====\n\
\n\
**Prototype:** GemRB.CountSpells (PartyID, SpellName[, SpellType=-1, Flag=0])\n\
\n\
**Description:** Returns number of memorized spells of given name and type \n\
in PC's spellbook. If flag is set then spent spells are also count.\n\
\n\
**Parameters:**\n\
  * PartyID   - the PC's position in the party\n\
  * SpellName - spell to count\n\
  * SpellType:\n\
    - -1 - any\n\
    - 0 - priest\n\
    - 1 - wizard\n\
    - 2 - innate\n\
  * Flag      - count depleted spells too?\n\
\n\
**Return value:** integer\n\
\n\
**See also:** [GetMemorizableSpellsCount](GetMemorizableSpellsCount.md)");

static PyObject* GemRB_CountSpells(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType = -1;
	PyObject* SpellResRef;
	int Flag = 0;
	PARSE_ARGS(args, "iO|ii", &globalID, &SpellResRef, &SpellType, &Flag);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyLong_FromLong(actor->spellbook.CountSpells(ResRefFromPy(SpellResRef), SpellType, Flag));
}

PyDoc_STRVAR(GemRB_GetKnownSpellsCount__doc,
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
  * Level     - the known spell's level (-1 for any level)\n\
\n\
**Return value:** numeric\n\
\n\
**See also:** [GetMemorizedSpellsCount](GetMemorizedSpellsCount.md), [GetKnownSpell](GetKnownSpell.md)\n\
");

static PyObject* GemRB_GetKnownSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType, Level = -1;
	PARSE_ARGS(args, "ii|i", &globalID, &SpellType, &Level);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Level < 0) {
		int tmp = 0;
		for (int i = 0; i < 9; i++) {
			tmp += actor->spellbook.GetKnownSpellsCount(SpellType, i);
		}
		return PyLong_FromLong(tmp);
	}

	return PyLong_FromLong(actor->spellbook.GetKnownSpellsCount(SpellType, Level));
}

PyDoc_STRVAR(GemRB_GetKnownSpell__doc,
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
**See also:** [GetMemorizedSpell](GetMemorizedSpell.md)\n\
");

static PyObject* GemRB_GetKnownSpell(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index;
	PARSE_ARGS(args, "iiii", &globalID, &SpellType, &Level, &Index);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	const CREKnownSpell* ks = actor->spellbook.GetKnownSpell(SpellType, Level, Index);
	if (!ks) {
		return RuntimeError("Spell not found!");
	}

	return Py_BuildValue("{s:s}", "SpellResRef", ks->SpellResRef.c_str());
}


PyDoc_STRVAR(GemRB_GetMemorizedSpellsCount__doc,
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
**See also:** [GetMemorizedSpell](GetMemorizedSpell.md), [GetKnownSpellsCount](GetKnownSpellsCount.md)");

static PyObject* GemRB_GetMemorizedSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType, Level = -1;
	int castable;
	PARSE_ARGS(args, "iiii", &globalID, &SpellType, &Level, &castable);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Level < 0) {
		if (castable) {
			return PyLong_FromLong(actor->spellbook.GetSpellInfoSize(SpellType));
		} else {
			return PyLong_FromLong(actor->spellbook.GetMemorizedSpellsCount(SpellType, false));
		}
	} else {
		return PyLong_FromLong(actor->spellbook.GetMemorizedSpellsCount(SpellType, Level, castable));
	}
}

PyDoc_STRVAR(GemRB_GetMemorizedSpell__doc,
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
**See also:** [GetMemorizedSpellsCount](GetMemorizedSpellsCount.md)\n\
");

static PyObject* GemRB_GetMemorizedSpell(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index;
	PARSE_ARGS(args, "iiii", &globalID, &SpellType, &Level, &Index);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	const CREMemorizedSpell* ms = actor->spellbook.GetMemorizedSpell(SpellType, Level, Index);
	if (!ms) {
		return RuntimeError("Spell not found!");
	}

	return Py_BuildValue("{s:s,s:i}", "SpellResRef", ms->SpellResRef.c_str(), "Flags", ms->Flags);
}


PyDoc_STRVAR(GemRB_GetSpell__doc,
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
  * 'SpellLocation'   - the spell's header ability location\n\
\n\
**See also:** [GetItem](GetItem.md), [Button_SetSpellIcon](Button_SetSpellIcon.md), spell_structure(IESDP)\n\
");

static PyObject* GemRB_GetSpell(PyObject* /*self*/, PyObject* args)
{
	PyObject* cstr = nullptr;
	int silent = 0;
	PARSE_ARGS(args, "O|i", &cstr, &silent);

	ResRef resref = ResRefFromPy(cstr);
	if (silent && !gamedata->Exists(resref, IE_SPL_CLASS_ID, true)) {
		Py_RETURN_NONE;
	}

	const Spell* spell = gamedata->GetSpell(resref, silent);
	if (!spell) {
		Py_RETURN_NONE;
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "SpellType", PyLong_FromLong(spell->SpellType));
	PyDict_SetItemString(dict, "SpellName", PyLong_FromLong((signed) spell->SpellName));
	PyDict_SetItemString(dict, "SpellDesc", PyLong_FromLong((signed) spell->SpellDesc));
	PyDict_SetItemString(dict, "SpellbookIcon", PyString_FromResRef(spell->SpellbookIcon));
	PyDict_SetItemString(dict, "SpellExclusion", PyLong_FromLong(spell->ExclusionSchool)); //this will list school exclusions and alignment
	PyDict_SetItemString(dict, "SpellDivine", PyLong_FromLong(spell->PriestType)); //this will tell apart a priest spell from a druid spell
	PyDict_SetItemString(dict, "SpellSchool", PyLong_FromLong(spell->PrimaryType));
	PyDict_SetItemString(dict, "SpellSecondary", PyLong_FromLong(spell->SecondaryType));
	PyDict_SetItemString(dict, "SpellLevel", PyLong_FromLong(spell->SpellLevel));
	PyDict_SetItemString(dict, "Completion", PyString_FromResRef(spell->CompletionSound));
	PyDict_SetItemString(dict, "SpellTargetType", PyLong_FromLong(spell->GetExtHeader(0)->Target));
	PyDict_SetItemString(dict, "SpellLocation", PyLong_FromLong(spell->GetExtHeader(0)->Location));
	PyDict_SetItemString(dict, "HeaderFlags", PyLong_FromLong(spell->Flags));
	PyDict_SetItemString(dict, "NonHostile", PyLong_FromLong(!(spell->Flags & SF_HOSTILE) && !spell->ContainsDamageOpcode()));
	PyDict_SetItemString(dict, "SpellResRef", PyString_FromResRef(spell->Name));
	gamedata->FreeSpell(spell, resref, false);
	return dict;
}


PyDoc_STRVAR(GemRB_CheckSpecialSpell__doc,
	     "===== CheckSpecialSpell =====\n\
\n\
**Prototype:** GemRB.CheckSpecialSpell (globalID, SpellResRef)\n\
\n\
**Description:** Checks if an actor's spell is considered special (splspec.2da).\n\
\n\
**Parameters:**\n\
  * globalID - global or party ID of the actor to use\n\
  * SpellResRef - spell resource to check\n\
\n\
**Return value:** bitfield\n\
  * 0 for normal ones\n\
  * SP_IDENTIFY - any spell that cannot be cast from the menu\n\
  * SP_SILENCE  - any spell that can be cast in silence\n\
  * SP_SURGE    - any spell that cannot be cast during a wild surge\n\
  * SP_REST     - any spell that is cast upon rest if memorized");

static PyObject* GemRB_CheckSpecialSpell(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PyObject* SpellResRef = nullptr;
	PARSE_ARGS(args, "iO", &globalID, &SpellResRef);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	int ret = gamedata->CheckSpecialSpell(ResRefFromPy(SpellResRef), actor);
	return PyLong_FromLong(ret);
}

PyDoc_STRVAR(GemRB_GetSpelldataIndex__doc,
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
**Return value:** integer");

static PyObject* GemRB_GetSpelldataIndex(PyObject* /*self*/, PyObject* args)
{
	unsigned int globalID;
	PyObject* spellResRef = nullptr;
	int type;
	PARSE_ARGS(args, "iOi", &globalID, &spellResRef, &type);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	SpellExtHeader spelldata {};
	int ret = actor->spellbook.FindSpellInfo(&spelldata, ResRefFromPy(spellResRef), type);
	return PyLong_FromLong(ret - 1);
}

PyDoc_STRVAR(GemRB_GetSpelldata__doc,
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
");

static PyObject* GemRB_GetSpelldata(PyObject* /*self*/, PyObject* args)
{
	unsigned int globalID;
	int type = 255;
	PARSE_ARGS(args, "i|i", &globalID, &type);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	SpellExtHeader spelldata {};
	int count = actor->spellbook.GetSpellInfoSize(type);
	PyObject* spell_list = PyTuple_New(count);
	for (int i = 0; i < count; i++) {
		actor->spellbook.GetSpellInfo(&spelldata, type, i, 1);
		PyTuple_SetItem(spell_list, i, PyString_FromResRef(spelldata.spellName));
	}
	return spell_list;
}


PyDoc_STRVAR(GemRB_LearnSpell__doc,
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
**See also:** [MemorizeSpell](MemorizeSpell.md), [RemoveSpell](RemoveSpell.md)\n\
");

static PyObject* GemRB_LearnSpell(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PyObject* Spell = nullptr;
	int Flags = 0;
	int Booktype = -1;
	int Level = -1;

	if (!PyArg_ParseTuple(args, "iO|iii", &globalID, &Spell, &Flags, &Booktype, &Level)) {
		return NULL;
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Booktype != -1) Booktype = 1 << Booktype;

	int ret = actor->LearnSpell(ResRefFromPy(Spell), Flags, Booktype, Level); // returns 0 on success
	if (!ret) core->SetEventFlag(EF_ACTION);
	return PyLong_FromLong(ret);
}

PyDoc_STRVAR(GemRB_DispelEffect__doc,
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
**Return value:** N/A");

static EffectRef work_ref;

static PyObject* GemRB_DispelEffect(PyObject* /*self*/, PyObject* args)
{
	int globalID, Parameter2;
	const char* EffectName;
	PARSE_ARGS(args, "isi", &globalID, &EffectName, &Parameter2);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	work_ref.Name = EffectName;
	work_ref.opcode = -1;
	actor->fxqueue.RemoveAllEffectsWithParam(work_ref, Parameter2);

	Py_RETURN_NONE;
}


PyDoc_STRVAR(GemRB_RemoveEffects__doc,
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
**Return value:** N/A\n\
\n\
**See also:** [RemoveSpell](RemoveSpell.md), [RemoveItem](RemoveItem.md)\n\
");

static PyObject* GemRB_RemoveEffects(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PyObject* SpellResRef = nullptr;
	PARSE_ARGS(args, "iO", &globalID, &SpellResRef);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->fxqueue.RemoveAllEffects(ResRefFromPy(SpellResRef));

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_RemoveSpell__doc,
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
**See also:** [UnmemorizeSpell](UnmemorizeSpell.md), [GetKnownSpellsCount](GetKnownSpellsCount.md), [GetKnownSpell](GetKnownSpell.md), [LearnSpell](LearnSpell.md), [RemoveEffects](RemoveEffects.md)\n\
");

static PyObject* GemRB_RemoveSpell(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index;
	PyObject* cstr = nullptr;

	GET_GAME();

	if (PyArg_ParseTuple(args, "iO", &globalID, &cstr)) {
		GET_ACTOR_GLOBAL();
		ResRef SpellResRef = ResRefFromPy(cstr);
		int ret = actor->spellbook.KnowSpell(SpellResRef);
		actor->spellbook.RemoveSpell(SpellResRef);
		return PyLong_FromLong(ret);
	}
	PyErr_Clear(); //clear the type exception from above
	PARSE_ARGS(args, "iiii", &globalID, &SpellType, &Level, &Index);

	GET_ACTOR_GLOBAL();
	const CREKnownSpell* ks = actor->spellbook.GetKnownSpell(SpellType, Level, Index);
	if (!ks) {
		return RuntimeError("Spell not known!");
	}

	return PyLong_FromLong(actor->spellbook.RemoveSpell(ks));
}

PyDoc_STRVAR(GemRB_RemoveItem__doc,
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
**See also:** [CreateItem](CreateItem.md)");

static PyObject* GemRB_RemoveItem(PyObject* /*self*/, PyObject* args)
{
	int globalID, Slot;
	int Count = 0;
	PARSE_ARGS(args, "ii|i", &globalID, &Slot, &Count);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	int ok;

	Slot = core->QuerySlot(Slot);
	actor->inventory.UnEquipItem(Slot, false);
	CREItem* si = actor->inventory.RemoveItem(Slot, Count);
	if (si) {
		ok = true;
		delete si;
	} else {
		ok = false;
	}
	return PyLong_FromLong(ok);
}

PyDoc_STRVAR(GemRB_MemorizeSpell__doc,
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
**See also:** [GetKnownSpell](GetKnownSpell.md), [UnmemorizeSpell](UnmemorizeSpell.md)");

static PyObject* GemRB_MemorizeSpell(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index, enabled = 0;
	PARSE_ARGS(args, "iiii|i", &globalID, &SpellType, &Level, &Index, &enabled);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	const CREKnownSpell* ks = actor->spellbook.GetKnownSpell(SpellType, Level, Index);
	if (!ks) {
		return RuntimeError("Spell not found!");
	}

	// auto-refresh innates (memorisation defaults to depleted)
	if (core->HasFeature(GFFlags::HAS_SPELLLIST)) {
		if (SpellType == IE_IWD2_SPELL_INNATE) enabled = 1;
	} else {
		if (SpellType == IE_SPELL_TYPE_INNATE) enabled = 1;
	}

	return PyLong_FromLong(actor->spellbook.MemorizeSpell(ks, enabled));
}


PyDoc_STRVAR(GemRB_UnmemorizeSpell__doc,
	     "===== UnmemorizeSpell =====\n\
\n\
**Prototype:** GemRB.UnmemorizeSpell (PartyID, SpellType, Level, Index[, flags])\n\
\n\
**Description:** Unmemorizes specified memorized spell. If flags are set, \n\
they will limit removal to a spell with specified depletion (and the same \n\
resref as the provided spell).\n\
\n\
**Parameters:**\n\
  * PartyID      - the PC's position in the party\n\
  * SpellType    - 0 - priest, 1 - wizard, 2 - innate\n\
  * Level        - the memorized spell's level\n\
  * Index        - the memorized spell's index\n\
  * flags        - optional, remove only an already depleted (1) or non-depleted (2) spell\n\
\n\
**Return value:** boolean, 1 on success\n\
\n\
**See also:** [MemorizeSpell](MemorizeSpell.md), [GetMemorizedSpellsCount](GetMemorizedSpellsCount.md), [GetMemorizedSpell](GetMemorizedSpell.md)");

static PyObject* GemRB_UnmemorizeSpell(PyObject* /*self*/, PyObject* args)
{
	int globalID, SpellType, Level, Index;
	uint8_t onlyDepleted = 0;
	PARSE_ARGS(args, "iiii|b", &globalID, &SpellType, &Level, &Index, &onlyDepleted);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	const CREMemorizedSpell* ms = actor->spellbook.GetMemorizedSpell(SpellType, Level, Index);
	if (!ms) {
		return RuntimeError("Spell not found!\n");
	}
	if (onlyDepleted)
		return PyLong_FromLong(actor->spellbook.UnmemorizeSpell(ms->SpellResRef, false, onlyDepleted));
	else
		return PyLong_FromLong(actor->spellbook.UnmemorizeSpell(ms));
}

PyDoc_STRVAR(GemRB_GetInventoryInfo__doc,
	     "===== GetInventoryInfo =====\n\
\n\
**Prototype:** GemRB.GetInventoryInfo (globalID)\n\
\n\
**Description:** Returns several details about current slots in the specified actor's inventory\n\
\n\
**Parameters:**\n\
  * globalID - the actor's global ID or the PC's position in the party\n\
\n\
**Return value:** dictionary\n\
  * 'FistSlot'\n\
  * 'MagicSlot' - The magic slot if a magic weapon was spawned, None othewise.\n\
  * 'WeaponSlot' - The first melee slot.\n\
  * 'UsedSlot' - The equipped slot, the fist slot if nothing is equipped.\n\
  * 'HasEquippedAbilities' - Whether any inventory item is granting a usable ability\n\
\n\
**See also:** [GetSlotItem](GetSlotItem.md), [GetItem](GetItem.md)");

static PyObject* GemRB_GetInventoryInfo(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PARSE_ARGS(args, "i", &globalID);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	PyObject* dict = PyDict_New();
	int magicSlot = Inventory::GetMagicSlot();
	if (actor->inventory.IsSlotEmpty(magicSlot)) {
		Py_INCREF(Py_None);
		PyDict_SetItemString(dict, "MagicSlot", Py_None);
	} else {
		PyDict_SetItemString(dict, "MagicSlot", PyLong_FromLong(magicSlot));
	}
	PyDict_SetItemString(dict, "FistSlot", PyLong_FromLong(Inventory::GetFistSlot()));
	PyDict_SetItemString(dict, "WeaponSlot", PyLong_FromLong(Inventory::GetWeaponSlot()));
	PyDict_SetItemString(dict, "UsedSlot", PyLong_FromLong(actor->inventory.GetEquippedSlot()));
	std::vector<ItemExtHeader> itemData;
	PyDict_SetItemString(dict, "HasEquippedAbilities", PyBool_FromLong(actor->inventory.GetEquipmentInfo(itemData, 0, 0)));
	return dict;
}

PyDoc_STRVAR(GemRB_GetSlotItem__doc,
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
  * 'Slot'  - The same as the slot parameter.\n\
  * 'LauncherSlot' - The slot of the launcher, if any, 0 otherwise.\n\
\n\
**See also:** [GetItem](GetItem.md), [Button_SetItemIcon](Button_SetItemIcon.md), [ChangeItemFlag](ChangeItemFlag.md)");

static PyObject* GemRB_GetSlotItem(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int idx;
	int translated = 0; // inventory slots are numbered differently in CRE and need to be remapped
	PARSE_ARGS(args, "ii|i", &globalID, &idx, &translated);
	const CREItem* si;
	int header = -1;

	int launcherSlot = 0;
	if (globalID == 0) {
		si = core->GetDraggedItem()->item;
	} else {
		GET_GAME();
		GET_ACTOR_GLOBAL();

		auto slot = translated ? idx : core->QuerySlot(idx);
		header = actor->PCStats->GetHeaderForSlot(slot);
		si = actor->inventory.GetSlotItem(slot);
		launcherSlot = actor->inventory.FindSlotRangedWeapon(slot);
		if (launcherSlot == Inventory::GetFistSlot()) {
			launcherSlot = 0;
		}
	}
	if (!si) {
		Py_RETURN_NONE;
	}
	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "ItemResRef", PyString_FromResRef(si->ItemResRef));
	PyDict_SetItemString(dict, "Usages0", PyLong_FromLong(si->Usages[0]));
	PyDict_SetItemString(dict, "Usages1", PyLong_FromLong(si->Usages[1]));
	PyDict_SetItemString(dict, "Usages2", PyLong_FromLong(si->Usages[2]));
	PyDict_SetItemString(dict, "Flags", PyLong_FromLong(si->Flags));
	PyDict_SetItemString(dict, "Header", PyLong_FromLong(header));
	PyDict_SetItemString(dict, "Slot", PyLong_FromLong(idx));
	PyDict_SetItemString(dict, "LauncherSlot", PyLong_FromLong(launcherSlot));
	return dict;
}

PyDoc_STRVAR(GemRB_ChangeItemFlag__doc,
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
**Return value:** Returns 0 if the item was not found, 1 otherwise.\n\
\n\
**See also:** [GetSlotItem](GetSlotItem.md)");

static PyObject* GemRB_ChangeItemFlag(PyObject* /*self*/, PyObject* args)
{
	int globalID, Slot, Flags;
	BitOp Mode;
	PARSE_ARGS(args, "iiii", &globalID, &Slot, &Flags, &Mode);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (actor->inventory.ChangeItemFlag(core->QuerySlot(Slot), Flags, Mode)) {
		return PyLong_FromLong(1);
	}
	return PyLong_FromLong(0);
}


PyDoc_STRVAR(GemRB_CanUseItemType__doc,
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
**See also:** [DropDraggedItem](DropDraggedItem.md), [UseItem](UseItem.md)");

static PyObject* GemRB_CanUseItemType(PyObject* /*self*/, PyObject* args)
{
	int SlotType, globalID, Equipped;
	PyObject* cstr = nullptr;

	globalID = 0;
	PARSE_ARGS(args, "iO|ii", &SlotType, &cstr, &globalID, &Equipped);

	ResRef ItemName = ResRefFromPy(cstr);
	if (ItemName.IsEmpty()) {
		return PyLong_FromLong(0);
	}
	const Item* item = gamedata->GetItem(ItemName, true);
	if (!item) {
		Log(MESSAGE, "GUIScript", "Cannot find item {} to check!", ItemName);
		return PyLong_FromLong(0);
	}
	const Actor* actor = nullptr;
	if (globalID) {
		GET_GAME();

		if (globalID > 1000) {
			actor = game->GetActorByGlobalID(globalID);
		} else {
			actor = game->FindPC(globalID);
		}
		if (!actor) {
			return RuntimeError("Actor not found!\n");
		}
	}

	int ret = core->CanUseItemType(SlotType, item, actor, false, Equipped != 0);
	gamedata->FreeItem(item, ItemName, false);
	return PyLong_FromLong(ret);
}


PyDoc_STRVAR(GemRB_GetSlots__doc,
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
**See also:** [GetSlotType](GetSlotType.md)\n\
");

static PyObject* GemRB_GetSlots(PyObject* /*self*/, PyObject* args)
{
	int SlotType, Count, MaxCount, globalID;
	int flag = 1;
	PARSE_ARGS(args, "ii|i", &globalID, &SlotType, &flag);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	MaxCount = static_cast<int>(core->SlotTypes);
	Count = 0;
	for (int i = 0; i < MaxCount; i++) {
		int id = core->QuerySlot(i);
		if ((core->QuerySlotType(id) & (ieDword) SlotType) != (ieDword) SlotType) {
			continue;
		}
		const CREItem* slot = actor->inventory.GetSlotItem(id);
		if (flag) {
			if (flag < 0 && slot) continue;
			if (flag > 0 && !slot) continue;
		}
		Count++;
	}

	PyObject* tuple = PyTuple_New(Count);
	Count = 0;
	for (int i = 0; i < MaxCount; i++) {
		int id = core->QuerySlot(i);
		if ((core->QuerySlotType(id) & (ieDword) SlotType) != (ieDword) SlotType) {
			continue;
		}
		const CREItem* slot = actor->inventory.GetSlotItem(id);
		if (flag) {
			if (flag < 0 && slot) continue;
			if (flag > 0 && !slot) continue;
		}
		PyTuple_SetItem(tuple, Count++, PyLong_FromLong(i));
	}

	return tuple;
}

PyDoc_STRVAR(GemRB_FindItem__doc,
	     "===== FindItem =====\n\
\n\
**Prototype:** GemRB.FindItem (globalID, itemname)\n\
\n\
**Description:** Returns the slot number of the actor's item (or -1).\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * itemname - item resource reference\n\
\n\
**Return value:** integer, -1 if not found\n\
\n\
**See also:** [GetItem](GetItem.md), [GetSlots](GetSlots.md), [GetSlotType](GetSlotType.md)");

static PyObject* GemRB_FindItem(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PyObject* ItemName = nullptr;

	if (!PyArg_ParseTuple(args, "iO", &globalID, &ItemName)) {
		return NULL;
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int slot = actor->inventory.FindItem(ResRefFromPy(ItemName), IE_INV_ITEM_UNDROPPABLE);
	return PyLong_FromLong(slot);
}

PyDoc_STRVAR(GemRB_GetItem__doc,
	     "===== GetItem =====\n\
\n\
**Prototype:** GemRB.GetItem (ResRef)\n\
\n\
**Description:** Returns dictionary with the specified item's data.\n\
\n\
**Parameters:**\n\
  * ResRef - the resource reference of the item\n\
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
**See also:** [GetSlotItem](GetSlotItem.md), [GetSpell](GetSpell.md), [Button_SetItemIcon](Button_SetItemIcon.md)");

#define CAN_DRINK  1 //potions
#define CAN_READ   2 //scrolls
#define CAN_STUFF  4 //containers
#define CAN_SELECT 8 //items with more abilities

static PyObject* GemRB_GetItem(PyObject* /*self*/, PyObject* args)
{
	PyObject* cstr = nullptr;
	PARSE_ARGS(args, "O", &cstr);

	ResRef resref = ResRefFromPy(cstr);
	const Item* item = gamedata->GetItem(resref, true);
	if (!item) {
		Log(MESSAGE, "GUIScript", "Cannot get item {}!", resref);
		Py_RETURN_NONE;
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "ItemName", DecRef(PyLong_FromLong, (signed) item->GetItemName(false)));
	PyDict_SetItemString(dict, "ItemNameIdentified", DecRef(PyLong_FromLong, (signed) item->GetItemName(true)));
	PyDict_SetItemString(dict, "ItemDesc", DecRef(PyLong_FromLong, (signed) item->GetItemDesc(false)));
	PyDict_SetItemString(dict, "ItemDescIdentified", DecRef(PyLong_FromLong, (signed) item->GetItemDesc(true)));
	PyDict_SetItemString(dict, "ItemIcon", DecRef(PyString_FromResRef, item->ItemIcon));
	PyDict_SetItemString(dict, "DescIcon", DecRef(PyString_FromResRef, item->DescriptionIcon));
	PyDict_SetItemString(dict, "BrokenItem", DecRef(PyString_FromResRef, item->ReplacementItem));
	PyDict_SetItemString(dict, "MaxStackAmount", DecRef(PyLong_FromLong, item->MaxStackAmount));
	PyDict_SetItemString(dict, "Dialog", DecRef(PyString_FromResRef, item->Dialog));
	PyDict_SetItemString(dict, "DialogName", DecRef(PyLong_FromLong, (signed) item->DialogName));
	PyDict_SetItemString(dict, "Price", DecRef(PyLong_FromLong, item->Price));
	PyDict_SetItemString(dict, "Type", DecRef(PyLong_FromLong, item->ItemType));
	PyDict_SetItemString(dict, "AnimationType", DecRef(PyString_FromASCII<AnimRef>, item->AnimationType));
	PyDict_SetItemString(dict, "Exclusion", DecRef(PyLong_FromLong, item->ItemExcl));
	PyDict_SetItemString(dict, "LoreToID", DecRef(PyLong_FromLong, item->LoreToID));
	PyDict_SetItemString(dict, "Enchantment", PyLong_FromLong(item->Enchantment));
	PyDict_SetItemString(dict, "MaxCharge", PyLong_FromLong(0));

	size_t ehc = item->ext_headers.size();

	PyObject* tooltiptuple = PyTuple_New(ehc);
	PyObject* locationtuple = PyTuple_New(ehc);
	for (size_t i = 0; i < ehc; ++i) {
		const ITMExtHeader* eh = &item->ext_headers[i];
		PyTuple_SetItem(tooltiptuple, i, PyLong_FromStrRef(eh->Tooltip));
		PyTuple_SetItem(locationtuple, i, PyLong_FromLong(eh->Location));
		PyDict_SetItemString(dict, "MaxCharge", DecRef(PyLong_FromLong, eh->Charges));
	}

	PyDict_SetItemString(dict, "Tooltips", tooltiptuple);
	PyDict_SetItemString(dict, "Locations", locationtuple);

	Py_DecRef(tooltiptuple);
	Py_DecRef(locationtuple);

	int function = 0;

	if (core->CheckItemType(item, SLOT_POTION)) {
		function |= CAN_DRINK;
	}
	if (core->CheckItemType(item, SLOT_SCROLL)) {
		//determining if this is a copyable scroll
		if (ehc < 2) {
			goto not_a_scroll;
		}
		const ITMExtHeader* eh = &item->ext_headers[1];
		if (eh->features.size() < 1) {
			goto not_a_scroll;
		}
		const Effect* f = eh->features[0];

		//normally the learn spell opcode is 147
		EffectQueue::ResolveEffect(fx_learn_spell_ref);
		if (f->Opcode != (ieDword) fx_learn_spell_ref.opcode) {
			goto not_a_scroll;
		}
		//maybe further checks for school exclusion?
		//no, those were done by CanUseItemType
		function |= CAN_READ;
		PyDict_SetItemString(dict, "Spell", PyString_FromResRef(f->Resource));
	} else if (ehc > 1) {
		function |= CAN_SELECT;
	}
not_a_scroll:
	if (core->CheckItemType(item, SLOT_BAG) && gamedata->Exists(resref, IE_STO_CLASS_ID)) {
		//allow the open container flag only if there is
		//a store file (this fixes pst eye items, which
		//got the same item type as bags)
		//while this isn't required anymore, as bag itemtypes are customisable
		//we still better check for the existence of the store, or we
		//get a crash somewhere.
		function |= CAN_STUFF;
	}
	PyDict_SetItemString(dict, "Function", PyLong_FromLong(function));
	gamedata->FreeItem(item, resref, false);
	return dict;
}

static void DragItem(CREItem* si)
{
	if (!si) {
		return;
	}
	const Item* item = gamedata->GetItem(si->ItemResRef);
	if (!item) {
		return;
	}
	core->DragItem(si, item->ItemIcon);
	gamedata->FreeItem(item, si->ItemResRef, false);
}

static int CheckRemoveItem(const Actor* actor, const CREItem* si, int action)
{
	///check if item is undroppable because the actor likes it
	if (UsedItems.empty()) {
		ReadUsedItems();
	}

	for (const auto& usedItem : UsedItems) {
		if (usedItem.itemname.IsEmpty() || usedItem.itemname != si->ItemResRef) {
			continue;
		}
		//true if names don't match
		int nomatch = usedItem.username[0] && usedItem.username != actor->GetScriptName();

		switch (action) {
			//the named actor cannot remove it
			case CRI_REMOVE:
				if (usedItem.flags & 1) {
					if (nomatch) continue;
				} else
					continue;
				break;
			//the named actor can equip it
			case CRI_EQUIP:
				if (usedItem.flags & 2) {
					if (!nomatch) continue;
				} else
					continue;
				break;
			//the named actor can swap it
			case CRI_SWAP:
				if (usedItem.flags & 4) {
					if (!nomatch) continue;
				} else
					continue;
				break;
			//the named actor cannot remove it except when initiating a swap (used for plain inventory slots)
			// and make sure not to treat earrings improperly
			case CRI_REMOVEFORSWAP:
				if (!(usedItem.flags & 1) || usedItem.flags & 4) {
					continue;
				}
				break;
			default:
				break;
		}

		displaymsg->DisplayString(usedItem.GetFeedback(), GUIColors::WHITE, STRING_FLAGS::SOUND);
		return 1;
	}
	return 0;
}

// TNO has an ear and an eye slot that share the same slot type, so normal checks fail
// return false if we're trying to stick an earing into our eye socket or vice versa
static bool CheckEyeEarMatch(CREItem* NewItem, int Slot)
{
	if (UsedItems.empty()) {
		ReadUsedItems();
	}

	for (const auto& usedItem : UsedItems) {
		if (!usedItem.itemname.IsEmpty() && usedItem.itemname != NewItem->ItemResRef) {
			continue;
		}

		//8 - (pst) can only be equipped in eye slots
		//16 - (pst) can only be equipped in ear slots
		if (usedItem.flags & 8) {
			return Slot == 1; // eye/left ear/helmet
		} else if (usedItem.flags & 16) {
			return Slot == 7; //right ear/caleidoscope
		}

		return true;
	}
	return true;
}

static CREItem* TryToUnequip(Actor* actor, unsigned int Slot, unsigned int Count)
{
	//we should use getslotitem, because
	//getitem would remove the item from the inventory!
	CREItem* si = actor->inventory.GetSlotItem(Slot);
	if (!si) return nullptr;

	//it is always possible to put these items into the inventory
	// however in pst, we need to ensure immovable swappables are swappable
	bool isdragging = core->GetDraggedItem() != NULL;
	if (core->QuerySlotType(Slot) & SLOT_INVENTORY) {
		if (CheckRemoveItem(actor, si, CRI_REMOVEFORSWAP)) {
			return NULL;
		}
	} else {
		if (CheckRemoveItem(actor, si, isdragging ? CRI_SWAP : CRI_REMOVE)) {
			return NULL;
		}
	}
	if (!actor->inventory.UnEquipItem(Slot, false)) {
		// Item is currently undroppable/cursed
		if (si->Flags & IE_INV_ITEM_CURSED) {
			displaymsg->DisplayConstantString(HCStrings::Cursed, GUIColors::WHITE);
		} else {
			displaymsg->DisplayConstantString(HCStrings::CantDropItem, GUIColors::WHITE);
		}
		return NULL;
	}
	si = actor->inventory.RemoveItem(Slot, Count);
	return si;
}

PyDoc_STRVAR(GemRB_DragItem__doc,
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
**See also:** [DropDraggedItem](DropDraggedItem.md), [IsDraggingItem](IsDraggingItem.md)");

static PyObject* GemRB_DragItem(PyObject* /*self*/, PyObject* args)
{
	ResRef Sound;
	int globalID, Slot, Count = 0, Type = 0;
	PyObject* resref = nullptr;
	PARSE_ARGS(args, "iiO|ii", &globalID, &Slot, &resref, &Count, &Type);

	// FIXME
	// we should Drop the Dragged item in place of the current item
	// but only if the current item is draggable, tough!
	if (core->GetDraggedItem()) {
		Py_RETURN_NONE;
	}

	GET_GAME();
	Actor* actor;
	if (globalID > 1000) {
		actor = game->GetActorByGlobalID(globalID);
	} else {
		actor = game->FindPC(globalID);
	}

	if (!actor) {
		return RuntimeError("Actor not found!\n");
	}

	CREItem* si;
	if (Type) {
		Map* map = actor->GetCurrentArea();
		if (!map) {
			return RuntimeError("No current area!");
		}
		Container* cc = map->GetPile(actor->Pos);
		if (!cc) {
			return RuntimeError("No current container!");
		}
		si = cc->RemoveItem(Slot, Count);
	} else {
		if ((unsigned int) Slot > core->GetInventorySize()) {
			return AttributeError("Invalid slot");
		}
		si = TryToUnequip(actor, core->QuerySlot(Slot), Count);
		actor->RefreshEffects();
		actor->ReinitQuickSlots();
		core->SetEventFlag(EF_SELECTION);
	}
	if (!si) {
		Py_RETURN_NONE;
	}

	OverrideSound(si->ItemResRef, Sound, IS_GET);
	if (!Sound.IsEmpty()) {
		core->GetAudioDrv()->Play(Sound, SFXChannel::GUI);
	}

	//if res is positive, it is gold!
	int res = core->CanMoveItem(si);
	if (res > 0) {
		game->AddGold(res);
		delete si;
		Py_RETURN_NONE;
	}

	core->DragItem(si, ResRefFromPy(resref));
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_DropDraggedItem__doc,
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
**See also:** [DragItem](DragItem.md), [IsDraggingItem](IsDraggingItem.md), [CanUseItemType](CanUseItemType.md)\n\
");

static PyObject* GemRB_DropDraggedItem(PyObject* /*self*/, PyObject* args)
{
	ResRef Sound;
	int globalID, Slot;
	PARSE_ARGS(args, "ii", &globalID, &Slot);

	// FIXME
	if (!core->GetDraggedItem()) {
		Py_RETURN_NONE;
	}

	Label* l = core->GetMessageLabel();
	if (l) {
		// this is how BG2 behaves, not sure about others
		l->SetText(u""); // clear previous message
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (Slot == -2) {
		Map* map = actor->GetCurrentArea();
		if (!map) {
			return RuntimeError("No current area!");
		}
		Container* cc = map->GetPile(actor->Pos);
		if (!cc) {
			return RuntimeError("No current container!");
		}
		CREItem* si = core->GetDraggedItem()->item;
		int res = cc->AddItem(si);
		OverrideSound(si->ItemResRef, Sound, IS_DROP);
		if (!Sound.IsEmpty()) {
			core->GetAudioDrv()->Play(Sound, SFXChannel::GUI);
		}
		if (res == ASI_SUCCESS) {
			// Whole amount was placed
			core->ReleaseDraggedItem();
		}
		return PyLong_FromLong(res);
	}

	int Slottype, Effect;
	switch (Slot) {
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
			Slottype = core->QuerySlotType(Slot);
			Effect = core->QuerySlotEffects(Slot);
	}

	// too far away?
	const Actor* current = game->FindPC(game->GetSelectedPCSingle());
	if (current && current != actor &&
	    (actor->GetCurrentArea() != current->GetCurrentArea() ||
	     SquaredPersonalDistance(actor, current) > MAX_DISTANCE * MAX_DISTANCE)) {
		displaymsg->DisplayConstantString(HCStrings::TooFarAway, GUIColors::WHITE);
		return PyLong_FromLong(ASI_FAILED);
	}

	CREItem* slotitem = core->GetDraggedItem()->item;
	const Item* item = gamedata->GetItem(slotitem->ItemResRef);
	if (!item) {
		return PyLong_FromLong(ASI_FAILED);
	}
	bool ranged = item->GetWeaponHeader(true) != NULL;

	// can't equip item because of similar already equipped
	if (Effect) {
		if (item->ItemExcl & actor->inventory.GetEquipExclusion(Slot)) {
			displaymsg->DisplayConstantString(HCStrings::ItemExclusion, GUIColors::WHITE);
			//freeing the item before returning
			gamedata->FreeItem(item, slotitem->ItemResRef, false);
			return PyLong_FromLong(ASI_FAILED);
		}
	}

	if ((Slottype != -1) && (Slottype & SLOT_WEAPON)) {
		const CREItem* weapon = actor->inventory.GetUsedWeapon(false, Effect);
		if (weapon && (weapon->Flags & IE_INV_ITEM_CURSED)) {
			displaymsg->DisplayConstantString(HCStrings::Cursed, GUIColors::WHITE);
			return PyLong_FromLong(ASI_FAILED);
		}
	}

	// can't equip item because it is not identified
	if ((Slottype == SLOT_ITEM) && !(slotitem->Flags & IE_INV_ITEM_IDENTIFIED)) {
		const ITMExtHeader* eh = item->GetExtHeader(0);
		if (eh && eh->IDReq) {
			displaymsg->DisplayConstantString(HCStrings::ItemNeedsId, GUIColors::WHITE);
			gamedata->FreeItem(item, slotitem->ItemResRef, false);
			return PyLong_FromLong(ASI_FAILED);
		}
	}

	//it is always possible to put these items into the inventory
	if (!(Slottype & SLOT_INVENTORY)) {
		if (CheckRemoveItem(actor, slotitem, CRI_EQUIP)) {
			return PyLong_FromLong(ASI_FAILED);
		}
	}

	//CanUseItemType will check actor's class bits too
	Slottype = core->CanUseItemType(Slottype, item, actor, true) & SLOT_UMD_MASK;
	//resolve the equipping sound, it needs to be resolved before
	OverrideSound(slotitem->ItemResRef, Sound, IS_DROP);

	//freeing the item before returning
	gamedata->FreeItem(item, slotitem->ItemResRef, false);
	if (!Slottype) {
		return PyLong_FromLong(ASI_FAILED);
	}
	int res = actor->inventory.AddSlotItem(slotitem, Slot, Slottype, ranged);
	if (res) {
		//release it only when fully placed
		if (res == ASI_SUCCESS) {
			core->ReleaseDraggedItem();
		}
		// res == ASI_PARTIAL
		//EquipItem (in AddSlotItem) already called RefreshEffects
		actor->ReinitQuickSlots();
		//couldn't place item there, try swapping (only if slot is explicit)
	} else if (Slot >= 0) {
		//swapping won't cure this
		HCStrings msg = actor->inventory.WhyCantEquip(Slot, slotitem->Flags & IE_INV_ITEM_TWOHANDED, ranged);
		if (msg != HCStrings::count) {
			displaymsg->DisplayConstantString(msg, GUIColors::WHITE);
			return PyLong_FromLong(ASI_FAILED);
		}
		// pst: also check TNO earing/eye silliness: both share the same slot type
		if (Slottype == 1 && !CheckEyeEarMatch(slotitem, Slot)) {
			displaymsg->DisplayConstantString(HCStrings::WrongItemType, GUIColors::WHITE);
			return PyLong_FromLong(ASI_FAILED);
		}
		CREItem* tmp = TryToUnequip(actor, Slot, 0);
		if (tmp) {
			//this addslotitem MUST succeed because the slot was
			//just emptied (canuseitemtype already confirmed too)
			actor->inventory.AddSlotItem(slotitem, Slot, Slottype);
			core->ReleaseDraggedItem();
			DragItem(tmp);
			// switched items, not returned by normal AddSlotItem
			res = ASI_SWAPPED;
			//EquipItem (in AddSlotItem) already called RefreshEffects
			actor->RefreshEffects();
			actor->ReinitQuickSlots();
			core->SetEventFlag(EF_SELECTION);
		} else {
			res = ASI_FAILED;
		}
	} else {
		displaymsg->DisplayConstantString(HCStrings::InventoryFull, GUIColors::WHITE);
	}

	if (Sound && Sound[0]) {
		core->GetAudioDrv()->Play(Sound, SFXChannel::GUI);
	}
	return PyLong_FromLong(res);
}

PyDoc_STRVAR(GemRB_IsDraggingItem__doc,
	     "===== IsDraggingItem =====\n\
\n\
**Prototype:** GemRB.IsDraggingItem ()\n\
\n\
**Description:** Returns true if we are dragging some item.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** boolean\n\
\n\
**See also:** [DragItem](DragItem.md), [DropDraggedItem](DropDraggedItem.md)\n\
");

static PyObject* GemRB_IsDraggingItem(PyObject* /*self*/, PyObject* /*args*/)
{
	return PyBool_FromLong(core->GetDraggedItem() != NULL);
}

PyDoc_STRVAR(GemRB_GetSystemVariable__doc,
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
    * SV_SAVEPATH = 5 - path to the parent of save/mpsave/bpsave dir\n\
\n\
**Return value:** -1 if the index is invalid, otherwise the requested value.\n\
\n\
**See also:** [GetGameString](GetGameString.md)\n\
");

static PyObject* GemRB_GetSystemVariable(PyObject* /*self*/, PyObject* args)
{
	int Variable, value = 0;
	path_t path;
	PARSE_ARGS(args, "i", &Variable);
	switch (Variable) {
		case SV_BPP:
			value = core->config.Bpp;
			break;
		case SV_WIDTH:
			value = core->config.Width;
			break;
		case SV_HEIGHT:
			value = core->config.Height;
			break;
		case SV_GAMEPATH:
			path = core->config.GamePath;
			break;
		case SV_TOUCH:
			value = EventMgr::TouchInputEnabled;
			break;
		case SV_SAVEPATH:
			path = core->config.SavePath;
			break;
		default:
			value = -1;
			break;
	}
	if (path.length()) {
		return PyString_FromStringObj(path);
	} else {
		return PyLong_FromLong(value);
	}
}

PyDoc_STRVAR(GemRB_CreateItem__doc,
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
**Return value:** N/A");

static PyObject* GemRB_CreateItem(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int SlotID = -1;
	int Charge0 = 1, Charge1 = 0, Charge2 = 0;
	PyObject* ItemResRef = nullptr;
	PARSE_ARGS(args, "iO|iiii", &globalID, &ItemResRef, &SlotID, &Charge0, &Charge1, &Charge2);
	ResRef itemRef = ResRefFromPy(ItemResRef);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (SlotID == -1) {
		//This is already a slot ID we need later
		SlotID = actor->inventory.FindCandidateSlot(SLOT_INVENTORY, 0);
	} else {
		//I believe we need this only here
		SlotID = core->QuerySlot(SlotID);
	}

	if (SlotID == -1) {
		// Create item on ground
		Map* map = actor->GetCurrentArea();
		if (map) {
			CREItem* item = new CREItem();
			if (!CreateItemCore(item, itemRef, Charge0, Charge1, Charge2)) {
				delete item;
			} else {
				map->AddItemToLocation(actor->Pos, item);
			}
		}
	} else {
		// Note: this forcefully gets rid of any item currently
		// in the slot without properly unequipping it
		actor->inventory.SetSlotItemRes(itemRef, SlotID, Charge0, Charge1, Charge2);
		actor->inventory.EquipItem(SlotID);
		//EquipItem already called RefreshEffects
		actor->ReinitQuickSlots();
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetAvatarsValue__doc,
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
");
// NOTE: currently it can only lookup the animation prefixes!
static PyObject* GemRB_GetAvatarsValue(PyObject* /*self*/, PyObject* args)
{
	int globalID, col;

	if (!PyArg_ParseTuple(args, "ii", &globalID, &col)) {
		return NULL;
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyString_FromResRef(actor->GetAnims()->GetArmourLevel(col));
}

PyDoc_STRVAR(GemRB_SetMapAnimation__doc,
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
");

static PyObject* GemRB_SetMapAnimation(PyObject* /*self*/, PyObject* args)
{
	PyObject* pyRef = nullptr;
	int Cycle = 0;
	int Flags = 0x19;
	ieWordSigned Height = 0x1e;
	//the animation is cloned by AddAnimation, so we can keep the original on
	//the stack
	AreaAnimation anim;
	PARSE_ARGS(args, "iiO|iih", &anim.Pos.x, &anim.Pos.y, &pyRef, &Flags, &Cycle, &Height);

	GET_GAME();

	GET_MAP();

	ResRef resref = ResRefFromPy(pyRef);
	anim.appearance = 0xffffffff; //scheduled for every hour
	anim.Name = resref;
	anim.BAM = resref;
	anim.flags = AreaAnimation::Flags(Flags);
	anim.sequence = static_cast<AreaAnimation::index_t>(Cycle);
	anim.height = Height;
	map->AddAnimation(std::move(anim));
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetMapnote__doc,
	     "===== SetMapnote =====\n\
\n\
**Prototype:** GemRB.SetMapnote(X, Y[, color, text])\n\
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
**See also:** [Window_CreateMapControl](Window_CreateMapControl.md)\n\
");

static PyObject* GemRB_SetMapnote(PyObject* /*self*/, PyObject* args)
{
	Point point;
	ieWord color = 0;
	PyObject* textObject = nullptr;
	PARSE_ARGS(args, "ii|hO", &point.x, &point.y, &color, &textObject);

	GET_GAME();
	GET_MAP();

	String text { u"" };
	if (textObject) {
		text = PyString_AsStringObj(textObject);
	}

	if (text.length() > 0) {
		map->AddMapNote(point, MapNote(std::move(text), color, false));
	} else {
		map->RemoveMapNote(point);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetMapDoor__doc,
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
");

static PyObject* GemRB_SetMapDoor(PyObject* /*self*/, PyObject* args)
{
	const char* DoorName;
	int State;

	if (!PyArg_ParseTuple(args, "si", &DoorName, &State)) {
		return NULL;
	}

	GET_GAME();

	GET_MAP();

	Door* door = map->TMap->GetDoor(ieVariable(DoorName));
	if (!door) {
		return RuntimeError("No such door!");
	}

	door->SetDoorOpen(State, 0, 0);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetMapExit__doc,
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
**Return value:** N/A");

static PyObject* GemRB_SetMapExit(PyObject* /*self*/, PyObject* args)
{
	const char* ExitName;
	PyObject* NewArea = nullptr;
	const char* NewEntrance = NULL;
	PARSE_ARGS(args, "s|Os", &ExitName, &NewArea, &NewEntrance);

	GET_GAME();

	GET_MAP();

	InfoPoint* ip = map->TMap->GetInfoPoint(ieVariable(ExitName));
	if (!ip || ip->Type != ST_TRAVEL) {
		return RuntimeError("No such exit!");
	}

	if (!NewArea) {
		//disable entrance
		ip->Flags |= TRAP_DEACTIVATED;
	} else {
		//activate entrance
		ip->Flags &= ~TRAP_DEACTIVATED;
		//set destination area
		ip->Destination = ResRefFromPy(NewArea);
		//change entrance only if supplied
		if (NewEntrance) {
			ip->EntranceName = ieVariable(NewEntrance);
		}
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetMapRegion__doc,
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
");

static PyObject* GemRB_SetMapRegion(PyObject* /*self*/, PyObject* args)
{
	const char* Name;
	PyObject* TrapScript = nullptr;
	PARSE_ARGS(args, "s|O", &Name, &TrapScript);

	GET_GAME();
	GET_MAP();

	InfoPoint* ip = map->TMap->GetInfoPoint(ieVariable(Name));
	if (ip) {
		if (TrapScript) {
			ip->Flags &= ~TRAP_DEACTIVATED;
			ip->SetScript(ResRefFromPy(TrapScript), 0);
		} else {
			ip->Flags |= TRAP_DEACTIVATED;
		}
	}

	Py_RETURN_NONE;
}


PyDoc_STRVAR(GemRB_CreateCreature__doc,
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
**See also:** [CreateItem](CreateItem.md)");

static PyObject* GemRB_CreateCreature(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PyObject* cstr = nullptr;
	int PosX = -1, PosY = -1;
	PARSE_ARGS(args, "iO|ii", &globalID, &cstr, &PosX, &PosY);

	GET_GAME();
	GET_MAP();

	ResRef CreResRef = ResRefFromPy(cstr);
	if (PosX != -1 && PosY != -1) {
		map->SpawnCreature(Point(PosX, PosY), CreResRef);
	} else {
		GET_ACTOR_GLOBAL();
		map->SpawnCreature(actor->Pos, CreResRef, Size(10, 10));
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_RevealArea__doc,
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
**See also:** [ExploreArea](ExploreArea.md)");

static PyObject* GemRB_RevealArea(PyObject* /*self*/, PyObject* args)
{
	int x, y;
	int radius;
	int Value;
	PARSE_ARGS(args, "iiii", &x, &y, &radius, &Value);

	Point p(x, y);
	GET_GAME();
	GET_MAP();
	map->ExploreMapChunk(SearchmapPoint(p), radius, Value);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_ExploreArea__doc,
	     "===== ExploreArea =====\n\
\n\
**Prototype:** GemRB.ExploreArea ([explored=True])\n\
\n\
**Description:** Explores or unexplores the whole area.\n\
\n\
**Parameters:**\n\
  * explored:\n\
    * False - undo explore\n\
    * True - explore\n\
    * all other values give meaningless results\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [MoveToArea](MoveToArea.md), [RevealArea](RevealArea.md)");

static PyObject* GemRB_ExploreArea(PyObject* /*self*/, PyObject* args)
{
	PyObject* explored = nullptr;
	PARSE_ARGS(args, "|O", &explored);
	GET_GAME();

	GET_MAP();

	map->FillExplored(explored ? PyObject_IsTrue(explored) : false);

	Py_RETURN_NONE;
}


PyDoc_STRVAR(GemRB_GetRumour__doc,
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
**See also:** [EnterStore](EnterStore.md), [GetStoreDrink](GetStoreDrink.md), [GetStore](GetStore.md)\n\
");

static PyObject* GemRB_GetRumour(PyObject* /*self*/, PyObject* args)
{
	int percent;
	PyObject* rumourRef = nullptr;
	PARSE_ARGS(args, "iO", &percent, &rumourRef);
	if (RAND(0, 99) >= percent) {
		return PyLong_FromLong(-1);
	}

	ieStrRef strref = core->GetRumour(ResRefFromPy(rumourRef));
	return PyLong_FromStrRef(strref);
}

PyDoc_STRVAR(GemRB_GamePause__doc,
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
**Return value:** the resulting paused state");

static PyObject* GemRB_GamePause(PyObject* /*self*/, PyObject* args)
{
	int quiet;
	int ret;
	PauseState pause;
	PARSE_ARGS(args, "Ii", &pause, &quiet);

	GET_GAMECONTROL();

	//this will trigger when pause is not 0 or 1
	switch (pause) {
		case PauseState::Toggle:
			ret = int(core->TogglePause());
			break;
		case PauseState::Off:
		case PauseState::On:
			core->SetPause(pause, quiet);
			// fall through
		default:
			ret = gc->GetDialogueFlags() & DF_FREEZE_SCRIPTS;
	}
	RETURN_BOOL(ret);
}

PyDoc_STRVAR(GemRB_CheckFeatCondition__doc,
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
  * a_op ... d_op - operator to use for comparing x_stat to x_value:\n\
    * 0: less or equals\n\
    * 1: equals\n\
    * 2: less than\n\
    * 3: greater than\n\
    * 4: greater or equals\n\
    * 5: not equals\n\
\n\
**Return value:** bool\n\
\n\
**See also:** [GetPlayerStat](GetPlayerStat.md), [SetPlayerStat](SetPlayerStat.md)\n\
");

static PyObject* GemRB_CheckFeatCondition(PyObject* /*self*/, PyObject* args)
{
	std::string callback;
	PyObject* p[13] {};

	if (!PyArg_UnpackTuple(args, "ref", 9, 13, &p[0], &p[1], &p[2], &p[3], &p[4], &p[5], &p[6], &p[7], &p[8], &p[9], &p[10], &p[11], &p[12])) {
		return NULL;
	}

	if (!PyObject_TypeCheck(p[0], &PyLong_Type)) {
		return NULL;
	}
	int pc = static_cast<int>(PyLong_AsLong(p[0]));

	if (PyObject_TypeCheck(p[1], &PyUnicode_Type)) {
		callback = ASCIIStringFromPy<std::string>(p[1]); // callback
	}

	GET_GAME();

	const Actor* actor = game->FindPC(pc);
	if (!actor) {
		return RuntimeError("Actor not found!\n");
	}

	/* see if the special function exists */
	if (!callback.empty()) {
		std::string fname = fmt::format("Check_{}", callback);

		ScriptEngine::FunctionParameters params;
		params.push_back(ScriptEngine::Parameter(p[0]));
		for (int i = 3; i < 13; i++) {
			params.push_back(ScriptEngine::Parameter(p[i]));
		}

		PyObject* pValue = gs->RunPyFunction("Feats", fname.c_str(), params);
		if (pValue) {
			return pValue;
		}
		return RuntimeError("Callback failed");
	}

	std::array<int, 9> reqs;
	for (size_t i = 0; i < reqs.size(); i++) {
		reqs[i] = static_cast<int>(PyLong_AsLong(p[i]));
	}

	bool ret = true;
	if (reqs[1] || reqs[2]) {
		int op = p[9] ? static_cast<int>(PyLong_AsLong(p[9])) : GREATER_OR_EQUALS;
		ret = DiffCore(actor->GetBase(reqs[1]), reqs[2], op);
	}

	if (reqs[3] || reqs[4]) {
		int op = p[10] ? static_cast<int>(PyLong_AsLong(p[10])) : GREATER_OR_EQUALS;
		ret |= DiffCore(actor->GetBase(reqs[3]), reqs[4], op);
	}

	if (!ret)
		goto endofquest;

	if (reqs[5] || reqs[6]) {
		// no | because the formula is (a|b) & (c|d)
		int op = p[11] ? static_cast<int>(PyLong_AsLong(p[11])) : GREATER_OR_EQUALS;
		ret = DiffCore(actor->GetBase(reqs[5]), reqs[6], op);
	}

	if (reqs[7] || reqs[8]) {
		int op = p[12] ? static_cast<int>(PyLong_AsLong(p[12])) : GREATER_OR_EQUALS;
		ret |= DiffCore(actor->GetBase(reqs[7]), reqs[8], op);
	}

	if (PyErr_Occurred()) {
		return RuntimeError("Invalid parameter; expected a number.");
	}

endofquest:
	RETURN_BOOL(ret);
}

PyDoc_STRVAR(GemRB_HasFeat__doc,
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
**See also:** [CheckFeatCondition](CheckFeatCondition.md)\n\
");

static PyObject* GemRB_HasFeat(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	Feat featindex;
	PARSE_ARGS(args, "ib", &globalID, &featindex);
	GET_GAME();
	GET_ACTOR_GLOBAL();
	return PyLong_FromLong(actor->GetFeat(featindex));
}

PyDoc_STRVAR(GemRB_SetFeat__doc,
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
**See also:** [HasFeat](HasFeat.md)");

static PyObject* GemRB_SetFeat(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int value;
	Feat featIndex;
	PARSE_ARGS(args, "ibi", &globalID, &featIndex, &value);
	GET_GAME();
	GET_ACTOR_GLOBAL();
	actor->SetFeatValue(featIndex, value, false);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetMaxEncumbrance__doc,
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
**Return value:** integer");

static PyObject* GemRB_GetMaxEncumbrance(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PARSE_ARGS(args, "i", &globalID);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyLong_FromLong(actor->GetMaxEncumbrance());
}

PyDoc_STRVAR(GemRB_GetAbilityBonus__doc,
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
**See also:** [SetPlayerStat](SetPlayerStat.md), [GetPlayerStat](GetPlayerStat.md), [Table_GetValue](Table_GetValue.md)");

static PyObject* GemRB_GetAbilityBonus(PyObject* /*self*/, PyObject* args)
{
	int stat, column, value, ex = 0;
	int ret;
	PARSE_ARGS(args, "iii|i", &stat, &column, &value, &ex);

	GET_GAME();

	const Actor* actor = game->FindPC(game->GetSelectedPCSingle());
	if (!actor) {
		return RuntimeError("Actor not found!\n");
	}

	switch (stat) {
		case IE_STR:
			ret = core->GetStrengthBonus(column, value, ex);
			break;
		case IE_INT:
			ret = core->GetIntelligenceBonus(column, value);
			break;
		case IE_DEX:
			ret = core->GetDexterityBonus(column, value);
			break;
		case IE_CON:
			ret = core->GetConstitutionBonus(column, value);
			break;
		case IE_CHR:
			ret = core->GetCharismaBonus(column, value);
			break;
		case IE_LORE:
			ret = core->GetLoreBonus(column, value);
			break;
		case IE_REPUTATION: //both chr and reputation affect the reaction, but chr is already taken
			ret = GetReaction(actor, NULL); // this is used only for display, so the null is fine
			break;
		case IE_WIS:
			ret = core->GetWisdomBonus(column, value);
			break;
		default:
			return RuntimeError("Invalid ability!");
	}
	return PyLong_FromLong(ret);
}

PyDoc_STRVAR(GemRB_LeaveParty__doc,
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
**See also:** [GetPartySize](GetPartySize.md)\n\
");

static PyObject* GemRB_LeaveParty(PyObject* /*self*/, PyObject* args)
{
	int globalID, initDialog = 0;
	PARSE_ARGS(args, "i|i", &globalID, &initDialog);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (initDialog) {
		if (initDialog == 2) {
			GameScript::SetLeavePartyDialogFile(actor, nullptr);
		}
		if (actor->GetBase(IE_HITPOINTS) > 0) {
			actor->Stop();
			actor->AddAction("Dialogue([PC])");
		}
	}
	game->LeaveParty(actor, false);

	Py_RETURN_NONE;
}

typedef union pack {
	ieDword data;
	ieByte bytes[4];
} packtype;

static void ReadActionButtons()
{
	memset(GUIAction, -1, sizeof(GUIAction));
	memset(GUITooltip, -1, sizeof(GUITooltip));
	auto tab = gamedata->LoadTable("guibtact");
	assert(tab);
	for (unsigned int i = 0; i < MAX_ACT_COUNT; i++) {
		packtype row;

		row.bytes[0] = tab->QueryFieldUnsigned<ieByte>(i, 0);
		row.bytes[1] = tab->QueryFieldUnsigned<ieByte>(i, 1);
		row.bytes[2] = tab->QueryFieldUnsigned<ieByte>(i, 2);
		row.bytes[3] = tab->QueryFieldUnsigned<ieByte>(i, 3);
		GUIAction[i] = row.data;
		GUITooltip[i] = tab->QueryFieldAsStrRef(i, 4);
		GUIResRef[i] = tab->QueryField(i, 5);
		GUIEvent[i] = tab->GetRowName(i);
	}
}

static void SetButtonCycle(std::shared_ptr<const AnimationFactory> bam, Button* btn, AnimationFactory::index_t cycle, ButtonImage which)
{
	Holder<Sprite2D> tspr = bam->GetFrame(cycle, 0);
	btn->SetImage(which, std::move(tspr));
}

PyDoc_STRVAR(GemRB_Button_SetActionIcon__doc,
	     "===== Button_SetActionIcon =====\n\
\n\
**Metaclass Prototype:** SetActionIcon (Dict, ActionIndex[, Function])\n\
\n\
**Description:** Sets up an action button based on the guibtact table. \n\
The ActionIndex should be less than 34. This action will set the button's \n\
image, the tooltip and the push button event handler.\n\
\n\
**Parameters:**\n\
  * Dict - the environment you want to have access to\n\
  * ActionIndex - the row number in the guibtact.2da file\n\
  * Function - function key to assign\
\n\
**Return value:** N/A\n\
\n\
**See also:** [Button_SetSpellIcon](Button_SetSpellIcon.md), [Button_SetItemIcon](Button_SetItemIcon.md)");

static PyObject* SetActionIcon(Button* btn, PyObject* dict, int Index, int Function)
{
	if (Index >= MAX_ACT_COUNT) {
		return NULL;
	}
	ABORT_IF_NULL(btn);

	if (Index < 0) {
		btn->SetImage(ButtonImage::None, nullptr);
		btn->SetAction(nullptr, Control::Click, GEM_MB_ACTION, 0, 1);
		btn->SetAction(nullptr, Control::Click, GEM_MB_MENU, 0, 1);
		btn->SetTooltip(u"");
		//no incref
		return Py_None;
	}

	if (GUIAction[0] == 0xcccccccc) {
		ReadActionButtons();
	}

	auto bam = gamedata->GetFactoryResourceAs<const AnimationFactory>(GUIResRef[Index], IE_BAM_CLASS_ID);
	if (!bam) {
		return RuntimeError(fmt::format("{} BAM not found", GUIResRef[Index]));
	}
	packtype row;

	row.data = GUIAction[Index];
	SetButtonCycle(bam, btn, (char) row.bytes[0], ButtonImage::Unpressed);
	SetButtonCycle(bam, btn, (char) row.bytes[1], ButtonImage::Pressed);
	SetButtonCycle(bam, btn, (char) row.bytes[2], ButtonImage::Selected);
	SetButtonCycle(std::move(bam), btn, (char) row.bytes[3], ButtonImage::Disabled);
	btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE | IE_GUI_BUTTON_PICTURE, BitOp::NAND);
	PyObject* Event = PyUnicode_FromFormat("Action%sPressed", GUIEvent[Index].c_str());
	PyObject* func = PyDict_GetItem(dict, Event);
	btn->SetAction(PythonControlCallback(func), Control::Click, GEM_MB_ACTION, 0, 1);

	PyObject* Event2 = PyUnicode_FromFormat("Action%sRightPressed", GUIEvent[Index].c_str());
	PyObject* func2 = PyDict_GetItem(dict, Event2);
	btn->SetAction(PythonControlCallback(func2), Control::Click, GEM_MB_MENU, 0, 1);

	//cannot make this const, because it will be freed
	if (GUITooltip[Index] != ieStrRef::INVALID) {
		SetViewTooltipFromRef(btn, GUITooltip[Index]);
	}

	btn->SetHotKey(GEM_FUNCTIONX(Function), 0, true);

	//no incref
	return Py_None;
}

static PyObject* GemRB_Button_SetActionIcon(PyObject* self, PyObject* args)
{
	int Index;
	int Function = 0;
	PyObject* dict;
	PARSE_ARGS(args, "OOi|i", &self, &dict, &Index, &Function);

	Button* btn = GetView<Button>(self);
	PyObject* ret = SetActionIcon(btn, dict, Index, Function);
	if (ret) {
		Py_INCREF(ret);
	}
	return ret;
}

PyDoc_STRVAR(GemRB_HasResource__doc,
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
**Return value:** boolean");

static PyObject* GemRB_HasResource(PyObject* /*self*/, PyObject* args)
{
	PyObject* ResRef = nullptr;
	int ResType;
	int silent = 0;
	PARSE_ARGS(args, "Oi|i", &ResRef, &ResType, &silent);

	if (PyUnicode_Check(ResRef)) {
		// This may be checking of IWD2 sound folders
		RETURN_BOOL(gamedata->Exists(PyString_AsStringObj(ResRef), ResType, silent));
	} else {
		RETURN_BOOL(gamedata->Exists(PyString_AsStringView(ResRef), ResType, silent));
	}
}

PyDoc_STRVAR(GemRB_Window_SetupEquipmentIcons__doc,
	     "===== Window_SetupEquipmentIcons =====\n\
\n\
**Metaclass Prototype:** SetupEquipmentIcons (Dict, Slot[, Start, Offset])\n\
\n\
**Description:** Sets up all 12 action buttons for a player character \n\
with the usable equipment functions. \n\
It also sets up the scroll buttons left and right if needed. \n\
If Start is supplied, it will skip the first few items.\n\
\n\
**Parameters:**\n\
  * Dict - the environment you want to have access to\n\
  * Slot        - the player character's index in the party\n\
  * Start       - start the equipment list from this value\n\
  * Offset      - control ID offset to the first usable button\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [UseItem](UseItem.md)");

static PyObject* GemRB_Window_SetupEquipmentIcons(PyObject* self, PyObject* args)
{
	int globalID;
	int Start = 0;
	int Offset = 0; //control offset (iwd2 has the action buttons starting at 6)
	PyObject* dict;
	PARSE_ARGS(args, "OOi|ii", &self, &dict, &globalID, &Start, &Offset);

	const Window* win = GetView<Window>(self);
	ABORT_IF_NULL(win);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	// -1 because of the left/right scroll icons
	static std::vector<ItemExtHeader> ItemArray(GUIBT_COUNT);
	bool more = actor->inventory.GetEquipmentInfo(ItemArray, Start, GUIBT_COUNT - (Start ? 1 : 0));
	int i;
	if (Start || more) {
		Button* btn = GetControl<Button>(Offset, win);
		if (!btn || btn->ControlType != IE_GUI_BUTTON) {
			return RuntimeError("Cannot set action button!\n");
		}
		const PyObject* ret = SetActionIcon(btn, dict, ACT_LEFT, 0);
		if (!ret) {
			return NULL;
		}
	}
	// pst doesn't have a file, but uses the float window instead any way
	auto bam = gamedata->GetFactoryResourceAs<const AnimationFactory>(GUIResRef[9], IE_BAM_CLASS_ID);
	if (!bam) {
		return RuntimeError("guibtbut BAM not found");
	}

	for (i = 0; i < GUIBT_COUNT - (more ? 1 : 0); i++) {
		Button* btn = GetControl<Button>(i + Offset + (Start ? 1 : 0), win);
		if (!btn || btn->ControlType != IE_GUI_BUTTON) {
			Log(ERROR, "GUIScript", "Button {} not found!", i + Offset + (Start ? 1 : 0));
			continue;
		}
		PyObject* Function = PyDict_GetItemString(dict, "EquipmentPressed");
		btn->SetAction(PythonControlCallback(Function), Control::Click, GEM_MB_ACTION, 0, 1);
		btn->BindDictVariable("Equipment", Start + i);

		const ItemExtHeader& item = ItemArray[i];
		Holder<Sprite2D> Picture;

		if (!item.UseIcon.IsEmpty()) {
			Picture = gamedata->GetBAMSprite(item.UseIcon, 1, 0, true);
			// try cycle 0 if cycle 1 doesn't exist
			// (needed for e.g. sppr707b which is used by Daystar's Sunray)
			if (!Picture)
				Picture = gamedata->GetBAMSprite(item.UseIcon, 0, 0, true);
		}

		if (!Picture) {
			btn->SetState(Button::DISABLED);
			btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE, BitOp::SET);
			btn->SetTooltip(u"");
		} else {
			SetButtonCycle(bam, btn, 0, ButtonImage::Unpressed);
			SetButtonCycle(bam, btn, 1, ButtonImage::Pressed);
			SetButtonCycle(bam, btn, 2, ButtonImage::Selected);
			SetButtonCycle(bam, btn, 3, ButtonImage::Disabled);
			btn->SetPicture(std::move(Picture));
			btn->SetState(Button::UNPRESSED);
			btn->SetFlags(IE_GUI_BUTTON_PICTURE | IE_GUI_BUTTON_ALIGN_BOTTOM | IE_GUI_BUTTON_ALIGN_RIGHT, BitOp::SET);

			SetViewTooltipFromRef(btn, item.Tooltip);

			if (item.Charges && item.Charges != 0xffff) {
				SetItemText(btn, item.Charges, false);
			} else if (!item.Charges && item.ChargeDepletion == CHG_NONE) {
				btn->SetState(Button::DISABLED);
			}
		}
	}

	if (more) {
		Button* btn = GetControl<Button>(i + Offset + 1, win);
		if (!btn || btn->ControlType != IE_GUI_BUTTON) {
			return RuntimeError("Cannot set action button!\n");
		}
		const PyObject* ret = SetActionIcon(btn, dict, ACT_RIGHT, i + 1);
		if (!ret) {
			return NULL;
		}
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_ClearActions__doc,
	     "===== ClearActions =====\n\
\n\
**Prototype:** GemRB.ClearActions (globalID)\n\
\n\
**Description:** Stops an actor's movement and any pending action.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_ClearActions(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PARSE_ARGS(args, "i", &globalID);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (actor->GetInternalFlag() & IF_NOINT) {
		Log(MESSAGE, "GuiScript", "Cannot break action!");
		Py_RETURN_NONE;
	}
	if (!actor->GetPath() && !actor->Modal.State && !actor->objects.LastTarget && actor->objects.LastTargetPos.IsInvalid() && !actor->objects.LastSpellTarget) {
		Log(MESSAGE, "GuiScript", "No breakable action!");
		Py_RETURN_NONE;
	}
	actor->Stop(); //stop pending action involved walking
	actor->SetModal(Modal::None); // stop modal actions
	Py_RETURN_NONE;
}


PyDoc_STRVAR(GemRB_SetDefaultActions__doc,
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
");

static PyObject* GemRB_SetDefaultActions(PyObject* /*self*/, PyObject* args)
{
	int qslot;
	int slot1, slot2, slot3;
	PARSE_ARGS(args, "iiii", &qslot, &slot1, &slot2, &slot3);
	Actor::SetDefaultActions((bool) qslot, (ieByte) slot1, (ieByte) slot2, (ieByte) slot3);
	Py_RETURN_NONE;
}


PyDoc_STRVAR(GemRB_SetupQuickSpell__doc,
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
**Return value:** integer, target type constant — or None for actors with no quick slots\n\
");

static PyObject* GemRB_SetupQuickSpell(PyObject* /*self*/, PyObject* args)
{
	SpellExtHeader spelldata {};
	int globalID, which, slot, type;
	PARSE_ARGS(args, "iiii", &globalID, &slot, &which, &type);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	if (!actor->PCStats) {
		//no quick slots for this actor, which is not a fatal error
		Py_RETURN_NONE;
	}

	actor->spellbook.GetSpellInfo(&spelldata, type, which, 1);
	if (spelldata.spellName.IsEmpty()) {
		return RuntimeError("Invalid parameter! Spell not found!\n");
	}

	actor->PCStats->QuickSpells[slot] = spelldata.spellName;
	actor->PCStats->QuickSpellBookType[slot] = static_cast<ieByte>(type);

	return PyLong_FromLong(spelldata.Target);
}

PyDoc_STRVAR(GemRB_SetupQuickSlot__doc,
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
**See also:** [GetEquippedQuickSlot](GetEquippedQuickSlot.md), [SetEquippedQuickSlot](SetEquippedQuickSlot.md)\n\
");

static PyObject* GemRB_SetupQuickSlot(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int qslotID;
	ieWord slot;
	ieWord headerIndex = 0;
	PARSE_ARGS(args, "iiH|H", &globalID, &qslotID, &slot, &headerIndex);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	slot = static_cast<ieWord>(core->QuerySlot(slot));
	// recache info for potentially changed ammo or weapon ability
	actor->inventory.SetEquipped(static_cast<ieWordSigned>(actor->inventory.GetEquipped()), headerIndex); // reset EquippedHeader
	actor->SetupQuickSlot(qslotID, slot, headerIndex);
	actor->inventory.CacheAllWeaponInfo(); // needs to happen after SetupQuickSlot, so QuickWeaponHeaders is updated already
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetEquippedQuickSlot__doc,
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
**Return value:** N/A\n\
\n\
**See also:** [GetEquippedQuickSlot](GetEquippedQuickSlot.md), [SetupQuickSlot](SetupQuickSlot.md)\n\
");

static PyObject* GemRB_SetEquippedQuickSlot(PyObject* /*self*/, PyObject* args)
{
	int slot;
	int dummy;
	int globalID;
	int ability = -1;
	PARSE_ARGS(args, "ii|i", &globalID, &slot, &ability);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	const CREItem* item = actor->inventory.GetUsedWeapon(false, dummy);
	if (item && (item->Flags & IE_INV_ITEM_CURSED)) {
		displaymsg->DisplayConstantString(HCStrings::Cursed, GUIColors::WHITE);
		Py_RETURN_NONE;
	}

	// for iwd2 we also need to take care of the paired slot
	// otherwise we'll happily draw a bow and a shield in the same hand
	bool reequip = false;
	if (core->HasFeature(GFFlags::HAS_WEAPON_SETS) && actor->PCStats) {
		// unequip the old one, reequip the new one, if any
		reequip = true;
		int shieldSlot = actor->inventory.GetShieldSlot();

		if (!actor->inventory.IsSlotEmpty(shieldSlot)) {
			if (actor->inventory.UnEquipItem(shieldSlot, false)) {
				actor->SetUsedShield({}, IE_ANI_WEAPON_INVALID);
			}
		}
	}

	HCStrings ret = actor->SetEquippedQuickSlot(slot, ability);
	if (ret == HCStrings::count) {
		// set up the (new) offhand slot again
		if (reequip) {
			int shieldSlot = actor->inventory.GetShieldSlot();
			if (shieldSlot != -1) actor->inventory.EquipItem(shieldSlot);
		}
	} else {
		// error occurred
		displaymsg->DisplayConstantString(ret, GUIColors::WHITE);
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetEquippedQuickSlot__doc,
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
**See also:** [SetEquippedQuickSlot](SetEquippedQuickSlot.md), [GetEquippedAmmunition](GetEquippedAmmunition.md)\n\
");

static PyObject* GemRB_GetEquippedQuickSlot(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int NoTrans = 0;
	PARSE_ARGS(args, "i|i", &globalID, &NoTrans);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int ret = actor->inventory.GetEquippedSlot();
	if (!actor->PCStats) {
		return PyLong_FromLong(core->FindSlot(ret));
	}

	for (int i = 0; i < 4; i++) {
		if (ret == actor->PCStats->QuickWeaponSlots[i]) {
			if (NoTrans) {
				return PyLong_FromLong(i);
			}
			ret = i + Inventory::GetWeaponSlot();
			break;
		}
	}
	return PyLong_FromLong(core->FindSlot(ret));
}

PyDoc_STRVAR(GemRB_GetEquippedAmmunition__doc,
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
**See also:** [GetEquippedQuickSlot](GetEquippedQuickSlot.md)\n\
");

static PyObject* GemRB_GetEquippedAmmunition(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PARSE_ARGS(args, "i", &globalID);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int ret = actor->inventory.GetEquippedSlot();
	int effect = core->QuerySlotEffects(ret);
	if (effect == SLOT_EFFECT_MISSILE) {
		return PyLong_FromLong(core->FindSlot(ret));
	} else {
		return PyLong_FromLong(-1);
	}
}

PyDoc_STRVAR(GemRB_GetModalState__doc,
	     "===== GetModalState =====\n\
\n\
**Prototype:** GemRB.GetModalState (globalID)\n\
\n\
**Description:** Gets an actor's modal state. The modal states are listed \n\
in ie_modal.py.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** state number\n\
\n\
**See also:** [SetModalState](SetModalState.md)\n\
");

static PyObject* GemRB_GetModalState(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PARSE_ARGS(args, "i", &globalID);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyLong_FromLong(UnderType(actor->Modal.State));
}

PyDoc_STRVAR(GemRB_SetModalState__doc,
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
\n\
    GemRB.SetModalState (pc, MS_TURNUNDEAD)\n\
\n\
The above example makes the player start the turn undead action.\n\
\n\
**See also:** [SetPlayerStat](SetPlayerStat.md), [SetPlayerName](SetPlayerName.md)\n\
");

static PyObject* GemRB_SetModalState(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int state;
	PyObject* spell = nullptr;
	PARSE_ARGS(args, "ii|O", &globalID, &state, &spell);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->SetModal((Modal) state, false);
	actor->SetModalSpell((Modal) state, ResRefFromPy(spell));
	if (actor->ModalSpellSkillCheck()) {
		actor->ApplyModal(actor->Modal.Spell); // force immediate effect
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_PrepareSpontaneousCast__doc,
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
**Return value:** new spell's spellinfo index");

static PyObject* GemRB_PrepareSpontaneousCast(PyObject* /*self*/, PyObject* args)
{
	int globalID, type, level;
	PyObject* spell = nullptr;
	PyObject* spell2 = nullptr;

	PARSE_ARGS(args, "iOiiO", &globalID, &spell, &type, &level, &spell2);
	ResRef replacementSpell = ResRefFromPy(spell2);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	// deplete original memorisation
	actor->spellbook.UnmemorizeSpell(ResRefFromPy(spell), true, 2);
	// set spellinfo to all known spells of desired type
	std::vector<ResRef> data;
	actor->spellbook.SetCustomSpellInfo(data, ResRef(), 1 << type);
	SpellExtHeader spelldata {};
	int idx = actor->spellbook.FindSpellInfo(&spelldata, replacementSpell, 1 << type);

	return PyLong_FromLong(idx - 1);
}

PyDoc_STRVAR(GemRB_SpellCast__doc,
	     "===== SpellCast =====\n\
\n\
**Prototype:** GemRB.SpellCast (PartyID, Type, Spell[, ResRef])\n\
\n\
**Description:** Makes PartyID cast a spell of Type. This handles targeting \n\
and executes the appropriate scripting command.\n\
\n\
**Parameters:**\n\
  * PartyID - player character's index in the party\n\
  * Type    - switch between casting modes:\n\
    * spell(book) type bitfield (1-mage, 2-priest, 4-innate and iwd2 versions)\n\
    * -1: don't cast, but reinit the GUI spell list\n\
    * -2: cast from a quickspell slot\n\
    * -3: cast directly, does not require the spell to be memorized\n\
  * Spell   - spell's index in the memorised list\n\
  * ResRef  - (optional) spell resref for type -3 casts\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [UseItem](UseItem.md)\n\
");

static PyObject* GemRB_SpellCast(PyObject* /*self*/, PyObject* args)
{
	unsigned int globalID;
	int type;
	unsigned int spell;
	const char* resRef = nullptr;
	PARSE_ARGS(args, "iii|s", &globalID, &type, &spell, &resRef);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	//don't cast anything, just reinit the spell list
	if (type == -1) {
		actor->spellbook.ClearSpellInfo();
		Py_RETURN_NONE;
	}

	SpellExtHeader spelldata {};
	std::vector<ResRef> data;
	if (type == -3) {
		data.push_back(ResRef(resRef));
		actor->spellbook.SetCustomSpellInfo(data, ResRef(), 0);
		actor->spellbook.GetSpellInfo(&spelldata, 255, 0, 1);
	} else if (type == -2) {
		//resolve quick spell slot
		if (!actor->PCStats) {
			//no quick slots for this actor, is this an error?
			//return RuntimeError( "Actor has no quickslots!\n" );
			Py_RETURN_NONE;
		}
		actor->spellbook.FindSpellInfo(&spelldata, actor->PCStats->QuickSpells[spell], actor->PCStats->QuickSpellBookType[spell]);
	} else {
		ieDword ActionLevel = core->GetDictionary().Get("ActionLevel", 0);
		if (ActionLevel == 5) {
			// get the right spell, since the lookup below only checks the memorized list
			actor->spellbook.SetCustomSpellInfo(data, ResRef(), type);
		}
		actor->spellbook.GetSpellInfo(&spelldata, type, spell, 1);
	}

	Log(MESSAGE, "GUIScript", "Cast spell: {}", spelldata.spellName);
	Log(MESSAGE, "GUIScript", "Slot: {}", spelldata.slot);
	Log(MESSAGE, "GUIScript", "Type: {} ({} vs {})", spelldata.type, 1 << spelldata.type, type);
	//cannot make this const, because it will be freed
	String tmp = core->GetString(spelldata.strref);
	Log(MESSAGE, "GUIScript", "Spellname: {}", fmt::WideToChar { tmp });
	Log(MESSAGE, "GUIScript", "Target: {}", spelldata.Target);
	Log(MESSAGE, "GUIScript", "Range: {}", spelldata.Range);
	if (type > 0 && !((1 << spelldata.type) & type)) {
		return RuntimeError("Wrong type of spell!");
	}

	GET_GAMECONTROL();

	switch (spelldata.Target) {
		case TARGET_SELF:
			gc->SetupCasting(spelldata.spellName, spelldata.type, spelldata.level, spelldata.slot, GA_NO_DEAD, spelldata.TargetNumber);
			gc->TryToCast(actor, actor);
			break;
		case TARGET_NONE:
			//reset the cursor
			gc->ResetTargetMode();
			//this is always instant casting without spending the spell
			core->ApplySpell(spelldata.spellName, actor, actor, 0);
			break;
		case TARGET_AREA:
			gc->SetupCasting(spelldata.spellName, spelldata.type, spelldata.level, spelldata.slot, GA_POINT | GA_NO_DEAD, spelldata.TargetNumber);
			break;
		case TARGET_CREA:
			gc->SetupCasting(spelldata.spellName, spelldata.type, spelldata.level, spelldata.slot, GA_NO_DEAD, spelldata.TargetNumber);
			break;
		case TARGET_DEAD:
			gc->SetupCasting(spelldata.spellName, spelldata.type, spelldata.level, spelldata.slot, 0, spelldata.TargetNumber);
			break;
		case TARGET_INV:
			//bring up inventory in the end???
			//break;
		default:
			Log(ERROR, "GUIScript", "Unhandled target type: {}", spelldata.Target);
			break;
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_ApplySpell__doc,
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
  * casterID - global id of the desired caster or the index in the party\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [SpellCast](SpellCast.md), [ApplyEffect](ApplyEffect.md), [CountEffects](CountEffects.md)\n\
");

static PyObject* GemRB_ApplySpell(PyObject* /*self*/, PyObject* args)
{
	int globalID, casterID = 0;
	PyObject* spell = nullptr;
	PARSE_ARGS(args, "iO|i", &globalID, &spell, &casterID);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	Actor* caster = NULL;
	const Map* map = game->GetCurrentArea();
	if (casterID < 1000) {
		caster = game->FindPC(casterID);
	} else if (map) {
		caster = map->GetActorByGlobalID(casterID);
	}
	if (!caster) caster = game->GetActorByGlobalID(casterID);
	if (!caster) caster = actor;

	core->ApplySpell(ResRefFromPy(spell), actor, caster, 0);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_UseItem__doc,
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
**See also:** [CanUseItemType](CanUseItemType.md), [SpellCast](SpellCast.md), [Window_SetupEquipmentIcons](Window_SetupEquipmentIcons.md)\n\
");

static PyObject* GemRB_UseItem(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int slot;
	int header;
	int forcetarget = -1; //some crappy scrolls don't target self correctly!
	PARSE_ARGS(args, "iii|i", &globalID, &slot, &header, &forcetarget);

	GET_GAME();
	GET_GAMECONTROL();
	GET_ACTOR_GLOBAL();

	std::vector<ItemExtHeader> itemData(1);
	ItemExtHeader itemdata;
	int flags = 0;

	switch (slot) {
		case -1:
			//some equipment
			actor->inventory.GetEquipmentInfo(itemData, header, 1);
			itemdata = itemData[0];
			break;
		case -2:
			//quickslot
			actor->GetItemSlotInfo(&itemdata, header, -1);
			if (!itemdata.Charges) {
				Log(MESSAGE, "GUIScript", "QuickItem has no charges.");
				Py_RETURN_NONE;
			}
			break;
		default:
			//any normal slot
			actor->GetItemSlotInfo(&itemdata, core->QuerySlot(slot), header);
			flags = UI_SILENT;
			break;
	}

	if (forcetarget == -1) {
		forcetarget = itemdata.Target;
	}

	//is there any better check for a non existent item?
	if (itemdata.itemName.IsEmpty()) {
		Log(WARNING, "GUIScript", "Empty slot used?");
		Py_RETURN_NONE;
	}

	/// remove this after projectile is done
	Log(MESSAGE, "GUIScript", "Use item: {}", itemdata.itemName);
	Log(MESSAGE, "GUIScript", "Extended header: {}", itemdata.headerindex);
	Log(MESSAGE, "GUIScript", "Attacktype: {}", itemdata.AttackType);
	Log(MESSAGE, "GUIScript", "Range: {}", itemdata.Range);
	Log(MESSAGE, "GUIScript", "Target: {}", forcetarget);
	Log(MESSAGE, "GUIScript", "Projectile: {}", itemdata.ProjectileAnimation);
	int count = 1;
	switch (forcetarget) {
		case TARGET_SELF:
			if (core->HasFeature(GFFlags::TEAM_MOVEMENT)) count += 1000; // pst inventory workaround to avoid another parameter
			gc->SetupItemUse(itemdata.slot, itemdata.headerindex, GA_NO_DEAD, count);
			gc->TryToCast(actor, actor);
			break;
		case TARGET_NONE:
			gc->ResetTargetMode();
			actor->UseItem(itemdata.slot, static_cast<int>(itemdata.headerindex), nullptr, flags);
			break;
		case TARGET_AREA:
			gc->SetupItemUse(itemdata.slot, itemdata.headerindex, GA_POINT | GA_NO_DEAD, itemdata.TargetNumber);
			break;
		case TARGET_CREA:
			gc->SetupItemUse(itemdata.slot, itemdata.headerindex, GA_NO_DEAD, itemdata.TargetNumber);
			break;
		case TARGET_DEAD:
			gc->SetupItemUse(itemdata.slot, itemdata.headerindex, 0, itemdata.TargetNumber);
			break;
		default:
			Log(ERROR, "GUIScript", "Unhandled target type!");
			break;
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetGamma__doc,
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
**See also:** [SetFullScreen](SetFullScreen.md)\n\
");

static PyObject* GemRB_SetGamma(PyObject* /*self*/, PyObject* args)
{
	int brightness, contrast;

	if (!PyArg_ParseTuple(args, "ii", &brightness, &contrast)) {
		return NULL;
	}
	if (brightness < 0 || brightness > 40) {
		return RuntimeError("Brightness must be 0-40");
	}
	if (contrast < 0 || contrast > 5) {
		return RuntimeError("Contrast must be 0-5");
	}
	VideoDriver->SetGamma(brightness, contrast);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetMouseScrollSpeed__doc,
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
**See also:** [SetTooltipDelay](SetTooltipDelay.md)");

static PyObject* GemRB_SetMouseScrollSpeed(PyObject* /*self*/, PyObject* args)
{
	int mouseSpeed;
	PARSE_ARGS(args, "i", &mouseSpeed);
	core->SetMouseScrollSpeed(mouseSpeed);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetTooltipDelay__doc,
	     "===== SetTooltipDelay =====\n\
\n\
**Prototype:** GemRB.SetTooltipDelay (time)\n\
\n\
**Description:** Sets the tooltip delay.\n\
\n\
**Parameters:**\n\
  * time - delay in fifteenth of a second\n\
\n\
**See also:** [View_SetTooltip](View_SetTooltip.md), [SetMouseScrollSpeed](SetMouseScrollSpeed.md)\n\
");

static PyObject* GemRB_SetTooltipDelay(PyObject* /*self*/, PyObject* args)
{
	int tooltipDelay;
	PARSE_ARGS(args, "i", &tooltipDelay);
	WindowManager::SetTooltipDelay(tooltipDelay);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetFullScreen__doc,
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
**See also:** [SetGamma](SetGamma.md)\n\
");

static PyObject* GemRB_SetFullScreen(PyObject* /*self*/, PyObject* args)
{
	int fullscreen;
	PARSE_ARGS(args, "i", &fullscreen);
	VideoDriver->SetFullscreenMode(fullscreen);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_RunRestScripts__doc,
	     "===== RunRestScripts =====\n\
\n\
**Prototype:** GemRB.RunRestScripts ()\n\
\n\
**Description:** Executes the party pre-rest scripts if any.\n\
\n\
**Return value:** bool, true if a dream script ran Rest or RestParty.\n\
");

static PyObject* GemRB_RunRestScripts(PyObject* /*self*/, PyObject* /*args*/)
{
	int dreamed = 0;
	GET_GAME();

	// check if anyone wants to banter first (bg2)
	static int dreamer = -2;
	if (dreamer == -2) {
		AutoTable pdtable = gamedata->LoadTable("pdialog");
		assert(pdtable);
		dreamer = pdtable->GetColumnIndex("DREAM_SCRIPT_FILE");
	}
	if (dreamer < 0) {
		return PyLong_FromLong(dreamed);
	}

	AutoTable pdtable = gamedata->LoadTable("pdialog");
	assert(pdtable);
	int ii = game->GetPartySize(true); // party size, only alive
	bool bg2expansion = core->GetGame()->Expansion == GAME_TOB;
	while (ii--) {
		Actor* tar = game->GetPC(ii, true);
		const ieVariable& scriptname = tar->GetScriptName();
		if (pdtable->GetRowIndex(scriptname) != TableMgr::npos) {
			ResRef resRef;
			if (bg2expansion) {
				resRef = pdtable->QueryField(scriptname, "25DREAM_SCRIPT_FILE");
			} else {
				resRef = pdtable->QueryField(scriptname, "DREAM_SCRIPT_FILE");
			}
			GameScript* restscript = new GameScript(resRef, tar, 0, false);
			if (restscript->Update()) {
				// there could be several steps involved, so we can't reliably check tar->Timers.lastRested
				dreamed = 1;
			}
			delete restscript;
		}
	}

	return PyLong_FromLong(dreamed);
}

PyDoc_STRVAR(GemRB_RestParty__doc,
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
");

static PyObject* GemRB_RestParty(PyObject* /*self*/, PyObject* args)
{
	int flags;
	int dream, hp;
	PARSE_ARGS(args, "iii", &flags, &dream, &hp);
	GET_GAME();

	// check if resting is possible and if not, return the reason, otherwise rest
	// Error feedback is eventually handled this way:
	// - resting outside: popup an error window with the reason in pst, print it to message window elsewhere
	// - resting in inns: popup a GUISTORE error window with the reason
	PyObject* dict = PyDict_New();
	ieStrRef err = ieStrRef::INVALID;
	bool cannotRest = !game->CanPartyRest((RestChecks) flags, &err);
	// fall back to the generic: you may not rest at this time
	if (err == ieStrRef::INVALID) {
		if (core->HasFeature(GFFlags::AREA_OVERRIDE)) {
			err = DisplayMessage::GetStringReference(HCStrings::MayNotRest);
		} else {
			err = ieStrRef::NO_REST;
		}
	}
	PyDict_SetItemString(dict, "Error", PyBool_FromLong(cannotRest));
	if (cannotRest) {
		PyDict_SetItemString(dict, "ErrorMsg", PyLong_FromStrRef(err));
		PyDict_SetItemString(dict, "Cutscene", PyBool_FromLong(0));
	} else {
		PyDict_SetItemString(dict, "ErrorMsg", PyLong_FromLong(-1));
		// all is well, so do the actual resting
		PyDict_SetItemString(dict, "Cutscene", PyBool_FromLong(game->RestParty((RestChecks) flags & RestChecks::Area, dream, hp)));
	}

	return dict;
}

PyDoc_STRVAR(GemRB_ChargeSpells__doc,
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
");
static PyObject* GemRB_ChargeSpells(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PARSE_ARGS(args, "i", &globalID);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	actor->spellbook.ChargeAllSpells();

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_HasSpecialItem__doc,
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
**Return value:** bool");

//itemtype 1 - identify
static PyObject* GemRB_HasSpecialItem(PyObject* /*self*/, PyObject* args)
{
	int globalID, itemtype, useup;
	PARSE_ARGS(args, "iii", &globalID, &itemtype, &useup);
	if (SpecialItems.empty()) {
		ReadSpecialItems();
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	size_t i = SpecialItems.size();
	int slot = -1;
	while (i--) {
		if (itemtype & ieDword(SpecialItems[i].value)) {
			slot = actor->inventory.FindItem(SpecialItems[i].resref, 0);
			if (slot == -1) continue;
			// check if candidate is good enough — not depleted
			const CREItem* si = actor->inventory.GetSlotItem(slot);
			if (!si->Usages[0]) continue;
			if (useup) {
				break;
			} else {
				return PyLong_FromLong(1);
			}
		}
	}

	if (slot < 0) {
		return PyLong_FromLong(0);
	}

	if (useup) {
		//use the found item's first usage
		useup = actor->UseItem((ieDword) slot, 0, actor, UI_SILENT | UI_FAKE | UI_NOAURA);
	}
	return PyLong_FromLong(useup);
}

PyDoc_STRVAR(GemRB_HasSpecialSpell__doc,
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
**Return value:** bool");

//specialtype 1 - identify
//            2 - can use in silence
//            4 - cannot use in wildsurge
static PyObject* GemRB_HasSpecialSpell(PyObject* /*self*/, PyObject* args)
{
	int globalID, specialtype, useup;
	PARSE_ARGS(args, "iii", &globalID, &specialtype, &useup);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	gamedata->GetSpecialSpell("noop");
	const auto& special_spells = gamedata->GetSpecialSpells();
	size_t i = special_spells.size();
	if (i == 0) {
		return RuntimeError("Game has no splspec.2da table!");
	}
	bool found = false;
	while (i--) {
		if (specialtype & special_spells[i].flags) {
			if (actor->spellbook.HaveSpell(special_spells[i].resref, useup)) {
				found = true;
				break;
			}
		}
	}

	return PyLong_FromLong(found);
}

PyDoc_STRVAR(GemRB_ApplyEffect__doc,
	     "===== ApplyEffect =====\n\
\n\
**Prototype:** GemRB.ApplyEffect (globalID, opcode, param1, param2[, resref, resref2, resref3, source, timing])\n\
\n\
**Description:** Creates a basic effect and applies it on the actor marked \n\
by globalID. \n\
This function cam be used to add stats that are stored in effect blocks.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * opcode   - the effect opcode name (for values see effects.ids)\n\
  * param1   - parameter 1 for the opcode\n\
  * param2   - parameter 2 for the opcode\n\
  * resref   - (optional) resource reference to set in effect\n\
  * resref2  - (optional) resource reference to set in the effect\n\
  * resref3  - (optional) resource reference to set in the effect\n\
  * source   - (optional) source to set in the effect\n\
  * timing   - (optional) timing mode to set in the effect\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
\n\
    for i in range(ProfCount-8):\n\
      StatID = GemRB.GetTableValue (TmpTable, i+8, 0)\n\
      Value = GemRB.GetVar ('Prof ' + str(i))\n\
      if Value:\n\
        GemRB.ApplyEffect (MyChar, 'Proficiency', Value, StatID)\n\
\n\
The above example sets the weapon proficiencies in a bg2's CharGen9.py script.\n\
\n\
**See also:** [SpellCast](SpellCast.md), [SetPlayerStat](SetPlayerStat.md), [GetPlayerStat](GetPlayerStat.md), [CountEffects](CountEffects.md)");

static PyObject* GemRB_ApplyEffect(PyObject* /*self*/, PyObject* args)
{
	ieWord timing = FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES;
	int globalID;
	const char* opcodename;
	int param1, param2;
	PyObject* resref1 = nullptr;
	PyObject* resref2 = nullptr;
	PyObject* resref3 = nullptr;
	PyObject* source = nullptr;
	if (!PyArg_ParseTuple(args, "isii|OOOOH",
			      &globalID, &opcodename, &param1, &param2,
			      &resref1, &resref2, &resref3, &source, &timing)) {
		return NULL;
	}

	GET_GAME();
	GET_ACTOR_GLOBAL();

	work_ref.Name = opcodename;
	work_ref.opcode = -1;
	Effect* fx = EffectQueue::CreateEffect(work_ref, param1, param2, timing);
	if (!fx) {
		//invalid effect name didn't resolve to opcode
		return RuntimeError("Invalid effect name!\n");
	}
	fx->Resource = ResRefFromPy(resref1);
	fx->Resource2 = ResRefFromPy(resref2);
	fx->Resource3 = ResRefFromPy(resref3);
	fx->SourceRef = ResRefFromPy(source);
	// fx->SourceType is probably useless here, so we leave it unset
	//This is a hack...
	fx->Parameter3 = 1;

	//fx is not freed by this function
	core->ApplyEffect(fx, actor, actor);

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_CountEffects__doc,
	     "===== CountEffects =====\n\
\n\
**Prototype:** GemRB.CountEffects (globalID, opcode, param1, param2[, resref='', source=''])\n\
\n\
**Description:** Counts how many matching effects are applied on the actor. \n\
If a parameter is set to -1, it will be ignored.\n\
If a opcode is set to '', any opcode will be matched.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
  * opcode   - the effect opcode name (for values see effects.ids)\n\
  * param1   - parameter 1 for the opcode\n\
  * param2   - parameter 2 for the opcode\n\
  * resref   - optional resource reference to match the effect\n\
  * source   - optional resource reference to match the effect source\n\
\n\
**Return value:** the count\n\
\n\
**Examples:**\n\
\n\
    res = GemRB.CountEffects (MyChar, 'HLA', -1, -1, AbilityName)\n\
\n\
The above example returns how many HLA effects were applied on the character.\n\
\n\
**See also:** [ApplyEffect](ApplyEffect.md)");

static PyObject* GemRB_CountEffects(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	const char* opcodename;
	int param1, param2;
	PyObject* resref = nullptr;
	PyObject* sourceRef = nullptr;
	PARSE_ARGS(args, "isii|OO", &globalID, &opcodename, &param1, &param2, &resref, &sourceRef);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	work_ref.Name = opcodename;
	work_ref.opcode = -1;
	ieDword ret = actor->fxqueue.CountEffects(work_ref, param1, param2, ResRefFromPy(resref), ResRefFromPy(sourceRef));
	return PyLong_FromLong(ret);
}

PyDoc_STRVAR(GemRB_ModifyEffect__doc,
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
**See also:** [ApplyEffect](ApplyEffect.md), [CountEffects](CountEffects.md)\n\
");

static PyObject* GemRB_ModifyEffect(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	const char* opcodename;
	int px, py;
	PARSE_ARGS(args, "isii", &globalID, &opcodename, &px, &py);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	work_ref.Name = opcodename;
	work_ref.opcode = -1;
	actor->fxqueue.ModifyEffectPoint(work_ref, px, py);
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetEffects__doc,
	     "===== GetEffects =====\n\
\n\
**Prototype:** GemRB.GetEffects (globalID, opcode)\n\
\n\
**Description:** Returns a list of effects on the target matching opcode. Used for the contingency window.\n\
\n\
**Parameters:**\n\
  * globalID - the player character's index in the party\n\
  * opcode  - the effect opcode (for values see effects.ids)\n\
\n\
**Return value:** tuple of dicts with these keys derived from the effect:\n\
  * Param1: first parameter\n\
  * Param2: second parameter\n\
  * Resource1\n\
  * Resource2\n\
  * Resource3\n\
  * Spell1Icon: icon of the first resource if it is a spell\n\
  * Spell2Icon: icon of the second resource if it is a spell\n\
  * Spell3Icon: icon of the third resource if it is a spell\n\
\n\
**See also:** [ApplyEffect](ApplyEffect.md), [CountEffects](CountEffects.md), [ModifyEffect](ModifyEffect.md)\n\
");

static PyObject* GemRB_GetEffects(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	const char* opcodeName;
	PARSE_ARGS(args, "is", &globalID, &opcodeName);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	work_ref.Name = opcodeName;
	work_ref.opcode = -1;
	if (EffectQueue::ResolveEffect(work_ref) < 0) {
		return RuntimeError("Invalid effect name provided!");
	}

	size_t count = actor->fxqueue.CountEffects(work_ref, -1, -1);
	if (!count) return PyTuple_New(0);

	auto GetSpellIcon = [](const ResRef& spellRef) {
		const Spell* spl = gamedata->GetSpell(spellRef, true);
		if (!spl) return ResRef {};
		ResRef icon = spl->SpellbookIcon;
		gamedata->FreeSpell(spl, spellRef, false);
		return icon;
	};

	PyObject* effects = PyTuple_New(count);
	auto f = actor->fxqueue.GetFirstEffect();
	int i = 0;
	Effect* fx = actor->fxqueue.GetNextEffect(f);
	while (fx) {
		if (fx->Opcode != static_cast<ieDword>(work_ref.opcode)) {
			fx = actor->fxqueue.GetNextEffect(f);
			continue;
		}
		PyObject* dict = PyDict_New();
		PyDict_SetItemString(dict, "Param1", PyLong_FromLong(fx->Parameter1));
		PyDict_SetItemString(dict, "Param2", PyLong_FromLong(fx->Parameter2));
		PyDict_SetItemString(dict, "Resource1", DecRef(PyString_FromResRef, fx->Resource));
		PyDict_SetItemString(dict, "Resource2", DecRef(PyString_FromResRef, fx->Resource2));
		PyDict_SetItemString(dict, "Resource3", DecRef(PyString_FromResRef, fx->Resource3));
		PyDict_SetItemString(dict, "Spell1Icon", DecRef(PyString_FromResRef, GetSpellIcon(fx->Resource)));
		PyDict_SetItemString(dict, "Spell2Icon", DecRef(PyString_FromResRef, GetSpellIcon(fx->Resource2)));
		PyDict_SetItemString(dict, "Spell3Icon", DecRef(PyString_FromResRef, GetSpellIcon(fx->Resource3)));

		PyTuple_SetItem(effects, i, dict);
		fx = actor->fxqueue.GetNextEffect(f);
		i++;
	}
	return effects;
}

PyDoc_STRVAR(GemRB_StealFailed__doc,
	     "===== StealFailed =====\n\
\n\
**Prototype:** GemRB.StealFailed ()\n\
\n\
**Description:** Sends the steal failed trigger (attacked) to the owner \n\
of the current store — the Sender of the StartStore action.\n\
\n\
**Return value:** N/A\n\
\n\
**See also:** [GetStore](GetStore.md), [EnterStore](EnterStore.md), [LeaveStore](LeaveStore.md), [GameGetSelectedPCSingle](GameGetSelectedPCSingle.md)\n\
");

static PyObject* GemRB_StealFailed(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	const Store* store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No store loaded!");
	}
	GET_MAP();

	Actor* owner = map->GetActorByGlobalID(store->GetOwnerID());
	if (!owner) owner = game->GetActorByGlobalID(store->GetOwnerID());
	if (!owner) {
		Log(WARNING, "GUIScript", "No owner found!");
		Py_RETURN_NONE;
	}
	const Actor* attacker = game->FindPC(game->GetSelectedPCSingle());
	if (!attacker) {
		Log(WARNING, "GUIScript", "No thief found!");
		Py_RETURN_NONE;
	}

	// apply the reputation penalty
	int repmod = gamedata->GetReputationMod(2);
	if (repmod) {
		game->SetReputation(game->Reputation + repmod);
	}

	//not sure if this is ok
	//owner->LastDisarmFailed = attacker->GetGlobalID();
	if (core->HasFeature(GFFlags::STEAL_IS_ATTACK)) {
		owner->AttackedBy(attacker);
	}
	owner->AddTrigger(TriggerEntry(trigger_stealfailed, attacker->GetGlobalID()));
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_DisplayString__doc,
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
**See also:** [Control_SetText](Control_SetText.md)\n\
");

static PyObject* GemRB_DisplayString(PyObject* /*self*/, PyObject* args)
{
	ieStrRef strref = ieStrRef::INVALID;
	PyObject* pycol;
	int globalID = 0;
	PARSE_ARGS(args, "iO|i", &strref, &pycol, &globalID);
	if (globalID) {
		GET_GAME();
		GET_ACTOR_GLOBAL();

		displaymsg->DisplayStringName(strref, ColorFromPy(pycol), actor, STRING_FLAGS::SOUND);
	} else {
		displaymsg->DisplayString(strref, ColorFromPy(pycol), STRING_FLAGS::SOUND);
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetCombatDetails__doc,
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
'APR', 'CriticalMultiplier', 'CriticalRange', 'ProfDmgBon', 'HitHeaderNumDice', \n\
'HitHeaderDiceSides', 'HitHeaderDiceBonus', 'LauncherDmgBon', 'WeaponStrBonus', \n\
'AC' (dict), 'ToHitStats' (dict), 'DamageOpcodes' (dict)\n\
\n\
**See also:** [IsDualWielding](IsDualWielding.md)");

static PyObject* GemRB_GetCombatDetails(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int leftorright;
	PARSE_ARGS(args, "ii", &globalID, &leftorright);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	leftorright = leftorright & 1;
	const WeaponInfo& wi = actor->weaponInfo[leftorright && actor->IsDualWielding()];
	const ITMExtHeader* hittingheader = wi.extHeader; // same header, except for ranged weapons it is the ammo header
	int tohit = 20;
	int DamageBonus = 0;
	int CriticalBonus = 0;
	int speed = 0;
	int style = 0;

	PyObject* dict = PyDict_New();
	if (!actor->GetCombatDetails(tohit, leftorright, DamageBonus, speed, CriticalBonus, style, nullptr)) {
		return RuntimeError("Serious problem in GetCombatDetails: could not find the hitting header!");
	}
	PyDict_SetItemString(dict, "Slot", PyLong_FromLong(wi.slot));
	PyDict_SetItemString(dict, "Flags", PyLong_FromLong(wi.wflags));
	PyDict_SetItemString(dict, "Enchantment", PyLong_FromLong(wi.enchantment));
	PyDict_SetItemString(dict, "Range", PyLong_FromLong(wi.range));
	PyDict_SetItemString(dict, "Proficiency", PyLong_FromLong(wi.prof));
	PyDict_SetItemString(dict, "DamageBonus", PyLong_FromLong(DamageBonus));
	PyDict_SetItemString(dict, "Speed", PyLong_FromLong(speed));
	PyDict_SetItemString(dict, "CriticalBonus", PyLong_FromLong(CriticalBonus));
	PyDict_SetItemString(dict, "Style", PyLong_FromLong(style));
	PyDict_SetItemString(dict, "APR", PyLong_FromLong(actor->GetNumberOfAttacks()));
	PyDict_SetItemString(dict, "CriticalMultiplier", PyLong_FromLong(wi.critmulti));
	PyDict_SetItemString(dict, "CriticalRange", PyLong_FromLong(wi.critrange));
	PyDict_SetItemString(dict, "ProfDmgBon", PyLong_FromLong(wi.profdmgbon));
	PyDict_SetItemString(dict, "LauncherDmgBon", PyLong_FromLong(wi.launcherDmgBonus));
	PyDict_SetItemString(dict, "WeaponStrBonus", PyLong_FromLong(actor->WeaponDamageBonus(wi)));
	PyDict_SetItemString(dict, "HitHeaderNumDice", PyLong_FromLong(hittingheader->DiceThrown));
	PyDict_SetItemString(dict, "HitHeaderDiceSides", PyLong_FromLong(hittingheader->DiceSides));
	PyDict_SetItemString(dict, "HitHeaderDiceBonus", PyLong_FromLong(hittingheader->DamageBonus));

	PyObject* ac = PyDict_New();
	PyDict_SetItemString(ac, "Total", PyLong_FromLong(actor->AC.GetTotal()));
	PyDict_SetItemString(ac, "Natural", PyLong_FromLong(actor->AC.GetNatural()));
	PyDict_SetItemString(ac, "Armor", PyLong_FromLong(actor->AC.GetArmorBonus()));
	PyDict_SetItemString(ac, "Shield", PyLong_FromLong(actor->AC.GetShieldBonus()));
	PyDict_SetItemString(ac, "Deflection", PyLong_FromLong(actor->AC.GetDeflectionBonus()));
	PyDict_SetItemString(ac, "Generic", PyLong_FromLong(actor->AC.GetGenericBonus()));
	PyDict_SetItemString(ac, "Dexterity", PyLong_FromLong(actor->AC.GetDexterityBonus()));
	PyDict_SetItemString(ac, "Wisdom", PyLong_FromLong(actor->AC.GetWisdomBonus()));
	PyDict_SetItemString(dict, "AC", ac);

	PyObject* tohits = PyDict_New();
	PyDict_SetItemString(tohits, "Total", PyLong_FromLong(actor->ToHit.GetTotal()));
	PyDict_SetItemString(tohits, "Base", PyLong_FromLong(actor->ToHit.GetBase()));
	PyDict_SetItemString(tohits, "Armor", PyLong_FromLong(actor->ToHit.GetArmorBonus()));
	PyDict_SetItemString(tohits, "Shield", PyLong_FromLong(actor->ToHit.GetShieldBonus()));
	PyDict_SetItemString(tohits, "Proficiency", PyLong_FromLong(actor->ToHit.GetProficiencyBonus()));
	PyDict_SetItemString(tohits, "Generic", PyLong_FromLong(actor->ToHit.GetGenericBonus() + actor->ToHit.GetFxBonus()));
	PyDict_SetItemString(tohits, "Ability", PyLong_FromLong(actor->ToHit.GetAbilityBonus()));
	PyDict_SetItemString(tohits, "Weapon", PyLong_FromLong(actor->ToHit.GetWeaponBonus()));
	PyDict_SetItemString(dict, "ToHitStats", tohits);

	const Item* item = wi.item;
	if (!item) {
		Log(WARNING, "Actor", "{} has a missing or invalid weapon item equipped!", fmt::WideToChar { actor->GetName() });
		return dict;
	}

	// create a tuple with all the 100% probable damage opcodes' stats
	std::vector<DMGOpcodeInfo> damage_opcodes = item->GetDamageOpcodesDetails(hittingheader);
	PyObject* alldos = PyTuple_New(damage_opcodes.size());
	for (unsigned int i = 0; i < damage_opcodes.size(); i++) {
		PyObject* dos = PyDict_New();
		PyDict_SetItemString(dos, "TypeName", PyString_FromStringObj(damage_opcodes[i].TypeName));
		PyDict_SetItemString(dos, "NumDice", PyLong_FromLong(damage_opcodes[i].DiceThrown));
		PyDict_SetItemString(dos, "DiceSides", PyLong_FromLong(damage_opcodes[i].DiceSides));
		PyDict_SetItemString(dos, "DiceBonus", PyLong_FromLong(damage_opcodes[i].DiceBonus));
		PyDict_SetItemString(dos, "Chance", PyLong_FromLong(damage_opcodes[i].Chance));
		PyTuple_SetItem(alldos, i, dos);
	}
	PyDict_SetItemString(dict, "DamageOpcodes", alldos);

	return dict;
}

PyDoc_STRVAR(GemRB_GetDamageReduction__doc,
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
**See also:** [GetCombatDetails](GetCombatDetails.md)\n\
");
static PyObject* GemRB_GetDamageReduction(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	unsigned int enchantment = 0;
	int missile = 0;
	PARSE_ARGS(args, "ii|i", &globalID, &enchantment, &missile);
	GET_GAME();
	GET_ACTOR_GLOBAL();

	int total = 0;
	if (missile) {
		total = actor->GetDamageReduction(IE_RESISTMISSILE, enchantment);
	} else {
		total = actor->GetDamageReduction(IE_RESISTCRUSHING, enchantment);
	}

	return PyLong_FromLong(total);
}

PyDoc_STRVAR(GemRB_GetSpellFailure__doc,
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
");
static PyObject* GemRB_GetSpellFailure(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	int cleric = 0;
	PARSE_ARGS(args, "i|i", &globalID, &cleric);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	PyObject* failure = PyDict_New();
	// true means arcane, so reverse the passed argument
	PyDict_SetItemString(failure, "Total", PyLong_FromLong(actor->GetSpellFailure(!cleric)));
	// set also the shield and armor penalty - we can't reuse the ones for to-hit boni, since they also considered armor proficiency
	int am = 0, sm = 0;
	actor->GetArmorFailure(am, sm);
	PyDict_SetItemString(failure, "Armor", PyLong_FromLong(am));
	PyDict_SetItemString(failure, "Shield", PyLong_FromLong(sm));

	return failure;
}

PyDoc_STRVAR(GemRB_IsDualWielding__doc,
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
**See also:** [GetCombatDetails](GetCombatDetails.md)\n\
");

static PyObject* GemRB_IsDualWielding(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	PARSE_ARGS(args, "i", &globalID);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	int dualwield = actor->IsDualWielding();
	return PyLong_FromLong(dualwield);
}

PyDoc_STRVAR(GemRB_GetSelectedSize__doc,
	     "===== GetSelectedSize =====\n\
\n\
**Prototype:** GemRB.GetSelectedSize ()\n\
\n\
**Description:** Returns the number of actors selected in the party.\n\
\n\
**Return value:** int\n\
\n\
**See also:** [GetSelectedActors](GetSelectedActors.md), [GetSelectedPCSingle](GetSelectedPCSingle.md)");

static PyObject* GemRB_GetSelectedSize(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	return PyLong_FromLong(game->selected.size());
}

PyDoc_STRVAR(GemRB_GetSelectedActors__doc,
	     "===== GetSelectedActors =====\n\
\n\
**Prototype:** GemRB.GetSelectedActors ()\n\
\n\
**Description:** Returns the global ids of selected actors in a tuple.\n\
\n\
**Return value:** tuple of ints\n\
\n\
**See also:** [GetSelectedSize](GetSelectedSize.md), [GetSelectedPCSingle](GetSelectedPCSingle.md)");

static PyObject* GemRB_GetSelectedActors(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	size_t count = game->selected.size();
	PyObject* actor_list = PyTuple_New(count);
	for (size_t i = 0; i < count; i++) {
		PyTuple_SetItem(actor_list, i, PyLong_FromLong(game->selected[i]->GetGlobalID()));
	}
	return actor_list;
}

PyDoc_STRVAR(GemRB_GetSpellCastOn__doc,
	     "===== GetSpellCastOn =====\n\
\n\
**Prototype:** GemRB.GetSpellCastOn (pc)\n\
\n\
**Description:** Returns the last spell cast on a party member.\n\
\n\
**Parameters:**\n\
  * pc - PartyID\n\
\n\
**Return value:** resref");

static PyObject* GemRB_GetSpellCastOn(PyObject* /*self*/, PyObject* args)
{
	int globalID;
	ResRef splName;
	PARSE_ARGS(args, "i", &globalID);

	GET_GAME();
	GET_ACTOR_GLOBAL();

	ResolveSpellName(splName, actor->objects.LastSpellOnMe);
	return PyString_FromResRef(splName);
}

PyDoc_STRVAR(GemRB_SetTimer__doc,
	     "===== SetTimer =====\n\
\n\
**Prototype:** GemRB.SetTimer (callback, interval[, repeats])\n\
\n\
**Description:** Set callback to be called repeatedly at given interval. \n\
This is useful for things like running a twisted reactor.\n\
\n\
**Parameters:**\n\
  * callback - python function to run\n\
  * interval - time interval in ticks\n\
  * repeats - the number of times to repeat the action before expiring the timer\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_SetTimer(PyObject* /*self*/, PyObject* args)
{
	Timer::TimeInterval interval = 0;
	int repeats = -1;
	PyObject* function = NULL;

	PARSE_ARGS(args, "Oi|i", &function, &interval, &repeats);

	if (PyCallable_Check(function)) {
		EventHandler handler = PythonCallback(function);
		core->SetTimer(handler, interval, repeats);
		Py_RETURN_NONE;
	} else {
		return RuntimeError(fmt::format("Can't set timed event handler {}!", PyEval_GetFuncName(function)));
	}
}

PyDoc_STRVAR(GemRB_SetupMaze__doc,
	     "===== SetupMaze =====\n\
\n\
**Prototype:** GemRB.SetupMaze (x, y)\n\
\n\
**Description:** Initializes a maze of size XxY. The dimensions shouldn't \n\
exceed the maximum possible maze size (8x8).\n\
\n\
**Parameters:** \n\
  * x, y - dimensions\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_SetupMaze(PyObject* /*self*/, PyObject* args)
{
	int xsize, ysize;
	PARSE_ARGS(args, "ii", &xsize, &ysize);

	if ((unsigned) xsize > MAZE_MAX_DIM || (unsigned) ysize > MAZE_MAX_DIM) {
		return NULL;
	}

	GET_GAME();

	maze_header* h = reinterpret_cast<maze_header*>(game->AllocateMazeData() + MAZE_ENTRY_COUNT * MAZE_ENTRY_SIZE);
	memset(h, 0, MAZE_HEADER_SIZE);
	h->maze_sizex = xsize;
	h->maze_sizey = ysize;
	for (int i = 0; i < MAZE_ENTRY_COUNT; i++) {
		maze_entry* m = reinterpret_cast<maze_entry*>(game->mazedata + i * MAZE_ENTRY_SIZE);
		memset(m, 0, MAZE_ENTRY_SIZE);
		bool used = (i / MAZE_MAX_DIM < ysize) && (i % MAZE_MAX_DIM < xsize);
		m->valid = used;
		m->accessible = used;
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetMazeEntry__doc,
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
");

static PyObject* GemRB_SetMazeEntry(PyObject* /*self*/, PyObject* args)
{
	int entry, index, value;
	PARSE_ARGS(args, "iii", &entry, &index, &value);

	if (entry < 0 || entry > 63) {
		return NULL;
	}

	GET_GAME();

	if (!game->mazedata) {
		return RuntimeError("No maze set up!");
	}

	maze_entry* m = reinterpret_cast<maze_entry*>(game->mazedata + entry * MAZE_ENTRY_SIZE);
	maze_entry* m2;
	switch (index) {
		case ME_OVERRIDE:
			m->me_override = value;
			break;
		default:
		case ME_VALID:
		case ME_ACCESSIBLE:
			return NULL;
		case ME_TRAP: //trapped/traptype
			if (value == -1) {
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
				if (entry % MAZE_MAX_DIM != MAZE_MAX_DIM - 1) {
					m2 = reinterpret_cast<maze_entry*>(game->mazedata + (entry + 1) * MAZE_ENTRY_SIZE);
					m2->walls |= WALL_NORTH;
				}
			}

			if (value & WALL_NORTH) {
				if (entry % MAZE_MAX_DIM) {
					m2 = reinterpret_cast<maze_entry*>(game->mazedata + (entry - 1) * MAZE_ENTRY_SIZE);
					m2->walls |= WALL_SOUTH;
				}
			}

			if (value & WALL_EAST) {
				if (entry + MAZE_MAX_DIM < MAZE_ENTRY_COUNT) {
					m2 = reinterpret_cast<maze_entry*>(game->mazedata + (entry + MAZE_MAX_DIM) * MAZE_ENTRY_SIZE);
					m2->walls |= WALL_WEST;
				}
			}

			if (value & WALL_WEST) {
				if (entry >= MAZE_MAX_DIM) {
					m2 = reinterpret_cast<maze_entry*>(game->mazedata + (entry - MAZE_MAX_DIM) * MAZE_ENTRY_SIZE);
					m2->walls |= WALL_EAST;
				}
			}

			break;
		case ME_VISITED:
			m->visited = value;
			break;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_SetMazeData__doc,
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
**Return value:** N/A");

static PyObject* GemRB_SetMazeData(PyObject* /*self*/, PyObject* args)
{
	int entry;
	int value;
	PARSE_ARGS(args, "ii", &entry, &value);

	GET_GAME();

	if (!game->mazedata) {
		return RuntimeError("No maze set up!");
	}

	maze_header* h = reinterpret_cast<maze_header*>(game->mazedata + MAZE_ENTRY_COUNT * MAZE_ENTRY_SIZE);
	switch (entry) {
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
			return NULL;
	}

	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetMazeHeader__doc,
	     "===== GetMazeHeader =====\n\
\n\
**Prototype:** GemRB.GetMazeHeader ()\n\
\n\
**Description:** Returns the Maze header of Planescape Torment savegames.\n\
\n\
**Return value:** dict");

static PyObject* GemRB_GetMazeHeader(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();

	if (!game->mazedata) {
		Py_RETURN_NONE;
	}

	PyObject* dict = PyDict_New();
	const maze_header* h = reinterpret_cast<maze_header*>(game->mazedata + MAZE_ENTRY_COUNT * MAZE_ENTRY_SIZE);
	PyDict_SetItemString(dict, "MazeX", PyLong_FromLong(h->maze_sizex));
	PyDict_SetItemString(dict, "MazeY", PyLong_FromLong(h->maze_sizey));
	PyDict_SetItemString(dict, "Pos1X", PyLong_FromLong(h->pos1x));
	PyDict_SetItemString(dict, "Pos1Y", PyLong_FromLong(h->pos1y));
	PyDict_SetItemString(dict, "Pos2X", PyLong_FromLong(h->pos2x));
	PyDict_SetItemString(dict, "Pos2Y", PyLong_FromLong(h->pos2y));
	PyDict_SetItemString(dict, "Pos3X", PyLong_FromLong(h->pos3x));
	PyDict_SetItemString(dict, "Pos3Y", PyLong_FromLong(h->pos3y));
	PyDict_SetItemString(dict, "Pos4X", PyLong_FromLong(h->pos4x));
	PyDict_SetItemString(dict, "Pos4Y", PyLong_FromLong(h->pos4y));
	PyDict_SetItemString(dict, "TrapCount", PyLong_FromLong(h->trapcount));
	PyDict_SetItemString(dict, "Inited", PyLong_FromLong(h->initialized));
	return dict;
}

PyDoc_STRVAR(GemRB_GetMazeEntry__doc,
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
");

static PyObject* GemRB_GetMazeEntry(PyObject* /*self*/, PyObject* args)
{
	int entry;
	PARSE_ARGS(args, "i", &entry);

	if (entry < 0 || entry >= MAZE_ENTRY_COUNT) {
		return NULL;
	}

	GET_GAME();

	if (!game->mazedata) {
		return RuntimeError("No maze set up!");
	}

	PyObject* dict = PyDict_New();
	const maze_entry* m = reinterpret_cast<maze_entry*>(game->mazedata + entry * MAZE_ENTRY_SIZE);
	PyDict_SetItemString(dict, "Override", PyLong_FromLong(m->me_override));
	PyDict_SetItemString(dict, "Accessible", PyLong_FromLong(m->accessible));
	PyDict_SetItemString(dict, "Valid", PyLong_FromLong(m->valid));
	if (m->trapped) {
		PyDict_SetItemString(dict, "Trapped", PyLong_FromLong(m->traptype));
	} else {
		PyDict_SetItemString(dict, "Trapped", PyLong_FromLong(-1));
	}
	PyDict_SetItemString(dict, "Walls", PyLong_FromLong(m->walls));
	PyDict_SetItemString(dict, "Visited", PyLong_FromLong(m->visited));
	return dict;
}

std::string gameTypeHint;
int gameTypeHintWeight = 0;

PyDoc_STRVAR(GemRB_AddGameTypeHint__doc,
	     "===== AddGameTypeHint =====\n\
\n\
**Prototype:** GemRB.AddGameTypeHint (type, weight[, flags=0])\n\
\n\
**Description:** Asserts that GameType should be TYPE, with confidence WEIGHT. \n\
This is used by Autodetect.py scripts when GameType was set to 'auto'.\n\
\n\
**Parameters:**\n\
  * type - GameType (e.g. bg1, bg2, iwd, how, iwd2, pst and others)\n\
  * weight - numeric, confidence that TYPE is correct. Standard games should use values <= 100, (eventual) new games based on the standard ones should use values above 100.\n\
  * flags - numeric, not used now\n\
\n\
**Return value:** N/A");

static PyObject* GemRB_AddGameTypeHint(PyObject* /*self*/, PyObject* args)
{
	char* type;
	int weight;
	int flags = 0;
	PARSE_ARGS(args, "si|i", &type, &weight, &flags);

	if (weight > gameTypeHintWeight) {
		gameTypeHintWeight = weight;
		gameTypeHint = type;
	}

	Py_RETURN_NONE;
}


PyDoc_STRVAR(GemRB_GetAreaInfo__doc,
	     "===== GetAreaInfo =====\n\
\n\
**Prototype:** GemRB.GetAreaInfo ()\n\
\n\
**Description:** Returns important values about the current area.\n\
\n\
**Parameters:** N/A\n\
\n\
**Return value:** dict with 'CurrentArea', 'PositionX', 'PositionY'");

static PyObject* GemRB_GetAreaInfo(PyObject* /*self*/, PyObject* /*args*/)
{
	GET_GAME();
	GET_GAMECONTROL();

	PyObject* info = PyDict_New();
	PyDict_SetItemString(info, "CurrentArea", PyString_FromResRef(game->CurrentArea));
	Point mouse = gc->GameMousePos();
	PyDict_SetItemString(info, "PositionX", PyLong_FromLong(mouse.x));
	PyDict_SetItemString(info, "PositionY", PyLong_FromLong(mouse.y));

	return info;
}

PyDoc_STRVAR(GemRB_Log__doc,
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
**Return value:** N/A");

static PyObject* GemRB_Log(PyObject* /*self*/, PyObject* args)
{
	LogLevel level;
	char* owner;
	char* message;

	if (!PyArg_ParseTuple(args, "bss", &level, &owner, &message)) {
		return NULL;
	}

	Log(level, owner, "{}", message);
	Py_RETURN_NONE;
}


PyDoc_STRVAR(GemRB_SetFeature__doc,
	     "===== SetFeature =====\n\
\n\
**Prototype:** GemRB.SetFeature (feature, value)\n\
\n\
**Description:** Set GameType flag FEATURE to VALUE, either True or False.\n\
\n\
**Parameters:**\n\
  * FEATURE - GFFlags::xxx constant defined in GUIDefines.py and globals.h\n\
  * VALUE - value to set the feature to. Either True or False\n\
\n\
**Return value:** N/A\n\
\n\
**Examples:**\n\
\n\
    GemRB.SetFeature(GFFlags::ALL_STRINGS_TAGGED, True)\n\
\n\
**See also:** [SetVar](SetVar.md)");

static PyObject* GemRB_SetFeature(PyObject* /*self*/, PyObject* args)
{
	unsigned int feature;
	bool set;

	if (!PyArg_ParseTuple(args, "ib", &feature, &set)) {
		return NULL;
	}

	if (set) {
		core->SetFeature(EnumIndex<GFFlags>(feature));
	} else {
		core->ClearFeature(EnumIndex<GFFlags>(feature));
	}
	Py_RETURN_NONE;
}

PyDoc_STRVAR(GemRB_GetMultiClassPenalty__doc,
	     "===== GetMultiClassPenalty =====\n\
\n\
**Prototype:** GemRB.GetMultiClassPenalty (globalID)\n\
\n\
**Description:** Returns the experience penalty from unsynced classes.\n\
\n\
**Parameters:**\n\
  * globalID - party ID or global ID of the actor to use\n\
\n\
**Return value:** integer");

static PyObject* GemRB_GetMultiClassPenalty(PyObject* /*self*/, PyObject* args)
{
	int globalID;

	if (!PyArg_ParseTuple(args, "i", &globalID)) {
		return AttributeError(GemRB_GetMultiClassPenalty__doc);
	}
	GET_GAME();
	GET_ACTOR_GLOBAL();

	return PyLong_FromLong(actor->GetFavoredPenalties());
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
	METHOD(CreateView, METH_VARARGS),
	METHOD(RemoveScriptingRef, METH_VARARGS),
	METHOD(RemoveView, METH_VARARGS),
	METHOD(DeleteSaveGame, METH_VARARGS),
	METHOD(DispelEffect, METH_VARARGS),
	METHOD(DisplayString, METH_VARARGS),
	METHOD(DragItem, METH_VARARGS),
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
	METHOD(GameControlLocateActor, METH_VARARGS),
	METHOD(GameControlToggleAlwaysRun, METH_NOARGS),
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
	METHOD(GameSwapPCs, METH_VARARGS),
	METHOD(GetAreaInfo, METH_NOARGS),
	METHOD(GetAvatarsValue, METH_VARARGS),
	METHOD(GetAbilityBonus, METH_VARARGS),
	METHOD(GetCombatDetails, METH_VARARGS),
	METHOD(GetContainer, METH_VARARGS),
	METHOD(GetContainerItem, METH_VARARGS),
	METHOD(GetCurrentArea, METH_NOARGS),
	METHOD(GetDamageReduction, METH_VARARGS),
	METHOD(GetEffects, METH_VARARGS),
	METHOD(GetEquippedAmmunition, METH_VARARGS),
	METHOD(GetEquippedQuickSlot, METH_VARARGS),
	METHOD(GetGamePreview, METH_VARARGS),
	METHOD(GetGameString, METH_VARARGS),
	METHOD(GetGameTime, METH_NOARGS),
	METHOD(GetGameVar, METH_VARARGS),
	METHOD(GetGUIFlags, METH_VARARGS),
	METHOD(GetInventoryInfo, METH_VARARGS),
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
	METHOD(ConsoleWindowLog, METH_VARARGS),
	METHOD(GetModalState, METH_VARARGS),
	METHOD(GetPartySize, METH_NOARGS),
	METHOD(GetPCStats, METH_VARARGS),
	METHOD(GetPlayerActionRow, METH_VARARGS),
	METHOD(GetPlayerName, METH_VARARGS),
	METHOD(GetPlayerLevel, METH_VARARGS),
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
	METHOD(GetSprite, METH_VARARGS),
	METHOD(GetSlotType, METH_VARARGS),
	METHOD(GetStore, METH_VARARGS),
	METHOD(GetStoreDrink, METH_VARARGS),
	METHOD(GetStoreCure, METH_VARARGS),
	METHOD(GetStoreItem, METH_VARARGS),
	METHOD(GetSpell, METH_VARARGS),
	METHOD(GetSpelldata, METH_VARARGS),
	METHOD(GetSpelldataIndex, METH_VARARGS),
	METHOD(GetSprite, METH_VARARGS),
	METHOD(GetSlotItem, METH_VARARGS),
	METHOD(GetSlots, METH_VARARGS),
	METHOD(GetSystemVariable, METH_VARARGS),
	METHOD(GetToken, METH_VARARGS),
	METHOD(GetVar, METH_VARARGS),
	METHOD(GetView, METH_VARARGS),
	METHOD(HardEndPL, METH_NOARGS),
	METHOD(HasFeat, METH_VARARGS),
	METHOD(HasResource, METH_VARARGS),
	METHOD(HasSpecialItem, METH_VARARGS),
	METHOD(HasSpecialSpell, METH_VARARGS),
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
	METHOD(LoadWindow, METH_VARARGS),
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
	METHOD(SetTimer, METH_VARARGS),
	METHOD(SetTimedEvent, METH_VARARGS),
	METHOD(SetToken, METH_VARARGS),
	METHOD(SetTooltipDelay, METH_VARARGS),
	METHOD(SetupMaze, METH_VARARGS),
	METHOD(SetupQuickSlot, METH_VARARGS),
	METHOD(SetupQuickSpell, METH_VARARGS),
	METHOD(SetVar, METH_VARARGS),
	METHOD(SoftEndPL, METH_NOARGS),
	METHOD(SpellCast, METH_VARARGS),
	METHOD(StealFailed, METH_NOARGS),
	METHOD(UnmemorizeSpell, METH_VARARGS),
	METHOD(UpdateVolume, METH_VARARGS),
	METHOD(UpdateWorldMap, METH_VARARGS),
	METHOD(UseItem, METH_VARARGS),
	METHOD(ValidTarget, METH_VARARGS),
	METHOD(VerbalConstant, METH_VARARGS),
	// terminating entry
	{ NULL, NULL, 0, NULL }
};

static PyMethodDef GemRBInternalMethods[] = {
	METHOD(Button_EnableBorder, METH_VARARGS),
	METHOD(Button_SetActionIcon, METH_VARARGS),
	METHOD(Button_SetBorder, METH_VARARGS),
	METHOD(Button_SetHotKey, METH_VARARGS),
	METHOD(Button_SetAnchor, METH_VARARGS),
	METHOD(Button_SetAnimation, METH_VARARGS),
	METHOD(Button_SetPushOffset, METH_VARARGS),
	METHOD(Button_SetItemIcon, METH_VARARGS),
	METHOD(Button_SetOverlay, METH_VARARGS),
	METHOD(Button_SetPLT, METH_VARARGS),
	METHOD(Button_SetPicture, METH_VARARGS),
	METHOD(Button_SetPictureClipping, METH_VARARGS),
	METHOD(Button_SetSpellIcon, METH_VARARGS),
	METHOD(Button_SetSprites, METH_VARARGS),
	METHOD(Button_SetState, METH_VARARGS),
	METHOD(Control_QueryText, METH_VARARGS),
	METHOD(Control_SetAction, METH_VARARGS),
	METHOD(Control_SetActionInterval, METH_VARARGS),
	METHOD(Control_SetColor, METH_VARARGS),
	METHOD(Control_SetFont, METH_VARARGS),
	METHOD(Control_SetStatus, METH_VARARGS),
	METHOD(Control_SetText, METH_VARARGS),
	METHOD(Control_SetValue, METH_VARARGS),
	METHOD(Control_SetVarAssoc, METH_VARARGS),
	METHOD(SaveGame_GetDate, METH_VARARGS),
	METHOD(SaveGame_GetGameDate, METH_VARARGS),
	METHOD(SaveGame_GetName, METH_VARARGS),
	METHOD(SaveGame_GetPortrait, METH_VARARGS),
	METHOD(SaveGame_GetPreview, METH_VARARGS),
	METHOD(SaveGame_GetSaveID, METH_VARARGS),
	METHOD(Scrollable_Scroll, METH_VARARGS),
	METHOD(Symbol_GetValue, METH_VARARGS),
	METHOD(Table_FindValue, METH_VARARGS),
	METHOD(Table_GetColumnCount, METH_VARARGS),
	METHOD(Table_GetColumnIndex, METH_VARARGS),
	METHOD(Table_GetColumnName, METH_VARARGS),
	METHOD(Table_GetRowCount, METH_VARARGS),
	METHOD(Table_GetRowIndex, METH_VARARGS),
	METHOD(Table_GetRowName, METH_VARARGS),
	METHOD(Table_GetValue, METH_VARARGS),
	METHOD(TextArea_Append, METH_VARARGS),
	METHOD(TextArea_ListResources, METH_VARARGS),
	METHOD(TextArea_SetOptions, METH_VARARGS),
	METHOD(TextArea_SetChapterText, METH_VARARGS),
	METHOD(TextEdit_SetBufferLength, METH_VARARGS),
	METHOD(View_AddAlias, METH_VARARGS),
	METHOD(View_AddSubview, METH_VARARGS),
	METHOD(View_GetFrame, METH_VARARGS),
	METHOD(View_SetBackground, METH_VARARGS),
	METHOD(View_SetEventProxy, METH_VARARGS),
	METHOD(View_SetFrame, METH_VARARGS),
	METHOD(View_SetFlags, METH_VARARGS),
	METHOD(View_SetResizeFlags, METH_VARARGS),
	METHOD(View_SetTooltip, METH_VARARGS),
	METHOD(View_Focus, METH_VARARGS),
	METHOD(Window_Focus, METH_VARARGS),
	METHOD(Window_SetAction, METH_VARARGS),
	METHOD(Window_SetupEquipmentIcons, METH_VARARGS),
	METHOD(Window_ShowModal, METH_VARARGS),
	METHOD(WorldMap_GetDestinationArea, METH_VARARGS),
	// terminating entry
	{ NULL, NULL, 0, NULL }
};

GUIScript::GUIScript(void)
{
	gs = this;
}

GUIScript::~GUIScript(void)
{
	if (Py_IsInitialized()) {
		if (pModule) {
			Py_DECREF(pModule);
		}
		Py_Finalize();
	}

	GUIAction[0] = UNINIT_IEDWORD;

	// free the memory from the global scrollbar template
	auto view = ScriptingRefCast<View>(ScriptEngine::GetScriptingRef("SBGLOB", 0));
	delete view;
}

PyDoc_STRVAR(GemRB__doc,
	     "Module exposing GemRB data and engine internals\n\n"
	     "This module exposes to python GUIScripts GemRB engine data and internals. "
	     "It's implemented in gemrb/plugins/GUIScript/GUIScript.cpp");

PyDoc_STRVAR(GemRB_internal__doc,
	     "Internal module for GemRB metaclasses.\n\n"
	     "This module is only for implementing GUIClass.py."
	     "It's implemented in gemrb/plugins/GUIScript/GUIScript.cpp");

/** Initialization Routine */

PyMODINIT_FUNC PyInit__GemRB();
PyMODINIT_FUNC PyInit_GemRB();

PyMODINIT_FUNC
	PyInit__GemRB()
{
	static PyModuleDef moddef = {
		PyModuleDef_HEAD_INIT,
		"_GemRB", /* m_name */
		GemRB_internal__doc, /* m_doc */
		-1, /* m_size */
		GemRBInternalMethods, /* m_methods */
		NULL, /* m_reload */
		NULL, /* m_traverse */
		NULL, /* m_clear */
		NULL, /* m_free */
	};
	return PyModule_Create(&moddef);
}

PyMODINIT_FUNC
	PyInit_GemRB()
{
	static PyModuleDef moddef = {
		PyModuleDef_HEAD_INIT,
		"GemRB", /* m_name */
		GemRB__doc, /* m_doc */
		-1, /* m_size */
		GemRBMethods, /* m_methods */
		NULL, /* m_reload */
		NULL, /* m_traverse */
		NULL, /* m_clear */
		NULL, /* m_free */
	};
	return PyModule_Create(&moddef);
}

bool GUIScript::Init(void)
{
	// Add built-in modules, before Py_Initialize
	if (PyImport_AppendInittab("GemRB", PyInit_GemRB) == -1) {
		return false;
	}
	if (PyImport_AppendInittab("_GemRB", PyInit__GemRB) == -1) {
		return false;
	}

	Py_Initialize();
	if (!Py_IsInitialized()) {
		return false;
	}

	PyObject* pGemRB = PyImport_ImportModule("GemRB");
	PyObject* pMainMod = PyImport_AddModule("__main__");
	/* pMainMod is a borrowed reference */
	pMainDic = PyModule_GetDict(pMainMod);
	/* pMainDic is a borrowed reference */

	path_t path = PathJoin(core->config.GUIScriptsPath, "GUIScripts");

	char string[256] = "path";
	PyObject* sysPath = PySys_GetObject(string);
	if (!sysPath) {
		Log(ERROR, "GUIScripts", "Unable to set 'sys.path'.");
		return false;
	}

	// Add generic script path early, so GameType detection works
	PyList_Append(sysPath, PyString_FromStringObj(path));

	PyModule_AddStringConstant(pGemRB, "GEMRB_VERSION", GEMRB_STRING);

	path_t main = PathJoin(path, "Main.py");
	if (!ExecFile(main.c_str())) {
		Log(ERROR, "GUIScript", "Failed to execute {}", main);
		return false;
	}

	snprintf(string, 255, "GemRB.Version = '%s'", VERSION_GEMRB);
	PyRun_SimpleString(string);

	// Detect GameType if it was set to auto
	if (core->config.GameType == "auto") {
		Autodetect();
	}

	// use the iwd guiscripts for how, but leave its override
	// same for bg2 vs bg2ee and bg1ee, while iwdee might need a different approach
	path_t path2;
	if (core->config.GameType == "how") {
		path2 = PathJoin(path, "iwd");
	} else if (core->config.GameType == "bg2ee") {
		path2 = PathJoin(path, "bg2");
	} else {
		path2 = PathJoin(path, core->config.GameType);
	}

	// GameType-specific import path must have a higher priority than
	// the generic one, so insert it before it
	PyList_Insert(sysPath, -1, PyString_FromStringObj(path2));
	PyModule_AddStringConstant(pGemRB, "GameType", core->config.GameType.c_str());

	PyObject* pClassesMod = PyImport_AddModule("GUIClasses");
	/* pClassesMod is a borrowed reference */
	pGUIClasses = PyModule_GetDict(pClassesMod);
	/* pGUIClasses is a borrowed reference */

	PyObject* pFunc = PyDict_GetItemString(pMainDic, "Init");
	if (!CallObjectWrapper(pFunc, nullptr)) {
		Log(ERROR, "GUIScript", "Failed to execute Init() in {}", main);
		PyErr_Print();
		return false;
	}

	return true;
}

bool GUIScript::Autodetect(void)
{
	Log(MESSAGE, "GUIScript", "Detecting GameType.");

	DirectoryIterator iter(PathJoin(core->config.GUIScriptsPath, "GUIScripts"));
	if (!iter)
		return false;

	iter.SetFlags(DirectoryIterator::Directories);
	do {
		const path_t& dirent = iter.GetName();

		// NOTE: these methods subtly differ in sys.path content, need for __init__.py files ...
		// Method1:
		path_t moduleName = PathJoin(core->config.GUIScriptsPath, "GUIScripts", dirent, "Autodetect.py");
		ExecFile(moduleName.c_str());
		// Method2:
		//strcpy( module, dirent );
		//strcat( module, ".Autodetect");
		//LoadScript(module);
	} while (++iter);

	if (!gameTypeHint.empty()) {
		Log(MESSAGE, "GUIScript", "Detected GameType: {}", gameTypeHint);
		core->config.GameType = gameTypeHint;
		return true;
	} else {
		Log(ERROR, "GUIScript", "Failed to detect game type.");
		return false;
	}
}

bool GUIScript::LoadScript(const std::string& filename)
{
	if (!Py_IsInitialized()) {
		return false;
	}
	Log(MESSAGE, "GUIScript", "Loading Script {}.", filename);

	PyObject* pName = PyString_FromStringObj(filename);
	/* Error checking of pName left out */
	if (!pName) {
		Log(ERROR, "GUIScript", "Failed to create filename for script \"{}\".", filename);
		return false;
	}

	if (pModule) {
		Py_DECREF(pModule);
	}

	pModule = PyImport_Import(pName);
	Py_DECREF(pName);

	if (pModule) {
		pDict = PyModule_GetDict(pModule);
		if (PyDict_Merge(pDict, pMainDic, false) == -1)
			return false;
		/* pDict is a borrowed reference */
	} else {
		PyErr_Print();
		Log(ERROR, "GUIScript", "Failed to load script \"{}\".", filename);
		return false;
	}
	return true;
}

static PyObject* ParamToPython(const GUIScript::Parameter& p)
{
	const std::type_info& type = p.Type();

	if (type == typeid(char*)) {
		return PyString_FromString(p.Value<char*>());
	} else if (type == typeid(String&)) {
		return PyString_FromStringObj(p.Value<String>());
	} else if (type == typeid(std::string&)) {
		return PyString_FromStringObj(p.Value<std::string>());
	} else if (type == typeid(long)) {
		return PyLong_FromLong(p.Value<long>());
	} else if (type == typeid(unsigned long)) {
		return PyLong_FromUnsignedLong(p.Value<unsigned long>());
	} else if (type == typeid(std::nullptr_t)) {
		Py_RETURN_NONE;
	} else if (type == typeid(bool)) {
		return PyBool_FromLong(p.Value<bool>());
	} else if (type == typeid(Point)) {
		const Point& point = p.Value<Point>();
		return Py_BuildValue("{s:i,s:i}", "x", point.x, "y", point.y);
	} else if (type == typeid(Region)) {
		const Region& rgn = p.Value<Region>();
		return Py_BuildValue("{s:i,s:i,s:i,s:i}", "x", rgn.x, "y", rgn.y, "w", rgn.w, "h", rgn.h);
	} else if (type == typeid(View*)) {
		const View* view = p.Value<View*>();
		return gs->ConstructObjectForScriptable(view->GetScriptingRef());
	} else if (type == typeid(PyObject*)) {
		return p.Value<PyObject*>();
	} else {
		Log(ERROR, "GUIScript", "Unknown parameter type: %s", type.name());
		// need to insert a None placeholder so remaining parameters are correct
		Py_RETURN_NONE;
	}
}

GUIScript::Parameter GUIScript::RunFunction(const char* Modulename, const char* FunctionName, const FunctionParameters& params, bool report_error)
{
	// convert PyObject to C++ object
	PyObject* pyret = RunPyFunction(Modulename, FunctionName, params, report_error);
	Parameter ret; // failure state

	if (pyret) {
		if (PyBool_Check(pyret)) {
			ret = Parameter(bool(PyObject_IsTrue(pyret)));
		} else if (PyLong_Check(pyret)) {
			ret = Parameter(PyLong_AsLong(pyret));
		} else if (PyUnicode_Check(pyret)) {
			ret = Parameter(PyString_AsStringObj(pyret));
		} else if (pyret == Py_None) {
			// any python function that doesnt return a value returns None
			ret = Parameter(pyret);
		} else {
			Log(ERROR, "GUIScript", "Unhandled return type in {}::{}", Modulename, FunctionName);
			// this is a success, but we dont know how to convert it
			// still needs a value of some kind
			ret = Parameter(pyret);
		}
		Py_DecRef(pyret);
	}

	return ret;
}

/* Similar to RunFunction, but with parameters, and doesn't necessarily fail */
PyObject* GUIScript::RunPyFunction(const char* Modulename, const char* FunctionName, const FunctionParameters& params, bool report_error)
{
	size_t size = params.size();

	if (size) {
		auto pyParams = DecRef(PyTuple_New, size);

		for (size_t i = 0; i < size; ++i) {
			PyObject* pyParam = ParamToPython(params[i]); // a "stolen" reference for PyTuple_SetItem
			Py_INCREF(pyParam); // could depend on Python version, see #2198
			PyTuple_SetItem(pyParams, i, pyParam);
		}
		return RunPyFunction(Modulename, FunctionName, pyParams, report_error);
	} else {
		return RunPyFunction(Modulename, FunctionName, nullptr, report_error);
	}
}

PyObject* GUIScript::RunPyFunction(const char* moduleName, const char* functionName, PyObject* pArgs, bool report_error)
{
	if (!Py_IsInitialized()) {
		return NULL;
	}

	PyObject* pyModule;
	if (moduleName) {
		pyModule = PyImport_ImportModule(moduleName);
	} else {
		pyModule = pModule;
		Py_XINCREF(pyModule);
	}
	if (!pyModule) {
		PyErr_Print();
		return NULL;
	}
	PyObject* dict = PyModule_GetDict(pyModule);

	PyObject* pFunc = PyDict_GetItemString(dict, functionName);

	/* pFunc: Borrowed reference */
	if (!PyCallable_Check(pFunc)) {
		if (report_error) {
			Log(ERROR, "GUIScript", "Missing function: {} from {}", functionName, moduleName);
		}
		Py_DECREF(pyModule);
		return NULL;
	}

	PyObject* pValue = CallObjectWrapper(pFunc, pArgs);
	if (!pValue) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
	}
	Py_DECREF(pyModule);
	return pValue;
}

bool GUIScript::ExecFile(const char* file)
{
	FileStream fs;
	if (!fs.Open(file))
		return false;

	size_t len = fs.Remains();
	if (len <= 0)
		return false;

	std::string buffer(len, '\0');
	if (fs.Read(&buffer[0], len) == GEM_ERROR) {
		return false;
	}

	return ExecString(buffer);
}

/** Exec a single String */
bool GUIScript::ExecString(const std::string& string, bool feedback)
{
	PyObject* run = PyRun_String(string.c_str(), Py_file_input, pMainDic, pMainDic);

	if (run) {
		// success
		if (!feedback) {
			Py_DECREF(run);
			return true;
		}

		PyObject* pyGUI = PyImport_ImportModule("GUICommon");
		if (pyGUI) {
			PyObject* catcher = PyObject_GetAttrString(pyGUI, "outputFunnel");
			if (catcher) {
				PyObject* output = PyObject_GetAttrString(catcher, "lastLine");
				displaymsg->DisplayString(PyString_AsStringObj(output), GUIColors::WHITE, nullptr);
				Py_DECREF(catcher);
			}
			Py_DECREF(pyGUI);
		}

		Py_DECREF(run);
		return true;
	} else {
		// failure
		PyObject *ptype, *pvalue, *ptraceback;
		PyErr_Fetch(&ptype, &pvalue, &ptraceback);

		//Get error message
		String errorString = PyString_AsStringObj(pvalue);
		if (displaymsg) {
			displaymsg->DisplayString(u"Error: " + errorString, GUIColors::RED, nullptr);
		} else {
			Log(ERROR, "GUIScript", "{}", fmt::WideToChar { errorString });
		}

		Py_DECREF(ptype);
		Py_DECREF(pvalue);
		Py_XDECREF(ptraceback);
	}
	PyErr_Clear();
	return false;
}

void GUIScript::AssignViewAttributes(PyObject* obj, View* view) const
{
	static PyObject* controlClass = PyDict_GetItemString(pGUIClasses, "GControl");
	static PyObject* windowClass = PyDict_GetItemString(pGUIClasses, "GWindow");

	PyObject_SetAttrString(obj, "Flags", DecRef(PyLong_FromLong, view->Flags()));
	Window* win = view->GetWindow();
	if (win) {
		PyObject* pywin = ConstructObjectForScriptable(win->GetScriptingRef());
		PyObject_SetAttrString(obj, "Window", pywin);
		Py_DecRef(pywin);
	} else {
		PyObject_SetAttrString(obj, "Window", Py_None);
	}

	if (PyObject_IsInstance(obj, controlClass)) {
		const Control* ctl = static_cast<Control*>(view);
		PyObject_SetAttrString(obj, "ControlID", DecRef(PyLong_FromUnsignedLong, ctl->ControlID));
		PyObject_SetAttrString(obj, "VarName", DecRef(PyString_FromStringView, ctl->DictVariable()));
		Control::value_t val = ctl->GetValue();
		if (val == Control::INVALID_VALUE) {
			PyObject_SetAttrString(obj, "Value", Py_None);
		} else {
			PyObject_SetAttrString(obj, "Value", DecRef(PyLong_FromUnsignedLong, val));
		}
	} else if (PyObject_IsInstance(obj, windowClass)) {
		const Window* win = static_cast<Window*>(view);
		PyObject_SetAttrString(obj, "HasFocus", DecRef(PyBool_FromLong, win->HasFocus()));
	}
}

PyObject* GUIScript::ConstructObjectForScriptable(const ScriptingRefBase* ref) const
{
	if (!ref) return RuntimeError("Cannot construct object with null ref.");

	PyObject* obj = ConstructObject(ref->ScriptingClass(), ref->Id);
	if (!obj) return RuntimeError("Failed to construct object");

	static PyObject* viewClass = PyDict_GetItemString(pGUIClasses, "GView");
	if (PyObject_IsInstance(obj, viewClass)) {
		PyObject_SetAttrString(obj, "SCRIPT_GROUP", DecRef(PyString_FromASCII<ScriptingGroup_t>, ref->ScriptingGroup()));
		View* view = ScriptingRefCast<View>(ref);
		AssignViewAttributes(obj, view);
	}

	return obj;
}

PyObject* GUIScript::ConstructObject(const std::string& pyclassname, ScriptingId id) const
{
	PyObject* kwargs = Py_BuildValue("{s:K}", "ID", id);
	PyObject* ret = gs->ConstructObject(pyclassname, NULL, kwargs);
	Py_DECREF(kwargs);
	return ret;
}

PyObject* GUIScript::ConstructObject(const std::string& pyclassname, PyObject* pArgs, PyObject* kwArgs) const
{
	std::string classname = "G" + pyclassname;
	if (!pGUIClasses) {
		return RuntimeError(fmt::format("Tried to use an object ({}) before script compiled!", classname));
	}

	PyObject* cobj = PyDict_GetItemString(pGUIClasses, classname.c_str());
	if (!cobj) {
		return RuntimeError(fmt::format("Failed to lookup name '{}'", classname));
	}
	if (!pArgs) {
		// PyObject_Call requires pArgs not be NULL
		pArgs = PyTuple_New(0);
	} else {
		Py_INCREF(pArgs);
	}
	PyObject* ret = PyObject_Call(cobj, pArgs, kwArgs);
	Py_DECREF(pArgs);
	if (!ret) {
		return RuntimeError("Failed to call constructor");
	}
	return ret;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x1B01BE6B, "GUI Script Engine (Python)")
PLUGIN_CLASS(IE_GUI_SCRIPT_CLASS_ID, GUIScript)
END_PLUGIN()
