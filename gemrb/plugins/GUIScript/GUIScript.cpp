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

#include "Interface.h"
#include "Map.h"
#include "Label.h"
#include "GameControl.h"
#include "WorldMapControl.h"
#include "MapControl.h"
#include "Audio.h"
#include "GSUtils.h" //checkvariable
#include "TileMap.h"
#include "Video.h"
#include "Palette.h"
#include "TextEdit.h"
#include "Button.h"
#include "Spell.h"
#include "Item.h"
#include "MusicMgr.h"
#include "SaveGameIterator.h"
#include "Game.h"
#include "ControlAnimation.h"
#include "DataFileMgr.h"
#include "WorldMap.h"
#include "EffectQueue.h"
#include "ImageFactory.h"
#include "ResourceDesc.h"
#include "PalettedImageMgr.h"
#include "ImageMgr.h"
#include "GameData.h"

#include <cstdio>

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
static int SpecialSpellsCount = -1;
static int StoreSpellsCount = -1;
static int UsedItemsCount = -1;
static int ItemSoundsCount = -1;

//#define UIT_ALLOW_REPLACE    1 //item is replaceable with another item on this list

struct UsedItemType {
	ieResRef itemname;
	ieVariable username; //death variable
	ieStrRef value;
//	int flags;           //UIT flags
};

struct SpellDescType {
	ieResRef resref;
	ieStrRef value;
};

typedef char EventNameType[17];
#define IS_DROP   	0
#define IS_GET    	1

typedef ieResRef ResRefPairs[2];

#define UNINIT_IEDWORD 0xcccccccc

static SpellDescType *SpecialItems = NULL;
static SpellDescType *SpecialSpells = NULL;

#define SP_IDENTIFY  1      //any spell that cannot be cast from the menu
#define SP_SILENCE   2      //any spell that can be cast in silence

static SpellDescType *StoreSpells = NULL;
static ItemExtHeader *ItemArray = NULL;
static SpellExtHeader *SpellArray = NULL;
static UsedItemType *UsedItems = NULL;
static ResRefPairs *ItemSounds = NULL;

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

static EffectRef fx_learn_spell_ref={"Spell:Learn",NULL,-1};

// Like PyString_FromString(), but for ResRef
inline PyObject* PyString_FromResRef(char* ResRef)
{
	unsigned int i = strnlen(ResRef,sizeof(ieResRef));
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
inline PyObject* RuntimeError(const char* msg)
{
	printMessage( "GUIScript", "Runtime Error:\n", LIGHT_RED );
	PyErr_SetString( PyExc_RuntimeError, msg );
	if (QuitOnError) {
		core->Quit();
	}
	return NULL;
}

/* Prints error msg for invalid function parameters and also the function's
 * doc string (given as an argument). Then returns NULL, so this function
 * can be called in `return'. The exception should be set by previous
 * call to e.g. PyArg_ParseTuple()
 */
inline PyObject* AttributeError(const char* doc_string)
{
	printMessage( "GUIScript", "Syntax Error:\n", LIGHT_RED );
	PyErr_SetString(PyExc_AttributeError, doc_string);
	if (QuitOnError) {
		core->Quit();
	}
	return NULL;
}

inline Control *GetControl( int wi, int ci, int ct)
{
	char errorbuffer[256];

	Window* win = core->GetWindow( wi );
	if (win == NULL) {
		snprintf(errorbuffer, sizeof(errorbuffer), "Cannot find window #%d", wi);
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

//sets tooltip with Fx key prepended
static inline void SetFunctionTooltip(int WindowIndex, int ControlIndex, char *txt, int Function)
{
	if (txt) {
		if (txt[0]) {
			char *txt2 = (char *) malloc(strlen(txt)+10);
			if (Function) {
				sprintf(txt2,"F%d - %s",Function,txt);
			} else {
				sprintf(txt2,"F%d - %s",ControlIndex+1,txt);
			}
			core->FreeString(txt);
			core->SetTooltip((ieWord) WindowIndex, (ieWord) ControlIndex, txt2);
			free (txt2);
			return;
		}
		core->FreeString(txt);
	}
	core->SetTooltip((ieWord) WindowIndex, (ieWord) ControlIndex, "");
}

static void ReadItemSounds()
{
	int table = gamedata->LoadTable( "itemsnd" );
	if (table<0) {
		ItemSoundsCount = 0;
		ItemSounds = NULL;
		return;
	}
	TableMgr *tab = gamedata->GetTable( table );
	ItemSoundsCount = tab->GetRowCount();
	ItemSounds = (ResRefPairs *) malloc( sizeof(ResRefPairs)*ItemSoundsCount);
	for (int i = 0; i < ItemSoundsCount; i++) {
		strnlwrcpy(ItemSounds[i][0], tab->QueryField(i,0), 8);
		strnlwrcpy(ItemSounds[i][1], tab->QueryField(i,1), 8);
	}
	gamedata->DelTable( table );
}

static void GetItemSound(ieResRef &Sound, ieDword ItemType, const char *ID, ieDword Col)
{
	Sound[0]=0;
	if (Col>1) {
		return;
	}

	if (ItemSoundsCount<0) {
		ReadItemSounds();
	}

	if (ID[1]=='A') {
		//the last 4 item sounds are used for '1A', '2A', '3A' and '4A'
		//item animation types
		ItemType = ItemSoundsCount-4+ID[0]-'1';
	}

	if (ItemType>=(ieDword) ItemSoundsCount) {
		return;
	}
	strnlwrcpy(Sound, ItemSounds[ItemType][Col], 8);
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

static int GetCreatureStat(Actor *actor, unsigned int StatID, int Mod)
{
	if (Mod) {
		return actor->GetStat( StatID );
	}
	return actor->GetBase( StatID );
}

static int SetCreatureStat(Actor *actor, unsigned int StatID, int StatValue)
{
	actor->SetBase( StatID, StatValue );
	actor->CreateDerivedStats();
	return 1;
}

/* create an item entry
	 TODO: this code snippet exists in many copies, maybe consolidate */
static CREItem *CreateCreItem(const char *ItemResRef, int Charge0, int Charge1, int Charge2)
{
	CREItem *TmpItem = new CREItem();
	strnlwrcpy(TmpItem->ItemResRef, ItemResRef, 8);
	TmpItem->Expired=0;
	TmpItem->Usages[0]=(ieWord) Charge0;
	TmpItem->Usages[1]=(ieWord) Charge1;
	TmpItem->Usages[2]=(ieWord) Charge2;
	TmpItem->Flags=0;
	if (core->ResolveRandomItem(TmpItem) && gamedata->Exists(TmpItem->ItemResRef, IE_ITM_CLASS_ID)) {
		return TmpItem;
	}
	/* item couldn't be resolved */
	delete TmpItem;
	return NULL;
}

PyDoc_STRVAR( GemRB_SetInfoTextColor__doc,
"SetInfoTextColor(red, green, blue, [alpha])\n\n"
"Sets the color of Floating Messages in GameControl." );

static PyObject* GemRB_SetInfoTextColor(PyObject*, PyObject* args)
{
	int r,g,b,a;

	if (!PyArg_ParseTuple( args, "iii|i", &r, &g, &b, &a)) {
		return AttributeError( GemRB_SetInfoTextColor__doc );
	}
	Color c = {r,g,b,a};
	core->SetInfoTextColor( c );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_UnhideGUI__doc,
"UnhideGUI()\n\n"
"Shows the Game GUI and redraws windows." );

static PyObject* GemRB_UnhideGUI(PyObject*, PyObject* /*args*/)
{
	GameControl* gc = (GameControl *) GetControl( 0, 0, IE_GUI_GAMECONTROL);
	if (!gc) {
		return RuntimeError("No gamecontrol!");
	}
	gc->UnhideGUI();
	//this enables mouse in dialogs, which is wrong
	//gc->SetCutSceneMode( false );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_HideGUI__doc,
"HideGUI()=>returns 1 if it did something\n\n"
"Hides the Game GUI." );

static PyObject* GemRB_HideGUI(PyObject*, PyObject* /*args*/)
{
	GameControl* gc = (GameControl *) GetControl( 0, 0, IE_GUI_GAMECONTROL);
	if (!gc) {
		return RuntimeError("No gamecontrol!");
	}
	int ret = gc->HideGUI();

	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_GetGameString__doc,
"GetGameString(Index) => string\n\n"
"Returns various string attributes of the Game object, see the docs.");

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
		case 0:
			return PyString_FromString( game->LoadMos );
		case 1:
			return PyString_FromString( game->CurrentArea );
		}
	}

	return AttributeError( GemRB_GetGameString__doc );
}

PyDoc_STRVAR( GemRB_LoadGame__doc,
"LoadGame(Index)\n\n"
"Loads and enters the Game." );

static PyObject* GemRB_LoadGame(PyObject*, PyObject* args)
{
	int GameIndex, VersionOverride = 0;

	if (!PyArg_ParseTuple( args, "i|i", &GameIndex, &VersionOverride )) {
		return AttributeError( GemRB_LoadGame__doc );
	}
	core->SetupLoadGame(GameIndex, VersionOverride);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_EnterGame__doc,
"EnterGame()\n\n"
"Starts new game and enters it." );

static PyObject* GemRB_EnterGame(PyObject*, PyObject* /*args*/)
{
	core->QuitFlag|=QF_ENTERGAME;

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_QuitGame__doc,
"QuitGame()\n\n"
"Stops the current game.");
static PyObject* GemRB_QuitGame(PyObject*, PyObject* /*args*/)
{
	core->QuitFlag=QF_QUITGAME;
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_TextArea_MoveText__doc,
"MoveTAText(srcWin, srcCtrl, dstWin, dstCtrl)\n\n"
"Copies a TextArea content to another.");

static PyObject* GemRB_TextArea_MoveText(PyObject * /*self*/, PyObject* args)
{
	int srcWin, srcCtrl, dstWin, dstCtrl;

	if (!PyArg_ParseTuple( args, "iiii", &srcWin, &srcCtrl, &dstWin, &dstCtrl )) {
		return AttributeError( GemRB_TextArea_MoveText__doc );
	}

	TextArea* SrcTA = ( TextArea* ) GetControl( srcWin, srcCtrl, IE_GUI_TEXTAREA);
	if (!SrcTA) {
		return NULL;
	}

	TextArea* DstTA = ( TextArea* ) GetControl( dstWin, dstCtrl, IE_GUI_TEXTAREA);
	if (!DstTA) {
		return NULL;
	}

	SrcTA->CopyTo( DstTA );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_TextArea_Rewind__doc,
"RewindTA(Win, Ctrl, Ticks)\n\n"
"Sets up a TextArea for scrolling. Ticks is the delay between the steps in scrolling.");

static PyObject* GemRB_TextArea_Rewind(PyObject * /*self*/, PyObject* args)
{
	int Win, Ctrl, Ticks;

	if (!PyArg_ParseTuple( args, "iii", &Win, &Ctrl, &Ticks)) {
		return AttributeError( GemRB_TextArea_Rewind__doc );
	}

	TextArea* ctrl = ( TextArea* ) GetControl( Win, Ctrl, IE_GUI_TEXTAREA);
	if (!ctrl) {
		return NULL;
	}

	ctrl->SetupScroll(Ticks);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_TextArea_SetHistory__doc,
"SetTAHistory(Win, Ctrl, KeepLines)\n\n"
"Sets up a TextArea to expire scrolled out lines.");

static PyObject* GemRB_TextArea_SetHistory(PyObject * /*self*/, PyObject* args)
{
	int Win, Ctrl, Keep;

	if (!PyArg_ParseTuple( args, "iii", &Win, &Ctrl, &Keep)) {
		return AttributeError( GemRB_TextArea_SetHistory__doc );
	}

	TextArea* ctrl = ( TextArea* ) GetControl( Win, Ctrl, IE_GUI_TEXTAREA);
	if (!ctrl) {
		return NULL;
	}

	ctrl->SetPreservedRow(Keep);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_StatComment__doc,
"StatComment(Strref, X, Y) => string\n\n"
"Replaces values X and Y into an strref in place of %%d." );

static PyObject* GemRB_StatComment(PyObject * /*self*/, PyObject* args)
{
	ieStrRef Strref;
	int X, Y;
	PyObject* ret;

	if (!PyArg_ParseTuple( args, "iii", &Strref, &X, &Y )) {
		return AttributeError( GemRB_StatComment__doc );
	}
	char* text = core->GetString( Strref );
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
"GetString(strref[,flags]) => string\n\n"
"Returns string for given strref. " );

static PyObject* GemRB_GetString(PyObject * /*self*/, PyObject* args)
{
	ieStrRef strref;
	int flags = 0;
	PyObject* ret;

	if (!PyArg_ParseTuple( args, "i|i", &strref, &flags )) {
		return AttributeError( GemRB_GetString__doc );
	}

	char *text = core->GetString( strref, flags );
	ret=PyString_FromString( text );
	core->FreeString( text );
	return ret;
}

PyDoc_STRVAR( GemRB_EndCutSceneMode__doc,
"EndCutSceneMode()\n\n"
"Exits the CutScene Mode." );

static PyObject* GemRB_EndCutSceneMode(PyObject * /*self*/, PyObject* /*args*/)
{
	core->SetCutSceneMode( false );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_LoadWindowPack__doc,
"LoadWindowPack(CHUIResRef, [Width=0, Height=0])\n\n"
"Loads a WindowPack into the Window Manager Module. "
"Width and Height set winpack's natural screen size if nonzero." );

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
		printMessage("GUIScript","Screen is too small!\n",LIGHT_RED);
		printf("This window requires %d x %d resolution.\n",width,height);
		return RuntimeError("Please change your settings.");
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_LoadWindow__doc,
"LoadWindow(WindowID) => WindowIndex\n\n"
"Returns a Window." );

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

	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_LoadWindowObject__doc,
"LoadWindowObject(WindowID) => GWindow\n\n"
"Returns a Window as an object." );

static PyObject* GemRB_LoadWindowObject(PyObject * self, PyObject* args)
{
	int WindowID;
	if (!PyArg_ParseTuple( args, "i", &WindowID )) {
		return AttributeError( GemRB_LoadWindowObject__doc );
	}

	PyObject* win = GemRB_LoadWindow(self, args);
	if (!win || !PyObject_TypeCheck( win, &PyInt_Type ))
		return win; // exception

	PyObject* wintuple = PyTuple_New(1);
	PyTuple_SET_ITEM(wintuple, 0, win);

	GUIScript *gs = (GUIScript *) core->GetGUIScriptEngine();
	PyObject* ret = gs->ConstructObject("Window", wintuple);
	Py_DECREF(wintuple);
	if (!ret) {
		char buf[256];
		snprintf( buf, sizeof( buf ), "Couldn't construct Window object for window %d!", WindowID );
		return RuntimeError(buf);
	}
	return ret;
}


PyDoc_STRVAR( GemRB_Window_SetSize__doc,
"SetWindowSize(WindowIndex, Width, Height)\n\n"
"Resizes a Window.");

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_SetFrame__doc,
"SetWindowFrame(WindowIndex)\n\n"
"Sets Window frame used to fill screen on higher resolutions.");

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_LoadWindowFrame__doc,
"LoadWindowFrame(MOSResRef_Left, MOSResRef_Right, MOSResRef_Top, MOSResRef_Bottom))\n\n"
"Load the parts of window frame used to decorate windows on higher resolutions." );

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

		ImageMgr* im = ( ImageMgr* ) gamedata->GetResource( ResRef[i], &ImageMgr::ID );
		if (im == NULL) {
			return NULL;
		}

		Sprite2D* Picture = im->GetSprite2D();
		if (Picture == NULL) {
			im->release();
			return NULL;
		}

		// FIXME: delete previous WindowFrames
		//core->WindowFrames[i] = Picture;
		core->SetWindowFrame(i, Picture);
		im->release();
	}

	Py_INCREF( Py_None );
	return Py_None;
}


PyDoc_STRVAR( GemRB_EnableCheatKeys__doc,
"EnableCheatKeys(flag)\n\n"
"Sets CheatFlags." );

static PyObject* GemRB_EnableCheatKeys(PyObject * /*self*/, PyObject* args)
{
	int Flag;

	if (!PyArg_ParseTuple( args, "i", &Flag )) {
		return AttributeError( GemRB_EnableCheatKeys__doc );
	}

	core->EnableCheatKeys( Flag );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_SetPicture__doc,
"SetWindowPicture(WindowIndex, MosResRef)\n\n"
"Changes the background of a Window." );

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

	ImageMgr* mos = ( ImageMgr* ) gamedata->GetResource( MosResRef, &ImageMgr::ID );
	if (mos != NULL) {
		win->SetBackGround( mos->GetSprite2D(), true );
		mos->release();
	}
	win->Invalidate();

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_SetPos__doc,
"SetWindowPos(WindowIndex, X, Y, [Flags=WINDOW_TOPLEFT])\n\n"
"Moves a Window to pos. (X, Y).\n"
"Flags is a bitmask of WINDOW_(TOPLEFT|CENTER|ABSCENTER|RELATIVE|SCALE|BOUNDED) and "
"they are used to modify the meaning of X and Y.\n"
"TOPLEFT: X, Y are coordinates of upper-left corner.\n"
"CENTER: X, Y are coordinates of window's center.\n"
"ABSCENTER: window is placed at screen center, moved by X, Y.\n"
"RELATIVE: window is moved by X, Y.\n"
"SCALE: window is moved by diff of screen size and X, Y, divided by 2.\n"
"BOUNDED: the window is kept within screen boundaries." );

static PyObject* GemRB_Window_SetPos(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, X, Y, Flags = WINDOW_TOPLEFT;

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
		// FIXME: grrrr, should be < 0!!!
		if (X > 32767 || X < 0)
			X = 0;
		if (Y > 32767 || Y < 0)
			Y = 0;

		if (X + win->Width >= core->Width)
			X = core->Width - win->Width;
		if (Y + win->Height >= core->Height)
			Y = core->Height - win->Height;
	}

	win->XPos = X;
	win->YPos = Y;
	win->Invalidate();

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_LoadTable__doc,
"LoadTable(2DAResRef, [ignore_error=0]) => TableIndex\n\n"
"Loads a 2DA Table." );

static PyObject* GemRB_LoadTable(PyObject * /*self*/, PyObject* args)
{
	char* tablename;
	int noerror = 0;

	if (!PyArg_ParseTuple( args, "s|i", &tablename, &noerror )) {
		return AttributeError( GemRB_LoadTable__doc );
	}

	int ind = gamedata->LoadTable( tablename );
	if (!noerror && ind == -1) {
		return RuntimeError("Can't find resource");
	}
	return PyInt_FromLong( ind );
}

PyDoc_STRVAR( GemRB_LoadTableObject__doc,
"LoadTableObject(2DAResRef, [ignore_error=0]) => GTable\n\n"
"Loads a 2DA Table and returns it as an object." );

static PyObject* GemRB_LoadTableObject(PyObject * self, PyObject* args)
{
	const char* tablename;
	int noerror = 0;

	if (!PyArg_ParseTuple( args, "s|i", &tablename, &noerror )) {
		return AttributeError( GemRB_LoadTable__doc );
	}

	PyObject* table = GemRB_LoadTable(self, args);
	if (!table || !PyObject_TypeCheck( table, &PyInt_Type ))
		return table; // exception

	PyObject* tabtuple = PyTuple_New(1);
	PyTuple_SET_ITEM(tabtuple, 0, table);

	GUIScript *gs = (GUIScript *) core->GetGUIScriptEngine();
	PyObject* ret = gs->ConstructObject("Table", tabtuple);
	Py_DECREF(tabtuple);
	if (!ret) {
		char buf[256];
		snprintf( buf, sizeof( buf ), "Couldn't construct Table object for 2DA Table %s!", tablename );
		return RuntimeError(buf);
	}
	return ret;
}


PyDoc_STRVAR( GemRB_Table_Unload__doc,
"UnloadTable(TableIndex)\n\n"
"Unloads a 2DA Table." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Table_GetValue__doc,
"GetTableValue(TableIndex, RowIndex/RowString, ColIndex/ColString, type) => value\n\n"
"Returns a field of a 2DA Table. If Type is omitted the return type is the autodetected, "
"otherwise 0 means string, 1 means integer, 2 means stat symbol translation." );

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
		printMessage( "GUIScript",
			"Type Error: RowIndex/RowString and ColIndex/ColString must be the same type\n",
			LIGHT_RED );
		return NULL;
	}
	if (PyObject_TypeCheck( row, &PyString_Type ) &&
		( !PyObject_TypeCheck( col, &PyString_Type ) )) {
		printMessage( "GUIScript",
			"Type Error: RowIndex/RowString and ColIndex/ColString must be the same type\n",
			LIGHT_RED );
		return NULL;
	}
	TableMgr* tm = gamedata->GetTable( TableIndex );
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
	//if which = 1 then return number
	//if which = -1 (omitted) then return the best format
	if (valid_number( ret, val ) || (which==1) ) {
		return PyInt_FromLong( val );
	}
	if (which==2) {
		val = core->TranslateStat(ret);
		return PyInt_FromLong( val );
	}
	return PyString_FromString( ret );
}

PyDoc_STRVAR( GemRB_Table_FindValue__doc,
"FindTableValue(TableIndex, ColumnIndex, Value[, StartRow]) => Row\n\n"
"Returns the first rowcount of a field of a 2DA Table." );

static PyObject* GemRB_Table_FindValue(PyObject * /*self*/, PyObject* args)
{
	int ti, col;
	int start = 0;
	long Value;

	if (!PyArg_ParseTuple( args, "iil|i", &ti, &col, &Value, &start )) {
		return AttributeError( GemRB_Table_FindValue__doc );
	}

	TableMgr* tm = gamedata->GetTable( ti );
	if (tm == NULL) {
		return RuntimeError("Can't find resource");
	}
	return PyInt_FromLong(tm->FindTableValue(col, Value, start));
}

PyDoc_STRVAR( GemRB_Table_GetRowIndex__doc,
"GetTableRowIndex(TableIndex, RowName) => Row\n\n"
"Returns the Index of a Row in a 2DA Table." );

static PyObject* GemRB_Table_GetRowIndex(PyObject * /*self*/, PyObject* args)
{
	int ti;
	char* rowname;

	if (!PyArg_ParseTuple( args, "is", &ti, &rowname )) {
		return AttributeError( GemRB_Table_GetRowIndex__doc );
	}

	TableMgr* tm = gamedata->GetTable( ti );
	if (tm == NULL) {
		return RuntimeError("Can't find resource");
	}
	int row = tm->GetRowIndex( rowname );
	//no error if the row doesn't exist
	return PyInt_FromLong( row );
}

PyDoc_STRVAR( GemRB_Table_GetRowName__doc,
"GetTableRowName(TableIndex, RowIndex) => string\n\n"
"Returns the Name of a Row in a 2DA Table." );

static PyObject* GemRB_Table_GetRowName(PyObject * /*self*/, PyObject* args)
{
	int ti, row;

	if (!PyArg_ParseTuple( args, "ii", &ti, &row )) {
		return AttributeError( GemRB_Table_GetRowName__doc );
	}

	TableMgr* tm = gamedata->GetTable( ti );
	if (tm == NULL) {
		return RuntimeError("Can't find resource");
	}
	const char* str = tm->GetRowName( row );
	if (str == NULL) {
		return NULL;
	}

	return PyString_FromString( str );
}

PyDoc_STRVAR( GemRB_Table_GetColumnIndex__doc,
"GetTableColumnIndex(TableIndex, ColumnName) => Column\n\n"
"Returns the Index of a Column in a 2DA Table." );

static PyObject* GemRB_Table_GetColumnIndex(PyObject * /*self*/, PyObject* args)
{
	int ti;
	char* colname;

	if (!PyArg_ParseTuple( args, "is", &ti, &colname )) {
		return AttributeError( GemRB_Table_GetColumnIndex__doc );
	}

	TableMgr* tm = gamedata->GetTable( ti );
	if (tm == NULL) {
		return RuntimeError("Can't find resource");
	}
	int col = tm->GetColumnIndex( colname );
	//no error if the column doesn't exist
	return PyInt_FromLong( col );
}

PyDoc_STRVAR( GemRB_Table_GetColumnName__doc,
"GetTableColumnName(TableIndex, ColumnIndex) => string\n\n"
"Returns the Name of a Column in a 2DA Table." );

static PyObject* GemRB_Table_GetColumnName(PyObject * /*self*/, PyObject* args)
{
	int ti, col;

	if (!PyArg_ParseTuple( args, "ii", &ti, &col )) {
		return AttributeError( GemRB_Table_GetColumnName__doc );
	}

	TableMgr* tm = gamedata->GetTable( ti );
	if (tm == NULL) {
		return RuntimeError("Can't find resource");
	}
	const char* str = tm->GetColumnName( col );
	if (str == NULL) {
		return NULL;
	}

	return PyString_FromString( str );
}

PyDoc_STRVAR( GemRB_Table_GetRowCount__doc,
"GetTableRowCount(TableIndex) => RowCount\n\n"
"Returns the number of rows in a 2DA Table." );

static PyObject* GemRB_Table_GetRowCount(PyObject * /*self*/, PyObject* args)
{
	int ti;

	if (!PyArg_ParseTuple( args, "i", &ti )) {
		return AttributeError( GemRB_Table_GetRowCount__doc );
	}

	TableMgr* tm = gamedata->GetTable( ti );
	if (tm == NULL) {
		return RuntimeError("Can't find resource");
	}

	return PyInt_FromLong( tm->GetRowCount() );
}

PyDoc_STRVAR( GemRB_Table_GetColumnCount__doc,
"GetTableColumnCount(TableIndex[, Row]) => ColumnCount\n\n"
"Returns the number of columns in the given row of a 2DA Table. Row may be omitted." );

static PyObject* GemRB_Table_GetColumnCount(PyObject * /*self*/, PyObject* args)
{
	int ti;
	int row = 0;

	if (!PyArg_ParseTuple( args, "i|i", &ti, &row )) {
		return AttributeError( GemRB_Table_GetColumnCount__doc );
	}

	TableMgr* tm = gamedata->GetTable( ti );
	if (tm == NULL) {
		return RuntimeError("Can't find resource");
	}

	return PyInt_FromLong( tm->GetColumnCount(row) );
}

PyDoc_STRVAR( GemRB_LoadSymbol__doc,
"LoadSymbol(IDSResRef) => SymbolIndex\n\n"
"Loads an IDS Symbol Table." );

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

	return PyInt_FromLong( ind );
}

PyDoc_STRVAR( GemRB_Symbol_Unload__doc,
"UnloadSymbol(SymbolIndex)\n\n"
"Unloads an IDS Symbol Table." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Symbol_GetValue__doc,
"GetSymbolValue(SymbolIndex, StringVal) => int\n"
"GetSymbolValue(SymbolIndex, IntVal) => string\n\n"
"Returns a field of an IDS Symbol Table." );

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
			SymbolMgr* sm = core->GetSymbol( SymbolIndex );
			if (!sm)
				return NULL;
			long val = sm->GetValue( syms );
			return PyInt_FromLong( val );
		}
		if (PyObject_TypeCheck( sym, &PyInt_Type )) {
			long symi = PyInt_AsLong( sym );
			SymbolMgr* sm = core->GetSymbol( SymbolIndex );
			if (!sm)
				return NULL;
			const char* str = sm->GetValue( symi );
			return PyString_FromString( str );
		}
	}
	return AttributeError( GemRB_Symbol_GetValue__doc );
}

PyDoc_STRVAR( GemRB_GetControl__doc,
"GetControl(WindowIndex, ControlID) => ControlIndex\n\n"
"Returns a control in a Window." );

static PyObject* GemRB_GetControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlID )) {
		return AttributeError( GemRB_GetControl__doc );
	}


	int ret = core->GetControl( WindowIndex, ControlID );
	if (ret == -1) {
		return RuntimeError( "Control is not found" );
	}

	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_Window_GetControl__doc,
"GetControlObject(WindowID, ControlID) => GControl, or\n"
"Window.GetControl(ControlID) => GControl\n\n"
"Returns a control as an object." );

static PyObject* GemRB_Window_GetControl(PyObject * self, PyObject* args)
{
	int WindowIndex, ControlID;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlID )) {
		return AttributeError( GemRB_Window_GetControl__doc );
	}

	PyObject* control = GemRB_GetControl( self, args );
	if (!control || !PyObject_TypeCheck( control, &PyInt_Type ))
		return control; // exception

	PyObject* ctrltuple = PyTuple_New(2);
	PyTuple_SET_ITEM(ctrltuple, 0, PyInt_FromLong(WindowIndex));
	PyTuple_SET_ITEM(ctrltuple, 1, control);

	PyObject* ret = 0;
	// TODO: get this from 'control' python variable
	int ctrlindex = core->GetControl(WindowIndex, ControlID);
	Control *ctrl = GetControl(WindowIndex, ctrlindex, -1);
	if (!ctrl) {
		// GetControl will already have raised an exception
		return 0;
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
	GUIScript *gs = (GUIScript *) core->GetGUIScriptEngine();
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
"HasControl(WindowIndex, ControlID[, ControlType]) => bool\n\n"
"Returns true if the control exists." );

static PyObject* GemRB_Window_HasControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;
	int Type = -1;

	if (!PyArg_ParseTuple( args, "ii|i", &WindowIndex, &ControlID, &Type )) {
		return AttributeError( GemRB_Window_HasControl__doc );
	}
	int ret = core->GetControl( WindowIndex, ControlID );
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
"QueryText(WindowIndex, ControlIndex) => string\n\n"
"Returns the Text of a TextEdit control." );

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
	switch(ctrl->ControlType) {
	case IE_GUI_LABEL:
		return PyString_FromString(((Label *) ctrl)->QueryText() );
	case IE_GUI_EDIT:
		return PyString_FromString(((TextEdit *) ctrl)->QueryText() );
	case IE_GUI_TEXTAREA:
		return PyString_FromString(((TextArea *) ctrl)->QueryText() );
	default:
		return RuntimeError("Invalid control type");
	}
}

PyDoc_STRVAR( GemRB_TextEdit_SetBufferLength__doc,
"SetBufferLength(WindowIndex, ControlIndex, Length)\n\n"
"Sets the maximum text length of a TextEdit Control. It cannot be more than 65535." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Control_SetText__doc,
"SetText(WindowIndex, ControlIndex, String|Strref) => int\n\n"
"Sets the Text of a control in a Window." );

static PyObject* GemRB_Control_SetText(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci, * str;
	long WindowIndex, ControlIndex, StrRef;
	char* string;
	int ret;

	if (!PyArg_UnpackTuple( args, "ref", 3, 3, &wi, &ci, &str )) {
		return AttributeError( GemRB_Control_SetText__doc );
	}

	if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
		!PyObject_TypeCheck( ci, &PyInt_Type ) ||
		( !PyObject_TypeCheck( str, &PyString_Type ) &&
		!PyObject_TypeCheck( str, &PyInt_Type ) )) {
		return AttributeError( GemRB_Control_SetText__doc );
	}

	WindowIndex = PyInt_AsLong( wi );
	ControlIndex = PyInt_AsLong( ci );
	if (PyObject_TypeCheck( str, &PyString_Type )) {
		string = PyString_AsString( str );
		if (string == NULL) {
			return RuntimeError("Null string received");
		}
		ret = core->SetText( (ieWord) WindowIndex, (ieWord) ControlIndex, string );
		if (ret == -1) {
			return RuntimeError("Cannot set text");
		}
	} else {
		StrRef = PyInt_AsLong( str );
		if (StrRef == -1) {
			ret = core->SetText( (ieWord) WindowIndex, (ieWord) ControlIndex, GEMRB_STRING );
		} else {
			char *tmpstr = core->GetString( StrRef );
			ret = core->SetText( (ieWord) WindowIndex, (ieWord) ControlIndex, tmpstr );
			core->FreeString( tmpstr );
		}
		if (ret == -1) {
			return RuntimeError("Cannot set text");
		}
	}
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_TextArea_Append__doc,
"TextAreaAppend(WindowIndex, ControlIndex, String|Strref [, Row[, Flag]]) => int\n\n"
"Appends the Text to the TextArea Control in the Window. "
"If Row is given then it will insert the text after that row. "
"If Flag is given, then it will use that value as a GetString flag.");

static PyObject* GemRB_TextArea_Append(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci, * str;
	PyObject* row = NULL;
	PyObject* flag = NULL;
	long WindowIndex, ControlIndex;
	long StrRef, Row, Flag = 0;
	char* string;
	int ret;

	if (!PyArg_UnpackTuple( args, "ref", 3, 5, &wi, &ci, &str, &row, &flag )) {
		return AttributeError( GemRB_TextArea_Append__doc );
	}
	if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
		!PyObject_TypeCheck( ci, &PyInt_Type ) ||
		( !PyObject_TypeCheck( str, &PyString_Type ) &&
		!PyObject_TypeCheck( str, &PyInt_Type ) )) {
		return AttributeError( GemRB_TextArea_Append__doc );
	}
	WindowIndex = PyInt_AsLong( wi );
	ControlIndex = PyInt_AsLong( ci );

	TextArea* ta = ( TextArea* ) GetControl( WindowIndex, ControlIndex, IE_GUI_TEXTAREA);
	if (!ta) {
		return NULL;
	}
	if (row) {
		if (!PyObject_TypeCheck( row, &PyInt_Type )) {
			printMessage( "GUIScript",
				"Syntax Error: AppendText row must be integer\n", LIGHT_RED );
			return NULL;
		}
		Row = PyInt_AsLong( row );
		if (Row > ta->GetRowCount() - 1)
			Row = -1;
	} else
		Row = ta->GetRowCount() - 1;

	if (flag) {
		if (!PyObject_TypeCheck( flag, &PyInt_Type )) {
			printMessage( "GUIScript",
				"Syntax Error: GetString flag must be integer\n", LIGHT_RED );
			return NULL;
		}
		Flag = PyInt_AsLong( flag );
	}

	if (PyObject_TypeCheck( str, &PyString_Type )) {
		string = PyString_AsString( str );
		if (string == NULL)
			return RuntimeError("Null string received");
		ret = ta->AppendText( string, Row );
	} else {
		StrRef = PyInt_AsLong( str );
		char* str = core->GetString( StrRef, Flag );
		ret = ta->AppendText( str, Row );
		core->FreeString( str );
	}

	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_TextArea_Clear__doc,
"TextAreaClear(WindowIndex, ControlIndex)\n\n"
"Clears the Text from the TextArea Control in the Window." );

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
	ta->Clear();

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_TextArea_Scroll__doc,
"TextAreaScroll(WindowIndex, ControlIndex, offset)\n\n"
"Scrolls the textarea up or down by offset." );

static PyObject* GemRB_TextArea_Scroll(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, offset;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &offset)) {
			return AttributeError( GemRB_TextArea_Scroll__doc );
	}
	TextArea* ta = ( TextArea* ) GetControl( WindowIndex, ControlIndex, IE_GUI_TEXTAREA);
	if (!ta) {
		return NULL;
	}
	int row = ta->GetTopIndex()+offset;
	if (row<0) {
		row = 0;
	}
	ta->SetRow( row );
	core->RedrawAll();
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Control_SetTooltip__doc,
"SetTooltip(WindowIndex, ControlIndex, String|Strref) => int\n\n"
"Sets control's tooltip." );

static PyObject* GemRB_Control_SetTooltip(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci, * str;
	long WindowIndex, ControlIndex, StrRef;
	char* string;
	int ret;

	if (!PyArg_UnpackTuple( args, "ref", 3, 3, &wi, &ci, &str )) {
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
	if (PyObject_TypeCheck( str, &PyString_Type )) {
		string = PyString_AsString( str );
		if (string == NULL) {
			return RuntimeError("Null string received");
		}
		ret = core->SetTooltip( (ieWord) WindowIndex, (ieWord) ControlIndex, string );
		if (ret == -1) {
			return RuntimeError("Cannot set tooltip");
		}
	} else {
		StrRef = PyInt_AsLong( str );
		if (StrRef == -1) {
			ret = core->SetTooltip( (ieWord) WindowIndex, (ieWord) ControlIndex, GEMRB_STRING );
		} else {
			char* str = core->GetString( StrRef );
			ret = core->SetTooltip( (ieWord) WindowIndex, (ieWord) ControlIndex, str );
			core->FreeString( str );
		}
		if (ret == -1) {
			return RuntimeError("Cannot set tooltip");
		}
	}

	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_Window_SetVisible__doc,
"SetVisible(WindowIndex, Visible)\n\n"
"Sets the Visibility Flag of a Window." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

//useful only for ToB and HoW, sets masterscript/worldmap name
PyDoc_STRVAR( GemRB_SetMasterScript__doc,
"SetMasterScript(ScriptResRef, WMPResRef)\n\n"
"Sets the worldmap and masterscript names." );

PyObject* GemRB_SetMasterScript(PyObject * /*self*/, PyObject* args)
{
	char* script;
	char* worldmap;

	if (!PyArg_ParseTuple( args, "ss", &script, &worldmap )) {
		return AttributeError( GemRB_SetMasterScript__doc );
	}
	strnlwrcpy( core->GlobalScript, script, 8 );
	strnlwrcpy( core->WorldMapName, worldmap, 8 );
	core->UpdateMasterScript();
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_ShowModal__doc,
"ShowModal(WindowIndex, [Shadow=MODAL_SHADOW_NONE])\n\n"
"Show a Window on Screen setting the Modal Status. "
"If Shadow is MODAL_SHADOW_GRAY, other windows are grayed. "
"If Shadow is MODAL_SHADOW_BLACK, they are blacked out." );

static PyObject* GemRB_Window_ShowModal(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, Shadow = MODAL_SHADOW_NONE;

	if (!PyArg_ParseTuple( args, "i|i", &WindowIndex, &Shadow )) {
		return AttributeError( GemRB_Window_ShowModal__doc );
	}

	int ret = core->ShowModal( WindowIndex, Shadow );
	if (ret == -1) {
		return NULL;
	}

	core->PlaySound(DS_WINDOW_OPEN);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetTimedEvent__doc,
"SetTimedEvent(FunctionName, Rounds)\n\n"
"Sets a timed event, the timing is handled by the game object\n"
"if the game object doesn't exist, this command is ignored." );

static PyObject* GemRB_SetTimedEvent(PyObject * /*self*/, PyObject* args)
{
	char* funcName;
	int rounds;

	if (!PyArg_ParseTuple( args, "si", &funcName, &rounds )) {
		return AttributeError( GemRB_SetTimedEvent__doc );
	}

	Game *game = core->GetGame();
	if (game) {
		game->SetTimedEvent(funcName, rounds);
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Control_SetEvent__doc,
"SetEvent(WindowIndex, ControlIndex, EventMask, FunctionName)\n\n"
"Sets an event of a control on a window to a script defined function." );

static PyObject* GemRB_Control_SetEvent(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int event;
	char* funcName;

	if (!PyArg_ParseTuple( args, "iiis", &WindowIndex, &ControlIndex, &event,
			&funcName )) {
		return AttributeError( GemRB_Control_SetEvent__doc );
	}

	Control* ctrl = GetControl( WindowIndex, ControlIndex, -1 );
	if (!ctrl)
		return NULL;

	if (! ctrl->SetEvent( event, funcName )) {
		char buf[256];
		snprintf( buf, sizeof( buf ), "Can't set event handler: %s!", funcName );
		return RuntimeError( buf );
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetNextScript__doc,
"SetNextScript(GUIScriptName)\n\n"
"Sets the Next Script File to be loaded." );

static PyObject* GemRB_SetNextScript(PyObject * /*self*/, PyObject* args)
{
	const char* funcName;

	if (!PyArg_ParseTuple( args, "s", &funcName )) {
		return AttributeError( GemRB_SetNextScript__doc );
	}

	core->SetNextScript(funcName);

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Control_SetStatus__doc,
"SetControlStatus(WindowIndex, ControlIndex, Status)\n\n"
"Sets the status of a Control." );

static PyObject* GemRB_Control_SetStatus(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int status;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &status )) {
		return AttributeError( GemRB_Control_SetStatus__doc );
	}

	int ret = core->SetControlStatus( WindowIndex, ControlIndex, status );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Control_AttachScrollBar__doc,
"AttachScrollBar(WindowIndex, ControlIndex, ScrollBarControlIndex)\n\n"
"Attaches a ScrollBar to another control." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Control_SetVarAssoc__doc,
"SetVarAssoc(WindowIndex, ControlIndex, VariableName, LongValue)\n\n"
"Sets the name of the Variable associated with a control." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_Unload__doc,
"UnloadWindow(WindowIndex)\n\n"
"Unloads a previously Loaded Window." );

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
	int ret = core->DelWindow( arg );
	if (ret == -1) {
		return RuntimeError( "Can't unload window!" );
	}

	core->PlaySound(DS_WINDOW_CLOSE);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_Invalidate__doc,
"InvalidateWindow(WindowIndex)\n\n"
"Invalidates the given Window." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_CreateWindow__doc,
"CreateWindow(WindowID, X, Y, Width, Height, MosResRef) => WindowIndex\n\n"
"Creates a new empty window and returns its index.");

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
"CreateLabelOnButton(WindowIndex, ControlIndex, NewControlID, font, align)"
"Creates a label on top of a button, copying the button's size and position." );

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
	Label* lbl = new Label( core->GetFont( font ) );
	lbl->XPos = btn->XPos;
	lbl->YPos = btn->YPos;
	lbl->Width = btn->Width;
	lbl->Height = btn->Height;
	lbl->ControlID = ControlID;
	lbl->ControlType = IE_GUI_LABEL;
	lbl->Owner = win;
	lbl->SetAlignment( align );
	win->AddControl( lbl );

	int ret = core->GetControl( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
	//Py_INCREF( Py_None );
	//return Py_None;
}

PyDoc_STRVAR( GemRB_Window_CreateLabel__doc,
"CreateLabel(WindowIndex, ControlID, x, y, w, h, font, text, align)\n\n"
"Creates and adds a new Label to a Window." );

static PyObject* GemRB_Window_CreateLabel(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h, align;
	char *font, *text;

	if (!PyArg_ParseTuple( args, "iiiiiissi", &WindowIndex, &ControlID, &x,
			&y, &w, &h, &font, &text, &align )) {
		return AttributeError( GemRB_Window_CreateLabel__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	Label* lbl = new Label( core->GetFont( font ) );
	lbl->XPos = x;
	lbl->YPos = y;
	lbl->Width = w;
	lbl->Height = h;
	lbl->ControlID = ControlID;
	lbl->ControlType = IE_GUI_LABEL;
	lbl->Owner = win;
	lbl->SetText( text );
	lbl->SetAlignment( align );
	win->AddControl( lbl );

	int ret = core->GetControl( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
	//Py_INCREF( Py_None );
	//return Py_None;
}

PyDoc_STRVAR( GemRB_Label_SetTextColor__doc,
"SetLabelTextColor(WindowIndex, ControlIndex, red, green, blue)\n\n"
"Sets the Text Color of a Label Control." );

static PyObject* GemRB_Label_SetTextColor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, r, g, b;

	if (!PyArg_ParseTuple( args, "iiiii", &WindowIndex, &ControlIndex, &r, &g,
			&b )) {
		return AttributeError( GemRB_Label_SetTextColor__doc );
	}

	Label* lab = ( Label* ) GetControl(WindowIndex, ControlIndex, IE_GUI_LABEL);
	if (!lab) {
		return NULL;
	}

	Color fore = {r,g, b, 0}, back = {0, 0, 0, 0};
	lab->SetColor( fore, back );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_CreateTextEdit__doc,
"CreateTextEdit(WindowIndex, ControlID, x, y, w, h, font, text)\n\n"
"Creates and adds a new TextEdit to a Window." );

static PyObject* GemRB_Window_CreateTextEdit(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h;
	char *font, *text;

	if (!PyArg_ParseTuple( args, "iiiiiiss", &WindowIndex, &ControlID, &x,
			&y, &w, &h, &font, &text )) {
		return AttributeError( GemRB_Window_CreateTextEdit__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	//there is no need to set these differently, currently
	TextEdit* edit = new TextEdit( 500, 0, 0);
	edit->SetFont( core->GetFont( font ) );
	edit->XPos = x;
	edit->YPos = y;
	edit->Width = w;
	edit->Height = h;
	edit->ControlID = ControlID;
	edit->ControlType = IE_GUI_EDIT;
	edit->Owner = win;
	edit->SetText( text );

	Sprite2D* spr = core->GetCursorSprite();
	if (spr)
		edit->SetCursor( spr );
	else
		return RuntimeError( "BAM not found" );

	win->AddControl( edit );

	int ret = core->GetControl( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
	//Py_INCREF( Py_None );
	//return Py_None;
}

PyDoc_STRVAR( GemRB_Window_CreateScrollBar__doc,
"CreateScrollBar(WindowIndex, ControlID, x, y, w, h) => ControlIndex\n\n"
"Creates and adds a new ScrollBar to a Window.");

static PyObject* GemRB_Window_CreateScrollBar(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h;

	if (!PyArg_ParseTuple( args, "iiiiii", &WindowIndex, &ControlID, &x, &y,
			&w, &h )) {
		return AttributeError( GemRB_Window_CreateScrollBar__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}

	ScrollBar* sb = new ScrollBar( );
	sb->XPos = x;
	sb->YPos = y;
	sb->Width = w;
	sb->Height = h;
	sb->ControlID = ControlID;
	sb->ControlType = IE_GUI_SCROLLBAR;
	sb->Owner = win;
	win->AddControl( sb );

	int ret = core->GetControl( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
	//Py_INCREF( Py_None );
	//return Py_None;
}


PyDoc_STRVAR( GemRB_Window_CreateButton__doc,
"CreateButton(WindowIndex, ControlID, x, y, w, h) => ControlIndex\n\n"
"Creates and adds a new Button to a Window." );

static PyObject* GemRB_Window_CreateButton(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h;

	if (!PyArg_ParseTuple( args, "iiiiii", &WindowIndex, &ControlID, &x, &y,
			&w, &h )) {
		return AttributeError( GemRB_Window_CreateButton__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}

	Button* btn = new Button( );
	btn->XPos = x;
	btn->YPos = y;
	btn->Width = w;
	btn->Height = h;
	btn->ControlID = ControlID;
	btn->ControlType = IE_GUI_BUTTON;
	btn->Owner = win;
	win->AddControl( btn );

	int ret = core->GetControl( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
	//Py_INCREF( Py_None );
	//return Py_None;
}


PyDoc_STRVAR( GemRB_TextEdit_ConvertEdit__doc,
"ConvertEdit(WindowIndex, ControlIndex, ScrollBarID) => ControlIndex\n\n"
"Converts a simple Edit Control to a TextArea, keeping its ControlID." );

static PyObject* GemRB_TextEdit_ConvertEdit(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	Color fore={255,255,255,0};
	Color init={255,255,255,0};
	Color back={0,0,0,0};
	int ScrollBarID=0;

	if (!PyArg_ParseTuple( args, "ii|i", &WindowIndex, &ControlIndex, &ScrollBarID)) {
		return AttributeError( GemRB_TextEdit_ConvertEdit__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}

	TextEdit *ctrl = (TextEdit *) GetControl(WindowIndex, ControlIndex, IE_GUI_EDIT);
	if (!ctrl) {
		return NULL;
	}
	TextArea* ta = new TextArea( fore, init, back );
	ta->XPos = ctrl->XPos;
	ta->YPos = ctrl->YPos;
	ta->Width = ctrl->Width;
	ta->Height = ctrl->Height;
	ta->ControlID = ctrl->ControlID;
	ta->ControlType = IE_GUI_TEXTAREA;
	ta->Owner = win;
	ta->SetFonts (ctrl->GetFont(), ctrl->GetFont() );
	ta->Flags |= IE_GUI_TEXTAREA_EDITABLE;
	win->AddControl( ta );
	win->Link( ScrollBarID, ( unsigned short ) ta->ControlID );

	int ret = core->GetControl( WindowIndex, ta->ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_ScrollBar_SetSprites__doc,
"SetScrollBarSprites(WindowIndex, ControlIndex, ResRef, Cycle, UpUnpressedFrame, UpPressedFrame, DownUnpressedFrame, DownPressedFrame, TroughFrame, SliderFrame)\n\n"
"Sets a ScrollBar Sprites Images." );

static PyObject* GemRB_ScrollBar_SetSprites(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, cycle, upunpressed, uppressed;
	int downunpressed, downpressed, trough, knob;
	char *ResRef;

	if (!PyArg_ParseTuple( args, "iisiiiiiii", &WindowIndex, &ControlIndex,
			&ResRef, &cycle, &upunpressed, &uppressed, &downunpressed, &downpressed, &trough, &knob )) {
		return AttributeError( GemRB_ScrollBar_SetSprites__doc );
	}

	ScrollBar* sb = ( ScrollBar* ) GetControl(WindowIndex, ControlIndex, IE_GUI_SCROLLBAR);
	if (!sb) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		sb->SetImage( IE_GUI_SCROLLBAR_UP_UNPRESSED, 0 );
		sb->SetImage( IE_GUI_SCROLLBAR_UP_PRESSED, 0 );
		sb->SetImage( IE_GUI_SCROLLBAR_DOWN_UNPRESSED, 0 );
		sb->SetImage( IE_GUI_SCROLLBAR_DOWN_PRESSED, 0 );
		sb->SetImage( IE_GUI_SCROLLBAR_TROUGH, 0 );
		sb->SetImage( IE_GUI_SCROLLBAR_SLIDER, 0 );
		Py_INCREF( Py_None );
		return Py_None;
	}

	AnimationFactory* af = ( AnimationFactory* )
		gamedata->GetFactoryResource( ResRef,
				IE_BAM_CLASS_ID, IE_NORMAL );
	if (!af) {
		return RuntimeError( "BAM not found" );
	}
	Sprite2D *tspr = af->GetFrame(upunpressed, (unsigned char)cycle);
	sb->SetImage( IE_GUI_SCROLLBAR_UP_UNPRESSED, tspr );
	tspr = af->GetFrame( uppressed, (unsigned char) cycle);
	sb->SetImage( IE_GUI_SCROLLBAR_UP_PRESSED, tspr );
	tspr = af->GetFrame( downunpressed, (unsigned char)cycle);
	sb->SetImage( IE_GUI_SCROLLBAR_DOWN_UNPRESSED, tspr );
	tspr = af->GetFrame( downpressed, (unsigned char) cycle);
	sb->SetImage( IE_GUI_SCROLLBAR_DOWN_PRESSED, tspr );
	tspr = af->GetFrame( trough, (unsigned char) cycle);
	sb->SetImage( IE_GUI_SCROLLBAR_TROUGH, tspr );
	tspr = af->GetFrame( knob, (unsigned char) cycle);
	sb->SetImage( IE_GUI_SCROLLBAR_SLIDER, tspr );

	Py_INCREF( Py_None );
	return Py_None;
}


PyDoc_STRVAR( GemRB_Button_SetSprites__doc,
"SetButtonSprites(WindowIndex, ControlIndex, ResRef, Cycle, UnpressedFrame, PressedFrame, SelectedFrame, DisabledFrame)\n\n"
"Sets a Button Sprites Images." );

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
		btn->SetImage( IE_GUI_BUTTON_UNPRESSED, 0 );
		btn->SetImage( IE_GUI_BUTTON_PRESSED, 0 );
		btn->SetImage( IE_GUI_BUTTON_SELECTED, 0 );
		btn->SetImage( IE_GUI_BUTTON_DISABLED, 0 );
		Py_INCREF( Py_None );
		return Py_None;
	}

	AnimationFactory* af = ( AnimationFactory* )
		gamedata->GetFactoryResource( ResRef,
				IE_BAM_CLASS_ID, IE_NORMAL );
	if (!af) {
		return RuntimeError( "BAM not found" );
	}
	Sprite2D *tspr = af->GetFrame(unpressed, (unsigned char)cycle);
	btn->SetImage( IE_GUI_BUTTON_UNPRESSED, tspr );
	tspr = af->GetFrame( pressed, (unsigned char) cycle);
	btn->SetImage( IE_GUI_BUTTON_PRESSED, tspr );
	tspr = af->GetFrame( selected, (unsigned char) cycle);
	btn->SetImage( IE_GUI_BUTTON_SELECTED, tspr );
	tspr = af->GetFrame( disabled, (unsigned char) cycle);
	btn->SetImage( IE_GUI_BUTTON_DISABLED, tspr );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetOverlay__doc,
"SetButtonOverlay(WindowIndex, ControlIndex, Current, Max, r,g,b,a, r,g,b,a)\n\n"
"Sets up a portrait button for hitpoint overlay" );

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

	const Color src = { r1, g1, b1, a1 };
	const Color dest = { r2, g2, b2, a2 };

	if (Clipping<0.0) Clipping = 0.0;
	else if (Clipping>1.0) Clipping = 1.0;
	//can't call clipping, because the change of ratio triggers color change
	btn->SetHorizontalOverlay(Clipping, src, dest);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetBorder__doc,
"SetButtonBorder(WindowIndex, ControlIndex, BorderIndex, dx1, dy1, dx2, dy2, R, G, B, A, [enabled, filled])\n\n"
"Sets border/frame parameters for a button." );

static PyObject* GemRB_Button_SetBorder(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, BorderIndex, dx1, dy1, dx2, dy2, r, g, b, a, enabled = 0, filled = 0;

	if (!PyArg_ParseTuple( args, "iiiiiiiiiii|ii", &WindowIndex, &ControlIndex,
		&BorderIndex, &dx1, &dy1, &dx2, &dy2, &r, &g, &b, &a, &enabled, &filled)) {
		return AttributeError( GemRB_Button_SetBorder__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	const Color color = { r, g, b, a };
	btn->SetBorder( BorderIndex, dx1, dy1, dx2, dy2, color, (bool)enabled, (bool)filled );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_EnableBorder__doc,
"EnableButtonBorder(WindowIndex, ControlIndex, BorderIndex, enabled)\n\n"
"Enable or disable specified border/frame." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetFont__doc,
"SetButtonFont(WindowIndex, ControlIndex, FontResRef)\n\n"
"Sets font used for drawing button label." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetTextColor__doc,
"SetButtonTextColor(WindowIndex, ControlIndex, red, green, blue[, invert=false])\n\n"
"Sets the Text Color of a Button Control. Invert is used for fonts with swapped background and text colors." );

static PyObject* GemRB_Button_SetTextColor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, r, g, b, swap = 0;

	if (!PyArg_ParseTuple( args, "iiiii|i", &WindowIndex, &ControlIndex, &r, &g, &b, &swap )) {
		return AttributeError( GemRB_Button_SetTextColor__doc );
	}

	Button* but = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!but) {
		return NULL;
	}

	Color fore = {r,g, b, 0}, back = {0, 0, 0, 0};

	// FIXME: swap is a hack for fonts which apparently have swapped f & B
	// colors. Maybe it depends on need_palette?
	if (! swap)
		but->SetTextColor( fore, back );
	else
		but->SetTextColor( back, fore );


	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_DeleteControl__doc,
"DeleteControl(WindowIndex, ControlID)\n\n"
"Deletes a control from a Window." );

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
	int CtrlIndex = core->GetControl( WindowIndex, ControlID );
	if (CtrlIndex == -1) {
		return RuntimeError( "Control is not found" );
	}
	win -> DelControl( CtrlIndex );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_WorldMap_AdjustScrolling__doc,
"AdjustScrolling(WindowIndex, ControlIndex, x, y)\n\n"
"Sets the scrolling offset of a WorldMapControl.");

static PyObject* GemRB_WorldMap_AdjustScrolling(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, x, y;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &x, &y )) {
		return AttributeError( GemRB_WorldMap_AdjustScrolling__doc );
	}

	core->AdjustScrolling( WindowIndex, ControlIndex, x, y );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_CreateMovement__doc,
"CreateMovement(Area, Entrance, Direction)\n\n"
"Moves actors to a new area." );

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
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	game->GetCurrentArea()->MoveToNewArea(area, entrance, (unsigned int)direction, everyone, NULL);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_WorldMap_GetDestinationArea__doc,
"GetDestinationArea(WindowIndex, ControlID[, RndEncounter]) => WorldMap entry\n\n"
"Returns the last area pointed on the worldmap.\n"
"If the random encounter flag is set, the random encounters will be evaluated too." );

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
		Py_INCREF( Py_None );
		return Py_None;
	}
	WorldMap *wm = core->GetWorldMap();
	PyObject* dict = PyDict_New();
	//the area the user clicked on
	PyDict_SetItemString(dict, "Target", PyString_FromString (wmc->Area->AreaName) );
	bool encounter;
	WMPAreaLink *wal = wm->GetEncounterLink(wmc->Area->AreaName, encounter);
	if (!wal) {
		PyDict_SetItemString(dict, "Distance", PyInt_FromLong (-1) );
		return dict;
	}
	PyDict_SetItemString(dict, "Destination", PyString_FromString (wmc->Area->AreaName) );
	PyDict_SetItemString(dict, "Entrance", PyString_FromString (wal->DestEntryPoint) );
	PyDict_SetItemString(dict, "Direction", PyInt_FromLong (wal->DirectionFlags) );
	//the area the user will fall on
	if (encounter && eval) {
		int i=rand()%5;

		if(wal->EncounterChance>=100) {
			wal->EncounterChance-=100;
		}
		for(int j=0;j<5;j++) {
			if (wal->EncounterAreaResRef[(i+j)%5][0]) {
				PyDict_SetItemString(dict, "Destination", PyString_FromString (wal->EncounterAreaResRef[(i+j)%5]) );
				PyDict_SetItemString(dict, "Entrance", PyString_FromString ("") );
				// do we need to change Direction here?
				break;
			}
		}
	}

	//the entrance the user will fall on
	PyDict_SetItemString(dict, "Distance", PyInt_FromLong (wm->GetDistance(wmc->Area->AreaName)) );
	return dict;
}

PyDoc_STRVAR( GemRB_Window_CreateWorldMapControl__doc,
"CreateWorldMapControl(WindowIndex, ControlID, x, y, w, h, direction[, font])\n\n"
"Creates and adds a new WorldMap control to a Window." );

static PyObject* GemRB_Window_CreateWorldMapControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h, direction;
	char *font=NULL;

	if (!PyArg_ParseTuple( args, "iiiiiii|s", &WindowIndex, &ControlID, &x,
			&y, &w, &h, &direction, &font )) {
		return AttributeError( GemRB_Window_CreateWorldMapControl__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	int CtrlIndex = core->GetControl( WindowIndex, ControlID );
	if (CtrlIndex != -1) {
		Control *ctrl = win->GetControl( CtrlIndex );
		x = ctrl->XPos;
		y = ctrl->YPos;
		w = ctrl->Width;
		h = ctrl->Height;
		//flags = ctrl->Value;
		win->DelControl( CtrlIndex );
	}
	WorldMapControl* wmap = new WorldMapControl( font?font:"", direction );
	wmap->XPos = x;
	wmap->YPos = y;
	wmap->Width = w;
	wmap->Height = h;
	wmap->ControlID = ControlID;
	wmap->ControlType = IE_GUI_WORLDMAP;
	wmap->Owner = win;
	win->AddControl( wmap );

	int ret = core->GetControl( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
	//Py_INCREF( Py_None );
	//return Py_None;
}

PyDoc_STRVAR( GemRB_WorldMap_SetTextColor__doc,
"SetWorldMapTextColor(WindowIndex, ControlIndex, which, red, green, blue)\n\n"
"Sets the label colors of a WorldMap Control. WHICH selects color affected"
"and is one of IE_GUI_WMAP_COLOR_(NORMAL|SELECTED|NOTVISITED)." );

static PyObject* GemRB_WorldMap_SetTextColor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, which, r, g, b, a;

	if (!PyArg_ParseTuple( args, "iiiiiii", &WindowIndex, &ControlIndex, &which, &r, &g, &b, &a )) {
		return AttributeError( GemRB_WorldMap_SetTextColor__doc );
	}

	WorldMapControl* wmap = ( WorldMapControl* ) GetControl( WindowIndex, ControlIndex, IE_GUI_WORLDMAP);
	if (!wmap) {
		return NULL;
	}

	Color color = {r, g, b, a};
	wmap->SetColor( which, color );

	Py_INCREF( Py_None );
	return Py_None;
}


PyDoc_STRVAR( GemRB_Window_CreateMapControl__doc,
"CreateMapControl(WindowIndex, ControlID, x, y, w, h, "
"[LabelID, FlagResRef[, Flag2ResRef]])\n\n"
"Creates and adds a new Area Map Control to a Window.\n"
"Note: LabelID is an ID, not an index. "
"If there are two flags given, they will be considered a BMP.");

static PyObject* GemRB_Window_CreateMapControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h;
	int LabelID;
	char *Flag=NULL;
	char *Flag2=NULL;

	if (!PyArg_ParseTuple( args, "iiiiiiis|s", &WindowIndex, &ControlID,
			&x, &y, &w, &h, &LabelID, &Flag, &Flag2)) {
		Flag=NULL;
		PyErr_Clear(); //clearing the exception
		if (!PyArg_ParseTuple( args, "iiiiii", &WindowIndex, &ControlID,
			&x, &y, &w, &h)) {
			return AttributeError( GemRB_Window_CreateMapControl__doc );
		}
	}
	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return RuntimeError("Cannot find window!");
	}
	int CtrlIndex = core->GetControl( WindowIndex, ControlID );
	if (CtrlIndex != -1) {
		Control *ctrl = win->GetControl( CtrlIndex );
		x = ctrl->XPos;
		y = ctrl->YPos;
		w = ctrl->Width;
		h = ctrl->Height;
		// do *not* delete the existing control, we want to replace
		// it in the sort order!
		//win->DelControl( CtrlIndex );
	}

	MapControl* map = new MapControl( );
	map->XPos = x;
	map->YPos = y;
	map->Width = w;
	map->Height = h;
	map->ControlID = ControlID;
	map->ControlType = IE_GUI_MAP;
	map->Owner = win;
	if (Flag2) { //pst flavour
		map->convertToGame = false;
		CtrlIndex = core->GetControl( WindowIndex, LabelID );
		Control *lc = win->GetControl( CtrlIndex );
		map->LinkedLabel = lc;
		ImageMgr *anim = ( ImageMgr* ) gamedata->GetResource( Flag, &ImageMgr::ID );
		if (anim) {
			map->Flag[0] = anim->GetSprite2D();
			anim->release();
		}
		anim = ( ImageMgr* ) gamedata->GetResource( Flag2, &ImageMgr::ID );
		if (anim) {
			map->Flag[1] = anim->GetSprite2D();
			anim->release();
		}
		goto setup_done;
	}
	if (Flag) {
		CtrlIndex = core->GetControl( WindowIndex, LabelID );
		Control *lc = win->GetControl( CtrlIndex );
		map->LinkedLabel = lc;
		AnimationFactory* af = ( AnimationFactory* )
			gamedata->GetFactoryResource( Flag,
					IE_BAM_CLASS_ID, IE_NORMAL );
		if (af) {
			for (int i=0;i<8;i++) {
				map->Flag[i] = af->GetFrame(0,i);
			}

		}
	}
setup_done:
	win->AddControl( map );

	int ret = core->GetControl( WindowIndex, ControlID );

	if (ret<0) {
		return NULL;
	}
	return PyInt_FromLong( ret );
	//Py_INCREF( Py_None );
	//return Py_None;
}

PyDoc_STRVAR( GemRB_Control_SetPos__doc,
"SetControlPos(WindowIndex, ControlIndex, X, Y)\n\n"
"Moves a Control." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Control_SetSize__doc,
"SetControlSize(WindowIndex, ControlIndex, Width, Height)\n\n"
"Resizes a Control." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Label_SetUseRGB__doc,
"SetLabelUseRGB(WindowIndex, ControlIndex, status)\n\n"
"Tells a Label to use the RGB colors with the text." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameSetPartySize__doc,
"GameSetPartySize(size)\n\n"
"Sets the maximum party size." );

static PyObject* GemRB_GameSetPartySize(PyObject * /*self*/, PyObject* args)
{
	int Flags;

	if (!PyArg_ParseTuple( args, "i", &Flags )) {
		return AttributeError( GemRB_GameSetPartySize__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	game->SetPartySize( Flags );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameSetProtagonistMode__doc,
"GameSetProtagonistMode(PM)\n\n"
"Sets the protagonist mode, 0-no check, 1-protagonist, 2-team." );

static PyObject* GemRB_GameSetProtagonistMode(PyObject * /*self*/, PyObject* args)
{
	int Flags;

	if (!PyArg_ParseTuple( args, "i", &Flags )) {
		return AttributeError( GemRB_GameSetProtagonistMode__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	game->SetProtagonistMode( Flags );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameSetExpansion__doc,
"GameSetExpansion(expmode)\n\n"
"Sets the expansion mode, 0-no expansion, 1-expansion." );

static PyObject* GemRB_GameSetExpansion(PyObject * /*self*/, PyObject* args)
{
	int Flags;

	if (!PyArg_ParseTuple( args, "i", &Flags )) {
		return AttributeError( GemRB_GameSetExpansion__doc );
	}

	Game *game = core->GetGame();
	if (game) {
		game->SetExpansion( Flags );
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameSetScreenFlags__doc,
"GameSetScreenFlags(Flags, Operation)\n\n"
"Sets the Display Flags of the main game screen (pane status, dialog textarea size)." );

static PyObject* GemRB_GameSetScreenFlags(PyObject * /*self*/, PyObject* args)
{
	int Flags, Operation;

	if (!PyArg_ParseTuple( args, "ii", &Flags, &Operation )) {
		return AttributeError( GemRB_GameSetScreenFlags__doc );
	}
	if (Operation < BM_SET || Operation > BM_NAND) {
		printMessage( "GUIScript",
			"Syntax Error: operation must be 0-4\n", LIGHT_RED );
		return NULL;
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	game->SetControlStatus( Flags, Operation );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameControlSetScreenFlags__doc,
"GameControlSetScreenFlags(Flags, Operation)\n\n"
"Sets the Display Flags of the main game screen control (centeronactor,...)." );

static PyObject* GemRB_GameControlSetScreenFlags(PyObject * /*self*/, PyObject* args)
{
	int Flags, Operation;

	if (!PyArg_ParseTuple( args, "ii", &Flags, &Operation )) {
		return AttributeError( GemRB_GameControlSetScreenFlags__doc );
	}
	if (Operation < BM_SET || Operation > BM_NAND) {
		printMessage( "GUIScript",
			"Syntax Error: operation must be 0-4\n", LIGHT_RED );
		return NULL;
	}

	GameControl *gc = core->GetGameControl();
	if (!gc) {
		printMessage ("GUIScript", "Flag cannot be set!\n",LIGHT_RED);
		return NULL;
	}

	gc->SetScreenFlags( Flags, Operation );

	Py_INCREF( Py_None );
	return Py_None;
}


PyDoc_STRVAR( GemRB_GameControlSetTargetMode__doc,
"GameControlSetTargetMode(Mode, Types)\n\n"
"Sets the targetting mode of the main game screen control (attack, cast spell,...) and type of target (ally, enemy and/or neutral; all by default)" );

static PyObject* GemRB_GameControlSetTargetMode(PyObject * /*self*/, PyObject* args)
{
	int Mode;
	int Types = GA_SELECT | GA_NO_DEAD | GA_NO_HIDDEN;

	if (!PyArg_ParseTuple( args, "i|i", &Mode, &Types )) {
		return AttributeError( GemRB_GameControlSetTargetMode__doc );
	}

	GameControl *gc = core->GetGameControl();
	if (!gc) {
		return RuntimeError("Can't find GameControl!");
	}

	//target mode is only the low bits (which is a number)
	gc->target_mode = Mode&GA_ACTION;
	//target type is all the bits
	gc->target_types = (Mode&GA_ACTION)|Types;

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameControlGetTargetMode__doc,
"GameControlGetTargetMode() => Mode\n\n"
"Returns the targetting mode of the main game screen control (attack, cast spell,...)." );

static PyObject* GemRB_GameControlGetTargetMode(PyObject * /*self*/, PyObject* /*args*/)
{
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		return RuntimeError("Can't find GameControl!");
	}

	return PyInt_FromLong(gc->target_mode);
}

PyDoc_STRVAR( GemRB_Button_SetFlags__doc,
"SetButtonFlags(WindowIndex, ControlIndex, Flags, Operation)\n\n"
"Sets the Display Flags of a Button." );

static PyObject* GemRB_Button_SetFlags(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Flags, Operation;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &Flags, &Operation )) {
		return AttributeError( GemRB_Button_SetFlags__doc );
	}
	if (Operation < BM_SET || Operation > BM_NAND) {
		printMessage( "GUIScript",
			"Syntax Error: operation must be 0-4\n", LIGHT_RED );
		return NULL;
	}

	Control* btn = ( Control* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	if (btn->SetFlags( Flags, Operation ) ) {
		printMessage ("GUIScript", "Flag cannot be set!\n",LIGHT_RED);
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Control_TextArea_SetFlags__doc,
"SetTextAreaFlags(WindowIndex, ControlIndex, Flags, Operation)\n\n"
"Sets the Display Flags of a TextArea. Flags are: IE_GUI_TA_SELECTABLE, IE_GUI_TA_AUTOSCROLL, IE_GUI_TA_SMOOTHSCROLL. Operation defaults to OP_SET." );

static PyObject* GemRB_Control_TextArea_SetFlags(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Flags;
	int Operation=0;

	if (!PyArg_ParseTuple( args, "iii|i", &WindowIndex, &ControlIndex, &Flags, &Operation )) {
		return AttributeError( GemRB_Control_TextArea_SetFlags__doc );
	}
	if (Operation < BM_SET || Operation > BM_NAND) {
		printMessage( "GUIScript",
			"Syntax Error: operation must be 0-4\n", LIGHT_RED );
		return NULL;
	}

	Control* ta = ( Control* ) GetControl(WindowIndex, ControlIndex, IE_GUI_TEXTAREA);
	if (!ta) {
		return NULL;
	}

	if (ta->SetFlags( Flags, Operation )) {
		printMessage ("GUIScript", "Flag cannot be set!\n",LIGHT_RED);
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_ScrollBar_SetDefaultScrollBar__doc,
"SetDefaultScrollBar(WindowIndex, ControlIndex)\n\n"
"Sets the ScrollBar control as default." );

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

	sb->SetFlags( (IE_GUI_SCROLLBAR<<24) | IE_GUI_SCROLLBAR_DEFAULT, BM_OR );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetState__doc,
"SetButtonState(WindowIndex, ControlIndex, State)\n\n"
"Sets the state of a Button Control." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetPictureClipping__doc,
"SetButtonPictureClipping(Window, Button, ClippingPercent)\n\n"
"Sets percent (0-1.0) of width to which button picture will be clipped." );

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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetPicture__doc,
"SetButtonPicture(WindowIndex, ControlIndex, PictureResRef, DefaultResRef)\n\n"
"Sets the Picture of a Button Control from a BMP file. You can also supply a default picture." );

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
		return NULL;
	}

	if (ResRef[0] == 0) {
		btn->SetPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

	ImageFactory* fact = ( ImageFactory* )
		gamedata->GetFactoryResource( ResRef, IE_BMP_CLASS_ID, IE_NORMAL );

	//if the resource doesn't exist, but we have a default resource
	//use this resource
	if (!fact && DefResRef) {
		fact = ( ImageFactory* )
			gamedata->GetFactoryResource( DefResRef, IE_BMP_CLASS_ID, IE_NORMAL );
	}

	if (!fact) {
		return NULL;
	}

	Sprite2D* Picture = fact->GetSprite2D();
	if (Picture == NULL) {
		return NULL;
	}

	btn->SetPicture( Picture );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetSprite2D__doc,
"Button.SetSprite2D(WindowIndex, ControlIndex, Sprite2D)\n\n"
"Sets a Sprite2D onto a button as picture." );

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

	if (spr)
		spr->acquire();
	btn->SetPicture( spr.get() );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetMOS__doc,
"SetButtonMOS(WindowIndex, ControlIndex, MOSResRef)\n\n"
"Sets the Picture of a Button Control from a MOS file." );

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
		Py_INCREF( Py_None );
		return Py_None;
	}

	ImageMgr* im = ( ImageMgr* ) gamedata->GetResource( ResRef, &ImageMgr::ID );
	if (im == NULL) {
		return NULL;
	}

	Sprite2D* Picture = im->GetSprite2D();
	if (Picture == NULL) {
		im->release();
		return NULL;
	}

	btn->SetPicture( Picture );

	im->release();

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetPLT__doc,
"SetButtonPLT(WindowIndex, ControlIndex, PLTResRef, col1, col2, col3, col4, col5, col6, col7, col8, type)\n\n"
"Sets the Picture of a Button Control from a PLT file." );

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
		Py_INCREF( Py_None );
		return Py_None;
	}

	Sprite2D *Picture;
	Sprite2D *Picture2=NULL;

	PalettedImageMgr* im = ( PalettedImageMgr* ) gamedata->GetResource( ResRef, &PalettedImageMgr::ID );

	if (im == NULL ) {
		AnimationFactory* af = ( AnimationFactory* )
			gamedata->GetFactoryResource( ResRef,
			IE_BAM_CLASS_ID, IE_NORMAL );
		if (!af) {
			printMessage("GUISCript","PLT/BAM not found for ref: ",YELLOW);
			printf("%s\n", ResRef);
			textcolor(WHITE);
			if (type == 0)
				return NULL;
			else {
				Py_INCREF( Py_None );
				return Py_None;
			}
		}

		Picture = af->GetPaperdollImage(col[0]==0xFFFFFFFF?0:col, Picture2,(unsigned int)type);
		if (Picture == NULL) {
			printf ("Picture == NULL\n");
			return NULL;
		}
	} else {
		Picture = im->GetSprite2D(type, col);
		im->release();
		if (Picture == NULL) {
			printf ("Picture == NULL\n");
			return NULL;
		}
	}

	if (type == 0)
		btn->ClearPictureList();
	btn->StackPicture(Picture);
	if (Picture2) {
		btn->SetFlags ( IE_GUI_BUTTON_BG1_PAPERDOLL, BM_OR );
		btn->StackPicture( Picture2 );
	} else if (type == 0) {
		btn->SetFlags ( ~IE_GUI_BUTTON_BG1_PAPERDOLL, BM_AND );
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetBAM__doc,
"SetButtonBAM(WindowIndex, ControlIndex, BAMResRef, CycleIndex, FrameIndex, col1)\n\n"
"Sets the Picture of a Button Control from a BAM file. If col1 is >= 0, changes palette picture's palette to one specified by col1. Since it uses 12 colors palette, it has issues in PST." );

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
		Picture = core->GetVideoDriver()->DuplicateSprite(old);
		core->GetVideoDriver()->FreeSprite(old);

		Palette* newpal = Picture->GetPalette()->Copy();
		core->GetPalette( col1, 12, &newpal->col[4]);
		Picture->SetPalette( newpal );
		newpal->Release();
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
"SetAnimationPalette(WindowIndex, ControlIndex, col1, col2, col3, col4, col5, col6, col7, col8)\n\n"
"Sets the palette of an animation already assigned to the button.");

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
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Control_SetAnimation__doc,
"SetAnimation(WindowIndex, ControlIndex, BAMResRef[, Cycle])\n\n"
"Sets the animation of a Control (usually a Button) from a BAM file. Optionally an animation cycle could be set too.");

static PyObject* GemRB_Control_SetAnimation(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	char *ResRef;
	int Cycle = 0;

	if (!PyArg_ParseTuple( args, "iis|i", &wi, &ci, &ResRef, &Cycle )) {
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
		if(ctl->animation->SameResource(ResRef, Cycle)) {
			Py_INCREF( Py_None );
			return Py_None;
		}
		delete ctl->animation;
		ctl->animation = NULL;
	}

	if (ResRef[0] == 0) {
		ctl->SetAnimPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

	ControlAnimation* anim = new ControlAnimation( ctl, ResRef, Cycle );

	anim->UpdateAnimation();

	Py_INCREF( Py_None );
	return Py_None;
}


PyDoc_STRVAR( GemRB_PlaySound__doc,
"PlaySound(SoundResource, xpos, ypos, type)\n\n"
"Plays a Sound." );

static PyObject* GemRB_PlaySound(PyObject * /*self*/, PyObject* args)
{
	char *ResRef;
	int xpos = 0;
	int ypos = 0;
	unsigned int flags = 1; //GEM_SND_RELATIVE

	if (!PyArg_ParseTuple( args, "z|iii", &ResRef, &xpos, &ypos, &flags )) {
		return AttributeError( GemRB_PlaySound__doc );
	}

	core->GetAudioDrv()->Play( ResRef, xpos, ypos, flags );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_DrawWindows__doc,
"DrawWindows()\n\n"
"Refreshes the User Interface." );

static PyObject* GemRB_DrawWindows(PyObject * /*self*/, PyObject * /*args*/)
{
	core->DrawWindows();

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Quit__doc,
"Quit()\n\n"
"Quits GemRB." );

static PyObject* GemRB_Quit(PyObject * /*self*/, PyObject * /*args*/)
{
	bool ret = core->Quit();
	if (!ret) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_LoadMusicPL__doc,
"LoadMusicPL(MusicPlayListResource, HardEnd)\n\n"
"Loads and starts a Music PlayList." );

static PyObject* GemRB_LoadMusicPL(PyObject * /*self*/, PyObject* args)
{
	char *ResRef;
	int HardEnd = 0;

	if (!PyArg_ParseTuple( args, "s|i", &ResRef, &HardEnd )) {
		return AttributeError( GemRB_LoadMusicPL__doc );
	}

	core->GetMusicMgr()->SwitchPlayList( ResRef, (bool) HardEnd );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SoftEndPL__doc,
"SoftEndPL()\n\n"
"Ends a Music Playlist softly." );

static PyObject* GemRB_SoftEndPL(PyObject * /*self*/, PyObject * /*args*/)
{
	core->GetMusicMgr()->End();

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_HardEndPL__doc,
"HardEndPL()\n\n"
"Ends a Music Playlist immediately." );

static PyObject* GemRB_HardEndPL(PyObject * /*self*/, PyObject * /*args*/)
{
	core->GetMusicMgr()->HardEnd();

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetToken__doc,
"SetToken(VariableName, Value)\n\n"
"Set/Create a token to be replaced in StrRefs." );

static PyObject* GemRB_SetToken(PyObject * /*self*/, PyObject* args)
{
	char *Variable;
	char *value;

	if (!PyArg_ParseTuple( args, "ss", &Variable, &value )) {
		return AttributeError( GemRB_SetToken__doc );
	}
	core->GetTokenDictionary()->SetAtCopy( Variable, value );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetVar__doc,
"SetVar(VariableName, Value)\n\n"
"Set/Create a Variable in the Global Dictionary." );

static PyObject* GemRB_SetVar(PyObject * /*self*/, PyObject* args)
{
	char *Variable;
	//this should be 32 bits, always, but i cannot tell that to Python
	unsigned long value;

	if (!PyArg_ParseTuple( args, "sl", &Variable, &value )) {
		return AttributeError( GemRB_SetVar__doc );
	}

	core->GetDictionary()->SetAt( Variable, (ieDword) value );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetMessageWindowSize__doc,
"GetMessageWindowSize() => int\n\n"
"Returns current MessageWindowSize, it works only when a game is loaded." );

static PyObject* GemRB_GetMessageWindowSize(PyObject * /*self*/, PyObject* /*args*/)
{
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	return PyInt_FromLong( game->ControlStatus );
}

PyDoc_STRVAR( GemRB_GetToken__doc,
"GetToken(VariableName) => string\n\n"
"Get a Variable value from the Token Dictionary." );

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
"GetVar(VariableName) => int\n\n"
"Get a Variable value from the Global Dictionary." );

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
"CheckVar(VariableName, Context) => long\n\n"
"Return (and output on terminal) the value of a Game Variable." );

static PyObject* GemRB_CheckVar(PyObject * /*self*/, PyObject* args)
{
	char *Variable;
	char *Context;

	if (!PyArg_ParseTuple( args, "ss", &Variable, &Context )) {
		return AttributeError( GemRB_CheckVar__doc );
	}
	GameControl *gc = core->GetGameControl();
	if (!gc) {
		return RuntimeError("Can't find GameControl!");
	}
	Scriptable *Sender = (Scriptable *) gc->GetLastActor();
	if (!Sender) {
		Sender = (Scriptable *) core->GetGame()->GetCurrentArea();
	}
	if (!Sender) {
		printMessage("GUIScript","No Game!\n", LIGHT_RED);
		return NULL;
	}
	long value =(long) CheckVariable(Sender, Variable, Context);
	printMessage("GUISCript"," ",YELLOW);
	printf("%s %s=%ld\n",Context, Variable, value);
	textcolor(WHITE);
	return PyInt_FromLong( value );
}

PyDoc_STRVAR( GemRB_SetGlobal__doc,
"SetGlobal(VariableName, Context, Value)\n\n"
"Sets a gamescript variable to the specificed numeric value." );

static PyObject* GemRB_SetGlobal(PyObject * /*self*/, PyObject* args)
{
	char *Variable;
	char *Context;
	int Value;

	if (!PyArg_ParseTuple( args, "ssi", &Variable, &Context, &Value )) {
		return AttributeError( GemRB_SetGlobal__doc );
	}

	Scriptable *Sender = NULL;
	Game *game = core->GetGame();
	if (!game) {
		printMessage("GUIScript","No Game!\n", LIGHT_RED);
		return NULL;
	}
	if (!strnicmp(Context, "MYAREA", 6) || !strnicmp(Context, "LOCALS", 6)) {
		GameControl *gc = core->GetGameControl();
		if (!gc) {
			return RuntimeError("Can't find GameControl!");
		}
		Sender = (Scriptable *) gc->GetLastActor();
		if (!Sender) {
			Sender = (Scriptable *) game->GetCurrentArea();
		}
		if (!Sender) {
			printMessage("GUIScript","No Sender!\n", LIGHT_RED);
			return NULL;
		}
	} // else GLOBAL, area name or KAPUTZ

	SetVariable(Sender, Variable, Context, (ieDword) Value);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetGameVar__doc,
"GetGameVar(VariableName) => long\n\n"
"Get a Variable value from the Game Global Dictionary." );

static PyObject* GemRB_GetGameVar(PyObject * /*self*/, PyObject* args)
{
	const char *Variable;
	ieDword value;

	if (!PyArg_ParseTuple( args, "s", &Variable )) {
		return AttributeError( GemRB_GetGameVar__doc );
	}

	if (!core->GetGame()->locals->Lookup( Variable, value )) {
		return PyInt_FromLong( ( unsigned long ) 0 );
	}

	return PyInt_FromLong( (unsigned long) value );
}

PyDoc_STRVAR( GemRB_PlayMovie__doc,
"PlayMovie(MOVResRef[, flag]) => int\n\n"
"Plays the movie named. If flag is set to 1, it won't play it if it was already played once (set in .ini). It returns 0 if it played the movie." );

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

PyDoc_STRVAR( GemRB_SaveCharacter__doc,
"SaveCharacter(PartyID, CharName)\n\n"
"Exports the character from party.");

static PyObject* GemRB_SaveCharacter(PyObject * /*self*/, PyObject * args)
{
	int PartyID;
	const char *name;

	if (!PyArg_ParseTuple( args, "is", &PartyID, &name )) {
		return AttributeError( GemRB_SaveCharacter__doc );
	}
	if (!name[0]) {
		return AttributeError( GemRB_SaveCharacter__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor *actor = game->FindPC(PartyID);
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	return PyInt_FromLong(core->WriteCharacter(name, actor) );
}

PyDoc_STRVAR( GemRB_SaveGame__doc,
"SaveGame(SlotCount, GameName[,version])\n\n"
"Dumps the save game, if version is given, it will save a specific version.");

static PyObject* GemRB_SaveGame(PyObject * /*self*/, PyObject * args)
{
	int SlotCount;
	int Version = -1;
	const char *folder;

	if (!PyArg_ParseTuple( args, "is|i", &SlotCount, &folder, &Version )) {
		return AttributeError( GemRB_SaveGame__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	SaveGameIterator *sgi = core->GetSaveGameIterator();
	if (!sgi) {
		return RuntimeError("No savegame iterator");
	}

	if (Version>0) {
		game->version = Version;
	}
	return PyInt_FromLong(sgi->CreateSaveGame(SlotCount, folder) );
}

PyDoc_STRVAR( GemRB_GetSaveGameCount__doc,
"GetSaveGameCount() => int\n\n"
"Returns the number of saved games." );

static PyObject* GemRB_GetSaveGameCount(PyObject * /*self*/,
	PyObject * /*args*/)
{
	return PyInt_FromLong(
			core->GetSaveGameIterator()->GetSaveGameCount() );
}

PyDoc_STRVAR( GemRB_DeleteSaveGame__doc,
"DeleteSaveGame(SlotCount)\n\n"
"Deletes a saved game folder completely." );

static PyObject* GemRB_DeleteSaveGame(PyObject * /*self*/, PyObject* args)
{
	int SlotCount;

	if (!PyArg_ParseTuple( args, "i", &SlotCount )) {
		return AttributeError( GemRB_DeleteSaveGame__doc );
	}
	core->GetSaveGameIterator()->DeleteSaveGame( SlotCount );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetSaveGameName__doc,
"GetSaveGameName(SaveSlotCount)\n\n"
"Returns savegame date." );

static PyObject* GemRB_GetSaveGameName(PyObject * /*self*/, PyObject* args)
{
	int SlotCount;

	if (!PyArg_ParseTuple( args, "i", &SlotCount )) {
		return AttributeError( GemRB_GetSaveGameName__doc );
	}

	Holder<SaveGame> sg = core->GetSaveGameIterator()->GetSaveGame(SlotCount);
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}

	return PyString_FromString(sg->GetName());
}

PyDoc_STRVAR( GemRB_GetSaveGameDate__doc,
"GetSaveGameDate(SaveSlotCount)\n\n"
"Returns savegame date." );

static PyObject* GemRB_GetSaveGameDate(PyObject * /*self*/, PyObject* args)
{
	int SlotCount;

	if (!PyArg_ParseTuple( args, "i", &SlotCount )) {
		return AttributeError( GemRB_GetSaveGameDate__doc );
	}

	Holder<SaveGame> sg = core->GetSaveGameIterator()->GetSaveGame(SlotCount);
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}

	return PyString_FromString(sg->GetDate());
}

PyDoc_STRVAR( GemRB_GetSaveGameGameDate__doc,
"GetSaveGameGameDate(SaveSlotCount)\n\n"
"Returns savegame in-game date." );

static PyObject* GemRB_GetSaveGameGameDate(PyObject * /*self*/, PyObject* args)
{
	int SlotCount;

	if (!PyArg_ParseTuple( args, "i", &SlotCount )) {
		return AttributeError( GemRB_GetSaveGameGameDate__doc );
	}

	Holder<SaveGame> sg = core->GetSaveGameIterator()->GetSaveGame(SlotCount);
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}

	return PyString_FromString(sg->GetGameDate());
}

PyDoc_STRVAR( GemRB_GetSaveGameSaveID__doc,
"GetSaveGameSaveID(SaveSlotCount)\n\n"
"Returns savegame slot ID." );

static PyObject* GemRB_GetSaveGameSaveID(PyObject * /*self*/, PyObject* args)
{
	int SlotCount;

	if (!PyArg_ParseTuple( args, "i", &SlotCount )) {
		return AttributeError( GemRB_GetSaveGameSaveID__doc );
	}

	Holder<SaveGame> sg = core->GetSaveGameIterator()->GetSaveGame(SlotCount);
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}

	return PyLong_FromLong(sg->GetSaveID());
}

PyDoc_STRVAR( GemRB_GetSaveGamePortrait__doc,
"GetSaveGamePortrait(SaveSlotCount, PCSlotCount)\n\n"
"Returns a savegame PC portrait." );

static PyObject* GemRB_GetSaveGamePortrait(PyObject * /*self*/, PyObject* args)
{
	int SaveSlotCount, PCSlotCount;

	if (!PyArg_ParseTuple( args, "ii", &SaveSlotCount, &PCSlotCount )) {
		return AttributeError( GemRB_GetSaveGamePortrait__doc );
	}

	Holder<SaveGame> sg = core->GetSaveGameIterator()->GetSaveGame(SaveSlotCount);
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}
	if (sg->GetPortraitCount() <= PCSlotCount) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	return CObject<Sprite2D>(sg->GetPortrait(PCSlotCount));
}

PyDoc_STRVAR( GemRB_GetSaveGamePreview__doc,
"GetSaveGamePreview(SaveSlotCount)\n\n"
"Returns a savegame area preview." );

static PyObject* GemRB_GetSaveGamePreview(PyObject * /*self*/, PyObject* args)
{
	int SlotCount;

	if (!PyArg_ParseTuple( args, "i", &SlotCount )) {
		return AttributeError( GemRB_GetSaveGamePreview__doc );
	}

	Holder<SaveGame> sg = core->GetSaveGameIterator()->GetSaveGame(SlotCount);
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}

	return CObject<Sprite2D>(sg->GetPreview());
}

PyDoc_STRVAR( GemRB_GetGamePreview__doc,
"GetGamePreview()\n\n"
"Gets current game area preview." );

static PyObject* GemRB_GetGamePreview(PyObject * /*self*/, PyObject* args)
{
	if (!PyArg_ParseTuple( args, "" )) {
		return AttributeError( GemRB_GetGamePreview__doc );
	}

	return CObject<Sprite2D>(core->GetGameControl()->GetPreview());
}

PyDoc_STRVAR( GemRB_GetGamePortraitPreview__doc,
"GetGamePortraitPreview(PCSlotCount)\n\n"
"Gets a current game PC portrait." );

static PyObject* GemRB_GetGamePortraitPreview(PyObject * /*self*/, PyObject* args)
{
	int PCSlotCount;

	if (!PyArg_ParseTuple( args, "i", &PCSlotCount )) {
		return AttributeError( GemRB_GetGamePreview__doc );
	}

	return CObject<Sprite2D>(core->GetGameControl()->GetPortraitPreview(PCSlotCount));
}

PyDoc_STRVAR( GemRB_Roll__doc,
"Roll(Dice, Size, Add) => int\n\n"
"Calls traditional dice roll." );

static PyObject* GemRB_Roll(PyObject * /*self*/, PyObject* args)
{
	int Dice, Size, Add;

	if (!PyArg_ParseTuple( args, "iii", &Dice, &Size, &Add )) {
		return AttributeError( GemRB_Roll__doc );
	}
	return PyInt_FromLong( core->Roll( Dice, Size, Add ) );
}

PyDoc_STRVAR( GemRB_TextArea_GetPortraits__doc,
"GetPortraits(WindowIndex, ControlIndex, SmallOrLarge) => int\n\n"
"Reads in the contents of the portraits subfolder." );

static PyObject* GemRB_TextArea_GetPortraits(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	int suffix;

	if (!PyArg_ParseTuple( args, "iii", &wi, &ci, &suffix )) {
		return AttributeError( GemRB_TextArea_GetPortraits__doc );
	}
	TextArea* ta = ( TextArea* ) GetControl( wi, ci, IE_GUI_TEXTAREA );
	if (!ta) {
		return NULL;
	}
	return PyInt_FromLong( core->GetPortraits( ta, suffix ) );
}

PyDoc_STRVAR( GemRB_TextArea_GetCharSounds__doc,
"GetCharSounds(WindowIndex, ControlIndex) => int\n\n"
"Reads in the contents of the sounds subfolder." );

static PyObject* GemRB_TextArea_GetCharSounds(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;

	if (!PyArg_ParseTuple( args, "ii", &wi, &ci )) {
		return AttributeError( GemRB_TextArea_GetCharSounds__doc );
	}
	TextArea* ta = ( TextArea* ) GetControl( wi, ci, IE_GUI_TEXTAREA );
	if (!ta) {
		return NULL;
	}
	return PyInt_FromLong( core->GetCharSounds( ta ) );
}

PyDoc_STRVAR( GemRB_TextArea_GetCharacters__doc,
"GetCharacters(WindowIndex, ControlIndex) => int\n\n"
"Reads in the contents of the characters subfolder." );

static PyObject* GemRB_TextArea_GetCharacters(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;

	if (!PyArg_ParseTuple( args, "ii", &wi, &ci )) {
		return AttributeError( GemRB_TextArea_GetCharacters__doc );
	}
	TextArea* ta = ( TextArea* ) GetControl( wi, ci, IE_GUI_TEXTAREA );
	if (!ta) {
		return NULL;
	}
	return PyInt_FromLong( core->GetCharacters( ta ) );
}

PyDoc_STRVAR( GemRB_GetPartySize__doc,
"GetPartySize() => int\n\n"
"Returns the number of PCs." );

static PyObject* GemRB_GetPartySize(PyObject * /*self*/, PyObject * /*args*/)
{
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	return PyInt_FromLong( game->GetPartySize(0) );
}

PyDoc_STRVAR( GemRB_GetGameTime__doc,
"GetGameTime() => int\n\n"
"Returns current game time." );

static PyObject* GemRB_GetGameTime(PyObject * /*self*/, PyObject* /*args*/)
{
	unsigned long GameTime = core->GetGame()->GameTime/AI_UPDATE_TIME;
	return PyInt_FromLong( GameTime );
}

PyDoc_STRVAR( GemRB_GameGetReputation__doc,
"GameGetReputation() => int\n\n"
"Returns party reputation." );

static PyObject* GemRB_GameGetReputation(PyObject * /*self*/, PyObject* /*args*/)
{
	int Reputation = (int) core->GetGame()->Reputation;
	return PyInt_FromLong( Reputation );
}

PyDoc_STRVAR( GemRB_GameSetReputation__doc,
"GameSetReputation(Reputation)\n\n"
"Sets current party reputation." );

static PyObject* GemRB_GameSetReputation(PyObject * /*self*/, PyObject* args)
{
	int Reputation;

	if (!PyArg_ParseTuple( args, "i", &Reputation )) {
		return AttributeError( GemRB_GameSetReputation__doc );
	}
	core->GetGame()->SetReputation( (unsigned int) Reputation );

	Py_INCREF( Py_None );
	return Py_None;
}

void ReadReputation()
{
	int table = gamedata->LoadTable( "reputati" );
	if (table<0) {
		memset(ReputationIncrease,0,sizeof(ReputationIncrease) );
		memset(ReputationDonation,0,sizeof(ReputationDonation) );
		return;
	}
	TableMgr *tab = gamedata->GetTable( table );
	for (int i = 0; i < 20; i++) {
		ReputationIncrease[i] = atoi( tab->QueryField(i,4) );
		ReputationDonation[i] = atoi( tab->QueryField(i,8) );
	}
	gamedata->DelTable( table );
}

PyDoc_STRVAR( GemRB_IncreaseReputation__doc,
"IncreaseReputation( donation ) => int\n\n"
"Increases party reputation according to the donation. (See reputati.2da)" );

static PyObject* GemRB_IncreaseReputation(PyObject * /*self*/, PyObject* args)
{
	int Donation;
	int Increase = 0;

	if (!PyArg_ParseTuple( args, "i", &Donation )) {
		return AttributeError( GemRB_IncreaseReputation__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	int Row = (game->Reputation-9)/10;
	if (Row>19) {
		Row = 19;
	}
	if (ReputationDonation[0]==(int) UNINIT_IEDWORD) {
		ReadReputation();
	}
	int Limit = ReputationDonation[Row];
	if (Limit<=Donation) {
		Increase = ReputationIncrease[Row];
		if (Increase) {
			game->SetReputation( game->Reputation + Increase );
		}
	}
	return PyInt_FromLong ( Increase );
}

PyDoc_STRVAR( GemRB_GameGetPartyGold__doc,
"GameGetPartyGold() => int\n\n"
"Returns current party gold." );

static PyObject* GemRB_GameGetPartyGold(PyObject * /*self*/, PyObject* /*args*/)
{
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	int Gold = game->PartyGold;
	return PyInt_FromLong( Gold );
}

PyDoc_STRVAR( GemRB_GameSetPartyGold__doc,
"GameSetPartyGold(Gold)\n\n"
"Sets current party gold." );

static PyObject* GemRB_GameSetPartyGold(PyObject * /*self*/, PyObject* args)
{
	int Gold, flag = 0;

	if (!PyArg_ParseTuple( args, "i|i", &Gold, &flag )) {
		return AttributeError( GemRB_GameSetPartyGold__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	if (flag) {
		game->AddGold((ieDword) Gold);
	} else {
		game->PartyGold=Gold;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameGetFormation__doc,
"GameGetFormation([Which]) => int\n\n"
"Returns current party formation. If Which was supplied, it returns one of the preset formations." );

static PyObject* GemRB_GameGetFormation(PyObject * /*self*/, PyObject* args)
{
	int Which = -1;
	int Formation;

	if (!PyArg_ParseTuple( args, "|i", &Which )) {
		return AttributeError( GemRB_GameGetFormation__doc );
	}
	if (Which<0) {
		Formation = core->GetGame()->WhichFormation;
	} else {
		if (Which>4) {
			return AttributeError( GemRB_GameGetFormation__doc );
		}
		Formation = core->GetGame()->Formations[Which];
	}
	return PyInt_FromLong( Formation );
}

PyDoc_STRVAR( GemRB_GameSetFormation__doc,
"GameSetFormation(Formation[, Which])\n\n"
"Sets party formation. If Which was supplied, it sets one of the preset formations." );

static PyObject* GemRB_GameSetFormation(PyObject * /*self*/, PyObject* args)
{
	int Formation, Which=-1;

	if (!PyArg_ParseTuple( args, "i|i", &Formation, &Which )) {
		return AttributeError( GemRB_GameSetFormation__doc );
	}
	if (Which<0) {
		core->GetGame()->WhichFormation = Formation;
	} else {
		if (Which>4) {
			return AttributeError( GemRB_GameSetFormation__doc );
		}
		core->GetGame()->Formations[Which] = Formation;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetJournalSize__doc,
"GetJournalSize(chapter[, section]) => int\n\n"
"Returns the number of entries in the given section of journal." );

static PyObject* GemRB_GetJournalSize(PyObject * /*self*/, PyObject * args)
{
	int chapter;
	int section = -1;

	if (!PyArg_ParseTuple( args, "i|i", &chapter, &section )) {
		return AttributeError( GemRB_GetJournalSize__doc );
	}

	int count = 0;
	for (unsigned int i = 0; i < core->GetGame()->GetJournalCount(); i++) {
		GAMJournalEntry* je = core->GetGame()->GetJournalEntry( i );
		//printf ("JE: sec: %d; text: %d, time: %d, chapter: %d, un09: %d, un0b: %d\n", je->Section, je->Text, je->GameTime, je->Chapter, je->unknown09, je->unknown0B);
		if ((section == -1 || section == je->Section) && (chapter==je->Chapter) )
			count++;
	}

	return PyInt_FromLong( count );
}

PyDoc_STRVAR( GemRB_GetJournalEntry__doc,
"GetJournalEntry(chapter, index[, section]) => JournalEntry\n\n"
"Returns dictionary representing journal entry w/ given chapter, section and index." );

static PyObject* GemRB_GetJournalEntry(PyObject * /*self*/, PyObject * args)
{
	int section=-1, index, chapter;

	if (!PyArg_ParseTuple( args, "ii|i", &chapter, &index, &section )) {
		return AttributeError( GemRB_GetJournalEntry__doc );
	}

	int count = 0;
	for (unsigned int i = 0; i < core->GetGame()->GetJournalCount(); i++) {
		GAMJournalEntry* je = core->GetGame()->GetJournalEntry( i );
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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameIsBeastKnown__doc,
"GameIsBeastKnown(index) => int\n\n"
"Returns whether beast with given index is known to PCs (works only on PST)." );

static PyObject* GemRB_GameIsBeastKnown(PyObject * /*self*/, PyObject * args)
{
	int index;
	if (!PyArg_ParseTuple( args, "i", &index )) {
		return AttributeError( GemRB_GameIsBeastKnown__doc );
	}

	return PyInt_FromLong( core->GetGame()->IsBeastKnown( index ));
}

PyDoc_STRVAR( GemRB_GetINIPartyCount__doc,
"GetINIPartyCount() =>int\n\n"
"Returns the Number of Party defined in Party.ini (works only on IWD2)." );

static PyObject* GemRB_GetINIPartyCount(PyObject * /*self*/,
	PyObject * /*args*/)
{
	if (!core->GetPartyINI()) {
		return RuntimeError( "INI resource not found!" );
	}
	return PyInt_FromLong( core->GetPartyINI()->GetTagsCount() );
}

PyDoc_STRVAR( GemRB_GetINIQuestsKey__doc,
"GetINIQuestsKey(Tag, Key, Default) => string\n\n"
"Returns a Value from the quests.ini File (works only on PST)." );

static PyObject* GemRB_GetINIQuestsKey(PyObject * /*self*/, PyObject* args)
{
	char *Tag, *Key, *Default;

	if (!PyArg_ParseTuple( args, "sss", &Tag, &Key, &Default )) {
		return AttributeError( GemRB_GetINIQuestsKey__doc );
	}
	if (!core->GetQuestsINI()) {
		return RuntimeError( "INI resource not found!" );
	}
	return PyString_FromString(
			core->GetQuestsINI()->GetKeyAsString( Tag, Key, Default ) );
}

PyDoc_STRVAR( GemRB_GetINIBeastsKey__doc,
"GetINIBeastsKey(Tag, Key, Default) => string\n\n"
"Returns a Value from the beasts.ini File (works only on PST)." );

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
"GetINIPartyKey(Tag, Key, Default) => string\n\n"
"Returns a Value from the Party.ini File (works only on IWD2)." );

static PyObject* GemRB_GetINIPartyKey(PyObject * /*self*/, PyObject* args)
{
	const char *Tag, *Key, *Default;

	if (!PyArg_ParseTuple( args, "sss", &Tag, &Key, &Default )) {
		return AttributeError( GemRB_GetINIPartyKey__doc );
	}
	if (!core->GetPartyINI()) {
		return RuntimeError( "INI resource not found!" );
	}
	return PyString_FromString(
			core->GetPartyINI()->GetKeyAsString( Tag, Key, Default ) );
}

PyDoc_STRVAR( GemRB_CreatePlayer__doc,
"CreatePlayer(CREResRef, Slot [,Import] ) => PlayerSlot\n\n"
"Creates or removes a player character in a given party slot. If import is nonzero, then reads a CHR instead of a CRE." );

static PyObject* GemRB_CreatePlayer(PyObject * /*self*/, PyObject* args)
{
	const char *CreResRef;
	int PlayerSlot, Slot;
	int Import=0;

	if (!PyArg_ParseTuple( args, "si|i", &CreResRef, &PlayerSlot, &Import)) {
		return AttributeError( GemRB_CreatePlayer__doc );
	}
	//PlayerSlot is zero based
	Slot = ( PlayerSlot & 0x7fff );
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
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
			return RuntimeError("Slot is already filled!");
		}
	}
	if (CreResRef[0]) {
		PlayerSlot = gamedata->LoadCreature( CreResRef, Slot, (bool) Import );
	} else {
		//just destroyed the previous actor, not going to create one
		PlayerSlot = 0;
	}
	if (PlayerSlot < 0) {
		return RuntimeError("File not found!");
	}
	return PyInt_FromLong( PlayerSlot );
}

PyDoc_STRVAR( GemRB_GetPlayerStates__doc,
"GetPlayerStates(PartyID) => string\n\n"
"Returns the state string for the player.");

static PyObject* GemRB_GetPlayerStates(PyObject * /*self*/, PyObject* args)
{
	int PartyID;

	if (!PyArg_ParseTuple( args, "i", &PartyID )) {
		return AttributeError( GemRB_GetPlayerStates__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* MyActor = game->FindPC( PartyID );
	if (!MyActor) {
		return RuntimeError( "Actor not found" );
	}
	return PyString_FromString((const char *) MyActor->GetStateString() );
}

PyDoc_STRVAR( GemRB_GetPlayerName__doc,
"GetPlayerName(PartyID[, LongOrShort]) => string\n\n"
"Queries the player name." );

static PyObject* GemRB_GetPlayerName(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Which;

	Which = 0;
	if (!PyArg_ParseTuple( args, "i|i", &PartyID, &Which )) {
		return AttributeError( GemRB_GetPlayerName__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* MyActor = game->FindPC( PartyID );
	if (!MyActor) {
		return RuntimeError( "Actor not found" );
	}
	return PyString_FromString( MyActor->GetName(Which) );
}

PyDoc_STRVAR( GemRB_SetPlayerName__doc,
"SetPlayerName(Slot, Name[, LongOrShort])\n\n"
"Sets the player name." );

static PyObject* GemRB_SetPlayerName(PyObject * /*self*/, PyObject* args)
{
	const char *Name=NULL;
	int PlayerSlot, Which;

	Which = 0;
	if (!PyArg_ParseTuple( args, "is|i", &PlayerSlot, &Name, &Which )) {
		return AttributeError( GemRB_SetPlayerName__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* MyActor = game->FindPC( PlayerSlot );
	if (!MyActor) {
		return RuntimeError( "Actor not found!" );
	}
	MyActor->SetName(Name, Which);
	MyActor->SetMCFlag(MC_EXPORTABLE,BM_OR);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_CreateString__doc,
"CreateString( Text )->StrRef\n\n"
"Creates a custom string." );

static PyObject* GemRB_CreateString(PyObject * /*self*/, PyObject* args)
{
	const char *Text;
	ieStrRef strref;

	if (!PyArg_ParseTuple( args, "is", &strref, &Text )) {
		return AttributeError( GemRB_CreateString__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	strref = core->UpdateString(strref, Text);
	return PyInt_FromLong( strref );
}

PyDoc_STRVAR( GemRB_SetPlayerString__doc,
"SetPlayerString(PlayerSlot, StringSlot, StrRef)\n\n"
"Sets one of the player character's verbal constants. Mostly useful for setting biography." );

static PyObject* GemRB_SetPlayerString(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, StringSlot, StrRef;

	if (!PyArg_ParseTuple( args, "iii", &PlayerSlot, &StringSlot, &StrRef )) {
		return AttributeError( GemRB_SetPlayerString__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* MyActor = game->FindPC( PlayerSlot );
	if (!MyActor) {
		return RuntimeError( "Actor not found!" );
	}

	if (StringSlot>=VCONST_COUNT) {
		return AttributeError( "StringSlot is out of range!" );
	}

	MyActor->StrRefs[StringSlot]=StrRef;

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetPlayerSound__doc,
"SetPlayerSound(Slot, SoundFolder)\n\n"
"Sets the player character's sound set." );

static PyObject* GemRB_SetPlayerSound(PyObject * /*self*/, PyObject* args)
{
	const char *Sound=NULL;
	int PlayerSlot;

	if (!PyArg_ParseTuple( args, "is", &PlayerSlot, &Sound )) {
		return AttributeError( GemRB_SetPlayerSound__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* MyActor = game->FindPC( PlayerSlot );
	if (!MyActor) {
		return RuntimeError( "Actor not found!" );
	}
	MyActor->SetSoundFolder(Sound);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetSlotType__doc,
"GetSlotType(idx[, PartyID]) => dict\n\n"
"Returns dictionary of an itemslot type (slottype.2da).");

static PyObject* GemRB_GetSlotType(PyObject * /*self*/, PyObject* args)
{
	int idx;
	int PartyID = 0;
	Actor *actor = NULL;

	if (!PyArg_ParseTuple( args, "i|i", &idx, &PartyID )) {
		return AttributeError( GemRB_GetSlotType__doc );
	}

	if (PartyID) {
		Game *game = core->GetGame();
		if (!game) {
			return RuntimeError( "No game loaded!" );
		}
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
"GetPCStats(PartyID) => dict\n\n"
"Returns dictionary of PC's performance stats." );

static PyObject* GemRB_GetPCStats(PyObject * /*self*/, PyObject* args)
{
	int PartyID;

	if (!PyArg_ParseTuple( args, "i", &PartyID )) {
		return AttributeError( GemRB_GetPCStats__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* MyActor = game->FindPC( PartyID );
	if (!MyActor || !MyActor->PCStats) {
		Py_INCREF( Py_None );
		return Py_None;
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

	// FIXME!!!
	if (ps->FavouriteSpells[0][0]) {
		int largest = 0;

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
		PyDict_SetItemString(dict, "FavouriteSpell", PyString_FromString (""));
	}



	// FIXME!!!
	if (ps->FavouriteWeapons[0][0]) {
		int largest = 0;

		for (int i = 1; i < 4; ++i) {
			if (ps->FavouriteWeaponsCount[i] > ps->FavouriteWeaponsCount[largest]) {
				largest = i;
			}
		}

		Item* item = gamedata->GetItem(ps->FavouriteWeapons[largest]);
		if (item == NULL) {
			return RuntimeError( "Item not found!" );
		}

		PyDict_SetItemString(dict, "FavouriteWeapon", PyInt_FromLong ((signed) item->GetItemName(false)));

		gamedata->FreeItem( item, ps->FavouriteWeapons[largest], false );
	} else {
		PyDict_SetItemString(dict, "FavouriteWeapon", PyString_FromString (""));
	}

	return dict;
}


PyDoc_STRVAR( GemRB_GameSelectPC__doc,
"GameSelectPC(PartyID, Selected, [Flags = SELECT_NORMAL])\n\n"
"Selects or deselects PC."
"if PartyID=0, (De)selects all PC."
"Flags is combination of SELECT_REPLACE and SELECT_QUIET."
"SELECT_REPLACE: when selecting other party members, unselect the others." );

static PyObject* GemRB_GameSelectPC(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Select;
	int Flags = SELECT_NORMAL;

	if (!PyArg_ParseTuple( args, "ii|i", &PartyID, &Select, &Flags )) {
		return AttributeError( GemRB_GameSelectPC__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	Actor* actor;
	if (PartyID > 0) {
		actor = game->FindPC( PartyID );
		if (!actor) {
			Py_INCREF( Py_None );
			return Py_None;
		}
	} else {
		actor = NULL;
	}

	game->SelectActor( actor, (bool) Select, Flags );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameIsPCSelected__doc,
"GameIsPCSelected(Slot) => bool\n\n"
"Returns true if the PC is selected." );

static PyObject* GemRB_GameIsPCSelected(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot;

	if (!PyArg_ParseTuple( args, "i", &PlayerSlot )) {
		return AttributeError( GemRB_GameIsPCSelected__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* MyActor = game->FindPC( PlayerSlot );
	if (!MyActor) {
		return PyInt_FromLong( 0 );
	}
	return PyInt_FromLong( MyActor->IsSelected() );
}


PyDoc_STRVAR( GemRB_GameSelectPCSingle__doc,
"GameSelectPCSingle(index)\n\n"
"Selects one PC in non-walk environment (i.e. in shops, inventory,...)"
"Index must be greater than zero." );

static PyObject* GemRB_GameSelectPCSingle(PyObject * /*self*/, PyObject* args)
{
	int index;

	if (!PyArg_ParseTuple( args, "i", &index )) {
		return AttributeError( GemRB_GameSelectPCSingle__doc );
	}

	core->GetGame()->SelectPCSingle( index );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GameGetSelectedPCSingle__doc,
"GameGetSelectedPCSingle(flag) => int\n\n"
"Returns index of the selected PC in non-walk environment (i.e. in shops, inventory,...). Index should be greater than zero. If flag is set, then this function will return the PC currently talking." );

static PyObject* GemRB_GameGetSelectedPCSingle(PyObject * /*self*/, PyObject* args)
{
	int flag = 0;

	if (!PyArg_ParseTuple( args, "|i", &flag )) {
		return AttributeError( GemRB_GameGetSelectedPCSingle__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	if (flag) {
		GameControl *gc = core->GetGameControl();
		if (!gc) {
			return RuntimeError("Can't find GameControl!");
		}
		Actor *ac = gc->GetSpeaker();
		int ret = 0;
		if (ac) {
			ret = ac->InParty;
		}
		return PyInt_FromLong( ret );
	}
	return PyInt_FromLong( game->GetSelectedPCSingle() );
}

PyDoc_STRVAR( GemRB_GameGetFirstSelectedPC__doc,
"GameGetFirstSelectedPC() => int\n\n"
"Returns index of the first selected PC or 0 if none." );

static PyObject* GemRB_GameGetFirstSelectedPC(PyObject * /*self*/, PyObject* /*args*/)
{
	Actor *actor = core->GetFirstSelectedPC(false);
	if (actor) {
		return PyInt_FromLong( actor->InParty);
	}

	return PyInt_FromLong( 0 );
}

PyDoc_STRVAR( GemRB_ActOnPC__doc,
"ActOnPC(player)\n\n"
"Targets the selected PC for an action (cast spell, attack, ...)" );

static PyObject* GemRB_ActOnPC(PyObject * /*self*/, PyObject* args)
{
	int PartyID;

	if (!PyArg_ParseTuple( args, "i", &PartyID )) {
		return AttributeError( GemRB_ActOnPC__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* MyActor = game->FindPC( PartyID );
	if (MyActor) {
		GameControl* gc = core->GetGameControl();
		if(gc) {
			gc->PerformActionOn(MyActor);
		}
	}
	Py_INCREF(Py_None) ;
	return Py_None ;
}

PyDoc_STRVAR( GemRB_GetPlayerPortrait__doc,
"GetPlayerPortrait(Slot[, SmallOrLarge]) => string\n\n"
"Queries the player portrait." );

static PyObject* GemRB_GetPlayerPortrait(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, Which;

	Which = 0;
	if (!PyArg_ParseTuple( args, "i|i", &PlayerSlot, &Which )) {
		return AttributeError( GemRB_GetPlayerPortrait__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* MyActor = game->FindPC( PlayerSlot );
	if (!MyActor) {
		return PyString_FromString( "");
	}
	return PyString_FromString( MyActor->GetPortrait(Which) );
}

PyDoc_STRVAR( GemRB_GetPlayerString__doc,
"GetPlayerString(Slot, ID) => int\n\n"
"Queries a string reference (verbal constant) from the actor." );

static PyObject* GemRB_GetPlayerString(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, Index, StatValue;

	if (!PyArg_ParseTuple( args, "ii", &PlayerSlot, &Index)) {
		return AttributeError( GemRB_GetPlayerString__doc );
	}
	Actor* MyActor = core->GetGame()->FindPC( PlayerSlot );
	if (!MyActor) {
		return RuntimeError("Cannot find actor!\n");
	}
	if (Index>=VCONST_COUNT) {
		return RuntimeError("String reference too high!\n");
	}
	StatValue = GetCreatureStrRef( MyActor, Index );
	return PyInt_FromLong( StatValue );
}

PyDoc_STRVAR( GemRB_GetPlayerStat__doc,
"GetPlayerStat(Slot, ID[, BaseStat]) => int\n\n"
"Queries a stat." );

static PyObject* GemRB_GetPlayerStat(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, StatID, StatValue, BaseStat;

	BaseStat = 0;
	if (!PyArg_ParseTuple( args, "ii|i", &PlayerSlot, &StatID, &BaseStat )) {
		return AttributeError( GemRB_GetPlayerStat__doc );
	}
	Actor* MyActor = core->GetGame()->FindPC( PlayerSlot );
	if (!MyActor) {
		return RuntimeError("Cannot find actor!\n");
	}
	//returning the modified stat if BaseStat was 0 (default)
	StatValue = GetCreatureStat( MyActor, StatID, !BaseStat );
	return PyInt_FromLong( StatValue );
}

PyDoc_STRVAR( GemRB_SetPlayerStat__doc,
"SetPlayerStat(Slot, ID, Value)\n\n"
"Changes a stat." );

static PyObject* GemRB_SetPlayerStat(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, StatID, StatValue;

	if (!PyArg_ParseTuple( args, "iii", &PlayerSlot, &StatID, &StatValue )) {
		return AttributeError( GemRB_SetPlayerStat__doc );
	}
	Actor* MyActor = core->GetGame()->FindPC( PlayerSlot );
	if (!MyActor) {
		return RuntimeError("Cannot find actor!\n");
	}
	//Setting the creature's base stat
	SetCreatureStat( MyActor, StatID, StatValue);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetPlayerScript__doc,
"GetPlayerScript(Slot[, Index])\n\n"
"Retrieves the script resource for a player. If index is omitted, it will default to "
"the class script slot (customisable by players)." );

static PyObject* GemRB_GetPlayerScript(PyObject * /*self*/, PyObject* args)
{
	//class script is the custom slot for player scripts
	int PlayerSlot, Index = SCR_CLASS;

	if (!PyArg_ParseTuple( args, "i|i", &PlayerSlot, &Index )) {
		return AttributeError( GemRB_GetPlayerScript__doc );
	}
	Actor *actor = core->GetGame()->FindPC(PlayerSlot);
	if (!actor) {
		return RuntimeError("Cannot find actor!\n");
	}
	const char *scr = actor->GetScript(Index);
	if (scr[0]==0) {
		scr="None";
	}
	return PyString_FromString( scr );
}

PyDoc_STRVAR( GemRB_SetPlayerScript__doc,
"SetPlayerScript(Slot, Resource[, Index])\n\n"
"Sets the script resource for a player. If index is omitted, it will default to "
"the class script slot (customisable by players)." );

static PyObject* GemRB_SetPlayerScript(PyObject * /*self*/, PyObject* args)
{
	const char *ScriptName;
	int PlayerSlot, Index = SCR_CLASS;

	if (!PyArg_ParseTuple( args, "is|i", &PlayerSlot, &ScriptName, &Index )) {
		return AttributeError( GemRB_SetPlayerScript__doc );
	}
	Actor *actor = core->GetGame()->FindPC(PlayerSlot);
	if (!actor) {
		return RuntimeError("Cannot find actor!\n");
	}
	actor->SetScript(ScriptName, Index, true);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_FillPlayerInfo__doc,
"FillPlayerInfo(Slot[, Portrait1, Portrait2])\n\n"
"Fills basic character info, that is not stored in stats." );

static PyObject* GemRB_FillPlayerInfo(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot;
	const char *Portrait1=NULL, *Portrait2=NULL;

	if (!PyArg_ParseTuple( args, "i|ss", &PlayerSlot, &Portrait1, &Portrait2)) {
		return AttributeError( GemRB_FillPlayerInfo__doc );
	}
	// here comes some code to transfer icon/name to the PC sheet
	//
	//
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	Actor* MyActor = game->FindPC( PlayerSlot );
	if (!MyActor) {
		return RuntimeError( "Actor not found!" );
	}
	if (Portrait1) {
		MyActor->SetPortrait( Portrait1, 1);
	}
	if (Portrait2) {
		MyActor->SetPortrait( Portrait2, 2);
	}
	int mastertable = gamedata->LoadTable( "avprefix" );
	TableMgr* mtm = gamedata->GetTable( mastertable );
	int count = mtm->GetRowCount();
	if (count< 1 || count>8) {
		return RuntimeError("Table is invalid." );
	}
	const char *poi = mtm->QueryField( 0 );
	int AnimID = strtoul( poi, NULL, 0 );
	for (int i = 1; i < count; i++) {
		poi = mtm->QueryField( i );
		int table = gamedata->LoadTable( poi );
		TableMgr* tm = gamedata->GetTable( table );
		int StatID = atoi( tm->QueryField() );
		StatID = MyActor->GetBase( StatID );
		poi = tm->QueryField( StatID );
		AnimID += strtoul( poi, NULL, 0 );
		gamedata->DelTable( table );
	}
	MyActor->SetBase(IE_ANIMATION_ID, AnimID);
	//setting PST's starting stance to 18
	poi = mtm->QueryField( 0, 1 );
	if (*poi != '*') {
		MyActor->SetStance( atoi( poi ) );
	}
	gamedata->DelTable( mastertable );
	MyActor->SetOver( false );
	MyActor->InitButtons(MyActor->GetStat(IE_CLASS), true); //force re-init of actor's buttons
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Button_SetSpellIcon__doc,
"SetSpellIcon(WindowIndex, ControlIndex, SPLResRef[, type, tooltip, function])\n\n"
"Sets Spell icon image on a button. Type is the icon's type." );

PyObject *SetSpellIcon(int wi, int ci, const ieResRef SpellResRef, int type, int tooltip, int Function)
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
		printMessage( "GUIScript", " ", LIGHT_RED);
		printf("Spell not found :%.8s", SpellResRef );
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
		printf("Searched for: %s\n", IconResRef);
		return RuntimeError( "BAM not found" );
	}
	//small difference between pst and others
	if (af->GetCycleSize(0)!=4) { //non-pst
		btn->SetPicture( af->GetFrame(0, 0));
	}
	else { //pst
		btn->SetImage( IE_GUI_BUTTON_UNPRESSED, af->GetFrame(0, 0));
		btn->SetImage( IE_GUI_BUTTON_PRESSED, af->GetFrame(1, 0));
		btn->SetImage( IE_GUI_BUTTON_SELECTED, af->GetFrame(2, 0));
		btn->SetImage( IE_GUI_BUTTON_DISABLED, af->GetFrame(3, 0));
	}
	if (tooltip) {
		char *str = core->GetString(spell->SpellName,0);
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
		return gamedata->GetBAMSprite(ieh->UseIcon, -1, which);
	}
	return gamedata->GetBAMSprite(item->ItemIcon, -1, which);
}

static void SetItemText(int wi, int ci, int charges, bool oneisnone)
{
	Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
	if (!btn) {
		return;
	}
	char tmp[10];

	if (charges && (charges>1 || !oneisnone) ) {
		sprintf(tmp,"%d",charges);
	} else {
		tmp[0]=0;
	}
	btn->SetText(tmp);
}

PyDoc_STRVAR( GemRB_Button_SetItemIcon__doc,
"SetItemIcon(WindowIndex, ControlIndex, ITMResRef[, type, tooltip, Function, ITM2ResRef])\n\n"
"Sets Item icon image on a button. 0/1 - Inventory Icons, 2 - Description Icon, 3 - No icon,\n"
" 4/5 - Weapon icons, 6 and above - Extended header icons." );

PyObject *SetItemIcon(int wi, int ci, const char *ItemResRef, int Which, int tooltip, int Function, const char *Item2ResRef)
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
	Item* item = gamedata->GetItem(ItemResRef);
	if (item == NULL) {
		btn->SetPicture(NULL);
		//no incref here!
		return Py_None;
	}

	btn->SetFlags( IE_GUI_BUTTON_PICTURE, BM_OR );
	Sprite2D* Picture;
	bool setpicture = true;
	int i;
	switch (Which) {
	case 0: case 1:
		Picture = gamedata->GetBAMSprite(item->ItemIcon, -1, Which);
		break;
	case 2:
		btn->SetPicture( NULL ); // also calls ClearPictureList
		for (i=0;i<4;i++) {
			Picture = gamedata->GetBAMSprite(item->DescriptionIcon, -1, i);
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
			Item* item2 = gamedata->GetItem(Item2ResRef);
			if (item2) {
				Sprite2D* Picture2;
				Picture2 = gamedata->GetBAMSprite(item2->ItemIcon, -1, Which-4);
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
			Picture = gamedata->GetBAMSprite(eh->UseIcon, -1, 0);
		}
		else {
			Picture = NULL;
		}
	}

	if (setpicture)
		btn->SetPicture( Picture );
	if (tooltip) {
		//later getitemname could also return tooltip stuff
		char *str = core->GetString(item->GetItemName(tooltip==2),0);
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
"EnterStore(STOResRef)\n\n"
"Loads the store referenced and opens the store window." );

static PyObject* GemRB_EnterStore(PyObject * /*self*/, PyObject* args)
{
	const char* StoreResRef;

	if (!PyArg_ParseTuple( args, "s", &StoreResRef )) {
		return AttributeError( GemRB_EnterStore__doc );
	}

	//stores are cached, bags could be opened while in shops
	//so better just switch to the requested store silently
	//the core will be intelligent enough to not do excess work
	core->SetCurrentStore( StoreResRef, NULL );

	//the error flag is not optional, we should open a store now
	//core->GetGUIScriptEngine()->RunFunction( "OpenStoreWindow", true);
	core->SetEventFlag(EF_OPENSTORE);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_LeaveStore__doc,
"LeaveStore(STOResRef)\n\n"
"Saves the current store to the Cache folder and frees it from memory." );

static PyObject* GemRB_LeaveStore(PyObject * /*self*/, PyObject* /*args*/)
{
	if (core->CloseCurrentStore() ) {
		return RuntimeError("Cannot save store!");
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_LeaveContainer__doc,
"LeaveContainer()\n\n"
"Clears the current container variable and initiates the 'CloseContainerWindow' guiscript call in the next window update cycle.");

static PyObject* GemRB_LeaveContainer(PyObject * /*self*/, PyObject* /*args*/)
{
	core->CloseCurrentContainer();
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetContainer__doc,
"GetContainer( PartyID, autoselect ) => dictionary\n\n"
"Returns relevant data of the container used by the selected actor. Use autoselect if the container is an item pile at the feet of the actor. It will create the container if required." );

static PyObject* GemRB_GetContainer(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	int autoselect=0;

	if (!PyArg_ParseTuple( args, "i|i", &PartyID, &autoselect )) {
		return AttributeError( GemRB_GetContainer__doc );
	}

	Actor *actor;

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	if (PartyID) {
		actor = game->FindPC( PartyID );
	} else {
		actor = core->GetFirstSelectedPC(false);
	}
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	Container *container = NULL;
	if (autoselect) { //autoselect works only with piles
		Map *map = actor->GetCurrentArea();
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
"GetContainerItem(PartyID, idx) => dictionary\n\n"
"Returns the container item referenced by the index. If PartyID is 0 then the container was opened manually and should be the current container. If PartyID is not 0 then the container is autoselected and should be at the feet of the player." );

static PyObject* GemRB_GetContainerItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	int index;

	if (!PyArg_ParseTuple( args, "ii", &PartyID, &index )) {
		return AttributeError( GemRB_GetContainerItem__doc );
	}
	Container *container;

	if (PartyID) {
		Game *game = core->GetGame();
		if (!game) {
			return RuntimeError( "No game loaded!" );
		}
		Actor *actor = game->FindPC( PartyID );
		if (!actor) {
			return RuntimeError( "Actor not found" );
		}
		Map *map = actor->GetCurrentArea();
		container = map->TMap->GetContainer(actor->Pos, IE_CONTAINER_PILE);
	} else {
		container = core->GetCurrentContainer();
	}
	if (!container) {
		return RuntimeError("No current container!");
	}
	if (index>=(int) container->inventory.GetSlotCount()) {
		Py_INCREF( Py_None );
		return Py_None;
	}
	PyObject* dict = PyDict_New();

	CREItem *ci=container->inventory.GetSlotItem( index );

	PyDict_SetItemString(dict, "ItemResRef", PyString_FromResRef( ci->ItemResRef ));
	PyDict_SetItemString(dict, "Usages0", PyInt_FromLong (ci->Usages[0]));
	PyDict_SetItemString(dict, "Usages1", PyInt_FromLong (ci->Usages[1]));
	PyDict_SetItemString(dict, "Usages2", PyInt_FromLong (ci->Usages[2]));
	PyDict_SetItemString(dict, "Flags", PyInt_FromLong (ci->Flags));

	Item *item = gamedata->GetItem( ci->ItemResRef );

	bool identified = ci->Flags & IE_INV_ITEM_IDENTIFIED;
	PyDict_SetItemString(dict, "ItemName", PyInt_FromLong( (signed) item->GetItemName( identified )) );
	PyDict_SetItemString(dict, "ItemDesc", PyInt_FromLong( (signed) item->GetItemDesc( identified )) );
	gamedata->FreeItem( item, ci->ItemResRef, false );
	return dict;
}

PyDoc_STRVAR( GemRB_ChangeContainerItem__doc,
"ChangeContainerItem(PartyID, slot, action)\n\n"
"Takes an item from a container, or puts it there. "
"If PC is 0 then it uses the first selected PC and the current container, "
"if it is not 0 then it autoselects the container. "
"action=0: move item from PC to container."
"action=1: move item from container to PC.");

static PyObject* GemRB_ChangeContainerItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Slot;
	int action;

	if (!PyArg_ParseTuple( args, "iii", &PartyID, &Slot, &action)) {
		return AttributeError( GemRB_ChangeContainerItem__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor;
	Container *container;
	if (PartyID) {
		actor = game->FindPC( PartyID );
		if (!actor) {
			return RuntimeError( "Actor not found" );
		}
		Map *map = actor->GetCurrentArea();
		container = map->TMap->GetContainer(actor->Pos, IE_CONTAINER_PILE);
	} else {
		actor = core->GetFirstSelectedPC(false);
		if (!actor) {
			return RuntimeError( "Actor not found" );
		}
		container = core->GetCurrentContainer();
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

		res = core->CanMoveItem(container->inventory.GetSlotItem(Slot) );
		if (!res) { //cannot move
			printMessage("GUIScript","Cannot move item, it is undroppable!\n", GREEN);
			Py_INCREF( Py_None );
			return Py_None;
		}

		//this will update the container
		si = container->RemoveItem(Slot,0);
		if (!si) {
			printMessage("GUIScript","Cannot move item, there is something weird!\n", YELLOW);
			Py_INCREF( Py_None );
			return Py_None;
		}
 		Item *item = gamedata->GetItem(si->ItemResRef);
		if (item) {
 			if (core->HasFeature(GF_HAS_PICK_SOUND) && item->ReplacementItem[0]) {
				memcpy(Sound,item->ReplacementItem,8);
			} else {
				GetItemSound(Sound, item->ItemType, item->AnimationType, IS_DROP);
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
			printMessage("GUIScript","Cannot move item, it is undroppable!\n", GREEN);
			Py_INCREF( Py_None );
			return Py_None;
		}

		si = actor->inventory.RemoveItem(core->QuerySlot(Slot));
		if (!si) {
			printMessage("GUIScript","Cannot move item, there is something weird!\n", YELLOW);
			Py_INCREF( Py_None );
			return Py_None;
		}
 		Item *item = gamedata->GetItem(si->ItemResRef);
		if (item) {
 			if (core->HasFeature(GF_HAS_PICK_SOUND) && item->DescriptionIcon[0]) {
				memcpy(Sound,item->DescriptionIcon,8);
			} else {
				GetItemSound(Sound, item->ItemType, item->AnimationType, IS_GET);
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
		core->GetAudioDrv()->Play(Sound);
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetStore__doc,
"GetStore() => dictionary\n\n"
"Returns relevant data of the current store." );

#define STORETYPE_COUNT  7
static int storebuttons[STORETYPE_COUNT][4]={
//store
{STA_BUYSELL,STA_IDENTIFY|STA_OPTIONAL,STA_STEAL|STA_OPTIONAL,STA_CURE|STA_OPTIONAL},
//tavern
{STA_DRINK,STA_BUYSELL|STA_OPTIONAL,STA_IDENTIFY|STA_OPTIONAL,STA_STEAL|STA_OPTIONAL},
//inn
{STA_ROOMRENT,STA_BUYSELL|STA_OPTIONAL,STA_DRINK|STA_OPTIONAL,STA_STEAL|STA_OPTIONAL},
//temple
{STA_CURE, STA_DONATE|STA_OPTIONAL,STA_BUYSELL|STA_OPTIONAL,STA_IDENTIFY|STA_OPTIONAL},
//iwd container
{STA_BUYSELL,-1,-1,-1,},
//no need to steal from your own container (original engine had STEAL instead of DRINK)
{STA_BUYSELL,STA_IDENTIFY|STA_OPTIONAL,STA_DRINK|STA_OPTIONAL,STA_CURE|STA_OPTIONAL},
//gemrb specific store type: (temple 2), added steal, removed identify
{STA_BUYSELL,STA_STEAL|STA_OPTIONAL,STA_DONATE|STA_OPTIONAL,STA_CURE|STA_OPTIONAL} };

//buy/sell, identify, steal, cure, donate, drink, rent
static int storebits[7]={IE_STORE_BUY|IE_STORE_SELL,IE_STORE_ID,IE_STORE_STEAL,
IE_STORE_CURE,IE_STORE_DONATE,IE_STORE_DRINK,IE_STORE_RENT};

static PyObject* GemRB_GetStore(PyObject * /*self*/, PyObject* args)
{
	if (!PyArg_ParseTuple( args, "" )) {
		return AttributeError( GemRB_GetStore__doc );
	}

	Store *store = core->GetCurrentStore();
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

	p = PyTuple_New( 4 );
	j=0;
	for (i = 0; i < 4; i++) {
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
	for (;j<4;j++) {
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
"IsValidStoreItem(pc, idx[, type]) => int\n\n"
"Returns if a pc's inventory item or a store item is valid for buying, selling, identifying or stealing. It also has a flag for selected items. "
"Type is 1 for store items and 0 for PC items." );

static PyObject* GemRB_IsValidStoreItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Slot, ret;
	int type = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &PartyID, &Slot, &type)) {
		return AttributeError( GemRB_IsValidStoreItem__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}

	const char *ItemResRef;
	ieDword Flags;

	if (type) {
		STOItem* si = store->GetItem( Slot );
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
	Item *item = gamedata->GetItem( ItemResRef );
	if (!item) {
		printMessage("GUIScript", " ", LIGHT_RED);
		printf("Invalid resource reference: %s\n", ItemResRef);
		return PyInt_FromLong(0);
	}

	ret = store->AcceptableItemType( item->ItemType, Flags, !type );

	//don't allow putting a bag into itself
	if (!strnicmp(ItemResRef, store->Name, sizeof(ieResRef)) ) {
		ret &= ~IE_STORE_SELL;
	}
	//this is a hack to report on selected items
	if (Flags & IE_INV_ITEM_SELECTED) {
		ret |= IE_STORE_SELECT;
	}
	gamedata->FreeItem( item, ItemResRef, false );
	return PyInt_FromLong(ret);
}

PyDoc_STRVAR( GemRB_SetPurchasedAmount__doc,
"SetPurchasedAmount(idx, amount)\n\n"
"Sets the amount of purchased items of a type.");

static PyObject* GemRB_SetPurchasedAmount(PyObject * /*self*/, PyObject* args)
{
	int Slot, tmp;
	ieDword amount;

	if (!PyArg_ParseTuple( args, "ii", &Slot, &tmp)) {
		return AttributeError( GemRB_SetPurchasedAmount__doc );
	}
	amount = (ieDword) tmp;
	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}
	STOItem* si = store->GetItem( Slot );
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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_ChangeStoreItem__doc,
"ChangeStoreItem(PartyID, Slot, action)=>int\n\n"
"Performs an action of buying, selling, identifying or stealing in a store. "
"It can also toggle the selection of an item." );

static PyObject* GemRB_ChangeStoreItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Slot;
	int action;
	int res = ASI_FAILED;

	if (!PyArg_ParseTuple( args, "iii", &PartyID, &Slot, &action)) {
		return AttributeError( GemRB_ChangeStoreItem__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}
	switch (action) {
	case IE_STORE_BUY: case IE_STORE_STEAL:
	{
		STOItem* si = store->GetItem( Slot );
		if (!si) {
			return RuntimeError("Store item not found!");
		}
		//the amount of items is stored in si->PurchasedAmount
		//it will adjust AmountInStock/PurchasedAmount
		actor->inventory.AddStoreItem(si, action);
		if (si->PurchasedAmount) {
			//was not able to buy it due to lack of space
			res = ASI_FAILED;
			break;
		}
		//if no item remained, remove it
		if (si->AmountInStock) {
			si->Flags &= ~IE_INV_ITEM_SELECTED;
		} else {
			store->RemoveItem( Slot );
		}
		res = ASI_SUCCESS;
		break;
	}
	case IE_STORE_ID:
	{
		CREItem* si = actor->inventory.GetSlotItem( core->QuerySlot(Slot) );
		if (!si) {
			return RuntimeError( "Item not found!" );
		}
		si->Flags |= IE_INV_ITEM_IDENTIFIED;
		res = ASI_SUCCESS;
		break;
	}
	case IE_STORE_SELECT|IE_STORE_BUY:
	{
		STOItem* si = store->GetItem( Slot );
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
	{
		//this is  not removeitem, because the item is just marked
		CREItem* si = actor->inventory.GetSlotItem( core->QuerySlot(Slot) );
		if (!si) {
			return RuntimeError( "Item not found!" );
		}
		si->Flags ^= IE_INV_ITEM_SELECTED;
		res = ASI_SUCCESS;
		break;
	}
	case IE_STORE_SELL:
	{
		//store/bag is at full capacity
		if (store->Capacity && (store->Capacity <= store->GetRealStockSize()) ) {
			printMessage("GUIScript", "Store is full.\n", GREEN);
			res = ASI_FAILED;
			break;
		}
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
		res = ASI_SUCCESS;
		break;
	}
	}
	return PyInt_FromLong(res);
}

PyDoc_STRVAR( GemRB_GetStoreItem__doc,
"GetStoreItem(idx) => dictionary\n\n"
"Returns the store item referenced by the index." );

static PyObject* GemRB_GetStoreItem(PyObject * /*self*/, PyObject* args)
{
	int index;

	if (!PyArg_ParseTuple( args, "i", &index )) {
		return AttributeError( GemRB_GetStoreItem__doc );
	}
	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError("No current store!");
	}
	if (index>=(int) store->GetRealStockSize()) {
		printMessage("GUIScript","Item is not available???",YELLOW);
		Py_INCREF( Py_None );
		return Py_None;
	}
	PyObject* dict = PyDict_New();
	STOItem *si=store->GetItem( index );
	if (!si) {
		printMessage("GUIScript","Item is not available???",YELLOW);
		Py_INCREF( Py_None );
		return Py_None;
	}
	PyDict_SetItemString(dict, "ItemResRef", PyString_FromResRef( si->ItemResRef ));
	PyDict_SetItemString(dict, "Usages0", PyInt_FromLong (si->Usages[0]));
	PyDict_SetItemString(dict, "Usages1", PyInt_FromLong (si->Usages[1]));
	PyDict_SetItemString(dict, "Usages2", PyInt_FromLong (si->Usages[2]));
	PyDict_SetItemString(dict, "Flags", PyInt_FromLong (si->Flags));
	PyDict_SetItemString(dict, "Purchased", PyInt_FromLong (si->PurchasedAmount) );

	int amount;
	if (si->InfiniteSupply==-1) {
		PyDict_SetItemString(dict, "Amount", PyInt_FromLong( -1 ) );
		amount = 100;
	} else {
		amount = si->AmountInStock;
		PyDict_SetItemString(dict, "Amount", PyInt_FromLong( amount ) );
	}

	Item *item = gamedata->GetItem( si->ItemResRef );

	if (!item) {
		printMessage("GUIScript","Item is not available???",YELLOW);
		Py_INCREF( Py_None );
		return Py_None;
	}

	int identified = !!(si->Flags & IE_INV_ITEM_IDENTIFIED);
	PyDict_SetItemString(dict, "ItemName", PyInt_FromLong( (signed) item->GetItemName( (bool) identified )) );
	PyDict_SetItemString(dict, "ItemDesc", PyInt_FromLong( (signed) item->GetItemDesc( (bool) identified )) );

	int price = item->Price * store->SellMarkup / 100;
	//calculate depreciation too
	//store->DepreciationRate, mount

	if (item->StackAmount>1) {
		price *= si->Usages[0];
	}
	//is this correct?
	if (price<1) {
		price = 1;
	}
	PyDict_SetItemString(dict, "Price", PyInt_FromLong( price ) );

	gamedata->FreeItem( item, si->ItemResRef, false );
	return dict;
}

PyDoc_STRVAR( GemRB_GetStoreDrink__doc,
"GetStoreDrink(idx) => dictionary\n\n"
"Returns the drink structure indexed. Returns None if the index is wrong." );

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

static void ReadSpecialSpells()
{
	int i;

	SpecialSpellsCount = 0;
	int table = gamedata->LoadTable("splspec");
	if (table>=0) {
		TableMgr *tab = gamedata->GetTable(table);
		if (!tab) goto table_loaded;
		SpecialSpellsCount = tab->GetRowCount();
		SpecialSpells = (SpellDescType *) malloc( sizeof(SpellDescType) * SpecialSpellsCount);
		for (i=0;i<SpecialSpellsCount;i++) {
			strnlwrcpy(SpecialSpells[i].resref, tab->GetRowName(i),8 );
			//if there are more flags, compose this value into a bitfield
			SpecialSpells[i].value = atoi(tab->QueryField(i,0) );
		}
table_loaded:
		gamedata->DelTable(table);
	}
}

int GetSpecialSpell(ieResRef resref)
{
	if (SpecialSpellsCount==-1) {
		ReadSpecialSpells();
	}
	for (int i=0;i<SpecialSpellsCount;i++) {
		if (!strnicmp(resref, SpecialSpells[i].resref, sizeof(ieResRef))) {
			return SpecialSpells[i].value;
		}
	}
	return 0;
}

//disable spells based on some circumstances
int CheckSpecialSpell(ieResRef resref, Actor *actor)
{
	int sp = GetSpecialSpell(resref);

	//the identify spell is always disabled on the menu
	if (sp&SP_IDENTIFY) {
		return 1;
	}

	//if actor is silenced, and spell cannot be cast in silence, disable it
	if (actor->GetStat(IE_STATE_ID) & STATE_SILENCED ) {
		if (!(sp&SP_SILENCE)) {
			return 1;
		}
	}

	return 0;
}

static void ReadUsedItems()
{
	int i;

	UsedItemsCount = 0;
	int table = gamedata->LoadTable("item_use");
	if (table>=0) {
		TableMgr *tab = gamedata->GetTable(table);
		if (!tab) goto table_loaded;
		UsedItemsCount = tab->GetRowCount();
		UsedItems = (UsedItemType *) malloc( sizeof(UsedItemType) * UsedItemsCount);
		for (i=0;i<UsedItemsCount;i++) {
			strnlwrcpy(UsedItems[i].itemname, tab->GetRowName(i),8 );
			strnlwrcpy(UsedItems[i].username, tab->QueryField(i,0),32 );
			//this is an strref
			UsedItems[i].value = atoi(tab->QueryField(i,1) );
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
		TableMgr *tab = gamedata->GetTable(table);
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
			TableMgr *tab = gamedata->GetTable(table);
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
"GetStoreCure(idx) => dictionary\n\n"
"Returns the cure structure indexed. Returns None if the index is wrong." );

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
"ExecuteString(String[,PC])\n\n"
"Executes an In-Game Script Action in the current Area Script Context. "
"If a number was given, it will execute the action in the numbered PC's context." );

static PyObject* GemRB_ExecuteString(PyObject * /*self*/, PyObject* args)
{
	char* String;
	int actornum=0;

	if (!PyArg_ParseTuple( args, "s|i", &String, &actornum )) {
		return AttributeError( GemRB_ExecuteString__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	if (actornum) {
		Actor *pc = game->FindPC(actornum);
		if (pc) {
			GameScript::ExecuteString( pc, String );
		} else {
			printMessage("GUIScript","Player not found!\n", YELLOW);
		}
	} else {
		GameScript::ExecuteString( game->GetCurrentArea( ), String );
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_RunEventHandler__doc,
"RunEventHandler(String[, error])\n\n"
"Executes a GUIScript event handler function named String. "
"If error is set to nonzero, then a missing handler will cause an error." );

static PyObject* GemRB_RunEventHandler(PyObject * /*self*/, PyObject* args)
{
	char* String;
	int error = 0;

	if (!PyArg_ParseTuple( args, "s|i", &String, error )) {
		return AttributeError( GemRB_RunEventHandler__doc );
	}
	core->GetGUIScriptEngine()->RunFunction( String, error );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_EvaluateString__doc,
"EvaluateString(String)\n\n"
"Evaluate an In-Game Script Trigger in the current Area Script Context." );

static PyObject* GemRB_EvaluateString(PyObject * /*self*/, PyObject* args)
{
	char* String;

	if (!PyArg_ParseTuple( args, "s", &String )) {
		return AttributeError( GemRB_EvaluateString__doc );
	}
	if (GameScript::EvaluateString( core->GetGame()->GetCurrentArea( ), String )) {
		printf( "%s returned True\n", String );
	} else {
		printf( "%s returned False\n", String );
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_UpdateMusicVolume__doc,
"UpdateMusicVolume()\n\n"
"Update music volume on-the-fly." );

static PyObject* GemRB_UpdateMusicVolume(PyObject * /*self*/, PyObject* /*args*/)
{
	core->GetAudioDrv()->UpdateVolume( GEM_SND_VOL_MUSIC );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_UpdateAmbientsVolume__doc,
"UpdateAmbientsVolume()\n\n"
"Update ambients volume on-the-fly." );

static PyObject* GemRB_UpdateAmbientsVolume(PyObject * /*self*/, PyObject* /*args*/)
{
	core->GetAudioDrv()->UpdateVolume( GEM_SND_VOL_AMBIENTS );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetCurrentArea__doc,
"GetCurrentArea()=>resref\n\n"
"Returns current area's ResRef." );

static PyObject* GemRB_GetCurrentArea(PyObject * /*self*/, PyObject* /*args*/)
{
	return PyString_FromString( core->GetGame()->CurrentArea );
}

PyDoc_STRVAR( GemRB_MoveToArea__doc,
"MoveToArea(resref)\n\n"
"Moves the selected characters to the area." );

static PyObject* GemRB_MoveToArea(PyObject * /*self*/, PyObject* args)
{
	const char *String;

	if (!PyArg_ParseTuple( args, "s", &String )) {
		return AttributeError( GemRB_MoveToArea__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Map* map2 = game->GetMap(String, true);
	if (!map2) {
		return RuntimeError( "Map not found!" );
	}
	int i = game->GetPartySize(true);
	while (i--) {
		Actor* actor = game->GetPC(i, false);
		if (!actor->Selected) {
			continue;
		}
		Map* map1 = actor->GetCurrentArea();
		if (map1) {
			map1->RemoveActor( actor );
		}
		map2->AddActor( actor );
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetMemorizableSpellsCount__doc,
"GetMemorizableSpellsCount(PartyID, SpellType, Level [,Bonus])=>int\n\n"
"Returns number of memorizable spells of given type and level in PC's spellbook." );

static PyObject* GemRB_GetMemorizableSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level, Bonus=1;

	if (!PyArg_ParseTuple( args, "iii|i", &PartyID, &SpellType, &Level, &Bonus )) {
		return AttributeError( GemRB_GetMemorizableSpellsCount__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	//this isn't in the actor's spellbook, handles Wisdom
	return PyInt_FromLong(actor->spellbook.GetMemorizableSpellsCount( (ieSpellType) SpellType, Level, (bool) Bonus ) );
}

PyDoc_STRVAR( GemRB_SetMemorizableSpellsCount__doc,
"SetMemorizableSpellsCount(PartyID, Value, SpellType, Level, [Bonus])=>int\n\n"
"Sets number of memorizable spells of given type and level in PC's spellbook." );

static PyObject* GemRB_SetMemorizableSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int PartyID, Value, SpellType, Level;

	if (!PyArg_ParseTuple( args, "iiii", &PartyID, &Value, &SpellType, &Level)) {
		return AttributeError( GemRB_SetMemorizableSpellsCount__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	//the bonus increased value (with wisdom too) is handled by the core
	actor->spellbook.SetMemorizableSpellsCount( Value, (ieSpellType) SpellType, Level, 0 );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetKnownSpellsCount__doc,
"GetKnownSpellsCount(PartyID, SpellType, Level)=>int\n\n"
"Returns number of known spells of given type and level in PC's spellbook." );

static PyObject* GemRB_GetKnownSpellsCount(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level;

	if (!PyArg_ParseTuple( args, "iii", &PartyID, &SpellType, &Level )) {
		return AttributeError( GemRB_GetKnownSpellsCount__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	return PyInt_FromLong(actor->spellbook.GetKnownSpellsCount( SpellType, Level ) );
}

PyDoc_STRVAR( GemRB_GetKnownSpell__doc,
"GetKnownSpell(PartyID, SpellType, Level, Index)=>dict\n\n"
"Returns dict with specified known spell from PC's spellbook." );

static PyObject* GemRB_GetKnownSpell(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level, Index;

	if (!PyArg_ParseTuple( args, "iiii", &PartyID, &SpellType, &Level, &Index )) {
		return AttributeError( GemRB_GetKnownSpell__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found!" );
	}

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
"GetMemorizedSpellsCount(PartyID, SpellType[, Level])=>int\n\n"
"Returns number of spells of given type and level in PartyID's memory. "
"If level is omitted then it returns the number of distinct spells memorised." );

static PyObject* GemRB_GetMemorizedSpellsCount(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level = -1;

	if (!PyArg_ParseTuple( args, "ii|i", &PartyID, &SpellType, &Level )) {
		return AttributeError( GemRB_GetMemorizedSpellsCount__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found!" );
	}

	if (Level<0) {
		return PyInt_FromLong( actor->spellbook.GetSpellInfoSize( SpellType ) );
	} else {
		return PyInt_FromLong( actor->spellbook.GetMemorizedSpellsCount( SpellType, Level ) );
	}
}

PyDoc_STRVAR( GemRB_GetMemorizedSpell__doc,
"GetMemorizedSpell(PartyID, SpellType, Level, Index)=>dict\n\n"
"Returns dict with specified memorized spell from PC's spellbook." );

static PyObject* GemRB_GetMemorizedSpell(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level, Index;

	if (!PyArg_ParseTuple( args, "iiii", &PartyID, &SpellType, &Level, &Index )) {
		return AttributeError( GemRB_GetMemorizedSpell__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found!" );
	}

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
"GetSpell(ResRef[, silent])=>dict\n\n"
"Returns dict with specified spell. Verbose by default." );

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
	PyDict_SetItemString(dict, "SpellName", PyInt_FromLong ((signed) spell->SpellName));
	PyDict_SetItemString(dict, "SpellDesc", PyInt_FromLong ((signed) spell->SpellDesc));
	PyDict_SetItemString(dict, "SpellbookIcon", PyString_FromResRef (spell->SpellbookIcon));
	PyDict_SetItemString(dict, "SpellExclusion", PyInt_FromLong (spell->ExclusionSchool)); //this will list school exclusions and alignment
	PyDict_SetItemString(dict, "SpellDivine", PyInt_FromLong (spell->PriestType)); //this will tell apart a priest spell from a druid spell
	PyDict_SetItemString(dict, "SpellSchool", PyInt_FromLong (spell->PrimaryType));
	PyDict_SetItemString(dict, "SpellType", PyInt_FromLong (spell->SecondaryType));
	PyDict_SetItemString(dict, "SpellLevel", PyInt_FromLong (spell->SpellLevel));
	gamedata->FreeSpell( spell, ResRef, false );
	return dict;
}


PyDoc_STRVAR( GemRB_LearnSpell__doc,
"LearnSpell(PartyID, SpellResRef[, Flags])=>int\n\n"
"Learns specified spell. Returns 0 on success." );

static PyObject* GemRB_LearnSpell(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	const char *Spell;
	int Flags=0;

	if (!PyArg_ParseTuple( args, "is|i", &PartyID, &Spell, &Flags )) {
		return AttributeError( GemRB_LearnSpell__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	int ret = actor->LearnSpell( Spell, Flags ); // returns 0 on success
	if (!ret) core->SetEventFlag( EF_ACTION );
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_DispelEffect__doc,
"DispelEffect(PartyID, EffectName, Parameter2)\n\n"
"Removes all effects from target whose opcode and second parameter matches the arguments." );

static EffectRef work_ref;

static PyObject* GemRB_DispelEffect(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Parameter2;
	const char *EffectName;

	if (!PyArg_ParseTuple( args, "isi", &PartyID, &EffectName, &Parameter2 )) {
		return AttributeError( GemRB_DispelEffect__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found!" );
	}

	work_ref.Name=EffectName;
	work_ref.EffText=-1;
	actor->fxqueue.RemoveAllEffectsWithParam(work_ref, Parameter2);

	Py_INCREF( Py_None );
	return Py_None;
}


PyDoc_STRVAR( GemRB_RemoveEffects__doc,
"RemoveEffects(PartyID, SpellResRef)\n\n"
"Removes all effects from target whose source is SpellResRef." );

static PyObject* GemRB_RemoveEffects(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	const char * SpellResRef;

	if (!PyArg_ParseTuple( args, "is", &PartyID, &SpellResRef )) {
		return AttributeError( GemRB_RemoveEffects__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found!" );
	}

	actor->fxqueue.RemoveAllEffects(SpellResRef);

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_RemoveSpell__doc,
"RemoveSpell(PartyID, SpellType, Level, Index)=>bool\n\n"
"Removes specified known spell. Returns 1 on success." );

static PyObject* GemRB_RemoveSpell(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level, Index;

	if (!PyArg_ParseTuple( args, "iiii", &PartyID, &SpellType, &Level, &Index )) {
		return AttributeError( GemRB_RemoveSpell__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found!" );
	}

	CREKnownSpell* ks = actor->spellbook.GetKnownSpell( SpellType, Level, Index );
	if (! ks) {
		return RuntimeError( "Spell not known!" );
	}

	return PyInt_FromLong( actor->spellbook.RemoveSpell( ks ) );
}

PyDoc_STRVAR( GemRB_RemoveItem__doc,
"RemoveItem(PartyID, Slot[, Count])=>bool\n\n"
"Removes (or decreases the charges) of a specified item. Returns 1 on success." );

static PyObject* GemRB_RemoveItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Slot;
	int Count = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &PartyID, &Slot, &Count )) {
		return AttributeError( GemRB_RemoveItem__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found!" );
	}

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
"MemorizeSpell(PartyID, SpellType, Level, Index)=>bool\n\n"
"Memorizes specified known spell. Returns 1 on success." );

static PyObject* GemRB_MemorizeSpell(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level, Index;

	if (!PyArg_ParseTuple( args, "iiii", &PartyID, &SpellType, &Level, &Index )) {
		return AttributeError( GemRB_MemorizeSpell__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found!" );
	}

	CREKnownSpell* ks = actor->spellbook.GetKnownSpell( SpellType, Level, Index );
	if (! ks) {
		return RuntimeError( "Spell not found!" );
	}

	bool enabled = false;
	if (SpellType == IE_SPELL_TYPE_INNATE) enabled = true;

	return PyInt_FromLong( actor->spellbook.MemorizeSpell( ks, enabled ) );
}


PyDoc_STRVAR( GemRB_UnmemorizeSpell__doc,
"UnmemorizeSpell(PartyID, SpellType, Level, Index)=>bool\n\n"
"Unmemorizes specified known spell. Returns 1 on success." );

static PyObject* GemRB_UnmemorizeSpell(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level, Index;

	if (!PyArg_ParseTuple( args, "iiii", &PartyID, &SpellType, &Level, &Index )) {
		return AttributeError( GemRB_UnmemorizeSpell__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found!" );
	}

	CREMemorizedSpell* ms = actor->spellbook.GetMemorizedSpell( SpellType, Level, Index );
	if (! ms) {
		return RuntimeError( "Spell not found!" );
	}

	return PyInt_FromLong( actor->spellbook.UnmemorizeSpell( ms ) );
}

PyDoc_STRVAR( GemRB_GetSlotItem__doc,
"GetSlotItem(PartyID, slot)=>dict\n\n"
"Returns dict with specified slot item from PC's inventory or the dragged item if PartyID is 0." );

static PyObject* GemRB_GetSlotItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Slot;

	if (!PyArg_ParseTuple( args, "ii", &PartyID, &Slot)) {
		return AttributeError( GemRB_GetSlotItem__doc );
	}
	CREItem *si;
	int header = -1;

	if (PartyID==0) {
		si = core->GetDraggedItem();
	} else {
		Game *game = core->GetGame();
		if (!game) {
			return RuntimeError( "No game loaded!" );
		}
		Actor *actor = game->FindPC( PartyID );
		if (!actor) {
			return RuntimeError( "Actor not found" );
		}

		Slot = core->QuerySlot(Slot);
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
"ChangeItemFlag(PartyID, slot, flags, op) => bool\n\n"
"Changes an item flag of a player character in inventory slot. Returns false if failed." );

static PyObject* GemRB_ChangeItemFlag(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Slot, Flags, Mode;

	if (!PyArg_ParseTuple( args, "iiii", &PartyID, &Slot, &Flags, &Mode)) {
		return AttributeError( GemRB_ChangeItemFlag__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor *actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	if (actor->inventory.ChangeItemFlag(core->QuerySlot(Slot), Flags, Mode)) {
		return PyInt_FromLong(1);
	}
	return PyInt_FromLong(0);
}


PyDoc_STRVAR( GemRB_CanUseItemType__doc,
"CanUseItemType( slottype, itemname[, actor])=>bool\n\n"
"Checks the itemtype vs. slottype, and also checks the usability flags vs. Actor's stats (alignment, class, race, kit etc.)" );

static PyObject* GemRB_CanUseItemType(PyObject * /*self*/, PyObject* args)
{
	int SlotType, PartyID;
	const char *ItemName;

	PartyID = 0;
	if (!PyArg_ParseTuple( args, "is|i", &SlotType, &ItemName, &PartyID)) {
		return AttributeError( GemRB_CanUseItemType__doc );
	}
	if (!ItemName[0]) {
		return PyInt_FromLong(0);
	}
	Item *item = gamedata->GetItem(ItemName);
	if (!item) {
		return PyInt_FromLong(0);
	}
	Actor* actor = 0;
	if (PartyID) {
		Game *game = core->GetGame();
		if (!game) {
			return RuntimeError( "No game loaded!" );
		}
		actor = game->FindPC( PartyID );
		if (!actor) {
			return RuntimeError( "Actor not found" );
		}
	}

	int ret=core->CanUseItemType(SlotType, item, actor, false);
	gamedata->FreeItem(item, ItemName, false);
	return PyInt_FromLong(ret);
}


PyDoc_STRVAR( GemRB_GetSlots__doc,
"GetSlots(PartyID, SlotType[,flag])=>dict\n\n"
"Returns a tuple of slots of the inventory of a PC matching the slot type criteria.\n"
"If the flag is >0, it will ignore empty slots.\n"
"If the flag is <0, it will ignore filled slots.\n"
"If the flag is 0, it will return all slots.\n"
"The default is 1." );

static PyObject* GemRB_GetSlots(PyObject * /*self*/, PyObject* args)
{
	int SlotType, Count, MaxCount, PartyID;
	int flag = 1;

	if (!PyArg_ParseTuple( args, "ii|i", &PartyID, &SlotType, &flag)) {
		return AttributeError( GemRB_GetSlots__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

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

PyDoc_STRVAR( GemRB_GetItem__doc,
"GetItem(ResRef)=>dict\n\n"
"Returns dict with specified item." );

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

	Item* item = gamedata->GetItem(ResRef);
	if (item == NULL) {
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
	PyDict_SetItemString(dict, "StackAmount", PyInt_FromLong (item->StackAmount));
	PyDict_SetItemString(dict, "Dialog", PyString_FromResRef (item->Dialog));
	PyDict_SetItemString(dict, "DialogName", PyInt_FromLong ((signed)item->DialogName));
	PyDict_SetItemString(dict, "Price", PyInt_FromLong (item->Price));
	PyDict_SetItemString(dict, "Type", PyInt_FromLong (item->ItemType));
	PyDict_SetItemString(dict, "AnimationType", PyString_FromAnimID(item->AnimationType));
	PyDict_SetItemString(dict, "Exclusion", PyInt_FromLong(item->ItemExcl));
	PyDict_SetItemString(dict, "LoreToID", PyInt_FromLong(item->LoreToID));

	int ehc = item->ExtHeaderCount;

	PyObject* tooltiptuple = PyTuple_New(ehc);
	for(int i=0;i<ehc;i++) {
		int tip = core->GetItemTooltip(ResRef, i, 1);
		PyTuple_SetItem(tooltiptuple, i, PyInt_FromLong(tip));
	}

	PyDict_SetItemString(dict, "Tooltips", tooltiptuple);

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
		if (f->Opcode!=(ieDword) fx_learn_spell_ref.EffText) {
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

void DragItem(CREItem *si)
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

int CheckRemoveItem(Actor *actor, CREItem *si)
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
		if (UsedItems[i].username[0] && strnicmp(UsedItems[i].username, actor->GetScriptName(), 32) ) {
			continue;
		}
		core->DisplayString(UsedItems[i].value,0xffffff, IE_STR_SOUND);
		return 1;
	}
	return 0;
}

CREItem *TryToUnequip(Actor *actor, unsigned int Slot, unsigned int Count)
{
	//we should use getslotitem, because
	//getitem would remove the item from the inventory!
	CREItem *si = actor->inventory.GetSlotItem(Slot);
	if (!si) {
		return NULL;
	}

	//it is always possible to put these items into the inventory
	if (!(core->QuerySlotType(Slot)&SLOT_INVENTORY)) {
		if (CheckRemoveItem(actor, si)) {
			return NULL;
		}
	}
	///fixme: make difference between cursed/unmovable
	if (! actor->inventory.UnEquipItem( Slot, false )) {
		// Item is currently undroppable/cursed
		core->DisplayConstantString(STR_CANT_DROP_ITEM,0xffffff);
		return NULL;
	}
	si = actor->inventory.RemoveItem( Slot, Count );
	return si;
}

PyDoc_STRVAR( GemRB_DragItem__doc,
"DragItem(PartyID, Slot, ResRef, [Count=0, Type])\n\n"
"Start dragging specified item, if Slot is negative, drag the PartyID portrait instead." );

static PyObject* GemRB_DragItem(PyObject * /*self*/, PyObject* args)
{
	ieResRef Sound;
	int PartyID, Slot, Count = 0, Type = 0;
	const char *ResRef;

	if (!PyArg_ParseTuple( args, "iis|ii", &PartyID, &Slot, &ResRef, &Count, &Type)) {
		return AttributeError( GemRB_DragItem__doc );
	}

	// FIXME
	// we should Drop the Dragged item in place of the current item
	// but only if the current item is draggable, tough!
	if (core->GetDraggedItem()) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	//allow -1,-1
	if (!actor && ( PartyID || ResRef[0]) ) {
		return RuntimeError( "Actor not found" );
	}

	//dragging a portrait
	if (!ResRef[0]) {
		core->SetDraggedPortrait(PartyID, Slot);
		Py_INCREF( Py_None );
		return Py_None;
	}

	if ((unsigned int) Slot>core->GetInventorySize()) {
		return AttributeError( "Invalid slot" );
	}
	CREItem* si;
	if (Type) {
		Map *map = actor->GetCurrentArea();
		Container *cc = map->GetPile(actor->Pos);
		if (!cc) {
			return RuntimeError( "No current container" );
		}
		si = cc->RemoveItem(Slot, Count);
	} else {
		si = TryToUnequip( actor, core->QuerySlot(Slot), Count );
		actor->RefreshEffects(NULL);
printf("Actor ac:%d\n", actor->Modified[IE_ARMORCLASS]);
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
			memcpy(Sound,item->DescriptionIcon,8);
		} else {
			GetItemSound(Sound, item->ItemType, item->AnimationType, IS_GET);
		}
		gamedata->FreeItem(item, si->ItemResRef,0);
	}
	if (Sound[0]) {
		core->GetAudioDrv()->Play(Sound);
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
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_DropDraggedItem__doc,
"DropDraggedItem(PartyID, Slot)=>int\n\n"
"Put currently dragged item to specified PC and slot. "
"If Slot==-1, puts it to a first usable slot. "
"If Slot==-2, puts it to a ground pile. "
"If Slot==-3, puts it to the first empty inventory slot. "
"Returns 0 (unsuccessful), 1 (partial success) or 2 (complete success)."
"Can also return 3 (swapped item)\n" );

static PyObject* GemRB_DropDraggedItem(PyObject * /*self*/, PyObject* args)
{
	ieResRef Sound;
	int PartyID, Slot;

	if (!PyArg_ParseTuple( args, "ii", &PartyID, &Slot)) {
		return AttributeError( GemRB_DropDraggedItem__doc );
	}

	// FIXME
	if (core->GetDraggedItem() == NULL) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	int res;

	if (Slot==-2) {
		Map *map = actor->GetCurrentArea();
		Container *cc = map->GetPile(actor->Pos);
		if (!cc) {
			return RuntimeError( "No current container" );
		}
		CREItem *si = core->GetDraggedItem();
		res = cc->AddItem(si);
 		Item *item = gamedata->GetItem(si->ItemResRef);
		if (item) {
 			if (core->HasFeature(GF_HAS_PICK_SOUND) && item->ReplacementItem[0]) {
				memcpy(Sound,item->ReplacementItem,8);
			} else {
				GetItemSound(Sound, item->ItemType, item->AnimationType, IS_DROP);
			}
			gamedata->FreeItem(item, si->ItemResRef,0);
			if (Sound[0]) {
				core->GetAudioDrv()->Play(Sound);
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
	CREItem * slotitem = core->GetDraggedItem();
	Item *item = gamedata->GetItem( slotitem->ItemResRef );
	if (!item) {
		return PyInt_FromLong( -1 );
	}

	// can't equip item because of similar already equipped
	if (Effect) {
		if (item->ItemExcl & actor->inventory.GetEquipExclusion(Slot)) {
			core->DisplayConstantString(STR_ITEMEXCL, 0xf0f0f0);
			//freeing the item before returning
			gamedata->FreeItem( item, slotitem->ItemResRef, false );
			return PyInt_FromLong( 0 );
		}
	}

	// can't equip item because it is not identified
	if ( (Slottype == SLOT_ITEM) && !(slotitem->Flags&IE_INV_ITEM_IDENTIFIED)) {
		ITMExtHeader *eh = item->GetExtHeader(0);
		if (eh && eh->IDReq) {
			core->DisplayConstantString(STR_ITEMID, 0xf0f0f0);
			gamedata->FreeItem( item, slotitem->ItemResRef, false );
			return PyInt_FromLong( 0 );
		}
	}

	//CanUseItemType will check actor's class bits too
	Slottype = core->CanUseItemType (Slottype, item, actor, true);
	//resolve the equipping sound, it needs to be resolved before
	//the item is freed
	if (core->HasFeature(GF_HAS_PICK_SOUND) && item->ReplacementItem[0]) {
		memcpy(Sound, item->ReplacementItem, 9);
	} else {
		GetItemSound(Sound, item->ItemType, item->AnimationType, IS_DROP);
	}

	//freeing the item before returning
	gamedata->FreeItem( item, slotitem->ItemResRef, false );
	if ( !Slottype) {
		return PyInt_FromLong( 0 );
	}
	res = actor->inventory.AddSlotItem( slotitem, Slot, Slottype );
	if (res) {
		//release it only when fully placed
		if (res==ASI_SUCCESS) {
			core->ReleaseDraggedItem ();
		}
		//EquipItem (in AddSlotItem) already called RefreshEffects
		actor->ReinitQuickSlots();
	//couldn't place item there, try swapping (only if slot is explicit)
	} else if ( Slot >= 0 ) {
		//swapping won't cure this
		res = actor->inventory.WhyCantEquip(Slot, slotitem->Flags&IE_INV_ITEM_TWOHANDED);
		if (res) {
			core->DisplayConstantString(res,0xffffff);
			return PyInt_FromLong( 0 );
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
			actor->ReinitQuickSlots();
			core->SetEventFlag(EF_SELECTION);
		} else {
			res = ASI_FAILED;
		}
	}
	if (Sound[0]) {
		core->GetAudioDrv()->Play(Sound);
	}
	return PyInt_FromLong( res );
}

PyDoc_STRVAR( GemRB_IsDraggingItem__doc,
"IsDraggingItem()=>int\n\n"
"Returns 1 if we are dragging some item.\n"
"Returns 2 if we are dragging a portrait." );

static PyObject* GemRB_IsDraggingItem(PyObject * /*self*/, PyObject* /*args*/)
{
	if (core->GetDraggedPortrait()) {
		return PyInt_FromLong(2);
	}
	return PyInt_FromLong( core->GetDraggedItem() != NULL );
}

PyDoc_STRVAR( GemRB_GetSystemVariable__doc,
"GetSystemVariable(Variable)=>int\n\n"
"Returns the named Interface attribute." );

static PyObject* GemRB_GetSystemVariable(PyObject * /*self*/, PyObject* args)
{
	int Variable, value;

	if (!PyArg_ParseTuple( args, "i", &Variable)) {
		return AttributeError( GemRB_GetSystemVariable__doc );
	}
	switch(Variable) {
		case SV_BPP: value = core->Bpp; break;
		case SV_WIDTH: value = core->Width; break;
		case SV_HEIGHT: value = core->Height; break;
		default: value = -1; break;
	}
	return PyInt_FromLong( value );
}

PyDoc_STRVAR( GemRB_CreateItem__doc,
"CreateItem(PartyID, ItemResRef, [SlotID, Charge0, Charge1, Charge2])\n\n"
"Creates Item in the inventory of the player character.");

static PyObject* GemRB_CreateItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	int SlotID=-1;
	int Charge0=1,Charge1=0,Charge2=0;
	const char *ItemResRef;

	if (!PyArg_ParseTuple( args, "is|iiii", &PartyID, &ItemResRef, &SlotID, &Charge0, &Charge1, &Charge2)) {
		return AttributeError( GemRB_CreateItem__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

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
			CREItem *item = CreateCreItem(ItemResRef, Charge0, Charge1, Charge2 );
			if (item) {
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
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetMapnote__doc,
"SetMapnote(X, Y, color, Text)\n\n"
"Adds or removes a mapnote.");

static PyObject* GemRB_SetMapnote(PyObject * /*self*/, PyObject* args)
{
	int x,y;
	int color=0;
	const char *txt=NULL;

	if (!PyArg_ParseTuple( args, "ii|is", &x, &y, &color, &txt)) {
		return AttributeError( GemRB_SetMapnote__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Map *map = game->GetCurrentArea();
	if (!map) {
		return RuntimeError( "No current area" );
	}
	ieStrRef strref;
	Point point;
	point.x=x;
	point.y=y;
	if (txt && txt[0]) {
		char* newvalue = ( char* ) malloc( strlen( txt ) + 1 ); //duplicating the string
		strcpy( newvalue, txt );
		MapNote *old = map->GetMapNote(point);
		if (old) strref = old->strref;
		else strref = 0xffffffff;
		map->AddMapNote(point, color, newvalue, strref);
	} else {
		map->RemoveMapNote(point);
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_CreateCreature__doc,
"CreateCreature(PartyID, CreResRef)\n\n"
"Creates Creature in vicinity of a player character.");

static PyObject* GemRB_CreateCreature(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	const char *CreResRef;

	if (!PyArg_ParseTuple( args, "is", &PartyID, &CreResRef)) {
		return AttributeError( GemRB_CreateCreature__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	Map *map=game->GetCurrentArea();
	if (!map) {
		return RuntimeError( "No current area" );
	}

	map->SpawnCreature(actor->Pos, CreResRef, 10);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_RevealArea__doc,
"RevealArea(x, y, radius, type)\n\n"
"Reveals part of the area.");

static PyObject* GemRB_RevealArea(PyObject * /*self*/, PyObject* args)
{
	int x,y;
	int radius;
	int Value;

	if (!PyArg_ParseTuple( args, "iiii", &x, &y, &radius, &Value)) {
		return AttributeError( GemRB_RevealArea__doc );
	}

	Point p(x,y);
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Map *map=game->GetCurrentArea();
	if (!map) {
		return RuntimeError( "No current area" );
	}
	map->ExploreMapChunk( p, radius, Value );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_ExploreArea__doc,
"ExploreArea([bitvalue=-1])\n\n"
"Explores or unexplores whole area.");

static PyObject* GemRB_ExploreArea(PyObject * /*self*/, PyObject* args)
{
	int Value=-1;

	if (!PyArg_ParseTuple( args, "|i", &Value)) {
		return AttributeError( GemRB_ExploreArea__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Map *map=game->GetCurrentArea();
	if (!map) {
		return RuntimeError( "No current area" );
	}
	map->Explore( Value );

	Py_INCREF( Py_None );
	return Py_None;
}


PyDoc_STRVAR( GemRB_GetRumour__doc,
"GetRumour(percent, ResRef) => ieStrRef\n\n"
"Returns a string to a rumour message. ResRef is a dialog resource.");

static PyObject* GemRB_GetRumour(PyObject * /*self*/, PyObject* args)
{
	int percent;
	const char *ResRef;

	if (!PyArg_ParseTuple( args, "is", &percent, &ResRef)) {
		return AttributeError( GemRB_GetRumour__doc );
	}
	if (rand()%100 >= percent) {
		return PyInt_FromLong( -1 );
	}

	ieStrRef strref = core->GetRumour( ResRef );
	return PyInt_FromLong( strref );
}

PyDoc_STRVAR( GemRB_GamePause__doc,
"GamePause(Pause, Quiet)\n\n"
"Pause or unpause the game or just toggle the pause.");

static PyObject* GemRB_GamePause(PyObject * /*self*/, PyObject* args)
{
	int pause, quiet;

	if (!PyArg_ParseTuple( args, "ii", &pause, &quiet)) {
		return AttributeError( GemRB_GamePause__doc );
	}

	GameControl *gc = core->GetGameControl();
	if (gc) {
		//this will trigger when pause is not 0 or 1
		if ((unsigned int) pause > 1) {
			pause = ~gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS;
		}

		if (pause) {
			gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BM_OR);
		} else {
			gc->SetDialogueFlags(DF_FREEZE_SCRIPTS, BM_NAND);
		}
		if (!quiet) {
			if (gc->GetDialogueFlags()&DF_FREEZE_SCRIPTS) {
				core->DisplayConstantString(STR_PAUSED,0xff0000);
			} else {
				core->DisplayConstantString(STR_UNPAUSED,0xff0000);
			}
		}
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_CheckFeatCondition__doc,
"CheckFeatCondition(partyslot, a_stat, a_value, b_stat, b_value, c_stat, c_value, d_stat, d_value[,a_op, b_op, c_op, d_op]) => bool\n\n"
"Checks if actor in partyslot is eligible for a feat, the formula is: (stat[a]~a or stat[b]~b) and (stat[c]~c or stat[d]~d). Where ~ is a relational operator. If the operators are omitted, the default operator is <=.");

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

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	Actor *actor = core->GetGame()->FindPC(v[0]);
	if (!actor) {
		return RuntimeError( "Actor not found" );
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

		//conversion is needed, because the interface is more generic
		GUIScript *gs = (GUIScript *) core->GetGUIScriptEngine();

		PyObject *pValue = gs->CallbackFunction(fname, param);

		/* we created this parameter, now we don't need it*/
		Py_DECREF( param );
		if (pValue) {
			/* don't think we need any incref */
			return pValue;
		}
		return RuntimeError( "Callback failed" );
	}

	bool ret = true;

	if (v[1] || v[2])
		ret = CheckStat(actor, v[1], v[2], v[9]);

	if (v[3] || v[4])
		ret |= CheckStat(actor, v[3], v[4], v[10]);

	if (!ret)
		goto endofquest;

	if (v[5] || v[6])
		// no | because the formula is (a|b) & (c|d)
		ret = CheckStat(actor, v[5], v[6], v[11]);

	if (v[7] || v[8])
		ret |= CheckStat(actor, v[7], v[8], v[12]);

endofquest:
	if (ret) {
		Py_INCREF( Py_True );
		return Py_True;
	} else {
		Py_INCREF( Py_False );
		return Py_False;
	}
}

PyDoc_STRVAR( GemRB_GetAbilityBonus__doc,
"GetAbilityBonus(stat, column, value[, ex])\n\n"
"Returns an ability bonus value based on various .2da files.");

static PyObject* GemRB_GetAbilityBonus(PyObject * /*self*/, PyObject* args)
{
	int stat, column, value, ex = 0;
	int ret;

	if (!PyArg_ParseTuple( args, "iii|i", &stat, &column, &value, &ex)) {
		return AttributeError( GemRB_GetAbilityBonus__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor *actor = game->FindPC(game->GetSelectedPCSingle());
	if (!actor) {
		return RuntimeError( "Actor not found" );
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
		default:
			return RuntimeError( "Invalid ability!");
	}
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_LeaveParty__doc,
"LeaveParty(Slot [,dialog])\n\n"
"Makes player in Slot leave party, and initiate dialog if demanded." );

static PyObject* GemRB_LeaveParty(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, initDialog;

	if (!PyArg_ParseTuple( args, "i|i", &PlayerSlot, &initDialog )) {
		return AttributeError( GemRB_LeaveParty__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor *actor = game->FindPC(PlayerSlot);
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	if (initDialog) {
		if (initDialog == 2)
		GameScript::SetLeavePartyDialogFile(actor, NULL);
		if(actor->GetBase(IE_HITPOINTS) > 0) {
			char Tmp[40];
			actor->ClearPath();
			actor->ClearActions();
			strncpy(Tmp,"Dialogue([PC])",sizeof(Tmp) );
			actor->AddAction( GenerateAction(Tmp) );
		}
	}
	game->LeaveParty (actor);

	Py_INCREF( Py_None );
	return Py_None;
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
	TableMgr *tab = gamedata->GetTable( table );
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
	btn->SetImage( which, tspr );
}

PyDoc_STRVAR( GemRB_Button_SetActionIcon__doc,
"SetActionIcon(Window, Button, ActionIndex[, Function])\n\n"
"Sets up an action button. The ActionIndex should be less than 34." );

static PyObject* SetActionIcon(int WindowIndex, int ControlIndex, int Index, int Function)
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
		btn->SetImage( IE_GUI_BUTTON_UNPRESSED, 0 );
		btn->SetImage( IE_GUI_BUTTON_PRESSED, 0 );
		btn->SetImage( IE_GUI_BUTTON_SELECTED, 0 );
		btn->SetImage( IE_GUI_BUTTON_DISABLED, 0 );
		btn->SetFlags( IE_GUI_BUTTON_NO_IMAGE, BM_SET );
		btn->SetEvent( IE_GUI_BUTTON_ON_PRESS, "" );
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
		return RuntimeError( "BAM not found" );
	}
	packtype row;

	row.data = GUIAction[Index];
	SetButtonCycle(bam, btn, (char) row.bytes[0], IE_GUI_BUTTON_UNPRESSED);
	SetButtonCycle(bam, btn, (char) row.bytes[1], IE_GUI_BUTTON_PRESSED);
	SetButtonCycle(bam, btn, (char) row.bytes[2], IE_GUI_BUTTON_SELECTED);
	SetButtonCycle(bam, btn, (char) row.bytes[3], IE_GUI_BUTTON_DISABLED);
	btn->SetFlags( IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_PICTURE, BM_NAND );
	ieVariable Event;
	snprintf(Event,sizeof(Event)-1, "Action%sPressed", GUIEvent[Index]);
	btn->SetEvent( IE_GUI_BUTTON_ON_PRESS, Event );
	//cannot make this const, because it will be freed
	char *txt = core->GetString( GUITooltip[Index] );
	//will free txt
	SetFunctionTooltip(WindowIndex, ControlIndex, txt, Function);
	//no incref
	return Py_None;
}

static PyObject* GemRB_Button_SetActionIcon(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Index;
	int Function = 0;

	if (!PyArg_ParseTuple( args, "iii|i", &WindowIndex, &ControlIndex, &Index, &Function )) {
		return AttributeError( GemRB_Button_SetActionIcon__doc );
	}

	PyObject* ret = SetActionIcon(WindowIndex, ControlIndex, Index, Function);
	if (ret) {
		Py_INCREF(ret);
	}
	return ret;
}

PyDoc_STRVAR( GemRB_HasResource__doc,
"HasResource(ResRef, ResType)\n\n"
"Returns true if resource is accessible." );

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
"SetupEquipmentIcons(WindowIndex, slot[, Start, Offset])\n\n"
"Automagically sets up the controls of the equipment list window for a PC indexed by slot.\n"
"Start is the beginning of the visible part of the item list.\n"
"Offset is the ID of the first usable button.");

static PyObject* GemRB_Window_SetupEquipmentIcons(PyObject * /*self*/, PyObject* args)
{
	int wi, slot;
	int Start = 0;
	int Offset = 0; //control offset (iwd2 has the action buttons starting at 6)

	if (!PyArg_ParseTuple( args, "ii|ii", &wi, &slot, &Start, &Offset )) {
		return AttributeError( GemRB_Window_SetupEquipmentIcons__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( slot );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	//-2 because of the left/right scroll icons
	if (!ItemArray) {
		ItemArray = (ItemExtHeader *) malloc((GUIBT_COUNT) * sizeof (ItemExtHeader) );
	}
	bool more = actor->inventory.GetEquipmentInfo(ItemArray, Start, GUIBT_COUNT-(Start?1:0));
	int i;
	if (Start) {
		PyObject *ret = SetActionIcon(wi,core->GetControl(wi, Offset),ACT_LEFT,0);
		if (!ret) {
			return RuntimeError("Cannot set action button!\n");
		}
	}
	//FIXME: this is a hardcoded resource (pst has no such one)
	AnimationFactory* bam = ( AnimationFactory* )
		gamedata->GetFactoryResource( "guibtbut",
				IE_BAM_CLASS_ID, IE_NORMAL );
	if (!bam) {
		return RuntimeError( "BAM not found" );
	}

	for (i=0;i<GUIBT_COUNT-(more?1:0);i++) {
		int ci = core->GetControl(wi, i+Offset+(Start?1:0) );
		Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
		btn->SetEvent(IE_GUI_BUTTON_ON_PRESS, "EquipmentPressed");
		strcpy(btn->VarName,"Equipment");
		btn->Value = Start+i;

		ItemExtHeader *item = ItemArray+i;
		Sprite2D *Picture = NULL;

		if (item->UseIcon[0]) {
			Picture = gamedata->GetBAMSprite(item->UseIcon, 1, 0);
		}

		if (!Picture) {
			btn->SetState(IE_GUI_BUTTON_DISABLED);
			btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE, BM_SET);
			btn->SetTooltip(NULL);
		} else {
			SetButtonCycle(bam, btn, 0, IE_GUI_BUTTON_UNPRESSED);
			SetButtonCycle(bam, btn, 1, IE_GUI_BUTTON_PRESSED);
			SetButtonCycle(bam, btn, 2, IE_GUI_BUTTON_SELECTED);
			SetButtonCycle(bam, btn, 3, IE_GUI_BUTTON_DISABLED);
			btn->SetPicture( Picture );
			btn->SetState(IE_GUI_BUTTON_UNPRESSED);
			btn->SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_RIGHT, BM_SET);

			const CREItem *item_slot = actor->inventory.GetSlotItem(item->slot);
			int tip = core->GetItemTooltip(item->itemname, item->headerindex, item_slot->Flags&IE_INV_ITEM_IDENTIFIED);
			if (tip>0) {
				//cannot make this const, because it will be freed
				char *tmp = core->GetString((ieStrRef) tip,0);
				btn->SetTooltip(tmp);
				core->FreeString(tmp);
			} else {
				btn->SetTooltip(NULL);
			}
			char usagestr[10];

			if (item->Charges && (item->Charges!=0xffff) ) {
				sprintf(usagestr,"%d", item->Charges);
				btn->SetText( usagestr );
			}
			if (!item->Charges && (item->ChargeDepletion==CHG_NONE) ) {
				btn->SetState(IE_GUI_BUTTON_DISABLED);
			}
		}
	}

	if (more) {
		PyObject *ret = SetActionIcon(wi,core->GetControl(wi, i+Offset+1),ACT_RIGHT,i+1);
		if (!ret) {
			return RuntimeError("Cannot set action button!\n");
		}
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_SetupSpellIcons__doc,
"SetupSpellIcons(WindowIndex, slot, type[, Start, Offset])\n\n"
"Automagically sets up the controls of the spell or innate list window for a PC indexed by slot.\n"
"Start is the beginning of the visible part of the spell list.\n"
"Offset is the ID of the first usable button.");

static PyObject* GemRB_Window_SetupSpellIcons(PyObject * /*self*/, PyObject* args)
{
	int wi, slot, Type;
	int Start = 0;
	int Offset = 0;

	if (!PyArg_ParseTuple( args, "iii|ii", &wi, &slot, &Type, &Start, &Offset )) {
		return AttributeError( GemRB_Window_SetupSpellIcons__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( slot );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	//-2 because of the left/right scroll icons
	if (!SpellArray) {
		SpellArray = (SpellExtHeader *) malloc((GUIBT_COUNT) * sizeof (SpellExtHeader) );
	}
	int more = actor->spellbook.GetSpellInfo(SpellArray, Type, Start, GUIBT_COUNT-(Start?1:0));
	int i;
	if (Start) {
		more |= 2;
	}
	if (more) {
		int ci = core->GetControl(wi, Offset);
		PyObject *ret = SetActionIcon(wi, ci, ACT_LEFT, 0);
		if (!ret) {
			return RuntimeError("Cannot set action button!\n");
		}
		Button * btn = (Button *) GetControl(wi, ci, IE_GUI_BUTTON);
		if (Start) {
			btn->SetState(IE_GUI_BUTTON_UNPRESSED);
		} else {
			btn->SetState(IE_GUI_BUTTON_DISABLED);
		}
	}

	//FIXME: this is a hardcoded resource (pst has no such one)
	AnimationFactory* bam = ( AnimationFactory* )
		gamedata->GetFactoryResource( "guibtbut",
				IE_BAM_CLASS_ID, IE_NORMAL );
	if (!bam) {
		return RuntimeError( "BAM not found" );
	}

	// disable all spells if fx_disable_spellcasting was run with the same type
	// but only if there are any spells of that type to disable
	int disabled_spellcasting = actor->fxqueue.DisabledSpellcasting(7);

	for (i=0;i<GUIBT_COUNT-(more?2:0);i++) {
		SpellExtHeader *spell = SpellArray+i;

		int ci = core->GetControl(wi, i+Offset+(more?1:0) );
		Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
		strcpy(btn->VarName,"Spell");
		btn->Value = i+Start;

		// disable spells that should be cast from the inventory
		// Identify is misclassified and has Target 3 (Dead char)

		if (CheckSpecialSpell(spell->spellname, actor) || (disabled_spellcasting&(1<<spell->type)) ) {
			btn->SetState(IE_GUI_BUTTON_DISABLED);
			btn->EnableBorder(1, IE_GUI_BUTTON_DISABLED);
			btn->SetEvent(IE_GUI_BUTTON_ON_PRESS,"UpdateActionsWindow"); //noop
		} else {
			btn->SetState(IE_GUI_BUTTON_UNPRESSED);
			btn->SetEvent(IE_GUI_BUTTON_ON_PRESS,"SpellPressed");
		}
		Sprite2D *Picture = NULL;

		if (spell->MemorisedIcon[0]) {
			Picture = gamedata->GetBAMSprite(spell->MemorisedIcon, 0, 0);
		}

		if (!Picture) {
			btn->SetState(IE_GUI_BUTTON_DISABLED);
			btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE, BM_SET);
			btn->SetTooltip(NULL);
		} else {
			SetButtonCycle(bam, btn, 0, IE_GUI_BUTTON_UNPRESSED);
			SetButtonCycle(bam, btn, 1, IE_GUI_BUTTON_PRESSED);
			SetButtonCycle(bam, btn, 2, IE_GUI_BUTTON_SELECTED);
			SetButtonCycle(bam, btn, 3, IE_GUI_BUTTON_DISABLED);
			btn->SetPicture( Picture );
			btn->SetFlags(IE_GUI_BUTTON_PICTURE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_RIGHT, BM_SET);
			//cannot make this const, because it will be freed
			char *tmp = core->GetString(spell->strref,0);
			btn->SetTooltip(tmp);
			core->FreeString(tmp);
			char usagestr[10];

			if (spell->count>0) {
				sprintf(usagestr,"%d", spell->count);
				btn->SetText( usagestr );
				btn->SetState(IE_GUI_BUTTON_UNPRESSED);
			} else {
				btn->SetState(IE_GUI_BUTTON_DISABLED);
			}
		}
	}

	if (more) {
		int ci = core->GetControl(wi, i+Offset+1);
		PyObject *ret = SetActionIcon(wi, ci, ACT_RIGHT, i+1);
		if (!ret) {
			return RuntimeError("Cannot set action button!\n");
		}
		Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
		if (more&1) {
			btn->SetState(IE_GUI_BUTTON_UNPRESSED);
		} else {
			btn->SetState(IE_GUI_BUTTON_DISABLED);
			btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE, BM_SET);
			btn->SetTooltip(NULL);
		}
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_Window_SetupControls__doc,
"SetupControls(WindowIndex, slot[, Start])\n\n"
"Automagically sets up the controls of the action window for a PC indexed by slot." );

static PyObject* GemRB_Window_SetupControls(PyObject * /*self*/, PyObject* args)
{
	int wi, slot;
	int Start = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &wi, &slot, &Start )) {
		return AttributeError( GemRB_Window_SetupControls__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( slot );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	ActionButtonRow myrow;
	actor->GetActionButtonRow(myrow);
	bool fistdrawn = true;
	ieDword magicweapon = actor->inventory.GetMagicSlot();
	if (!actor->inventory.HasItemInSlot("",magicweapon) ) {
		magicweapon = 0xffff;
	}
	ieDword fistweapon = actor->inventory.GetFistSlot();
	ieDword usedslot = actor->inventory.GetEquippedSlot();
	ieDword disabledbutton = actor->GetStat(IE_DISABLEDBUTTON);
	int tmp;
	for (int i=0;i<GUIBT_COUNT;i++) {
		int ci = core->GetControl(wi, i+Start);
		if (ci<0) {
			printf("Couldn't find button #%d on Window #%d\n", i+Start, wi);
			return RuntimeError ("No such control!\n");
		}
		int action = myrow[i];
		if (action==100) {
			action = -1;
		} else {
			action&=31;
		}
		Button * btn = (Button *) GetControl(wi,ci,IE_GUI_BUTTON);
		if (!btn) {
			return NULL;
		}
		btn->SetFlags(IE_GUI_BUTTON_NO_IMAGE|IE_GUI_BUTTON_ALIGN_BOTTOM|IE_GUI_BUTTON_ALIGN_RIGHT, BM_SET);
		SetItemText(wi, ci, 0, false);
		PyObject *ret = SetActionIcon(wi,ci,action,i+1);

		int state = IE_GUI_BUTTON_UNPRESSED;
		ieDword modalstate = actor->ModalState;
		int type;
		switch (action) {
		case ACT_INNATE:
			if (actor->spellbook.IsIWDSpellBook()) {
				type = IE_IWD2_SPELL_INNATE;
			} else {
				type = IE_SPELL_TYPE_INNATE;
			}
			if (!actor->spellbook.GetMemorizedSpellsCount(type)) {
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
			if (!actor->spellbook.GetSpellInfoSize(type)) {
				state = IE_GUI_BUTTON_DISABLED;
			}
			break;
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
			if (modalstate==MS_STEALTH) {
				state = IE_GUI_BUTTON_SELECTED;
			}
			break;
		case ACT_SEARCH:
			if (modalstate==MS_DETECTTRAPS) {
				state = IE_GUI_BUTTON_SELECTED;
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
					SetItemText(wi, ci, item->Usages[actor->PCStats->QuickWeaponHeaders[action-ACT_WEAPON1]], true);
					if (usedslot == slot) {
						btn->EnableBorder(0, true);
						if (core->GetGameControl()->target_mode == TARGET_MODE_ATTACK) {
							state = IE_GUI_BUTTON_SELECTED;
						} else {
							state = IE_GUI_BUTTON_THIRD;
						}
					} else {
						btn->EnableBorder(0, false);
					}
				}
			}
		}
			break;
		case ACT_QSPELL1:
		case ACT_QSPELL2:
		case ACT_QSPELL3:
		//fixme iwd2 has 9
		{
			SetButtonBAM(wi, ci, "stonspel",0,0,-1);
			ieResRef *poi = &actor->PCStats->QuickSpells[action-ACT_QSPELL1];
			if ((*poi)[0]) {
				SetSpellIcon(wi, ci, *poi, 1, 1, i+1);
			}
		}
			break;
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
						SetItemText(wi, ci, usages, false);
					}
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
		if (action<0 || (disabledbutton & (1<<action) )) {
			state = IE_GUI_BUTTON_DISABLED;
		}
		btn->SetState(state);
		//you have to set this overlay up
		btn->EnableBorder(1, state==IE_GUI_BUTTON_DISABLED);
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_ClearActions__doc,
"ClearActions(slot)\n\n"
"Stops an action for a PC indexed by slot." );

static PyObject* GemRB_ClearActions(PyObject * /*self*/, PyObject* args)
{
	int slot;

	if (!PyArg_ParseTuple( args, "i", &slot )) {
		return AttributeError( GemRB_ClearActions__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( slot );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	if (actor->GetInternalFlag()&IF_NOINT) {
		printMessage( "GuiScript","Cannot break action", GREEN);
		Py_INCREF( Py_None );
		return Py_None;
	}
	if (!(actor->GetNextStep()) && !actor->ModalState) {
		printMessage( "GuiScript","No breakable action", GREEN);
		Py_INCREF( Py_None );
		return Py_None;
	}
	actor->ClearPath();      //stop walking
	actor->ClearActions();   //stop pending action involved walking
	actor->SetModal(MS_NONE);//stop modal actions
	Py_INCREF( Py_None );
	return Py_None;
}


PyDoc_STRVAR( GemRB_SetDefaultActions__doc,
"SetDefaultActions(qslot, slot1, slot2, slot3)\n\n"
"Sets whether qslots need an additional translation like in iwd2. "
"Also sets up the first three default action types." );

static PyObject* GemRB_SetDefaultActions(PyObject * /*self*/, PyObject* args)
{
	int qslot;
	int slot1, slot2, slot3;

	if (!PyArg_ParseTuple( args, "iiii", &qslot, &slot1, &slot2, &slot3 )) {
		return AttributeError( GemRB_SetDefaultActions__doc );
	}
	Actor::SetDefaultActions((bool) qslot, (ieByte) slot1, (ieByte) slot2, (ieByte) slot3);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetupQuickSlot__doc,
"SetupQuickSlot(PartyID, quickslot, inventoryslot, headerindex)\n\n"
"Set up a quick slot or weapon slot of a PC to use a weapon ability.\n\n"
"If the inventoryslot number is -1, only the header index will be changed. "
"If the quick slot is 0, then the inventory slot will be used to find which "
"headerindex should be set. The default value for headerindex is 0.");

static PyObject* GemRB_SetupQuickSlot(PyObject * /*self*/, PyObject* args)
{
	int PartyID, which, slot, headerindex = 0;

	if (!PyArg_ParseTuple( args, "iii|i", &PartyID, &which, &slot, &headerindex )) {
		return AttributeError( GemRB_SetupQuickSlot__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	actor->SetupQuickSlot(which, slot, headerindex);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetEquippedQuickSlot__doc,
"SetEquippedQuickSlot(PartyID, QWeaponSlot[, ability])->int\n\n"
"Sets the named weapon/item slot as equipped weapon slot, optionally sets the used ability."
"Returns strref number of failure (0 success, -1 silent failure)." );

static PyObject* GemRB_SetEquippedQuickSlot(PyObject * /*self*/, PyObject* args)
{
	int slot;
	int PartyID;
	int ability = -1;

	if (!PyArg_ParseTuple( args, "ii|i", &PartyID, &slot, &ability)) {
		return AttributeError( GemRB_SetEquippedQuickSlot__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	int ret = actor->SetEquippedQuickSlot(slot, ability);
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_GetEquippedQuickSlot__doc,
"GetEquippedQuickSlot(PartyID[, NoTrans]) => Slot\n\n"
"Returns the inventory slot (translation) or quick weapon index (no translation) of the equipped weapon." );

static PyObject* GemRB_GetEquippedQuickSlot(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	int NoTrans = 0;

	if (!PyArg_ParseTuple( args, "i|i", &PartyID, &NoTrans)) {
		return AttributeError( GemRB_GetEquippedQuickSlot__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	int ret = actor->inventory.GetEquippedSlot();
	/*int effect = core->QuerySlotEffects(ret);
	if (effect == SLOT_EFFECT_MISSILE) {
		ret = actor->inventory.FindRangedWeapon();
	}*/
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
	} /*else {
		ret-=actor->inventory.GetWeaponSlot();
	}*/
	return PyInt_FromLong( core->FindSlot(ret) );
}

PyDoc_STRVAR( GemRB_GetEquippedAmmunition__doc,
"GetEquippedAmmunition(PartyID) => QSlot\n\n"
"Returns the equipped ammunition slot, if any; -1 if none." );

static PyObject* GemRB_GetEquippedAmmunition(PyObject * /*self*/, PyObject* args)
{
	int PartyID;

	if (!PyArg_ParseTuple( args, "i", &PartyID)) {
		return AttributeError( GemRB_GetEquippedQuickSlot__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	int ret = actor->inventory.GetEquippedSlot();
	int effect = core->QuerySlotEffects(ret);
	if (effect == SLOT_EFFECT_MISSILE) {
		return PyInt_FromLong( core->FindSlot(ret) );
	} else {
		return PyInt_FromLong( -1 );
	}
}

PyDoc_STRVAR( GemRB_SetModalState__doc,
"SetModalState(slot, state[,spell])\n\n"
"Sets the modal state of the actor.\n"
"If 'spell' is not given, it will set a default spell resource associated with the state.");

static PyObject* GemRB_SetModalState(PyObject * /*self*/, PyObject* args)
{
	int slot;
	int state;
	const char *spell=NULL;

	if (!PyArg_ParseTuple( args, "ii|s", &slot, &state, &spell )) {
		return AttributeError( GemRB_SetModalState__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( slot );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	actor->SetModal( (ieDword) state, 0);
	if (spell) {
		strnlwrcpy(actor->ModalSpell, spell, 8);
	} else {
		if (state >= (signed)core->ModalStates.size()) {
			actor->ModalSpell[0] = 0;
		} else {
			strnlwrcpy(actor->ModalSpell, core->ModalStates[state].spell, 8);
		}
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SpellCast__doc,
"SpellCast(actor, type, spell)\n\n"
"Makes the actor try to cast a spell. Type is the spell type like 3 for normal spells and 4 for innates.\n"
"Spell is the index of the spell in the memorised spell list.");

static PyObject* GemRB_SpellCast(PyObject * /*self*/, PyObject* args)
{
	unsigned int slot;
	unsigned int type;
	unsigned int spell;

	if (!PyArg_ParseTuple( args, "iii", &slot, &type, &spell )) {
		return AttributeError( GemRB_SpellCast__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( slot );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	SpellExtHeader spelldata; // = SpellArray[spell];
	actor->spellbook.GetSpellInfo(&spelldata, type, spell, 1);

	printf("Cast spell: %s\n", spelldata.spellname);
	printf("Slot: %d\n", spelldata.slot);
	printf("Type: %d\n", spelldata.type);
	//cannot make this const, because it will be freed
	char *tmp = core->GetString(spelldata.strref);
	printf("Spellname: %s\n", tmp);
	core->FreeString(tmp);
	printf("Target: %d\n", spelldata.Target);
	printf("Range: %d\n", spelldata.Range);
	if(! (1<<spelldata.type) & type) {
		return RuntimeError( "Wrong type of spell!");
	}

	GameControl *gc = core->GetGameControl();
	switch (spelldata.Target) {
		case TARGET_SELF:
			// FIXME: GA_NO_DEAD and such are not actually used by SetupCasting
			gc->SetupCasting(spelldata.type, spelldata.level, spelldata.slot, actor, GA_NO_DEAD, spelldata.TargetNumber);
			gc->TryToCast(actor, actor);
			break;
		case TARGET_NONE:
			//reset the cursor
			gc->target_mode = TARGET_MODE_NONE;
			//this is always instant casting
			core->ApplySpell(spelldata.spellname, actor, actor, 0);
			break;
		case TARGET_AREA:
			gc->SetupCasting(spelldata.type, spelldata.level, spelldata.slot, actor, GA_POINT, spelldata.TargetNumber);
			break;
		case TARGET_CREA:
			gc->SetupCasting(spelldata.type, spelldata.level, spelldata.slot, actor, GA_NO_DEAD, spelldata.TargetNumber);
			break;
		case TARGET_DEAD:
			gc->SetupCasting(spelldata.type, spelldata.level, spelldata.slot, actor, 0, spelldata.TargetNumber);
			break;
		case TARGET_INV:
			//bring up inventory in the end???
			//break;
		default:
			printf("Unhandled target type: %d\n", spelldata.Target);
			break;
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_ApplySpell__doc,
"ApplySpell(actor, spellname)\n\n"
"Applies a spell on actor.");

static PyObject* GemRB_ApplySpell(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	const char *spell;

	if (!PyArg_ParseTuple( args, "is", &PartyID, &spell )) {
		return AttributeError( GemRB_ApplySpell__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	core->ApplySpell(spell, actor, actor, 0);

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_UseItem__doc,
"UseItem(actor, slot, header[,forcetarget])\n\n"
"Makes the actor try to use an item. "
"If slot is -1, then header is the index of the item functionality in the use item list. "
"If slot is -2, then header is the quickslot index. "
"If slot is non-negative, then header is the header of the item in the 'slot'.");

static PyObject* GemRB_UseItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	int slot;
	int header;
	int forcetarget=-1; //some crappy scrolls don't target self correctly!

	if (!PyArg_ParseTuple( args, "iii|i", &PartyID, &slot, &header, &forcetarget )) {
		return AttributeError( GemRB_UseItem__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
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
				printMessage("GUIScript","QuickItem has no charges.\n", WHITE);
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
		printMessage("GUIScript","Empty slot used?\n", YELLOW);
		Py_INCREF( Py_None );
		return Py_None;
	}

	/// remove this after projectile is done
	printf("Use item: %s\n", itemdata.itemname);
	printf("Extended header: %d\n", itemdata.headerindex);
	printf("Attacktype: %d\n",itemdata.AttackType);
	printf("Range: %d\n",itemdata.Range);
	printf("Target: %d\n",forcetarget);
	printf("Projectile: %d\n", itemdata.ProjectileAnimation);
	//
	switch (forcetarget) {
		case TARGET_SELF:
			actor->UseItem(itemdata.slot, itemdata.headerindex, actor, flags);
			break;
		case TARGET_NONE:
			actor->UseItem(itemdata.slot, itemdata.headerindex, NULL, flags);
			break;
		case TARGET_AREA:
			core->GetGameControl()->SetupItemUse(itemdata.slot, itemdata.headerindex, actor, GA_POINT, itemdata.TargetNumber);
			break;
		case TARGET_CREA:
			core->GetGameControl()->SetupItemUse(itemdata.slot, itemdata.headerindex, actor, GA_NO_DEAD, itemdata.TargetNumber);
			break;
		case TARGET_DEAD:
			core->GetGameControl()->SetupItemUse(itemdata.slot, itemdata.headerindex, actor, 0, itemdata.TargetNumber);
			break;
		default:
			printMessage("GUIScript", "Unhandled target type!", LIGHT_RED );
			break;
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetGamma__doc,
"SetGamma(brightness, contrast)\n\n"
"Adjusts brightness and contrast.");

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
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetMouseScrollSpeed__doc,
"SetMouseScrollSpeed(mouseSpeed)\n\n"
"Adjusts mouse scroll speed.");

static PyObject* GemRB_SetMouseScrollSpeed(PyObject * /*self*/, PyObject* args)
{
	int mouseSpeed;

	if (!PyArg_ParseTuple( args, "i", &mouseSpeed)) {
		return AttributeError( GemRB_SetMouseScrollSpeed__doc );
	}

	core->SetMouseScrollSpeed(mouseSpeed);

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetTooltipDelay__doc,
"SetTooltipDelay(tooltipDelay)\n\n"
"Adjusts tooltip appearing speed.");

static PyObject* GemRB_SetTooltipDelay(PyObject * /*self*/, PyObject* args)
{
	int tooltipDelay;

	if (!PyArg_ParseTuple( args, "i", &tooltipDelay)) {
		return AttributeError( GemRB_SetTooltipDelay__doc );
	}

	core->TooltipDelay = tooltipDelay;

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetFullScreen__doc,
"SetFullScreen(int)\n\n"
"0 - windowed, 1 - fullscreen, -1 - toggle");

static PyObject* GemRB_SetFullScreen(PyObject * /*self*/, PyObject* args)
{
	int fullscreen;

	if (!PyArg_ParseTuple( args, "i", &fullscreen )) {
		return AttributeError( GemRB_SetFullScreen__doc );
	}
	core->GetVideoDriver()->ToggleFullscreenMode(fullscreen);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_RestParty__doc,
"RestParty(noareacheck, dream, hp)\n\n"
"Executes the party rest function, used from both stores and via the main screen.");

static PyObject* GemRB_RestParty(PyObject * /*self*/, PyObject* args)
{
	int noareacheck;
	int dream, hp;

	if (!PyArg_ParseTuple( args, "iii", &noareacheck, &dream, &hp)) {
		return AttributeError( GemRB_RestParty__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	game->RestParty(noareacheck, dream, hp);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_HasSpecialItem__doc,
"HasSpecialItem(pc, itemtype, useup) => bool\n\n"
"Checks if a team member has an item, optionally uses it.");

//itemtype 1 - identify
static PyObject* GemRB_HasSpecialItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID, itemtype, useup;

	if (!PyArg_ParseTuple( args, "iii", &PartyID, &itemtype, &useup)) {
		return AttributeError( GemRB_HasSpecialItem__doc );
	}
	if (SpecialItemsCount==-1) {
		ReadSpecialItems();
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
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
		useup = actor->UseItem((ieDword) slot, 0, actor, UI_SILENT);
	} else {
		CREItem *si = actor->inventory.GetSlotItem( slot );
		if (si->Usages[0]) useup = 1;
	}
	return PyInt_FromLong( useup );
}

PyDoc_STRVAR( GemRB_HasSpecialSpell__doc,
"HasSpecialSpell(pc, itemtype, useup) => bool\n\n"
"Checks if a team member has a spell, optionally uses it.");

//itemtype 1 - identify
static PyObject* GemRB_HasSpecialSpell(PyObject * /*self*/, PyObject* args)
{
	int PartyID, itemtype, useup;

	if (!PyArg_ParseTuple( args, "iii", &PartyID, &itemtype, &useup)) {
		return AttributeError( GemRB_HasSpecialSpell__doc );
	}
	if (SpecialSpellsCount==-1) {
		ReadSpecialSpells();
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	int i = SpecialSpellsCount;
	while(i--) {
		if (itemtype&SpecialSpells[i].value) {
			if (actor->spellbook.HaveSpell(SpecialSpells[i].resref,useup)) {
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
"ApplyEffect(pc, effect, param1, param2[,resref,resref2, resref3]])\n\n"
"Creates a basic effect and applies it on the player character. "
"This function could be used to add stats that are stored in effect blocks. "
"The resource fields are optional.");

static PyObject* GemRB_ApplyEffect(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	const char *opcodename;
	int param1, param2;
	const char *resref1 = NULL;
	const char *resref2 = NULL;
	const char *resref3 = NULL;
	const char *source = NULL;

	if (!PyArg_ParseTuple( args, "isii|ssss", &PartyID, &opcodename, &param1, &param2, &resref1, &resref2, &resref3, &source)) {
		return AttributeError( GemRB_ApplyEffect__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	work_ref.Name=opcodename;
	work_ref.EffText=-1;
	Effect *fx = EffectQueue::CreateEffect(work_ref, param1, param2, FX_DURATION_INSTANT_PERMANENT_AFTER_BONUSES);
	if (!fx) {
		//invalid effect name didn't resolve to opcode
		return RuntimeError( "Invalid effect name!" );
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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_CountEffects__doc,
"CountEffects(pc, effect, param1, param2[,resref])\n\n"
"Counts how many matching effects are applied on the player character. "
"This function could be used to get HLA information in ToB. "
"The resource field is optional.");

static PyObject* GemRB_CountEffects(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	const char *opcodename;
	int param1, param2;
	const char *resref = NULL;

	if (!PyArg_ParseTuple( args, "isii|s", &PartyID, &opcodename, &param1, &param2, &resref)) {
		return AttributeError( GemRB_CountEffects__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	work_ref.Name=opcodename;
	work_ref.EffText=-1;
	ieDword ret = actor->fxqueue.CountEffects(work_ref, param1, param2, resref);
	return PyInt_FromLong( ret );
}

PyDoc_STRVAR( GemRB_ModifyEffect__doc,
"ModifyEffect(pc, effect, p1, p2)\n\n"
"Changes/sets the target coordinates of the specified effect. "
"This command is used for the farsight spell.");

static PyObject* GemRB_ModifyEffect(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	const char *opcodename;
	int px, py;

	if (!PyArg_ParseTuple( args, "isii", &PartyID, &opcodename, &px, &py)) {
		return AttributeError( GemRB_ModifyEffect__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	work_ref.Name=opcodename;
	work_ref.EffText=-1;
	actor->fxqueue.ModifyEffectPoint(work_ref, px, py);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_StealFailed__doc,
"StealFailed()\n\n"
"Sends the steal failed trigger (attacked) to the owner of the current store. "
"The owner of the current store was set to the Sender of StartStore action.");

static PyObject* GemRB_StealFailed(PyObject * /*self*/, PyObject* /*args*/)
{
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Store *store = core->GetCurrentStore();
	if (!store) {
		return RuntimeError( "No store loaded!" );
	}
	Map *map = game->GetCurrentArea();
	if (!map) {
		return RuntimeError( "No area loaded!" );
	}
	Actor* owner = map->GetActor( store->GetOwner(), 0 );
	if (!owner) {
		printMessage("GUIScript", "No owner found!", YELLOW );
		Py_INCREF( Py_None );
		return Py_None;
	}
	Actor* attacker = game->FindPC((int) game->GetSelectedPCSingle() );
	if (!attacker) {
		printMessage("GUIScript", "No thief found!", YELLOW );
		Py_INCREF( Py_None );
		return Py_None;
	}
	//not sure if this is ok
	//owner->LastAttacker = attacker->GetID();
	owner->LastDisarmFailed = attacker->GetID();
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SwapPCs__doc,
"SwapPCs(idx1, idx2)\n\n"
"Swaps the party order for two player characters.");

static PyObject* GemRB_SwapPCs(PyObject * /*self*/, PyObject* args)
{
	int idx1, idx2;

	if (!PyArg_ParseTuple( args, "ii", &idx1, &idx2)) {
		return AttributeError( GemRB_SwapPCs__doc );
	}

	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}

	game->SwapPCs(game->FindPlayer(idx1), game->FindPlayer(idx2));
	//leader changed
	if (idx1==1 || idx2==1) {
		DisplayStringCore( game->FindPC(1), VB_LEADER, DS_CONST);
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetRepeatClickFlags__doc,
"SetRepeatClickFlags(value, op)\n\n"
"Sets the mode repeat clicks are handled.");

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
"DisplayString(strref, color[,actor])\n\n"
"Displays string on the MessageWindow using methods supplied by the engine core. "
"The optional actor is the party ID of the character whose name will be displayed.");

static PyObject* GemRB_DisplayString(PyObject * /*self*/, PyObject* args)
{
	int strref, color;
	int PartyID = 0;

	if (!PyArg_ParseTuple( args, "ii|i", &strref, &color, &PartyID)) {
		return AttributeError( GemRB_DisplayString__doc );
	}
	if (PartyID) {
		Game *game = core->GetGame();
		if (!game) {
			return RuntimeError( "No game loaded!" );
		}
		Actor *actor = game->FindPC(PartyID);
		if (!actor) {
			return RuntimeError( "Actor not found" );
		}
		core->DisplayStringName(strref, (unsigned int) color, actor, IE_STR_SOUND);
	} else {
		core->DisplayString(strref, (unsigned int) color, IE_STR_SOUND);
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetCombatDetails__doc,
"GetCombatDetails(pc, leftorright) => dict\n\n"
"Returns the current THAC0 and other data in relation to the equipped weapon.");

static PyObject* GemRB_GetCombatDetails(PyObject * /*self*/, PyObject* args)
{
	int PartyID;
	int leftorright;

	if (!PyArg_ParseTuple( args, "ii", &PartyID, &leftorright)) {
		return AttributeError( GemRB_GetCombatDetails__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	leftorright = leftorright&1;
	WeaponInfo wi;
	ITMExtHeader *header = NULL;
	ITMExtHeader *hittingheader = NULL;
	int tohit=20;
	ieDword Flags=0;
	int DamageBonus=0, CriticalBonus=0;
	int speed, style=0;

	PyObject* dict = PyDict_New();
	if (!actor->GetCombatDetails(tohit, leftorright, wi, header, hittingheader, Flags, DamageBonus, speed, CriticalBonus, style)) {
		//TODO: handle error, so tohit will still be set correctly?
	}
	PyDict_SetItemString(dict, "ToHit", PyInt_FromLong (tohit));
	PyDict_SetItemString(dict, "Flags", PyInt_FromLong (Flags));
	PyDict_SetItemString(dict, "DamageBonus", PyInt_FromLong (DamageBonus));
	PyDict_SetItemString(dict, "Speed", PyInt_FromLong (speed));
	PyDict_SetItemString(dict, "CriticalBonus", PyInt_FromLong (CriticalBonus));
	PyDict_SetItemString(dict, "Style", PyInt_FromLong (style));
	return dict;
}

PyDoc_STRVAR( GemRB_IsDualWielding__doc,
"IsDualWielding(pc)\n\n"
"1 if the pc is dual wielding; 0 otherwise.");

static PyObject* GemRB_IsDualWielding(PyObject * /*self*/, PyObject* args)
{
	int PartyID;

	if (!PyArg_ParseTuple( args, "i", &PartyID)) {
		return AttributeError( GemRB_IsDualWielding__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}

	int dualwield = actor->IsDualWielding();
	return PyInt_FromLong( dualwield );
}

PyDoc_STRVAR( GemRB_GetSelectedSize__doc,
"GetSelectedSize() => int\n\n"
"Returns the number of actors selected in the party.");

static PyObject* GemRB_GetSelectedSize(PyObject* /*self*/, PyObject* /*args*/)
{
	return PyInt_FromLong(core->GetGame()->selected.size());
}

PyDoc_STRVAR( GemRB_GetSpellCastOn__doc,
"GetSpellCastOn(pc) => resref\n\n"
"Returns the last spell cast on a partymember.");

static PyObject* GemRB_GetSpellCastOn(PyObject* /*self*/, PyObject* args)
{
	int PartyID;
	ieResRef splname;

	if (!PyArg_ParseTuple( args, "i", &PartyID )) {
		return AttributeError( GemRB_GetSpellCastOn__doc );
	}
	Game *game = core->GetGame();
	if (!game) {
		return RuntimeError( "No game loaded!" );
	}
	Actor* actor = game->FindPC( PartyID );
	if (!actor) {
		return RuntimeError( "Actor not found" );
	}
	
	ResolveSpellName(splname, actor->LastSpellOnMe);
	return PyString_FromString(splname);
}

static PyMethodDef GemRBMethods[] = {
	METHOD(ActOnPC, METH_VARARGS),
	METHOD(ApplyEffect, METH_VARARGS),
	METHOD(ApplySpell, METH_VARARGS),
	METHOD(CanUseItemType, METH_VARARGS),
	METHOD(ChangeContainerItem, METH_VARARGS),
	METHOD(ChangeItemFlag, METH_VARARGS),
	METHOD(ChangeStoreItem, METH_VARARGS),
	METHOD(CheckFeatCondition, METH_VARARGS),
	METHOD(CheckVar, METH_VARARGS),
	METHOD(ClearActions, METH_VARARGS),
	METHOD(CountEffects, METH_VARARGS),
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
	METHOD(EnableCheatKeys, METH_VARARGS),
	METHOD(EndCutSceneMode, METH_NOARGS),
	METHOD(EnterGame, METH_NOARGS),
	METHOD(EnterStore, METH_VARARGS),
	METHOD(EvaluateString, METH_VARARGS),
	METHOD(ExecuteString, METH_VARARGS),
	METHOD(ExploreArea, METH_VARARGS),
	METHOD(FillPlayerInfo, METH_VARARGS),
	METHOD(GameControlGetTargetMode, METH_NOARGS),
	METHOD(GameControlSetScreenFlags, METH_VARARGS),
	METHOD(GameControlSetTargetMode, METH_VARARGS),
	METHOD(GameGetReputation, METH_NOARGS),
	METHOD(GameSetReputation, METH_VARARGS),
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
	METHOD(GameSetFormation, METH_VARARGS),
	METHOD(GameSetPartyGold, METH_VARARGS),
	METHOD(GameSetPartySize, METH_VARARGS),
	METHOD(GameSetProtagonistMode, METH_VARARGS),
	METHOD(GameSetScreenFlags, METH_VARARGS),
	METHOD(GetAbilityBonus, METH_VARARGS),
	METHOD(GetCombatDetails, METH_VARARGS),
	METHOD(GetContainer, METH_VARARGS),
	METHOD(GetContainerItem, METH_VARARGS),
	METHOD(GetControl, METH_VARARGS),
	METHOD(GetCurrentArea, METH_NOARGS),
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
	METHOD(GetMemorizableSpellsCount, METH_VARARGS),
	METHOD(GetMemorizedSpell, METH_VARARGS),
	METHOD(GetMemorizedSpellsCount, METH_VARARGS),
	METHOD(GetMessageWindowSize, METH_NOARGS),
	METHOD(GetPartySize, METH_NOARGS),
	METHOD(GetPCStats, METH_VARARGS),
	METHOD(GetPlayerName, METH_VARARGS),
	METHOD(GetPlayerPortrait, METH_VARARGS),
	METHOD(GetPlayerStat, METH_VARARGS),
	METHOD(GetPlayerStates, METH_VARARGS),
	METHOD(GetPlayerScript, METH_VARARGS),
	METHOD(GetPlayerString, METH_VARARGS),
	METHOD(GetSaveGameCount, METH_VARARGS),
	METHOD(GetSaveGameDate, METH_VARARGS),
	METHOD(GetSaveGameGameDate, METH_VARARGS),
	METHOD(GetSaveGameName, METH_VARARGS),
	METHOD(GetSaveGamePortrait, METH_VARARGS),
	METHOD(GetSaveGamePreview, METH_VARARGS),
	METHOD(GetSaveGameSaveID, METH_VARARGS),
	METHOD(GetSelectedSize, METH_NOARGS),
	METHOD(GetString, METH_VARARGS),
	METHOD(GetSpellCastOn, METH_VARARGS),
	METHOD(GetToken, METH_VARARGS),
	METHOD(GetVar, METH_VARARGS),
	METHOD(GetSlotType, METH_VARARGS),
	METHOD(GetStore, METH_VARARGS),
	METHOD(GetStoreDrink, METH_VARARGS),
	METHOD(GetStoreCure, METH_VARARGS),
	METHOD(GetStoreItem, METH_VARARGS),
	METHOD(GetSpell, METH_VARARGS),
	METHOD(GetSlotItem, METH_VARARGS),
	METHOD(GetSlots, METH_VARARGS),
	METHOD(GetSystemVariable, METH_VARARGS),
	METHOD(GetRumour, METH_VARARGS),
	METHOD(HardEndPL, METH_NOARGS),
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
	METHOD(LoadSymbol, METH_VARARGS),
	METHOD(LoadTable, METH_VARARGS),
	METHOD(LoadTableObject, METH_VARARGS),
	METHOD(LoadWindowPack, METH_VARARGS),
	METHOD(LoadWindow, METH_VARARGS),
	METHOD(LoadWindowObject, METH_VARARGS),
	METHOD(LoadWindowFrame, METH_VARARGS),
	METHOD(MemorizeSpell, METH_VARARGS),
	METHOD(ModifyEffect, METH_VARARGS),
	METHOD(MoveToArea, METH_VARARGS),
	METHOD(Quit, METH_NOARGS),
	METHOD(QuitGame, METH_NOARGS),
	METHOD(PlaySound, METH_VARARGS),
	METHOD(PlayMovie, METH_VARARGS),
	METHOD(RemoveItem, METH_VARARGS),
	METHOD(RemoveSpell, METH_VARARGS),
	METHOD(RemoveEffects, METH_VARARGS),
	METHOD(RestParty, METH_VARARGS),
	METHOD(RevealArea, METH_VARARGS),
	METHOD(Roll, METH_VARARGS),
	METHOD(RunEventHandler, METH_VARARGS),
	METHOD(SaveCharacter, METH_VARARGS),
	METHOD(SaveGame, METH_VARARGS),
	METHOD(SetDefaultActions, METH_VARARGS),
	METHOD(SetEquippedQuickSlot, METH_VARARGS),
	METHOD(SetFullScreen, METH_VARARGS),
	METHOD(SetGamma, METH_VARARGS),
	METHOD(SetGlobal, METH_VARARGS),
	METHOD(SetInfoTextColor, METH_VARARGS),
	METHOD(SetMapnote, METH_VARARGS),
	METHOD(SetMasterScript, METH_VARARGS),
	METHOD(SetMemorizableSpellsCount, METH_VARARGS),
	METHOD(SetModalState, METH_VARARGS),
	METHOD(SetMouseScrollSpeed, METH_VARARGS),
	METHOD(SetNextScript, METH_VARARGS),
	METHOD(SetPlayerName, METH_VARARGS),
	METHOD(SetPlayerScript, METH_VARARGS),
	METHOD(SetPlayerStat, METH_VARARGS),
	METHOD(SetPlayerString, METH_VARARGS),
	METHOD(SetPlayerSound, METH_VARARGS),
	METHOD(SetPurchasedAmount, METH_VARARGS),
	METHOD(SetRepeatClickFlags, METH_VARARGS),
	METHOD(SetTimedEvent, METH_VARARGS),
	METHOD(SetToken, METH_VARARGS),
	METHOD(SetTooltipDelay, METH_VARARGS),
	METHOD(SetupQuickSlot, METH_VARARGS),
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
	METHOD(UseItem, METH_VARARGS),
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
	METHOD(Control_TextArea_SetFlags, METH_VARARGS),
	METHOD(Label_SetTextColor, METH_VARARGS),
	METHOD(Label_SetUseRGB, METH_VARARGS),
	METHOD(ScrollBar_SetDefaultScrollBar, METH_VARARGS),
	METHOD(ScrollBar_SetSprites, METH_VARARGS),
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
	METHOD(TextArea_GetCharSounds, METH_VARARGS),
	METHOD(TextArea_GetCharacters, METH_VARARGS),
	METHOD(TextArea_GetPortraits, METH_VARARGS),
	METHOD(TextArea_MoveText, METH_VARARGS),
	METHOD(TextArea_Rewind, METH_VARARGS),
	METHOD(TextArea_Scroll, METH_VARARGS),
	METHOD(TextArea_SetHistory, METH_VARARGS),
	METHOD(TextEdit_ConvertEdit, METH_VARARGS),
	METHOD(TextEdit_SetBufferLength, METH_VARARGS),
	METHOD(Window_CreateButton, METH_VARARGS),
	METHOD(Window_CreateLabel, METH_VARARGS),
	METHOD(Window_CreateMapControl, METH_VARARGS),
	METHOD(Window_CreateScrollBar, METH_VARARGS),
	METHOD(Window_CreateTextEdit, METH_VARARGS),
	METHOD(Window_CreateWorldMapControl, METH_VARARGS),
	METHOD(Window_DeleteControl, METH_VARARGS),
	METHOD(Window_GetControl, METH_VARARGS),
	METHOD(Window_HasControl, METH_VARARGS),
	METHOD(Window_Invalidate, METH_VARARGS),
	METHOD(Window_SetFrame, METH_VARARGS),
	METHOD(Window_SetPicture, METH_VARARGS),
	METHOD(Window_SetPos, METH_VARARGS),
	METHOD(Window_SetSize, METH_VARARGS),
	METHOD(Window_SetVisible, METH_VARARGS),
	METHOD(Window_SetupControls, METH_VARARGS),
	METHOD(Window_SetupEquipmentIcons, METH_VARARGS),
	METHOD(Window_SetupSpellIcons, METH_VARARGS),
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
	pDict = NULL; //borrowed, but used outside a function
	pModule = NULL; //should decref it
	pMainDic = NULL; //borrowed, but used outside a function
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
	if (SpecialSpells) {
		free(SpecialSpells);
		SpecialSpells=NULL;
	}
	if (SpecialItems) {
		free(SpecialItems);
		SpecialItems=NULL;
	}
	if (UsedItems) {
		free(UsedItems);
		UsedItems=NULL;
	}
	if (ItemSounds) {
		free(ItemSounds);
		ItemSounds=NULL;
	}

	StoreSpellsCount = -1;
	SpecialSpellsCount = -1;
	SpecialItemsCount = -1;
	UsedItemsCount = -1;
	ItemSoundsCount = -1;
	ReputationIncrease[0]=(int) UNINIT_IEDWORD;
	ReputationDonation[0]=(int) UNINIT_IEDWORD;
	GUIAction[0]=UNINIT_IEDWORD;
}

PyDoc_STRVAR( GemRB__doc,
"Module exposing GemRB data and engine internals\n\n"
"This module exposes to python GUIScripts GemRB engine data and internals."
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
	PyObject* pGemRB = Py_InitModule3( "GemRB", GemRBMethods, GemRB__doc );
	if (!pGemRB) {
		return false;
	}

	PyObject* p_GemRB = Py_InitModule3( "_GemRB", GemRBInternalMethods, GemRB_internal__doc );
	if (!p_GemRB) {
		return false;
	}

	char string[256];

	sprintf( string, "import sys" );
	if (PyRun_SimpleString( string ) == -1) {
		printMessage( "GUIScript", string, RED );
		return false;
	}
	char path[_MAX_PATH];
	char path2[_MAX_PATH];

	PathJoin(path, core->GUIScriptsPath, "GUIScripts", NULL);

	// use the iwd guiscripts for how, but leave its override
	if (stricmp( core->GameType, "how" ) == 0) {
		PathJoin(path2, path, "iwd", NULL);
	} else {
		PathJoin(path2, path, core->GameType, NULL);
	}

#ifdef WIN32
	char *p;

	for (p = path; *p != 0; p++)
	{
		if (*p == '\\')
			*p = '/';
	}

	for (p = path2; *p != 0; p++)
	{
		if (*p == '\\')
			*p = '/';
	}
#endif

	sprintf( string, "sys.path.append(\"%s\")", path2 );
	if (PyRun_SimpleString( string ) == -1) {
		printMessage( "GUIScript", string, RED );
		return false;
	}
	sprintf( string, "sys.path.append(\"%s\")", path );
	if (PyRun_SimpleString( string ) == -1) {
		printMessage( "GUIScript", string, RED );
		return false;
	}
	sprintf( string, "import GemRB\n");
	if (PyRun_SimpleString( "import GemRB" ) == -1) {
		printMessage( "GUIScript", string, RED );
		return false;
	}

	sprintf( string, "GemRB.GameType = \"%s\"", core->GameType);
	if (PyRun_SimpleString( string ) == -1) {
		printMessage( "GUIScript", string, RED );
		return false;
	}

#if PY_MAJOR_VERSION == 2
#if PY_MINOR_VERSION > 5
	// warn about python stuff that will go away in 3.0
	Py_Py3kWarningFlag = true;
#endif
#endif

	if (PyRun_SimpleString( "from GUIDefines import *" ) == -1) {
		printMessage( "GUIScript", " ", RED );
		printf("Check if %s/GUIDefines.py exists! ", path);
		return false;
	}

	if (PyRun_SimpleString( "from GUIClasses import *" ) == -1) {
		printMessage( "GUIScript", " ", RED );
		printf("Check if %s/GUIClasses.py exists! ", path);
		return false;
	}

	if (PyRun_SimpleString( "from GemRB import *" ) == -1) {
		printMessage( "GUIScript", " ", RED );
		printf("builtin GemRB module failed to load!!! ");
		return false;
	}

	PyObject *pMainMod = PyImport_AddModule( "__main__" );
	/* pMainMod is a borrowed reference */
	pMainDic = PyModule_GetDict( pMainMod );
	/* pMainDic is a borrowed reference */

	PyObject *pClassesMod = PyImport_AddModule( "GUIClasses" );
	/* pClassesMod is a borrowed reference */
	pGUIClasses = PyModule_GetDict( pClassesMod );
	/* pGUIClasses is a borrowed reference */

	return true;
}

bool GUIScript::LoadScript(const char* filename)
{
	if (!Py_IsInitialized()) {
		return false;
	}
	printMessage( "GUIScript", "Loading Script ", WHITE );
	printf( "%s...", filename );

	char path[_MAX_PATH];
	strcpy( path, filename );

	PyObject *pName = PyString_FromString( filename );
	/* Error checking of pName left out */
	if (pName == NULL) {
		printStatus( "ERROR", LIGHT_RED );
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
		printStatus( "ERROR", LIGHT_RED );
		return false;
	}
	printStatus( "OK", LIGHT_GREEN );
	return true;
}

/* Similar to RunFunction, but with parameters, and doesn't necessarily fails */
PyObject *GUIScript::CallbackFunction(const char* fname, PyObject* pArgs)
{
	if (!Py_IsInitialized()) {
		return NULL;
	}
	if (pDict == NULL) {
		return NULL;
	}
	PyObject *pFunc = PyDict_GetItemString( pDict, (char *) fname );
	/* pFunc: Borrowed reference */
	if (( !pFunc ) || ( !PyCallable_Check( pFunc ) )) {
		printMessage("GUIScript", " ", LIGHT_RED);
		printf("%s is not callable!\n", fname);
		return NULL;
	}
	PyObject *pValue = PyObject_CallObject( pFunc, pArgs );
	if (pValue == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
	}
	return pValue;
}

bool GUIScript::RunFunction(const char* fname, bool error, int intparam)
{
	if (!Py_IsInitialized()) {
		return false;
	}
	if (pDict == NULL) {
		return false;
	}

	PyObject* pFunc, * pArgs, * pValue;

	pFunc = PyDict_GetItemString( pDict, (char *) fname );
	/* pFunc: Borrowed reference */
	if (( !pFunc ) || ( !PyCallable_Check( pFunc ) )) {
		if (error) {
			printMessage( "GUIScript", "Missing function:", LIGHT_RED );
			printf("%s\n", fname);
		}
		return false;
	}
	if (intparam == -1) {
		pArgs = NULL;
	} else {
		pArgs = Py_BuildValue("(i)", intparam);
	}
	pValue = PyObject_CallObject( pFunc, pArgs );
	Py_XDECREF( pArgs );
	if (pValue == NULL) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
		return false;
	}
	Py_DECREF( pValue );
	return true;
}

/** Exec a single String */
void GUIScript::ExecString(const char* string)
{
	if (PyRun_SimpleString( (char *) string ) == -1) {
		if (PyErr_Occurred()) {
			PyErr_Print();
		}
	}
}

PyObject* GUIScript::ConstructObject(const char* type, PyObject* pArgs)
{
	char classname[_MAX_PATH] = "G";
	strncat(classname, type, _MAX_PATH - 2);
	if (!pGUIClasses) {
		fprintf(stderr, "Tried to use an object (%s) before script compiled!\n", classname);
		return 0;
	}

	PyObject* cobj = PyDict_GetItemString( pGUIClasses, classname );
	if (!cobj) {
		fprintf(stderr, "Failed to lookup name '%s'\n", classname);
		return 0;
	}
	PyObject* ret = PyObject_Call(cobj, pArgs, NULL);
	if (!ret) {
		fprintf(stderr, "Failed to call constructor\n");
		return 0;
	}
	return ret;
}

#include "plugindef.h"

GEMRB_PLUGIN(0x1B01BE6B, "GUI Script Engine (Python)")
PLUGIN_CLASS(IE_GUI_SCRIPT_CLASS_ID, GUIScript)
END_PLUGIN()
