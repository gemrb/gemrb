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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/GUIScript/GUIScript.cpp,v 1.229 2004/10/17 18:11:25 avenger_teambg Exp $
 *
 */

#include "GUIScript.h"
#include "../Core/Interface.h"
#include "../Core/Map.h"

#include "../Core/Label.h"
#include "../Core/AnimationMgr.h"
#include "../Core/GameControl.h"
#include "../Core/MapMgr.h"
#include "../Core/WorldMapMgr.h"
#include "../Core/SpellMgr.h"
#include "../Core/ItemMgr.h"
#include "../Core/StoreMgr.h"
#include "../Core/WorldMapControl.h"
#include "../Core/MapControl.h"
#include "../Core/SoundMgr.h"

//this stuff is missing from Python 2.2
#ifndef PyDoc_VAR
#define PyDoc_VAR(name) static char name[]
#endif

#ifndef PyDoc_STR
#  ifdef WITH_DOC_STRINGS
#  define PyDoc_STR(str) str
#  else
#  define PyDoc_STR(str) ""
#  endif
#endif

#ifndef PyDoc_STRVAR
#define PyDoc_STRVAR(name,str) PyDoc_VAR(name) = PyDoc_STR(str)
#endif

// a shorthand for declaring methods in method table
#define METHOD(name, args) {#name, GemRB_ ## name, args, GemRB_ ## name ## __doc}

inline bool valid_number(const char* string, long& val)
{
	char* endpr;

	val = strtol( string, &endpr, 0 );
	return ( const char * ) endpr != string;
}

// Like PyString_FromString(), but for ResRef
inline PyObject* PyString_FromResRef(char* ResRef)
{
	unsigned int i;

	for(i=0;i<sizeof(ieResRef);i++) {
		if(ResRef[i]==0) break;
	}
	return PyString_FromStringAndSize( ResRef, i );
}

/* Sets RuntimeError exception and returns NULL, so this function
 * can be called in `return'. 
 */
inline PyObject* RuntimeError(char* msg)
{
	printMessage( "GUIScript", "Runtime Error:\n", LIGHT_RED );
	PyErr_SetString( PyExc_RuntimeError, msg );
	return NULL;
}

/* Prints error msg for invalid function parameters and also the function's
 * doc string (given as an argument). Then returns NULL, so this function
 * can be called in `return'. The exception should be set by previous
 * call to e.g. PyArg_ParseTuple()
 */
inline PyObject* AttributeError(char* doc_string)
{
	printMessage( "GUIScript", "Syntax Error:\n", LIGHT_RED );
	printf( "%s\n\n", doc_string );
	return NULL;
	
}

inline Control *GetControl( int wi, int ci, int ct)
{
	char errorbuffer[256];

	Window* win = core->GetWindow( wi );
	if (!win) {
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
		snprintf(errorbuffer, sizeof(errorbuffer), "Invalid control type #%d", ct);
		RuntimeError(errorbuffer);
		return NULL;
	}
	return ctrl;
}

PyDoc_STRVAR( GemRB_SetInfoTextColor__doc, 
"SetInfoTextColor(red, green, blue, [alpha])\n\n"
"Sets the color of Floating Messages in GameControl." );

static PyObject* GemRB_SetInfoTextColor(PyObject*, PyObject* args)
{
	int r,g,b,a;

	a=255;
	if (!PyArg_ParseTuple( args, "iii|i", &r, &g, &b, &a)) {
		return AttributeError( GemRB_SetInfoTextColor__doc );
	}
	GameControl* gc = (GameControl *) GetControl( 0, 0, IE_GUI_GAMECONTROL);
	if (!gc) {
		return NULL;
	}
	Color c={r,g,b,a};
	gc->SetInfoTextColor( c );
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
		return NULL;
	}
	gc->UnhideGUI();
	gc->SetCutSceneMode( false );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_HideGUI__doc,
"HideGUI()\n\n"
"Hides the Game GUI." );

static PyObject* GemRB_HideGUI(PyObject*, PyObject* /*args*/)
{
	GameControl* gc = (GameControl *) GetControl( 0, 0, IE_GUI_GAMECONTROL);
	if (!gc) {
		return NULL;
	}
	gc->HideGUI();

	Py_INCREF( Py_None );
	return Py_None;
}

GameControl* StartGameControl()
{
	//making sure that our window is the first one
	core->DelWindow(~0); //deleting ALL windows
	Window* gamewin = new Window( 0xffff, 0, 0, core->Width, core->Height );
	GameControl* gc = new GameControl();
	gc->XPos = 0;
	gc->YPos = 0;
	gc->Width = core->Width;
	gc->Height = core->Height;
	gc->Owner = gamewin;
	gc->ControlID = 0x00000000;
	gc->ControlType = IE_GUI_GAMECONTROL;
	gamewin->AddControl( gc );
	core->AddWindow( gamewin );
	core->SetVisible( 0, 1 );
	//setting the focus to the game control 
	core->GetEventMgr()->SetFocused(gamewin, gc); 
	if (core->GetGUIScriptEngine()->LoadScript( "MessageWindow" )) {
		core->GetGUIScriptEngine()->RunFunction( "OnLoad" );
		gc->UnhideGUI();
	}
	if (core->ConsolePopped) {
		core->PopupConsole();
	}

	return gc;
}

PyDoc_STRVAR( GemRB_GetGameString__doc,
"GetGameVariable(Index\n\n"
"Returns various game strings, known values for index are:\n"
"0 - Loading Mos picture\n"
"1 and above - undefined.");

static PyObject* GemRB_GetGameString(PyObject*, PyObject* args)
{
	int Index;

	if (!PyArg_ParseTuple( args, "i", &Index )) {
		return AttributeError( GemRB_GetGameString__doc );
	}
	if(Index==0) {
		Game *game = core->GetGame();
		if(game) return Py_BuildValue("s", core->GetGame()->LoadMos);
		return Py_BuildValue("s", "");
	}

	return NULL;
}

PyDoc_STRVAR( GemRB_LoadGame__doc,
"LoadGame(Index)\n\n"
"Loads and enters the Game." );

static PyObject* GemRB_LoadGame(PyObject*, PyObject* args)
{
	int GameIndex;

	if (!PyArg_ParseTuple( args, "i", &GameIndex )) {
		return AttributeError( GemRB_LoadGame__doc );
	}
	core->LoadGame( GameIndex );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_EnterGame__doc,
"EnterGame()\n\n"
"Starts new game and enters it." );

static PyObject* GemRB_EnterGame(PyObject*, PyObject* /*args*/)
{
	Game* game = core->GetGame();
	if(!game) {
		return RuntimeError( "No game loaded!" );
	}
	GameControl* gc = StartGameControl();
	Actor* actor = game->GetPC (0);
	if(actor) {
		gc->ChangeMap(actor, true);
	}
	else {
		gc->SetCurrentArea(game->LoadMap(game->CurrentArea));
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_QuitGame__doc,
"QuitGame()\n\n"
"Stops the current game.");
static PyObject* GemRB_QuitGame(PyObject*, PyObject* /*args*/)
{
	core->QuitGame(false);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_MoveTAText__doc,
"MoveTAText(srcWin, srcCtrl, dstWin, dstCtrl)\n\n"
"Copies a TextArea content to another.");

static PyObject* GemRB_MoveTAText(PyObject * /*self*/, PyObject* args)
{
	int srcWin, srcCtrl, dstWin, dstCtrl;

	if (!PyArg_ParseTuple( args, "iiii", &srcWin, &srcCtrl, &dstWin, &dstCtrl )) {
		return AttributeError( GemRB_MoveTAText__doc );
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

PyDoc_STRVAR( GemRB_SetTAAutoScroll__doc,
"SetTAAutoScroll(WindowIndex, ControlIndex, State)\n\n"
"Sets the TextArea auto-scroll feature status.");

static PyObject* GemRB_SetTAAutoScroll(PyObject * /*self*/, PyObject* args)
{
	int wi, ci, state;

	if (!PyArg_ParseTuple( args, "iii", &wi, &ci, &state )) {
		return AttributeError( GemRB_SetTAAutoScroll__doc );
	}

	TextArea* ta = ( TextArea* ) GetControl( wi, ci, IE_GUI_TEXTAREA );
	if (!ta) {
		return NULL;
	}
	ta->AutoScroll = ( state != 0 );

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
	int bufflen = strlen( text ) + 12;
	if(bufflen<12) {
		return AttributeError( GemRB_StatComment__doc );
	}
	char* newtext = ( char* ) malloc( bufflen );
	//this could be DANGEROUS, not anymore (snprintf is your friend)
	snprintf( newtext, bufflen, text, X, Y );
	free( text );
	ret = Py_BuildValue( "s", newtext );
	free( newtext );
	return ret;
}

PyDoc_STRVAR( GemRB_GetString__doc,
"GetString(strref) => string\n\n"
"Returns string for given strref." );

static PyObject* GemRB_GetString(PyObject * /*self*/, PyObject* args)
{
	ieStrRef  strref;

	if (!PyArg_ParseTuple( args, "i", &strref )) {
		return AttributeError( GemRB_GetString__doc );
	}

	return PyString_FromString( core->GetString( strref ) );
}

PyDoc_STRVAR( GemRB_EndCutSceneMode__doc,
"EndCutScreneMode()\n\n"
"Exits the CutScene Mode." );

static PyObject* GemRB_EndCutSceneMode(PyObject * /*self*/, PyObject* /*args*/)
{
	core->SetCutSceneMode( false );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_LoadWindowPack__doc,
"LoadWindowPack(CHUIResRef)\n\n"
"Loads a WindowPack into the Window Manager Module." );

static PyObject* GemRB_LoadWindowPack(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		return AttributeError( GemRB_LoadWindowPack__doc );
	}

	if (!core->LoadWindowPack( string )) {
		return NULL;
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
		return NULL;
	}

	return Py_BuildValue( "i", ret );
}

PyDoc_STRVAR( GemRB_SetWindowSize__doc,
"SetWindowSize(WindowIndex, Width, Height)\n\n"
"Resizes a Window.");

static PyObject* GemRB_SetWindowSize(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, Width, Height;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &Width, &Height )) {
		return AttributeError( GemRB_SetWindowSize__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (!win) {
		return NULL;
	}

	win->Width = Width;
	win->Height = Height;
	win->Invalidate();

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

PyDoc_STRVAR( GemRB_SetWindowPicture__doc,
"SetWindowPicture(WindowIndex, MosResRef)\n\n"
"Changes the background of a Window." );

static PyObject* GemRB_SetWindowPicture(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;
	char* MosResRef;

	if (!PyArg_ParseTuple( args, "is", &WindowIndex, &MosResRef )) {
		return AttributeError( GemRB_SetWindowPicture__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (!win) {
		return NULL;
	}

	DataStream* bkgr = core->GetResourceMgr()->GetResource( MosResRef, IE_MOS_CLASS_ID );

	if (bkgr != NULL) {
		ImageMgr* mos = ( ImageMgr* ) core->GetInterface( IE_MOS_CLASS_ID );
		mos->Open( bkgr, true );
		win->SetBackGround( mos->GetImage(), true );
		core->FreeInterface( mos );
	}
	win->Invalidate();

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetWindowPos__doc,
"SetWindowPos(WindowIndex, X, Y)\n\n"
"Moves a Window." );

static PyObject* GemRB_SetWindowPos(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, X, Y;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &X, &Y )) {
		return AttributeError( GemRB_SetWindowPos__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (!win) {
		return NULL;
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
	char* string;
	int noerror = 0;

	if (!PyArg_ParseTuple( args, "s|i", &string, &noerror )) {
		return AttributeError( GemRB_LoadTable__doc );
	}

	int ind = core->LoadTable( string );
	if (!noerror && ind == -1) {
		printMessage( "GUIScript", "Can't find resource\n", LIGHT_RED );
		return NULL;
	}
	return Py_BuildValue( "i", ind );
}

PyDoc_STRVAR( GemRB_UnloadTable__doc,
"UnloadTable(TableIndex)\n\n"
"Unloads a 2DA Table." );

static PyObject* GemRB_UnloadTable(PyObject * /*self*/, PyObject* args)
{
	int ti;

	if (!PyArg_ParseTuple( args, "i", &ti )) {
		return AttributeError( GemRB_UnloadTable__doc );
	}

	int ind = core->DelTable( ti );
	if (ind == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

#if 0
PyDoc_STRVAR( GemRB_GetTable__doc,
"GetTable(2DAResRef) => TableIndex\n\n"
"Returns a loaded 2DA Table." );

static PyObject* GemRB_GetTable(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		return AttributeError( GemRB_GetTable__doc );
	}

	int ind = core->GetIndex( string );
	if (ind == -1) {
		return NULL;
	}

	return Py_BuildValue( "i", ind );
}
#endif //0

PyDoc_STRVAR( GemRB_GetTableValue__doc,
"GetTableValue(TableIndex, RowIndex/RowString, ColIndex/ColString) => value\n\n"
"Returns a field of a 2DA Table." );

static PyObject* GemRB_GetTableValue(PyObject * /*self*/, PyObject* args)
{
	PyObject* ti, * row, * col;

	if (PyArg_UnpackTuple( args, "ref", 3, 3, &ti, &row, &col )) {
		if (!PyObject_TypeCheck( ti, &PyInt_Type )) {
			return AttributeError( GemRB_GetTableValue__doc );
		}
		int TableIndex = PyInt_AsLong( ti );
		if (( !PyObject_TypeCheck( row, &PyInt_Type ) ) &&
			( !PyObject_TypeCheck( row, &PyString_Type ) )) {
			return AttributeError( GemRB_GetTableValue__doc );
		}
		if (( !PyObject_TypeCheck( col, &PyInt_Type ) ) &&
			( !PyObject_TypeCheck( col, &PyString_Type ) )) {
			return AttributeError( GemRB_GetTableValue__doc );
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
		TableMgr* tm = core->GetTable( TableIndex );
		if (!tm)
			return NULL;
		char* ret;
		if (PyObject_TypeCheck( row, &PyString_Type )) {
			char* rows = PyString_AsString( row );
			char* cols = PyString_AsString( col );
			ret = tm->QueryField( rows, cols );
		} else {
			int rowi = PyInt_AsLong( row );
			int coli = PyInt_AsLong( col );
			ret = tm->QueryField( rowi, coli );
		}
		if (ret == NULL)
			return NULL;

		long val;
		if (valid_number( ret, val )) {
			return Py_BuildValue( "l", val );
		}
		return Py_BuildValue( "s", ret );
	}

	return NULL;
}

PyDoc_STRVAR( GemRB_FindTableValue__doc,
"FindTableValue(TableIndex, ColumnIndex, Value) => Row\n\n"
"Returns the first rowcount of a field of a 2DA Table." );

static PyObject* GemRB_FindTableValue(PyObject * /*self*/, PyObject* args)
{
	int ti, col, row;
	long Value;

	if (!PyArg_ParseTuple( args, "iil", &ti, &col, &Value )) {
		return AttributeError( GemRB_FindTableValue__doc );
	}

	TableMgr* tm = core->GetTable( ti );
	if (tm == NULL) {
		return NULL;
	}
	for (row = 0; row < tm->GetRowCount(); row++) {
		char* ret = tm->QueryField( row, col );
		long val;
		if (valid_number( ret, val ) && (Value == val) )
			return Py_BuildValue( "i", row );
	}
	return Py_BuildValue( "i", -1 ); //row not found
}

PyDoc_STRVAR( GemRB_GetTableRowIndex__doc,
"GetTableRowIndex(TableIndex, RowName) => Row\n\n"
"Returns the Index of a Row in a 2DA Table." );

static PyObject* GemRB_GetTableRowIndex(PyObject * /*self*/, PyObject* args)
{
	int ti;
	char* rowname;

	if (!PyArg_ParseTuple( args, "is", &ti, &rowname )) {
		return AttributeError( GemRB_GetTableRowIndex__doc );
	}

	TableMgr* tm = core->GetTable( ti );
	if (tm == NULL) {
		return NULL;
	}
	int row = tm->GetRowIndex( rowname );
	//no error if the row doesn't exist
	return Py_BuildValue( "i", row );
}

PyDoc_STRVAR( GemRB_GetTableRowName__doc,
"GetTableRowName(TableIndex, RowIndex) => string\n\n"
"Returns the Name of a Row in a 2DA Table." );

static PyObject* GemRB_GetTableRowName(PyObject * /*self*/, PyObject* args)
{
	int ti, row;

	if (!PyArg_ParseTuple( args, "ii", &ti, &row )) {
		return AttributeError( GemRB_GetTableRowName__doc );
	}

	TableMgr* tm = core->GetTable( ti );
	if (tm == NULL) {
		return NULL;
	}
	const char* str = tm->GetRowName( row );
	if (str == NULL) {
		return NULL;
	}

	return Py_BuildValue( "s", str );
}

PyDoc_STRVAR( GemRB_GetTableRowCount__doc,
"GetTableRowCount(TableIndex) => RowCount\n\n"
"Returns the number of rows in a 2DA Table." );

static PyObject* GemRB_GetTableRowCount(PyObject * /*self*/, PyObject* args)
{
	int ti;

	if (!PyArg_ParseTuple( args, "i", &ti )) {
		return AttributeError( GemRB_GetTableRowCount__doc );
	}

	TableMgr* tm = core->GetTable( ti );
	if (tm == NULL) {
		return NULL;
	}

	return Py_BuildValue( "i", tm->GetRowCount() );
}

PyDoc_STRVAR( GemRB_LoadSymbol__doc,
"LoadSymbol(IDSResRef) => SymbolIndex\n\n" 
"Loads a IDS Symbol Table." );

static PyObject* GemRB_LoadSymbol(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		return AttributeError( GemRB_LoadSymbol__doc );
	}

	int ind = core->LoadSymbol( string );
	if (ind == -1) {
		return NULL;
	}

	return Py_BuildValue( "i", ind );
}

PyDoc_STRVAR( GemRB_UnloadSymbol__doc,
"UnloadSymbol(SymbolIndex)\n\n"
"Unloads a IDS Symbol Table." );

static PyObject* GemRB_UnloadSymbol(PyObject * /*self*/, PyObject* args)
{
	int si;

	if (!PyArg_ParseTuple( args, "i", &si )) {
		return AttributeError( GemRB_UnloadSymbol__doc );
	}

	int ind = core->DelSymbol( si );
	if (ind == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetSymbol__doc,
"GetSymbol(string) => int\n\n"
"Returns a Loaded IDS Symbol Table." );

static PyObject* GemRB_GetSymbol(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		return AttributeError( GemRB_GetSymbol__doc );
	}

	int ind = core->GetSymbolIndex( string );
	if (ind == -1) {
		return NULL;
	}

	return Py_BuildValue( "i", ind );
}

PyDoc_STRVAR( GemRB_GetSymbolValue__doc,
"GetSymbolValue(SymbolIndex, StringVal) => int\n"
"GetSymbolValue(SymbolIndex, IntVal) => string\n\n"
"Returns a field of a IDS Symbol Table." );

static PyObject* GemRB_GetSymbolValue(PyObject * /*self*/, PyObject* args)
{
	PyObject* si, * sym;

	if (PyArg_UnpackTuple( args, "ref", 2, 2, &si, &sym )) {
		if (!PyObject_TypeCheck( si, &PyInt_Type )) {
			return AttributeError( GemRB_GetSymbolValue__doc );
		}
		int SymbolIndex = PyInt_AsLong( si );
		if (PyObject_TypeCheck( sym, &PyString_Type )) {
			char* syms = PyString_AsString( sym );
			SymbolMgr* sm = core->GetSymbol( SymbolIndex );
			if (!sm)
				return NULL;
			long val = sm->GetValue( syms );
			return Py_BuildValue( "l", val );
		}
		if (PyObject_TypeCheck( sym, &PyInt_Type )) {
			int symi = PyInt_AsLong( sym );
			SymbolMgr* sm = core->GetSymbol( SymbolIndex );
			if (!sm)
				return NULL;
			const char* str = sm->GetValue( symi );
			return Py_BuildValue( "s", str );
		}
	}
	return AttributeError( GemRB_GetSymbolValue__doc );
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

	return Py_BuildValue( "i", ret );
}

PyDoc_STRVAR( GemRB_QueryText__doc,
"QueryText(WindowIndex, ControlIndex) => string\n\n"
"Returns the Text of a control in a Window." );

static PyObject* GemRB_QueryText(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;

	if (!PyArg_ParseTuple( args, "ii", &wi, &ci )) {
		return AttributeError( GemRB_QueryText__doc );
	}

	TextEdit *ctrl = (TextEdit *) GetControl(wi, ci, IE_GUI_EDIT);
	if (!ctrl) {
		return NULL;
	}

	return Py_BuildValue( "s", ctrl->QueryText() );
}

PyDoc_STRVAR( GemRB_SetText__doc,
"SetText(WindowIndex, ControlIndex, String|Strref) => int\n\n"
"Sets the Text of a control in a Window." );

static PyObject* GemRB_SetText(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci, * str;
	int WindowIndex, ControlIndex;
	long StrRef;
	char* string;
	int ret;

	if (PyArg_UnpackTuple( args, "ref", 3, 3, &wi, &ci, &str )) {
		if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
			!PyObject_TypeCheck( ci,
														&PyInt_Type ) ||
			( !PyObject_TypeCheck( str, &PyString_Type ) &&
			!PyObject_TypeCheck( str,
																&PyInt_Type ) )) {
			return AttributeError( GemRB_SetText__doc );
		}

		WindowIndex = PyInt_AsLong( wi );
		ControlIndex = PyInt_AsLong( ci );
		if (PyObject_TypeCheck( str, &PyString_Type )) {
			string = PyString_AsString( str );
			if (string == NULL)
				return NULL;
			ret = core->SetText( WindowIndex, ControlIndex, string );
			if (ret == -1)
				return NULL;
		} else {
			StrRef = PyInt_AsLong( str );
			char* str = NULL;
			if (StrRef == -1) {
				str = ( char * ) malloc( 20 );
				sprintf( str, "GemRB v%.1f.%d.%d", GEMRB_RELEASE / 1000.0,
					GEMRB_API_NUM, GEMRB_SDK_REV );
			} else
				str = core->GetString( StrRef );
			ret = core->SetText( WindowIndex, ControlIndex, str );
			if (ret == -1) {
				free( str );
				return NULL;
			}
			free( str );
		}
	} else {
		return NULL;
	}

	return Py_BuildValue( "i", ret );
}

PyDoc_STRVAR( GemRB_TextAreaAppend__doc,
"TextAreaAppend(WindowIndex, ControlIndex, String|Strref [, Row]) => int\n\n"
"Appends the Text to the TextArea Control in the Window." );

static PyObject* GemRB_TextAreaAppend(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci, * str, * row = NULL;
	int WindowIndex, ControlIndex, StrRef, Row;
	char* string;
	int ret;

	if (PyArg_UnpackTuple( args, "ref", 3, 4, &wi, &ci, &str, &row )) {
		if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
			!PyObject_TypeCheck( ci, &PyInt_Type ) ||
			( !PyObject_TypeCheck( str, &PyString_Type ) &&
			!PyObject_TypeCheck( str, &PyInt_Type ) )) {
			return AttributeError( GemRB_TextAreaAppend__doc );
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
					"Syntax Error: SetText row must be integer\n", LIGHT_RED );
				return NULL;
			}
			Row = PyInt_AsLong( row );
			if (Row > ta->GetRowCount() - 1)
				Row = -1;
		} else
			Row = ta->GetRowCount() - 1;
		if (PyObject_TypeCheck( str, &PyString_Type )) {
			string = PyString_AsString( str );
			if (string == NULL)
				return NULL;
			ret = ta->AppendText( string, Row );
		} else {
			StrRef = PyInt_AsLong( str );
			char* str = core->GetString( StrRef );
			ret = ta->AppendText( str, Row );
			free( str );
		}
	} else {
		return NULL;
	}

	return Py_BuildValue( "i", ret );
}

PyDoc_STRVAR( GemRB_TextAreaClear__doc,
"TextAreaClear(WindowIndex, ControlIndex)\n\n"
"Clears the Text from the TextArea Control in the Window." );

static PyObject* GemRB_TextAreaClear(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci;
	int WindowIndex, ControlIndex;

	if (PyArg_UnpackTuple( args, "ref", 2, 2, &wi, &ci )) {
		if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
			!PyObject_TypeCheck( ci, &PyInt_Type )) { 
			return AttributeError( GemRB_TextAreaClear__doc );
		}
		WindowIndex = PyInt_AsLong( wi );
		ControlIndex = PyInt_AsLong( ci );
		TextArea* ta = ( TextArea* ) GetControl( WindowIndex, ControlIndex, IE_GUI_TEXTAREA);
		if(!ta) {
			return NULL;
		}
		ta->PopLines( ta->GetRowCount() );
	} else {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetTooltip__doc,
"SetTooltip(WindowIndex, ControlIndex, String|Strref) => int\n\n"
"Sets control's tooltip." );

static PyObject* GemRB_SetTooltip(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci, * str;
	int WindowIndex, ControlIndex;
	long StrRef;
	char* string;
	int ret;

	if (PyArg_UnpackTuple( args, "ref", 3, 3, &wi, &ci, &str )) {
		if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
			!PyObject_TypeCheck( ci,
														&PyInt_Type ) ||
			( !PyObject_TypeCheck( str, &PyString_Type ) &&
			!PyObject_TypeCheck( str,
																&PyInt_Type ) )) {
			return AttributeError( GemRB_SetTooltip__doc );
		}

		WindowIndex = PyInt_AsLong( wi );
		ControlIndex = PyInt_AsLong( ci );
		if (PyObject_TypeCheck( str, &PyString_Type )) {
			string = PyString_AsString( str );
			if (string == NULL)
				return NULL;
			ret = core->SetTooltip( WindowIndex, ControlIndex, string );
			if (ret == -1)
				return NULL;
		} else {
			StrRef = PyInt_AsLong( str );
			char* str = NULL;
			if (StrRef == -1) {
				str = ( char * ) malloc( 20 );
				sprintf( str, "GemRB v%.1f.%d.%d", GEMRB_RELEASE / 1000.0,
					GEMRB_API_NUM, GEMRB_SDK_REV );
			} else
				str = core->GetString( StrRef );
			ret = core->SetTooltip( WindowIndex, ControlIndex, str );
			if (ret == -1) {
				free( str );
				return NULL;
			}
			free( str );
		}
	} else {
		return NULL;
	}

	return Py_BuildValue( "i", ret );
}

PyDoc_STRVAR( GemRB_SetVisible__doc,
"SetVisible(WindowIndex, Visible)\n\n"
"Sets the Visibility Flag of a Window." );

static PyObject* GemRB_SetVisible(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;
	int visible;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &visible )) {
		return AttributeError( GemRB_SetVisible__doc );
	}

	int ret = core->SetVisible( WindowIndex, visible );
	if (ret == -1) {
		return NULL;
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
	strncpy( core->GlobalScript, script, 8 );
	strncpy( core->WorldMapName, worldmap, 8 );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_ShowModal__doc,
"ShowModal(WindowIndex, [Shadow=MODAL_SHADOW_NONE])\n\n"
"Show a Window on Screen setting the Modal Status."
"If Shadow is MODAL_SHADOW_GRAY, other windows are grayed. "
"If Shadow is MODAL_SHADOW_BLACK, they are blacked out." );

static PyObject* GemRB_ShowModal(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, Shadow = MODAL_SHADOW_NONE;

	if (!PyArg_ParseTuple( args, "i|i", &WindowIndex, &Shadow )) {
		return AttributeError( GemRB_ShowModal__doc );
	}

	int ret = core->ShowModal( WindowIndex, Shadow );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetEvent__doc,
"SetEvent(WindowIndex, ControlIndex, EventMask, FunctionName)\n\n"
"Sets an event of a control on a window to a script defined function." );

static PyObject* GemRB_SetEvent(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int event;
	char* funcName;

	if (!PyArg_ParseTuple( args, "iiis", &WindowIndex, &ControlIndex, &event,
			&funcName )) {
		return AttributeError( GemRB_SetEvent__doc );
	}

	int ret = core->SetEvent( WindowIndex, ControlIndex, event, funcName );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetNextScript__doc,
"SetNextScript(GUIScriptName)\n\n"
"Sets the Next Script File to be loaded." );

static PyObject* GemRB_SetNextScript(PyObject * /*self*/, PyObject* args)
{
	char* funcName;

	if (!PyArg_ParseTuple( args, "s", &funcName )) {
		return AttributeError( GemRB_SetNextScript__doc );
	}

	strcpy( core->NextScript, funcName );
	core->ChangeScript = true;

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetControlStatus__doc,
"SetControlStatus(WindowIndex, ControlIndex, Status)\n\n"
"Sets the status of a Control." );

static PyObject* GemRB_SetControlStatus(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int status;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &status )) {
		return AttributeError( GemRB_SetControlStatus__doc );
	}

	int ret = core->SetControlStatus( WindowIndex, ControlIndex, status );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetVarAssoc__doc,
"SetVarAssoc(WindowIndex, ControlIndex, VariableName, LongValue)\n\n"
"Sets the name of the Variable associated with a control." );

static PyObject* GemRB_SetVarAssoc(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	ieDword Value;
	char* VarName;

	if (!PyArg_ParseTuple( args, "iisl", &WindowIndex, &ControlIndex,
			&VarName, &Value )) {
		return AttributeError( GemRB_SetVarAssoc__doc );
	}

	Control* ctrl = GetControl( WindowIndex, ControlIndex, -1 );
	if (!ctrl) {
		return NULL;
	}

	strncpy( ctrl->VarName, VarName, MAX_VARIABLE_LENGTH );
	ctrl->Value = Value;
	/** setting the correct state for this control */
	/** it is possible to set up a default value, if Lookup returns false, use it */
	Value = 0;
	core->GetDictionary()->Lookup( VarName, Value );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_UnloadWindow__doc,
"UnloadWindow(WindowIndex)\n\n"
"Unloads a previously Loaded Window." );

static PyObject* GemRB_UnloadWindow(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;

	if (!PyArg_ParseTuple( args, "i", &WindowIndex )) {
		return AttributeError( GemRB_UnloadWindow__doc );
	}

	int ret = core->DelWindow( WindowIndex );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_InvalidateWindow__doc,
"InvalidateWindow(WindowIndex)\n\n"
"Invalidates the given Window." );

static PyObject* GemRB_InvalidateWindow(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;

	if (!PyArg_ParseTuple( args, "i", &WindowIndex )) {
		return AttributeError( GemRB_InvalidateWindow__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (!win) {
		return NULL;
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

	return Py_BuildValue( "i", WindowIndex );
}

PyDoc_STRVAR( GemRB_CreateLabel__doc,
"CreateLabel(WindowIndex, ControlID, x, y, w, h, font, text, align) => ControlIndex\n\n"
"Creates and adds a new Label to a Window." );

static PyObject* GemRB_CreateLabel(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h, align;
	char* font, * text;

	if (!PyArg_ParseTuple( args, "iiiiiissi", &WindowIndex, &ControlID, &x,
			&y, &w, &h, &font, &text, &align )) {
		return AttributeError( GemRB_CreateLabel__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
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

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetLabelTextColor__doc,
"SetLabelTextColor(WindowIndex, ControlIndex, red, green, blue)\n\n"
"Sets the Text Color of a Label Control." );

static PyObject* GemRB_SetLabelTextColor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, r, g, b;

	if (!PyArg_ParseTuple( args, "iiiii", &WindowIndex, &ControlIndex, &r, &g,
			&b )) {
		return AttributeError( GemRB_SetLabelTextColor__doc );
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

PyDoc_STRVAR( GemRB_CreateButton__doc,
"CreateButton(WindowIndex, ControlID, x, y, w, h) => ControlIndex\n\n"
"Creates and adds a new Button to a Window." );

static PyObject* GemRB_CreateButton(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h;

	if (!PyArg_ParseTuple( args, "iiiiii", &WindowIndex, &ControlID, &x, &y,
			&w, &h )) {
		return AttributeError( GemRB_CreateButton__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Button* btn = new Button( false );
	btn->XPos = x;
	btn->YPos = y;
	btn->Width = w;
	btn->Height = h;
	btn->ControlID = ControlID;
	btn->ControlType = IE_GUI_BUTTON;
	btn->Owner = win;
	win->AddControl( btn );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonSprites__doc,
"SetButtonSprites(WindowIndex, ControlIndex, ResRef, Cycle, UnpressedFrame, PressedFrame, SelectedFrame, DisabledFrame)\n\n"
"Sets a Button Sprites Images." );

static PyObject* GemRB_SetButtonSprites(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, cycle, unpressed, pressed, selected,
		disabled;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iisiiiii", &WindowIndex, &ControlIndex,
			&ResRef, &cycle, &unpressed, &pressed, &selected, &disabled )) {
		return AttributeError( GemRB_SetButtonSprites__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if(!btn) {
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

	AnimationMgr* bam = ( AnimationMgr* )
		core->GetInterface( IE_BAM_CLASS_ID );
	DataStream *str = core->GetResourceMgr()->GetResource( ResRef, IE_BAM_CLASS_ID );
	if (!bam->Open(str, true) ) {
		printMessage( "GUIScript", "Error: %s.BAM not Found\n", LIGHT_RED );
		return NULL;
	}
	Sprite2D *tspr = bam->GetFrameFromCycle( (unsigned char) cycle, unpressed);
	btn->SetImage( IE_GUI_BUTTON_UNPRESSED, tspr );
	tspr = bam->GetFrameFromCycle( (unsigned char) cycle, pressed);
	btn->SetImage( IE_GUI_BUTTON_PRESSED, tspr );
	tspr = bam->GetFrameFromCycle( (unsigned char) cycle, selected);
	btn->SetImage( IE_GUI_BUTTON_SELECTED, tspr );
	tspr = bam->GetFrameFromCycle( (unsigned char) cycle, disabled);
	btn->SetImage( IE_GUI_BUTTON_DISABLED, tspr );
	core->FreeInterface( bam );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonBorder__doc,
"SetButtonBorder(WindowIndex, ControlIndex, BorderIndex, dx1, dy1, dx2, dy2, R, G, B, A, [enabled, filled])\n\n"
"Sets border/frame parameters for a button" );

static PyObject* GemRB_SetButtonBorder(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, BorderIndex, dx1, dy1, dx2, dy2, r, g, b, a, enabled = 0, filled = 0;

	if (!PyArg_ParseTuple( args, "iiiiiiiiiii|ii", &WindowIndex, &ControlIndex,
			&BorderIndex, &dx1, &dy1, &dx2, &dy2, &r, &g, &b, &a, &enabled, &filled)) {
		return AttributeError( GemRB_SetButtonBorder__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if(!btn) {
		return NULL;
	}

	Color color = { r, g, b, a };
	btn->SetBorder( BorderIndex, dx1, dy1, dx2, dy2, &color, (bool)enabled, (bool)filled );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_EnableButtonBorder__doc,
"EnableButtonBorder(WindowIndex, ControlIndex, BorderIndex, enabled)\n\n"
"Enable or disable specified border/frame" );

static PyObject* GemRB_EnableButtonBorder(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, BorderIndex, enabled;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex,
			&BorderIndex, &enabled)) {
		return AttributeError( GemRB_EnableButtonBorder__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if(!btn) {
		return NULL;
	}

	btn->EnableBorder( BorderIndex, (bool)enabled );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonFont__doc,
"SetButtonFont(WindowIndex, ControlIndex, FontResRef)\n\n"
"Sets font used for drawing button label" );

static PyObject* GemRB_SetButtonFont(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char* FontResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex,
			&FontResRef)) {
		return AttributeError( GemRB_SetButtonFont__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if(!btn) {
		return NULL;
	}

	btn->SetFont( core->GetFont( FontResRef ));

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonTextColor__doc,
"SetButtonTextColor(WindowIndex, ControlIndex, red, green, blue)\n\n"
"Sets the Text Color of a Button Control." );

static PyObject* GemRB_SetButtonTextColor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, r, g, b, swap = 0;

	if (!PyArg_ParseTuple( args, "iiiiii", &WindowIndex, &ControlIndex, &r, &g, &b, &swap )) {
		return AttributeError( GemRB_SetButtonTextColor__doc );
	}

	Button* but = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if (!but) {
		return NULL;
	}

	Color fore = {r,g, b, 0}, back = {0, 0, 0, 0};

	// FIXME: swap is a hack for fonts which apparently have swapped f & B
	//   clors. Maybe it depends on need_palette?
	if (! swap) 
		but->SetTextColor( fore, back );
	else
		but->SetTextColor( back, fore );


	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_DeleteControl__doc,
"DeleteControl(WindowIndex, ControlID)\n\n"
"Deletes a control from a Window." );

static PyObject* GemRB_DeleteControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlID)) {
		return AttributeError( GemRB_DeleteControl__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}
	int CtrlIndex = core->GetControl( WindowIndex, ControlID );
	if (CtrlIndex == -1) {
		return RuntimeError( "Control is not found" );
	}
	win -> DelControl( CtrlIndex );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_AdjustScrolling__doc,
"AdjustScrolling(WindowIndex, ControlID, x, y)\n\n");

static PyObject* GemRB_AdjustScrolling(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlID, &x, &y )) {
		return AttributeError( GemRB_AdjustScrolling__doc );
	}

	core->AdjustScrolling( WindowIndex, ControlID, x, y );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_CreateWorldMapControl__doc,
"CreateWorldMapControl(WindowIndex, ControlID, x, y, w, h, flags)\n\n"
"Creates and adds a new WorldMap control to a Window." );

static PyObject* GemRB_CreateWorldMapControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h, flags;

	if (!PyArg_ParseTuple( args, "iiiiiii", &WindowIndex, &ControlID, &x,
			&y, &w, &h, &flags )) {
		return AttributeError( GemRB_CreateWorldMapControl__doc );
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}
	int CtrlIndex = core->GetControl( WindowIndex, ControlID );
	if (CtrlIndex != -1) {
		Control *ctrl = win->GetControl( CtrlIndex );
		x = ctrl->XPos;
		y = ctrl->YPos;
		w = ctrl->Width;
		h = ctrl->Height;
		flags = ctrl->Value;
		win->DelControl( CtrlIndex );
	}
	WorldMapControl* wmap = new WorldMapControl( );
	wmap->XPos = x;
	wmap->YPos = y;
	wmap->Width = w;
	wmap->Height = h;
	wmap->ControlID = ControlID;
	wmap->ControlType = IE_GUI_WORLDMAP;
	wmap->Owner = win;
	wmap->Value = flags;
	win->AddControl( wmap );

	Py_INCREF( Py_None );
	return Py_None;
}


PyDoc_STRVAR( GemRB_CreateMapControl__doc,
"CreateMapControl(WindowIndex, ControlID, x, y, w, h, "
"[LabelID, FlagResRef[, Flag2ResRef]])\n\n"
"Creates and adds a new Area Map Control to a Window.\n"
"Note: LabelID is an ID, not an index. "
"If there are two flags given, they will be considered a BMP.\n");

static PyObject* GemRB_CreateMapControl(PyObject * /*self*/, PyObject* args)
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
			return AttributeError( GemRB_CreateMapControl__doc );
		}
	}
	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}
	int CtrlIndex = core->GetControl( WindowIndex, ControlID );
	if (CtrlIndex != -1) {
		Control *ctrl = win->GetControl( CtrlIndex );
		x = ctrl->XPos;
		y = ctrl->YPos;
		w = ctrl->Width;
		h = ctrl->Height;
		win->DelControl( CtrlIndex );
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
		map->ConvertToGame = false;
		CtrlIndex = core->GetControl( WindowIndex, LabelID );
		Control *lc = win->GetControl( CtrlIndex );
		map->LinkedLabel = lc;
		ImageMgr *anim = ( ImageMgr* ) core->GetInterface(IE_BMP_CLASS_ID );
		DataStream* str = core->GetResourceMgr()->GetResource( Flag, IE_BMP_CLASS_ID );
		if(anim -> Open(str, true) ) {
				map->Flag[0] = anim->GetImage();
			}
		str = core->GetResourceMgr()->GetResource( Flag2, IE_BMP_CLASS_ID );
		if(anim -> Open(str, true) ) {
				map->Flag[1] = anim->GetImage();
		}
		core->FreeInterface( anim );
		goto setup_done;
	}
	if (Flag) {
		CtrlIndex = core->GetControl( WindowIndex, LabelID );
		Control *lc = win->GetControl( CtrlIndex );
		map->LinkedLabel = lc;
		AnimationMgr *anim = ( AnimationMgr* ) core->GetInterface(IE_BAM_CLASS_ID );
		DataStream* str = core->GetResourceMgr()->GetResource( Flag, IE_BAM_CLASS_ID );
		if(anim -> Open(str, true) ) {
			for(int i=0;i<8;i++) {
				map->Flag[i] = anim->GetFrame(0,i);
			}
			
		}
		core->FreeInterface( anim );
	}
setup_done:
	win->AddControl( map );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetControlPos__doc,
"SetControlPos(WindowIndex, ControlIndex, X, Y)\n\n"
"Moves a Control." );

static PyObject* GemRB_SetControlPos(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, X, Y;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &X, &Y )) {
		return AttributeError( GemRB_SetControlPos__doc );
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

PyDoc_STRVAR( GemRB_SetControlSize__doc,
"SetControlSize(WindowIndex, ControlIndex, Width, Height)\n\n"
"Resizes a Control." );

static PyObject* GemRB_SetControlSize(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Width, Height;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &Width,
			&Height )) {
		return AttributeError( GemRB_SetControlSize__doc );
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

PyDoc_STRVAR( GemRB_SetLabelUseRGB__doc,
"SetLabelUseRGB(WindowIndex, ControlIndex, status)\n\n"
"Tells a Label to use the RGB colors with the text." );

static PyObject* GemRB_SetLabelUseRGB(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, status;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &status )) {
		return AttributeError( GemRB_SetLabelUseRGB__doc );
	}

	Label* lab = (Label *) GetControl(WindowIndex, ControlIndex, IE_GUI_LABEL);
	if (!lab) {
		return NULL;
	}

	lab->useRGB = ( status != 0 );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetTextAreaSelectable__doc,
"SetTextAreaSelectable(WindowIndex, ControlIndex, Flag)\n\n"
"Sets the Selectable Flag of a TextArea." );

static PyObject* GemRB_SetTextAreaSelectable(PyObject * /*self*/,
	PyObject* args)
{
	int WindowIndex, ControlIndex, Flag;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &Flag )) {
		return AttributeError( GemRB_SetTextAreaSelectable__doc );
	}

	TextArea* ta = (TextArea *) GetControl(WindowIndex, ControlIndex, IE_GUI_TEXTAREA);
	if (!ta) {
		return NULL;
	}

	ta->SetSelectable( !!Flag );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonFlags__doc,
"SetButtonFlags(WindowIndex, ControlIndex, Flags, Operation)\n\n"
"Sets the Display Flags of a Button." );

static PyObject* GemRB_SetButtonFlags(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Flags, Operation;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &Flags,
			&Operation )) {
		return AttributeError( GemRB_SetButtonFlags__doc );
	}
	if (Operation< 0 || Operation>2) {
		printMessage( "GUIScript",
			"Syntax Error: SetButtonFlags operation must be 0-2\n", LIGHT_RED );
		return NULL;
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if(!btn) {
		return NULL;
	}

	btn->SetFlags( Flags, Operation );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonState__doc,
"SetButtonState(WindowIndex, ControlIndex, State)\n\n"
"Sets the state of a Button Control." );

static PyObject* GemRB_SetButtonState(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, state;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &state )) {
		return AttributeError( GemRB_SetButtonState__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if(!btn) {
		return NULL;
	}

	btn->SetState( state );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonPicture__doc,
"SetButtonPicture(WindowIndex, ControlIndex, PictureResRef)\n\n"
"Sets the Picture of a Button Control from a BMP file." );

static PyObject* GemRB_SetButtonPicture(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex, &ResRef )) {
		return AttributeError( GemRB_SetButtonPicture__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if(!btn) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		btn->SetPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

	DataStream* str = core->GetResourceMgr()->GetResource( ResRef, IE_BMP_CLASS_ID );
	if (str == NULL) {
		return NULL;
	}
	ImageMgr* im = ( ImageMgr* ) core->GetInterface( IE_BMP_CLASS_ID );
	if (im == NULL) {
		delete ( str );
		return NULL;
	}

	if (!im->Open( str, true )) {
		core->FreeInterface( im );
		return NULL;
	}

	Sprite2D* Picture = im->GetImage();
	if (Picture == NULL) {
		core->FreeInterface( im );
		return NULL;
	}

	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonMOS__doc,
"SetButtonMOS(WindowIndex, ControlIndex, MOSResRef)\n\n"
"Sets the Picture of a Button Control from a MOS file." );

static PyObject* GemRB_SetButtonMOS(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex, &ResRef )) {
		return AttributeError( GemRB_SetButtonMOS__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if(!btn) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		btn->SetPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

	DataStream* str = core->GetResourceMgr()->GetResource( ResRef,
												IE_MOS_CLASS_ID );
	if (str == NULL) {
		return NULL;
	}
	ImageMgr* im = ( ImageMgr* ) core->GetInterface( IE_MOS_CLASS_ID );
	if (im == NULL) {
		delete ( str );
		return NULL;
	}

	if (!im->Open( str, true )) {
		core->FreeInterface( im );
		return NULL;
	}

	Sprite2D* Picture = im->GetImage();
	if (Picture == NULL) {
		core->FreeInterface( im );
		return NULL;
	}

	btn->SetFlags( 0x82, OP_OR );
	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonPLT__doc,
"SetButtonPLT(WindowIndex, ControlIndex, PLTResRef, col1, col2, col3, col4, col5, col6, col7, col8)\n\n"
"Sets the Picture of a Button Control from a PLT file." );

static PyObject* GemRB_SetButtonPLT(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, col1, col2, col3, col4, col5, col6, col7,
		col8;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iisiiiiiiii", &WindowIndex, &ControlIndex,
			&ResRef, &col1, &col2, &col3, &col4, &col5, &col6, &col7, &col8 )) {
		return AttributeError( GemRB_SetButtonPLT__doc );
	}

	Button* btn = ( Button* ) GetControl(WindowIndex, ControlIndex, IE_GUI_BUTTON);
	if(!btn) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		btn->SetPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

	DataStream* str = core->GetResourceMgr()->GetResource( ResRef,
												IE_PLT_CLASS_ID );
	if (str == NULL) {
		printf ("No stream\n");
		return NULL;
	}
	ImageMgr* im = ( ImageMgr* ) core->GetInterface( IE_PLT_CLASS_ID );
	if (im == NULL) {
		printf ("No image\n");
		delete ( str );
		return NULL;
	}

	if (!im->Open( str, true )) {
		printf ("Can't open image\n");
		core->FreeInterface( im );
		return NULL;
	}

	im->GetPalette( 0, col1, NULL );
	im->GetPalette( 1, col2, NULL );
	im->GetPalette( 2, col3, NULL );
	im->GetPalette( 3, col4, NULL );
	im->GetPalette( 4, col5, NULL );
	im->GetPalette( 5, col6, NULL );
	im->GetPalette( 6, col7, NULL );
	im->GetPalette( 7, col8, NULL );
	Sprite2D* Picture = im->GetImage();
	if (Picture == NULL) {
		printf ("Picture == NULL\n");
		core->FreeInterface( im );
		return NULL;
	}

	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetButtonBAM__doc,
"SetButtonBAM(WindowIndex, ControlIndex, BAMResRef, CycleIndex, FrameIndex, col1)\n\n"
"Sets the Picture of a Button Control from a BAM file. If col1 is >= 0, changes palette picture's palette to one specified by col1. Since it uses 12 colors palette, it has issues in PST." );

static PyObject* GemRB_SetButtonBAM(PyObject * /*self*/, PyObject* args)
{
	int wi, ci, CycleIndex, FrameIndex, col1;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iisiii", &wi, &ci,
			&ResRef, &CycleIndex, &FrameIndex, &col1 )) {
		return AttributeError( GemRB_SetButtonBAM__doc );
	}

	Button* btn = ( Button* ) GetControl(wi, ci, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		btn->SetPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

	DataStream* str = core->GetResourceMgr()->GetResource( ResRef, IE_BAM_CLASS_ID );
	if (str == NULL) {
		return NULL;
	}
	AnimationMgr* am = ( AnimationMgr* )
		core->GetInterface( IE_BAM_CLASS_ID );
	if (am == NULL) {
		delete ( str );
		return NULL;
	}

	if (!am->Open( str, true )) {
		core->FreeInterface( am );
		return NULL;
	}

	Sprite2D* Picture = am->GetFrameFromCycle( CycleIndex, FrameIndex );
	if (Picture == NULL) {
		core->FreeInterface( am );
		return NULL;
	}

	if (col1 >= 0) {
		Color* pal = core->GetPalette( col1, 12 );
		Color* orgpal = core->GetVideoDriver()->GetPalette( Picture );
		memcpy( &orgpal[4], pal, 12 * sizeof( Color ) );
		core->GetVideoDriver()->SetPalette( Picture, orgpal );
		free( pal );
		free( orgpal );
	}

	btn->SetPicture( Picture );

	core->FreeInterface( am );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_PlaySound__doc,
"PlaySound(SoundResource)\n\n"
"Plays a Sound." );

static PyObject* GemRB_PlaySound(PyObject * /*self*/, PyObject* args)
{
	char* ResRef;
	int XPos, YPos;
	unsigned long int flags;

	if (!PyArg_ParseTuple( args, "s", &ResRef )) {
		return AttributeError( GemRB_PlaySound__doc );
	}

	//this could be less hardcoded, but would do for a test
	XPos=YPos=0;
	flags=0;
	Game *game=core->GetGame();
	if(game) {
		Scriptable *pc=game->GetPC(game->GetSelectedPCSingle());
		if(pc) {
			XPos=pc->Pos.x;
			YPos=pc->Pos.y;
			flags |= GEM_SND_SPEECH;
		}
	}
	
	int ret = core->GetSoundMgr()->Play( ResRef, XPos, YPos, flags );
	if (!ret) {
		return NULL;
	}

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
"LoadMusicPL(MusicPlayListResource)\n\n"
"Loads and starts a Music PlayList." );

static PyObject* GemRB_LoadMusicPL(PyObject * /*self*/, PyObject* args)
{
	char* ResRef;
	int HardEnd = 0;

	if (!PyArg_ParseTuple( args, "s|i", &ResRef, &HardEnd )) {
		return AttributeError( GemRB_LoadMusicPL__doc );
	}

	core->GetMusicMgr()->SwitchPlayList( ResRef, HardEnd );

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
	char* Variable;
	char* value;

	if (!PyArg_ParseTuple( args, "ss", &Variable, &value )) {
		return AttributeError( GemRB_SetToken__doc );
	}

	char* newvalue = ( char* ) malloc( strlen( value ) + 1 );  //duplicating the string
	strcpy( newvalue, value );
	core->GetTokenDictionary()->SetAt( Variable, newvalue );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetVar__doc,
"SetVar(VariableName, Value)\n\n"
"Set/Create a Variable in the Global Dictionary." );

static PyObject* GemRB_SetVar(PyObject * /*self*/, PyObject* args)
{
	char* Variable;
	unsigned long value;

	if (!PyArg_ParseTuple( args, "sl", &Variable, &value )) {
		return AttributeError( GemRB_SetVar__doc );
	}

	core->GetDictionary()->SetAt( Variable, value );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetToken__doc,
"GetToken(VariableName) => string\n\n"
"Get a Variable value from the Token Dictionary." );

static PyObject* GemRB_GetToken(PyObject * /*self*/, PyObject* args)
{
	char* Variable;
	char* value;

	if (!PyArg_ParseTuple( args, "s", &Variable )) {
		return AttributeError( GemRB_GetToken__doc );
	}

	//returns only the pointer
	if (!core->GetTokenDictionary()->Lookup( Variable, value )) {
		return Py_BuildValue( "s", "" );
	}

	return Py_BuildValue( "s", value );
}

PyDoc_STRVAR( GemRB_GetVar__doc,
"GetVar(VariableName) => int\n\n"
"Get a Variable value from the Global Dictionary." );

static PyObject* GemRB_GetVar(PyObject * /*self*/, PyObject* args)
{
	char* Variable;
	ieDword value;

	if (!PyArg_ParseTuple( args, "s", &Variable )) {
		return AttributeError( GemRB_GetVar__doc );
	}

	if (!core->GetDictionary()->Lookup( Variable, value )) {
		return Py_BuildValue( "l", ( unsigned long ) 0 );
	}

	return Py_BuildValue( "l", value );
}

PyDoc_STRVAR( GemRB_CheckVar__doc,
"CheckVar(VariableName, Context) => long\n\n"
"Display the value of a Game Variable." );

static PyObject* GemRB_CheckVar(PyObject * /*self*/, PyObject* args)
{
	char* Variable;
	char* Context;

	if (!PyArg_ParseTuple( args, "ss", &Variable, &Context )) {
		return AttributeError( GemRB_CheckVar__doc );
	}
	GameControl *gc = core->GetGameControl();
	if(!gc) {
		printMessage("GUIScript","No Game!\n", LIGHT_RED);
		return NULL;
	}
	Scriptable *Sender = (Scriptable *) gc->GetLastActor();
	if(!Sender) {
		Sender = (Scriptable *) core->GetGame()->GetCurrentMap();
	}
	if(!Sender) {
		printMessage("GUIScript","No Game!\n", LIGHT_RED);
		return NULL;
	}
	long value =(long) GameScript::CheckVariable(Sender, Variable, Context);
	printMessage("GUISCript","",YELLOW);
	printf("%s %s=%ld\n",Context, Variable, value);
	textcolor(WHITE);
	return Py_BuildValue( "l", value );
}

PyDoc_STRVAR( GemRB_GetGameVar__doc,
"GetGameVar(VariableName) => long\n\n"
"Get a Variable value from the Game Global Dictionary." );

static PyObject* GemRB_GetGameVar(PyObject * /*self*/, PyObject* args)
{
	char* Variable;
	ieDword value;

	if (!PyArg_ParseTuple( args, "s", &Variable )) {
		return AttributeError( GemRB_GetGameVar__doc );
	}

	if (!core->GetGame()->globals->Lookup( Variable, value )) {
		return Py_BuildValue( "l", ( unsigned long ) 0 );
	}

	return Py_BuildValue( "l", value );
}

PyDoc_STRVAR( GemRB_PlayMovie__doc,
"PlayMovie(MOVResRef) => int\n\n"
"Starts the Movie Player." );

static PyObject* GemRB_PlayMovie(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		return AttributeError( GemRB_PlayMovie__doc );
	}

	int ind = core->PlayMovie( string );
	//don't return NULL
	return Py_BuildValue( "i", ind );
}

PyDoc_STRVAR( GemRB_GetSaveGameCount__doc,
"GetSaveGameCount() => int\n\n"
"Returns the number of saved games." );

static PyObject* GemRB_GetSaveGameCount(PyObject * /*self*/,
	PyObject * /*args*/)
{
	return Py_BuildValue( "i",
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

PyDoc_STRVAR( GemRB_GetSaveGameAttrib__doc,
"GetSaveGameAttrib(Type, SaveSlotCount) => string\n\n"
"Returns the name, path or prefix of the saved game." );

static PyObject* GemRB_GetSaveGameAttrib(PyObject * /*self*/, PyObject* args)
{
	int Type, SlotCount;

	if (!PyArg_ParseTuple( args, "ii", &Type, &SlotCount )) {
		return AttributeError( GemRB_GetSaveGameAttrib__doc );
	}
	SaveGame* sg = core->GetSaveGameIterator()->GetSaveGame( SlotCount );
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}
	PyObject* tmp;
	switch (Type) {
		case 0:
			tmp = Py_BuildValue( "s", sg->GetName() ); break;
		case 1:
			tmp = Py_BuildValue( "s", sg->GetPrefix() ); break;
		case 2:
			tmp = Py_BuildValue( "s", sg->GetPath() ); break;
		case 3:
			tmp = Py_BuildValue( "s", sg->GetDate() ); break;
		default:
			printMessage( "GUIScript",
				"Syntax Error: GetSaveGameAttrib(Type, SlotCount)\n",
				LIGHT_RED );
			return NULL;
	}
	delete sg;
	return tmp;
}

PyDoc_STRVAR( GemRB_SetSaveGamePortrait__doc,
"SetSaveGameAreaPortrait(WindowIndex, ControlIndex, SaveSlotCount, PCSlotCount)\n\n"
"Sets a savegame PC portrait bmp onto a button as picture." );

static PyObject* GemRB_SetSaveGamePortrait(PyObject * /*self*/, PyObject* args)
{
	int wi, ci, SaveSlotCount, PCSlotCount;

	if (!PyArg_ParseTuple( args, "iiii", &wi, &ci, &SaveSlotCount, &PCSlotCount )) {
		return AttributeError( GemRB_SetSaveGamePortrait__doc );
	}
	Button* btn = ( Button* ) GetControl( wi, ci, IE_GUI_BUTTON);
	if (!btn) {
		return NULL;
	}

	SaveGame* sg = core->GetSaveGameIterator()->GetSaveGame( SaveSlotCount );
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}
	if (sg->GetPortraitCount() <= PCSlotCount) {
		btn->SetPicture( NULL );
		delete sg;
		Py_INCREF( Py_None );
		return Py_None;
	}

	DataStream* str = sg->GetPortrait( PCSlotCount );
	delete sg;
	ImageMgr* im = ( ImageMgr* ) core->GetInterface( IE_BMP_CLASS_ID );
	if (im == NULL) {
		delete ( str );
		return NULL;
	}
	if (!im->Open( str, true )) {
		core->FreeInterface( im );
		return NULL;
	}

	Sprite2D* Picture = im->GetImage();
	if (Picture == NULL) {
		core->FreeInterface( im );
		return NULL;
	}

	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetSaveGamePreview__doc,
"SetSaveGameAreaPreview(WindowIndex, ControlIndex, SaveSlotCount)\n\n"
"Sets a savegame area preview bmp onto a button as picture." );

static PyObject* GemRB_SetSaveGamePreview(PyObject * /*self*/, PyObject* args)
{
	int wi, ci, SlotCount;

	if (!PyArg_ParseTuple( args, "iii", &wi, &ci, &SlotCount )) {
		return AttributeError( GemRB_SetSaveGamePreview__doc );
	}
	Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
	if (!btn) {
		return NULL;
	}

	SaveGame* sg = core->GetSaveGameIterator()->GetSaveGame( SlotCount );
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}
	DataStream* str = sg->GetScreen();
	delete sg;
	ImageMgr* im = ( ImageMgr* ) core->GetInterface( IE_BMP_CLASS_ID );
	if (im == NULL) {
		delete ( str );
		return NULL;
	}
	if (!im->Open( str, true )) {
		core->FreeInterface( im );
		return NULL;
	}

	Sprite2D* Picture = im->GetImage();
	if (Picture == NULL) {
		core->FreeInterface( im );
		return NULL;
	}

	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
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
	return Py_BuildValue( "i", core->Roll( Dice, Size, Add ) );
}

PyDoc_STRVAR( GemRB_GetCharSounds__doc,
"GetCharSounds(WindowIndex, ControlIndex) => int\n\n"
"Reads in the contents of the sounds subfolder." );

static PyObject* GemRB_GetCharSounds(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;

	if (!PyArg_ParseTuple( args, "ii", &wi, &ci )) {
		return AttributeError( GemRB_GetCharSounds__doc );
	}
	TextArea* ta = ( TextArea* ) GetControl( wi, ci, IE_GUI_TEXTAREA );
	if (!ta) {
		return NULL;
	}
	return Py_BuildValue( "i", core->GetCharSounds( ta ) );
}

PyDoc_STRVAR( GemRB_GetPartySize__doc,
"GetPartySize() => int\n\n"
"Returns the number of PCs." );

static PyObject* GemRB_GetPartySize(PyObject * /*self*/, PyObject * /*args*/)
{
	Game *game = core->GetGame();
	if(!game) {
		return NULL;
	}
	return Py_BuildValue( "i", game->GetPartySize(0) );
}

PyDoc_STRVAR( GemRB_GameGetPartyGold__doc,
"GameGetPartyGold() => int\n\n"
"Returns current party gold." );

static PyObject* GemRB_GameGetPartyGold(PyObject * /*self*/, PyObject* /*args*/)
{
	int Gold = core->GetGame()->PartyGold;
	return Py_BuildValue( "i", Gold );
}

PyDoc_STRVAR( GemRB_GameGetFormation__doc,
"GameGetFormation() => int\n\n"
"Returns current party formation." );

static PyObject* GemRB_GameGetFormation(PyObject * /*self*/, PyObject* /*args*/)
{
	int Formation = core->GetGame()->WhichFormation;
	return Py_BuildValue( "i", Formation );
}

PyDoc_STRVAR( GemRB_GameSetFormation__doc,
"GameSetFormation(Formation)\n\n"
"Sets party formation." );

static PyObject* GemRB_GameSetFormation(PyObject * /*self*/, PyObject* args)
{
	int Formation;

	if (!PyArg_ParseTuple( args, "i", &Formation )) {
		return AttributeError( GemRB_GameSetFormation__doc );
	}
	core->GetGame()->WhichFormation = Formation;

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetJournalSize__doc,
"GetJournalSize(section) => int\n\n"
"Returns the number of entries in the given section of journal." );

static PyObject* GemRB_GetJournalSize(PyObject * /*self*/, PyObject * args)
{
	int section;
	if (!PyArg_ParseTuple( args, "i", &section )) {
		return AttributeError( GemRB_GetJournalSize__doc );
	}

	int count = 0;
	for (int i = 0; i < core->GetGame()->GetJournalCount(); i++) {
		GAMJournalEntry* je = core->GetGame()->GetJournalEntry( i );
		//printf ("JE: sec: %d;   text: %d, time: %d, chapter: %d, un09: %d, un0b: %d\n", je->Section, je->Text, je->GameTime, je->Chapter, je->unknown09, je->unknown0B);
		if (section == je->Section)
			count++;
	}

	return Py_BuildValue( "i", count );
}

PyDoc_STRVAR( GemRB_GetJournalEntry__doc,
"GetJournalEntry(section, index) => JournalEntry\n\n"
"Returns dictionary representing journal entry w/ given section and index." );

static PyObject* GemRB_GetJournalEntry(PyObject * /*self*/, PyObject * args)
{
	int section, index;
	if (!PyArg_ParseTuple( args, "ii", &section, &index )) {
		return AttributeError( GemRB_GetJournalEntry__doc );
	}

	int count = 0;
	for (int i = 0; i < core->GetGame()->GetJournalCount(); i++) {
		GAMJournalEntry* je = core->GetGame()->GetJournalEntry( i );
		if (section == je->Section) {
			if (index == count) {
				PyObject* dict = PyDict_New();
				PyDict_SetItemString(dict, "Text", PyInt_FromLong (je->Text));
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

	return Py_BuildValue( "i", core->GetGame()->IsBeastKnown( index ));
}

PyDoc_STRVAR( GemRB_GetINIPartyCount__doc,
"GetINIPartyCount() =>int\n\n"
"Returns the Number of Party defined in Party.ini (works only on IWD2)." );

static PyObject* GemRB_GetINIPartyCount(PyObject * /*self*/,
	PyObject * /*args*/)
{
	if (!core->GetPartyINI()) {
		return NULL;
	}
	return Py_BuildValue( "i", core->GetPartyINI()->GetTagsCount() );
}

PyDoc_STRVAR( GemRB_GetINIQuestsKey__doc,
"GetINIQuestsKey(Tag, Key, Default) => string\n\n"
"Returns a Value from the quests.ini File (works only on PST)." );

static PyObject* GemRB_GetINIQuestsKey(PyObject * /*self*/, PyObject* args)
{
	char* Tag, * Key, * Default;
	if (!PyArg_ParseTuple( args, "sss", &Tag, &Key, &Default )) {
		return AttributeError( GemRB_GetINIQuestsKey__doc );
	}
	if (!core->GetQuestsINI()) {
		return NULL;
	}
	return Py_BuildValue( "s",
			core->GetQuestsINI()->GetKeyAsString( Tag, Key, Default ) );
}

PyDoc_STRVAR( GemRB_GetINIBeastsKey__doc,
"GetINIBeastsKey(Tag, Key, Default) => string\n\n"
"Returns a Value from the beasts.ini File (works only on PST)." );

static PyObject* GemRB_GetINIBeastsKey(PyObject * /*self*/, PyObject* args)
{
	char* Tag, * Key, * Default;
	if (!PyArg_ParseTuple( args, "sss", &Tag, &Key, &Default )) {
		return AttributeError( GemRB_GetINIBeastsKey__doc );
	}
	if (!core->GetBeastsINI()) {
		return NULL;
	}
	return Py_BuildValue( "s",
			core->GetBeastsINI()->GetKeyAsString( Tag, Key, Default ) );
}

PyDoc_STRVAR( GemRB_GetINIPartyKey__doc,
"GetINIPartyKey(Tag, Key, Default) => string\n\n"
"Returns a Value from the Party.ini File (works only on IWD2)." );

static PyObject* GemRB_GetINIPartyKey(PyObject * /*self*/, PyObject* args)
{
	char* Tag, * Key, * Default;
	if (!PyArg_ParseTuple( args, "sss", &Tag, &Key, &Default )) {
		return AttributeError( GemRB_GetINIPartyKey__doc );
	}
	if (!core->GetPartyINI()) {
		return NULL;
	}
	return Py_BuildValue( "s",
			core->GetPartyINI()->GetKeyAsString( Tag, Key, Default ) );
}

PyDoc_STRVAR( GemRB_CreatePlayer__doc,
"CreatePlayer(CREResRef, Slot [,Import] ) => PlayerSlot\n\n"
"Creates a player slot. If import is nonzero, then reads a CHR instead of a CRE." );

static PyObject* GemRB_CreatePlayer(PyObject * /*self*/, PyObject* args)
{
	char* CreResRef;
	int PlayerSlot, Slot;
	int Import=0;

	if (!PyArg_ParseTuple( args, "si|i", &CreResRef, &PlayerSlot, &Import)) {
		return AttributeError( GemRB_CreatePlayer__doc );
	}
	//PlayerSlot is zero based, if not, remove the +1
	//removed it!
	Slot = ( PlayerSlot & 0x7fff ); 
	if (PlayerSlot & 0x8000) {
		PlayerSlot = core->GetGame()->FindPlayer( Slot );
		if (PlayerSlot < 0) {
			PlayerSlot = core->LoadCreature( CreResRef, Slot, Import );
		}
	} else {
		PlayerSlot = core->GetGame()->FindPlayer( PlayerSlot );
		if (PlayerSlot >= 0) {
			printMessage( "GUIScript", "Slot is already filled!\n", LIGHT_RED );
			return NULL;
		}
		PlayerSlot = core->LoadCreature( CreResRef, Slot, Import ); //inparty flag
	}
	if (PlayerSlot < 0) {
		printMessage( "GUIScript", "Not found!\n", LIGHT_RED );
		return NULL;
	}
	return Py_BuildValue( "i", PlayerSlot );
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
	if(!game) {
		return NULL;
	}
	Actor* MyActor = game->FindPC( PartyID );
	if (!MyActor) {
		return Py_BuildValue( "s", "???");
	}
	return Py_BuildValue( "s", MyActor->GetName(Which) );
}

PyDoc_STRVAR( GemRB_SetPlayerName__doc,
"SetPlayerName(Slot, Name[, LongOrShort])\n\n"
"Sets the player name." );

static PyObject* GemRB_SetPlayerName(PyObject * /*self*/, PyObject* args)
{
	char *Name=NULL;
	int PlayerSlot, Which;

	Which = 0;
	if (!PyArg_ParseTuple( args, "is|i", &PlayerSlot, &Name, &Which )) {
		return AttributeError( GemRB_SetPlayerName__doc );
	}
	Game *game = core->GetGame();
	if(!game) {
		return NULL;
	}
	PlayerSlot = game->FindPlayer( PlayerSlot );
	Actor* MyActor = core->GetGame()->GetPC( PlayerSlot );
	if (!MyActor) {
		return NULL;
	}
	MyActor->SetText(Name, Which);
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetPCStats__doc,
"GetPCStats(PartyID) => dict\n\n"
"Returns dictionary or PC's performance stats." );

static PyObject* GemRB_GetPCStats(PyObject * /*self*/, PyObject* args)
{
	int PartyID;

	if (!PyArg_ParseTuple( args, "i", &PartyID )) {
		return AttributeError( GemRB_GetPCStats__doc );
	}
	Game *game = core->GetGame();
	if(!game) {
		return NULL;
	}
	Actor* MyActor = game->FindPC( PartyID );
	if (!MyActor || !MyActor->PCStats) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	PyObject* dict = PyDict_New();
	PCStatsStruct* ps = MyActor->PCStats;

	PyDict_SetItemString(dict, "BestKilledName", PyInt_FromLong (ps->BestKilledName));
	PyDict_SetItemString(dict, "BestKilledXP", PyInt_FromLong (ps->BestKilledXP));
	PyDict_SetItemString(dict, "JoinDate", PyInt_FromLong (ps->JoinDate));
	PyDict_SetItemString(dict, "KillsChapterXP", PyInt_FromLong (ps->KillsChapterXP));
	PyDict_SetItemString(dict, "KillsChapterCount", PyInt_FromLong (ps->KillsChapterCount));
	PyDict_SetItemString(dict, "KillsTotalXP", PyInt_FromLong (ps->KillsTotalXP));
	PyDict_SetItemString(dict, "KillsTotalCount", PyInt_FromLong (ps->KillsTotalCount));



	// FIXME!!!
	if (ps->FavouriteSpells[0][0]) {
		DataStream* str = core->GetResourceMgr()->GetResource( ps->FavouriteSpells[0], IE_SPL_CLASS_ID );
		SpellMgr* sm = ( SpellMgr* ) core->GetInterface( IE_SPL_CLASS_ID );
		if (sm == NULL) {
			delete ( str );
			return NULL;
		}
		if (!sm->Open( str, true )) {
			core->FreeInterface( sm );
			return NULL;
		}

		Spell* spell = sm->GetSpell();
		if (spell == NULL) {
			core->FreeInterface( sm );
			return NULL;
		}

		core->FreeInterface( sm );

		PyDict_SetItemString(dict, "FavouriteSpell", PyInt_FromLong (spell->SpellName));

		delete spell;
	} else {
		PyDict_SetItemString(dict, "FavouriteSpell", PyString_FromString (""));
	}



	// FIXME!!!
	if (ps->FavouriteWeapons[0][0]) {
		DataStream* str = core->GetResourceMgr()->GetResource( ps->FavouriteWeapons[0], IE_ITM_CLASS_ID );
		ItemMgr* sm = ( ItemMgr* ) core->GetInterface( IE_ITM_CLASS_ID );
		if (sm == NULL) {
			delete ( str );
			return NULL;
		}
		if (!sm->Open( str, true )) {
			core->FreeInterface( sm );
			return NULL;
		}

		Item* item = sm->GetItem();
		if (item == NULL) {
			core->FreeInterface( sm );
			return NULL;
		}

		core->FreeInterface( sm );

		PyDict_SetItemString(dict, "FavouriteWeapon", PyInt_FromLong (item->ItemName));

		delete item;
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

	printf("GemRB.GameSelectPC()\n");
	if (!PyArg_ParseTuple( args, "ii|i", &PartyID, &Select, &Flags )) {
		return AttributeError( GemRB_GameSelectPC__doc );
	}
	Game *game = core->GetGame();
	if(!game) {
		return NULL;
	}

	Actor* actor;
	if (PartyID > 0) {
		actor = game->FindPC( PartyID );
		if (! actor) {
			Py_INCREF( Py_None );
			return Py_None;
		}
	} else {
		actor = NULL;
	}

	game->SelectActor( actor, Select, Flags );

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
	if(!game) {
		return NULL;
	}
	PlayerSlot = game->FindPlayer( PlayerSlot );
	Actor* MyActor = core->GetGame()->GetPC( PlayerSlot );
	if (!MyActor) {
		return Py_BuildValue( "i", 0 );
	}
	return Py_BuildValue("i", MyActor->IsSelected() );
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
"GameGetSelectedPCSingle() => int\n\n"
"Returns index of the selected PC in non-walk environment (i.e. in shops, inventory,...). Index should be greater than zero." );

static PyObject* GemRB_GameGetSelectedPCSingle(PyObject * /*self*/, PyObject* /*args*/)
{
	return Py_BuildValue( "i", core->GetGame()->GetSelectedPCSingle() );
}

PyDoc_STRVAR( GemRB_GameGetFirstSelectedPC__doc,
"GameGetFirstSelectedPC() => int\n\n"
"Returns index of the first selected PC or 0 if none." );

static PyObject* GemRB_GameGetFirstSelectedPC(PyObject * /*self*/, PyObject* /*args*/)
{
	Game* game = core->GetGame();
	for (int i = 0; i < game->GetPartySize (false); i++) {
		Actor* actor = game->GetPC (i);
		if (actor->IsSelected()) {
			return Py_BuildValue( "i", actor->InParty); // FIXME: or i+1 ?
		}
	}

	return Py_BuildValue( "i", 0 );
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
	if(!game) {
		return NULL;
	}
	PlayerSlot = game->FindPlayer( PlayerSlot );
	Actor* MyActor = core->GetGame()->GetPC( PlayerSlot );
	if (!MyActor) {
		return Py_BuildValue( "s", "");
	}
	return Py_BuildValue( "s", MyActor->GetPortrait(Which) );
}

PyDoc_STRVAR( GemRB_GetPlayerStat__doc,
"GetPlayerStat(Slot, ID) => int\n\n"
"Queries a stat." );

static PyObject* GemRB_GetPlayerStat(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, StatID, StatValue, BaseStat;

	BaseStat = 0;
	if (!PyArg_ParseTuple( args, "ii|i", &PlayerSlot, &StatID, &BaseStat )) {
		return AttributeError( GemRB_GetPlayerStat__doc );
	}
	//returning the modified stat if BaseStat was 0 (default)
	StatValue = core->GetCreatureStat( PlayerSlot, StatID, !BaseStat );
	return Py_BuildValue( "i", StatValue );
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
	//Setting the creature's base stat, which gets saved (0)
	if (!core->SetCreatureStat( PlayerSlot, StatID, StatValue, 0 )) {
		return NULL;
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_FillPlayerInfo__doc,
"FillPlayerInfo(Slot[, Portrait1, Portrait2])\n\n"
"Fills basic character info, that is not stored in stats." );

static PyObject* GemRB_FillPlayerInfo(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot;
	char *Portrait1=NULL, *Portrait2=NULL;

	if (!PyArg_ParseTuple( args, "i|ss", &PlayerSlot, &Portrait1, &Portrait2)) {
		return AttributeError( GemRB_FillPlayerInfo__doc );
	}
	// here comes some code to transfer icon/name to the PC sheet
	//
	//
	Game *game = core->GetGame();
	if(!game) {
		return NULL;
	}
	PlayerSlot = game->FindPlayer( PlayerSlot );
	Actor* MyActor = core->GetGame()->GetPC( PlayerSlot );
	if (!MyActor) {
		return NULL;
	}
	if(Portrait1) {
		MyActor->SetPortrait( Portrait1, 1);
	}
	if(Portrait2) {
		MyActor->SetPortrait( Portrait2, 2);
	}
	int mastertable = core->LoadTable( "avprefix" );
	TableMgr* mtm = core->GetTable( mastertable );
	int count = mtm->GetRowCount();
	if (count< 1 || count>8) {
		printMessage( "GUIScript", "Table is invalid.\n", LIGHT_RED );
		return NULL;
	}
	char *poi = mtm->QueryField( 0 );
	int AnimID = strtoul( poi, NULL, 0 );
	printf( "Avatar animation base: 0x%0x", AnimID );
	for (int i = 1; i < count; i++) {
		poi = mtm->QueryField( i );
		printf( "Part table: %s\n", poi );
		int table = core->LoadTable( poi );
		printf( "Part table id:%d\n", table );
		TableMgr* tm = core->GetTable( table );
		printf( "Loaded part table\n" );
		int StatID = atoi( tm->QueryField() );
		printf( "Stat ID:%d\n", StatID );
		StatID = MyActor->GetBase( StatID );
		printf( "Value:%d\n", StatID );
		poi = tm->QueryField( StatID );
		printf( "Part: %s\n", poi );
		AnimID += strtoul( poi, NULL, 0 );
		core->DelTable( table );
	}
	printf( "Set animation complete: 0x%0x\n", AnimID );
	MyActor->SetBase(IE_ANIMATION_ID, AnimID);
	//setting PST's starting stance to 18
	poi = mtm->QueryField( 0, 1 );
	if (*poi != '*') {
		MyActor->SetStance( atoi( poi ) );
	}
	core->DelTable( mastertable );
	// 0 - single player, 1 - tutorial, 2 - multiplayer
	ieDword playmode = 0;
	core->GetDictionary()->Lookup( "PlayMode", playmode );
	playmode *= 2;
	int saindex = core->LoadTable( "STARTPOS" );
	TableMgr* strta = core->GetTable( saindex );
	MyActor->Pos.x = MyActor->Destination.x = atoi( strta->QueryField( playmode, PlayerSlot ) );
	MyActor->Pos.y = MyActor->Destination.y = atoi( strta->QueryField( playmode + 1, PlayerSlot ) );
	MyActor->SetOver( false );
	//core->GetGame()->SetPC(MyActor);
	core->DelTable( saindex );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetSpellIcon__doc,
"SetSpellIcon(WindowIndex, ControlIndex, SPLResRef)\n\n"
"FIXME: temporary Sets Spell icon image on a button." );

static PyObject* GemRB_SetSpellIcon(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;
	char* SpellResRef;

	if (!PyArg_ParseTuple( args, "iis", &wi, &ci, &SpellResRef )) {
		return AttributeError( GemRB_SetSpellIcon__doc );
	}
	Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
	if (!btn) {
		return NULL;
	}

	// FIXME!!!
	DataStream* str = core->GetResourceMgr()->GetResource( SpellResRef, IE_SPL_CLASS_ID );
	SpellMgr* im = ( SpellMgr* ) core->GetInterface( IE_SPL_CLASS_ID );
	if (im == NULL) {
		printMessage( "GUIScript", "Runtime Error: core->GetInterface()\n",
			LIGHT_RED );
		delete ( str );
		return NULL;
	}
	if (!im->Open( str, true )) {
		printMessage( "GUIScript", "Runtime Error: im->Open()\n", LIGHT_RED );
		core->FreeInterface( im );
		return NULL;
	}

	// FIXME - should use some already allocated in core
	Spell* spell = im->GetSpell();
	if (spell == NULL) {
		printMessage( "GUIScript", "Runtime Error: im->GetSpell()\n",
			LIGHT_RED );
		core->FreeInterface( im );
		return NULL;
	}

	btn->SetImage( IE_GUI_BUTTON_UNPRESSED,
			spell->SpellIconBAM->GetFrame( 0 ) );
	btn->SetImage( IE_GUI_BUTTON_PRESSED, spell->SpellIconBAM->GetFrame( 1 ) );
	delete spell;
	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_SetItemIcon__doc,
"SetItemIcon(WindowIndex, ControlIndex, ITMResRef)\n\n"
"FIXME: temporary Sets Item icon image on a button." );

static PyObject* GemRB_SetItemIcon(PyObject * /*self*/, PyObject* args)
{
	int wi, ci, Which = 0;
	char* ItemResRef;

	if (!PyArg_ParseTuple( args, "iis|i", &wi, &ci, &ItemResRef, &Which )) {
		return AttributeError( GemRB_SetItemIcon__doc );
	}

	Button* btn = (Button *) GetControl( wi, ci, IE_GUI_BUTTON );
	if (!btn) {
		return NULL;
	}

	if (ItemResRef[0]) {
		// FIXME!!!
		DataStream* str = core->GetResourceMgr()->GetResource( ItemResRef, IE_ITM_CLASS_ID );
		ItemMgr* im = ( ItemMgr* ) core->GetInterface( IE_ITM_CLASS_ID );
		if (im == NULL) {
			delete ( str );
			return NULL;
		}
		if (!im->Open( str, true )) {
			core->FreeInterface( im );
			return NULL;
		}

		// FIXME - should use some already allocated in core
		Item* item = im->GetItem();
		if (item == NULL) {
			core->FreeInterface( im );
			return NULL;
		}

		btn->SetFlags( IE_GUI_BUTTON_PICTURE, OP_OR );
		btn->SetPicture( item->ItemIconBAM->GetFrame( Which ) );
		core->FreeInterface( im );
		delete item;
	} else {
		btn->SetPicture( NULL );
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_EnterStore__doc,
"EnterStore(STOResRef)\n\n"
"FIXME: temporary Sets current store." );

// FIXME: ugly, should be in core
Store* store;

static PyObject* GemRB_EnterStore(PyObject * /*self*/, PyObject* args)
{
	char* StoreResRef;

	if (!PyArg_ParseTuple( args, "s", &StoreResRef )) {
		return AttributeError( GemRB_EnterStore__doc );
	}

	// FIXME!!!
	DataStream* str = core->GetResourceMgr()->GetResource( StoreResRef, IE_STO_CLASS_ID );
	StoreMgr* sm = ( StoreMgr* ) core->GetInterface( IE_STO_CLASS_ID );
	if (sm == NULL) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open( str, true )) {
		core->FreeInterface( sm );
		return NULL;
	}

	// FIXME - should use some already allocated in core
	store = sm->GetStore();
	if (store == NULL) {
		core->FreeInterface( sm );
		return NULL;
	}

	//btn->SetImage(IE_GUI_BUTTON_UNPRESSED, item->ItemIconBAM->GetFrame (0));
	//btn->SetImage(IE_GUI_BUTTON_PRESSED, item->ItemIconBAM->GetFrame (0));
	//delete store;
	core->FreeInterface( sm );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetStoreName__doc,
"GetStoreName() => string\n\n"
"FIXME: temporary Returns name of the current store" );

static PyObject* GemRB_GetStoreName(PyObject * /*self*/, PyObject* args)
{
	if (!PyArg_ParseTuple( args, "" )) {
		return AttributeError( GemRB_GetStoreName__doc );
	}

	return PyString_FromString( core->GetString( store->StoreName ) );
}

PyDoc_STRVAR( GemRB_GetStoreRoomPrices__doc,
"GetStoreRoomPrices() => tuple\n\n"
"FIXME: temporary Returns tuple of room prices for the current store." );

static PyObject* GemRB_GetStoreRoomPrices(PyObject * /*self*/, PyObject* args)
{
	if (!PyArg_ParseTuple( args, "" )) {
		return AttributeError( GemRB_GetStoreRoomPrices__doc );
	}

	PyObject* p = PyTuple_New( 4 );

	for (int i = 0; i < 4; i++) {
		PyTuple_SetItem( p, i, PyInt_FromLong( store->RoomPrices[i] ) );
	}

	return p;
}

PyDoc_STRVAR( GemRB_ExecuteString__doc,
"ExecuteString(String)\n\n"
"Executes an In-Game Script Action in the current Area Script Context" );

static PyObject* GemRB_ExecuteString(PyObject * /*self*/, PyObject* args)
{
	char* String;

	if (!PyArg_ParseTuple( args, "s", &String )) {
		return AttributeError( GemRB_ExecuteString__doc );
	}
	GameScript::ExecuteString( core->GetGame()->GetCurrentMap( ), String );
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_EvaluateString__doc,
"EvaluateString(String)\n\n"
"Evaluate an In-Game Script Trigger in the current Area Script Context" );

static PyObject* GemRB_EvaluateString(PyObject * /*self*/, PyObject* args)
{
	char* String;

	if (!PyArg_ParseTuple( args, "s", &String )) {
		return AttributeError( GemRB_EvaluateString__doc );
	}
	if (GameScript::EvaluateString( core->GetGame()->GetCurrentMap( ), String )) {
		printf( "%s returned True\n", String );
	} else {
		printf( "%s returned False\n", String );
	}
	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_UpdateMusicVolume__doc,
"UpdateMusicVolume()\n\n"
"Update music volume on-the-fly" );

static PyObject* GemRB_UpdateMusicVolume(PyObject * /*self*/, PyObject* /*args*/)
{
	core->GetSoundMgr()->UpdateVolume( GEM_SND_VOL_MUSIC );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_UpdateAmbientsVolume__doc,
"UpdateAmbientsVolume()\n\n"
"Update ambients volume on-the-fly" );

static PyObject* GemRB_UpdateAmbientsVolume(PyObject * /*self*/, PyObject* /*args*/)
{
	core->GetSoundMgr()->UpdateVolume( GEM_SND_VOL_AMBIENTS );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetCurrentArea__doc,
"GetCurrentArea()=>resref\n\n"
"Returns current area's ResRef" );

static PyObject* GemRB_GetCurrentArea(PyObject * /*self*/, PyObject* /*args*/)
{
	return PyString_FromString( core->GetGame()->CurrentArea );
}

PyDoc_STRVAR( GemRB_MoveToArea__doc,
"MoveToArea(resref)\n\n"
"Moves the selected characters to the area" );

static PyObject* GemRB_MoveToArea(PyObject * /*self*/, PyObject* args)
{
	char *String;

	if (!PyArg_ParseTuple( args, "s", &String )) {
		return AttributeError( GemRB_MoveToArea__doc );
	}
	Game *game = core->GetGame();
	Map* map2 = game->GetMap(String);
	if( !map2) {
		return NULL;
	}
	int i = game->GetPartySize(true);
	while (i--) {
		Actor* actor = game->GetPC(i);
		if( !actor->Selected ) {
			continue;
		}
		Map* map1 = game->GetMap(actor->Area);
		if( map1) {
			map1->RemoveActor( actor );
		}
		map2->AddActor( actor );
	}

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetMemorizableSpellsCount__doc,
"GetMemorizableSpellsCount(PartyID, SpellType, Level [,Bonus])=>int\n\n"
"Returns number of memorizable spells of given type and level in PartyID's spellbook" );

static PyObject* GemRB_GetMemorizableSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level, Bonus=1;

	if (!PyArg_ParseTuple( args, "iii|i", &PartyID, &SpellType, &Level, &Bonus )) {
		return AttributeError( GemRB_GetMemorizableSpellsCount__doc );
	}
	Game *game = core->GetGame();
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	//this isn't in the actor's spellbook, handles Wisdom
	return Py_BuildValue( "i", actor->spellbook.GetMemorizableSpellsCount( (ieSpellType) SpellType, Level, Bonus ) );
}

PyDoc_STRVAR( GemRB_SetMemorizableSpellsCount__doc,
"SetMemorizableSpellsCount(PartyID, Value, SpellType, Level, [Bonus])=>int\n\n"
"Returns number of memorizable spells of given type and level in PartyID's spellbook" );

static PyObject* GemRB_SetMemorizableSpellsCount(PyObject* /*self*/, PyObject* args)
{
	int PartyID, Value, SpellType, Level, Bonus=1;

	if (!PyArg_ParseTuple( args, "iiii|i", &PartyID, &Value, &SpellType, &Level, &Bonus )) {
		return AttributeError( GemRB_SetMemorizableSpellsCount__doc );
	}
	Game *game = core->GetGame();
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	//this isn't in the actor's spellbook, handles Wisdom
	actor->spellbook.SetMemorizableSpellsCount( Value, (ieSpellType) SpellType, Level, Bonus );

	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_GetKnownSpellsCount__doc,
"GetKnownSpellsCount(PartyID, SpellType, Level)=>int\n\n"
"Returns number of known spells of given type and level in PartyID's spellbook." );

static PyObject* GemRB_GetKnownSpellsCount(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level;

	if (!PyArg_ParseTuple( args, "iii", &PartyID, &SpellType, &Level )) {
		return AttributeError( GemRB_GetKnownSpellsCount__doc );
	}
	Game *game = core->GetGame();
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	return Py_BuildValue( "i", actor->spellbook.GetKnownSpellsCount( SpellType, Level ) );
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
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	CREKnownSpell* ks = actor->spellbook.GetKnownSpell( SpellType, Level, Index );
	if (! ks) {
		return NULL;
	}



	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "SpellResRef", PyString_FromResRef (ks->SpellResRef));
	//PyDict_SetItemString(dict, "Flags", PyInt_FromLong (ms->Flags));

	return dict;
}


PyDoc_STRVAR( GemRB_GetMemorizedSpellsCount__doc,
"GetMemorizedSpellsCount(PartyID, SpellType, Level)=>int\n\n"
"Returns number of spells of given type and level in PartyID's memory." );

static PyObject* GemRB_GetMemorizedSpellsCount(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level;

	if (!PyArg_ParseTuple( args, "iii", &PartyID, &SpellType, &Level )) {
		return AttributeError( GemRB_GetMemorizedSpellsCount__doc );
	}
	Game *game = core->GetGame();
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	return Py_BuildValue( "i", actor->spellbook.GetMemorizedSpellsCount( SpellType, Level ) );
}

PyDoc_STRVAR( GemRB_GetMemorizedSpell__doc,
"GetMemorizedSpell(PartyID, SpellType, Level, Index)=>dict\n\n"
"Returns dict with specified memorized spell from PC's spellbook" );

static PyObject* GemRB_GetMemorizedSpell(PyObject * /*self*/, PyObject* args)
{
	int PartyID, SpellType, Level, Index;

	if (!PyArg_ParseTuple( args, "iiii", &PartyID, &SpellType, &Level, &Index )) {
		return AttributeError( GemRB_GetMemorizedSpell__doc );
	}
	Game *game = core->GetGame();
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	CREMemorizedSpell* ms = actor->spellbook.GetMemorizedSpell( SpellType, Level, Index );
	if (! ms) {
		return NULL;
	}

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "SpellResRef", PyString_FromResRef (ms->SpellResRef));
	PyDict_SetItemString(dict, "Flags", PyInt_FromLong (ms->Flags));

	return dict;
}


PyDoc_STRVAR( GemRB_GetSpell__doc,
"GetSpell(ResRef)=>dict\n\n"
"Returns dict with specified spell" );

static PyObject* GemRB_GetSpell(PyObject * /*self*/, PyObject* args)
{
	char* ResRef;

	if (!PyArg_ParseTuple( args, "s", &ResRef)) {
		return AttributeError( GemRB_GetSpell__doc );
	}

	DataStream* str = core->GetResourceMgr()->GetResource( ResRef, IE_SPL_CLASS_ID );
	SpellMgr* sm = ( SpellMgr* ) core->GetInterface( IE_SPL_CLASS_ID );
	if (sm == NULL) {
		delete ( str );
		return NULL;
	}
	if (!sm->Open( str, true )) {
		core->FreeInterface( sm );
		return NULL;
	}

	Spell* spell = sm->GetSpell();
	if (spell == NULL) {
		core->FreeInterface( sm );
		return NULL;
	}

	core->FreeInterface( sm );

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "SpellName", PyInt_FromLong (spell->SpellName));
	PyDict_SetItemString(dict, "SpellDesc", PyInt_FromLong (spell->SpellDesc));
	PyDict_SetItemString(dict, "SpellbookIcon", PyString_FromResRef (spell->SpellbookIcon));

	delete spell;
	return dict;
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
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	CREKnownSpell* ks = actor->spellbook.GetKnownSpell( SpellType, Level, Index );
	if (! ks) {
		return NULL;
	}

	return Py_BuildValue( "i", actor->spellbook.MemorizeSpell( ks, false ) );
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
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	CREMemorizedSpell* ms = actor->spellbook.GetMemorizedSpell( SpellType, Level, Index );
	if (! ms) {
		return NULL;
	}

	return Py_BuildValue( "i", actor->spellbook.UnmemorizeSpell( ms ) );
}

PyDoc_STRVAR( GemRB_GetSlotItem__doc,
"GetSlotItem(PartyID, slot)=>dict\n\n"
"Returns dict with specified slot item from PC's inventory" );

static PyObject* GemRB_GetSlotItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Slot;

	if (!PyArg_ParseTuple( args, "ii", &PartyID, &Slot)) {
		return AttributeError( GemRB_GetSlotItem__doc );
	}
	Game *game = core->GetGame();
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	CREItem* si = actor->inventory.GetSlotItem( Slot );
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

	return dict;
}

PyDoc_STRVAR( GemRB_GetItem__doc,
"GetItem(ResRef)=>dict\n\n"
"Returns dict with specified item" );

static PyObject* GemRB_GetItem(PyObject * /*self*/, PyObject* args)
{
	char* ResRef;

	if (!PyArg_ParseTuple( args, "s", &ResRef)) {
		return AttributeError( GemRB_GetItem__doc );
	}

	DataStream* str = core->GetResourceMgr()->GetResource( ResRef, IE_ITM_CLASS_ID );
	ItemMgr* im = ( ItemMgr* ) core->GetInterface( IE_ITM_CLASS_ID );
	if (im == NULL) {
		delete ( str );
		printf("[GUIScript] Can't get item manager!\n");
		return NULL;
	}
	if (!im->Open( str, true )) {
		core->FreeInterface( im );
		printf("[GUIScript] Can't read item header!\n");
		return NULL;
	}

	Item* item = im->GetItem();
	if (item == NULL) {
		core->FreeInterface( im );
		printf("[GUIScript] Can't create item!\n");
		return NULL;
	}

	core->FreeInterface( im );

	PyObject* dict = PyDict_New();
	PyDict_SetItemString(dict, "ItemName", PyInt_FromLong (item->ItemName));
	PyDict_SetItemString(dict, "ItemNameIdentified", PyInt_FromLong (item->ItemNameIdentified));
	PyDict_SetItemString(dict, "ItemDesc", PyInt_FromLong (item->ItemDesc));
	PyDict_SetItemString(dict, "ItemDescIdentified", PyInt_FromLong (item->ItemDescIdentified));
	PyDict_SetItemString(dict, "ItemIcon", PyString_FromResRef (item->ItemIcon));
	PyDict_SetItemString(dict, "StackAmount", PyInt_FromLong (item->StackAmount));
	PyDict_SetItemString(dict, "Dialog", PyString_FromResRef (item->Dialog));

	delete item;
	return dict;
}

PyDoc_STRVAR( GemRB_DragItem__doc,
"DragItem(PartyID, Slot, ResRef, [Count=0])\\n\n"
"Start dragging specified item" );

static PyObject* GemRB_DragItem(PyObject * /*self*/, PyObject* args)
{
	int PartyID, Slot, Count = 0, CycleIndex = 0, FrameIndex = 0;
	char *ResRef;

	if (!PyArg_ParseTuple( args, "iis|iii", &PartyID, &Slot, &ResRef, &Count, &CycleIndex, &FrameIndex)) {
		return AttributeError( GemRB_DragItem__doc );
	}

	// FIXME
	if (core->GetDraggedItem()) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	Game *game = core->GetGame();
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	CREItem* si = actor->inventory.RemoveItem( Slot, Count );
	if (! si) {
		Py_INCREF( Py_None );
		return Py_None;
	}

	core->DragItem (si);





	DataStream* str = core->GetResourceMgr()->GetResource( ResRef,
												IE_BAM_CLASS_ID );
	if (str == NULL) {
		return NULL;
	}
	AnimationMgr* am = ( AnimationMgr* )
		core->GetInterface( IE_BAM_CLASS_ID );
	if (am == NULL) {
		delete ( str );
		return NULL;
	}

	if (!am->Open( str, true )) {
		core->FreeInterface( am );
		return NULL;
	}

	Sprite2D* Picture = am->GetFrameFromCycle( CycleIndex, FrameIndex );
	if (Picture == NULL) {
		core->FreeInterface( am );
		return NULL;
	}

// 	Color* pal = core->GetPalette( col1, 12 );
// 	Color* orgpal = core->GetVideoDriver()->GetPalette( Picture );
// 	memcpy( &orgpal[4], pal, 12 * sizeof( Color ) );
// 	core->GetVideoDriver()->SetPalette( Picture, orgpal );
// 	free( pal );
// 	free( orgpal );



	core->GetVideoDriver()->SetDragCursor (Picture);






	Py_INCREF( Py_None );
	return Py_None;
}

PyDoc_STRVAR( GemRB_DropDraggedItem__doc,
"DropDraggedItem(PartyID, Slot)=>int\\n\n"
"Stop dragging specified item. Returns 0 (unsuccessful), 1 (partial success) or 2 (complete success)." );

static PyObject* GemRB_DropDraggedItem(PyObject * /*self*/, PyObject* args)
{
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
	Actor* actor = game->FindPC( PartyID );
	if (! actor) {
		return NULL;
	}

	int res = actor->inventory.AddSlotItem( core->GetDraggedItem(), Slot );

	if (res == 2) {
		// Whole amount was placed
		core->DragItem (NULL);
		core->GetVideoDriver()->SetDragCursor (NULL);
	} 

	return Py_BuildValue( "i", res );
}

PyDoc_STRVAR( GemRB_IsDraggingItem__doc,
"IsDraggingItem()=>int\\n\n"
"Returns 1 if we are dragging some item." );

static PyObject* GemRB_IsDraggingItem(PyObject * /*self*/, PyObject* /*args*/)
{
	return Py_BuildValue( "i", core->GetDraggedItem() != NULL );
}

PyDoc_STRVAR( GemRB_GetSystemVariable__doc,
"GetSystemVariable(Variable)=>int\\n\n"
"Returns the named Interface attribute." );

static PyObject* GemRB_GetSystemVariable(PyObject * /*self*/, PyObject* args)
{
	int Variable, value;

	if (!PyArg_ParseTuple( args, "i", &Variable)) {
		return AttributeError( GemRB_DropDraggedItem__doc );
	}
	switch(Variable) {
		case SV_BPP: value = core->Bpp; break;
		case SV_WIDTH: value = core->Width; break;
		case SV_HEIGHT: value = core->Height; break;
		default: value = -1; break;
	}
	return Py_BuildValue( "i", value);
}

static PyMethodDef GemRBMethods[] = {
	METHOD(SetInfoTextColor, METH_VARARGS),
	METHOD(HideGUI, METH_NOARGS),
	METHOD(UnhideGUI, METH_NOARGS),
	METHOD(SetTAAutoScroll, METH_VARARGS),
	METHOD(MoveTAText, METH_VARARGS),
	METHOD(ExecuteString, METH_VARARGS),
	METHOD(EvaluateString, METH_VARARGS),
	METHOD(GetGameString, METH_VARARGS),
	METHOD(LoadGame, METH_VARARGS),
	METHOD(EnterGame, METH_NOARGS),
	METHOD(QuitGame, METH_NOARGS),
	METHOD(StatComment, METH_VARARGS),
	METHOD(GetString, METH_VARARGS),
	METHOD(EndCutSceneMode, METH_NOARGS),
	METHOD(GetPartySize, METH_NOARGS),
	METHOD(GameGetPartyGold, METH_NOARGS),
	METHOD(GameGetFormation, METH_NOARGS),
	METHOD(GameSetFormation, METH_VARARGS),
	METHOD(GetJournalSize, METH_VARARGS),
	METHOD(GetJournalEntry, METH_VARARGS),
	METHOD(GameIsBeastKnown, METH_VARARGS),
	METHOD(GetINIPartyCount, METH_NOARGS),
	METHOD(GetINIQuestsKey, METH_VARARGS),
	METHOD(GetINIBeastsKey, METH_VARARGS),
	METHOD(GetINIPartyKey, METH_VARARGS),
	METHOD(LoadWindowPack, METH_VARARGS),
	METHOD(LoadWindow, METH_VARARGS),
	METHOD(CreateWindow, METH_VARARGS),
	METHOD(SetWindowSize, METH_VARARGS),
	METHOD(SetWindowPos, METH_VARARGS),
	METHOD(SetWindowPicture, METH_VARARGS),
	METHOD(LoadTable, METH_VARARGS),
	METHOD(UnloadTable, METH_VARARGS),
	//METHOD(GetTable, METH_VARARGS),
	METHOD(GetTableValue, METH_VARARGS),
	METHOD(FindTableValue, METH_VARARGS),
	METHOD(GetTableRowIndex, METH_VARARGS),
	METHOD(GetTableRowName, METH_VARARGS),
	METHOD(GetTableRowCount, METH_VARARGS),
	METHOD(LoadSymbol, METH_VARARGS),
	METHOD(UnloadSymbol, METH_VARARGS),
	METHOD(GetSymbol, METH_VARARGS),
	METHOD(GetSymbolValue, METH_VARARGS),
	METHOD(GetControl, METH_VARARGS),
	METHOD(SetText, METH_VARARGS),
	METHOD(QueryText, METH_VARARGS),
	METHOD(TextAreaAppend, METH_VARARGS),
	METHOD(TextAreaClear, METH_VARARGS),
	METHOD(SetTooltip, METH_VARARGS),
	METHOD(SetVisible, METH_VARARGS),
	METHOD(SetMasterScript, METH_VARARGS),
	METHOD(ShowModal, METH_VARARGS),
	METHOD(SetEvent, METH_VARARGS),
	METHOD(SetNextScript, METH_VARARGS),
	METHOD(SetControlStatus, METH_VARARGS),
	METHOD(SetVarAssoc, METH_VARARGS),
	METHOD(UnloadWindow, METH_VARARGS),
	METHOD(CreateLabel, METH_VARARGS),
	METHOD(SetLabelTextColor, METH_VARARGS),
	METHOD(CreateButton, METH_VARARGS),
	METHOD(SetButtonSprites, METH_VARARGS),
	METHOD(SetButtonBorder, METH_VARARGS),
	METHOD(EnableButtonBorder, METH_VARARGS),
	METHOD(SetButtonFont, METH_VARARGS),
	METHOD(SetButtonTextColor, METH_VARARGS),
	METHOD(AdjustScrolling, METH_VARARGS),
	METHOD(CreateWorldMapControl, METH_VARARGS),
	METHOD(CreateMapControl, METH_VARARGS),
	METHOD(SetControlPos, METH_VARARGS),
	METHOD(SetControlSize, METH_VARARGS),
	METHOD(DeleteControl, METH_VARARGS),
	METHOD(SetTextAreaSelectable, METH_VARARGS),
	METHOD(SetButtonFlags, METH_VARARGS),
	METHOD(SetButtonState, METH_VARARGS),
	METHOD(SetButtonPicture, METH_VARARGS),
	METHOD(SetButtonMOS, METH_VARARGS),
	METHOD(SetButtonPLT, METH_VARARGS),
	METHOD(SetButtonBAM, METH_VARARGS),
	METHOD(SetLabelUseRGB, METH_VARARGS),
	METHOD(PlaySound, METH_VARARGS),
	METHOD(LoadMusicPL, METH_VARARGS),
	METHOD(SoftEndPL, METH_NOARGS),
	METHOD(HardEndPL, METH_NOARGS),
	METHOD(DrawWindows, METH_NOARGS),
	METHOD(Quit, METH_NOARGS),
	METHOD(GetVar, METH_VARARGS),
	METHOD(SetVar, METH_VARARGS),
	METHOD(GetToken, METH_VARARGS),
	METHOD(SetToken, METH_VARARGS),
	METHOD(GetGameVar, METH_VARARGS),
	METHOD(CheckVar, METH_VARARGS),
	METHOD(PlayMovie, METH_VARARGS),
	METHOD(Roll, METH_VARARGS),
	METHOD(GetCharSounds, METH_VARARGS),
	METHOD(GetSaveGameCount, METH_VARARGS),
	METHOD(DeleteSaveGame, METH_VARARGS),
	METHOD(GetSaveGameAttrib, METH_VARARGS),
	METHOD(SetSaveGamePreview, METH_VARARGS),
	METHOD(SetSaveGamePortrait, METH_VARARGS),
	METHOD(CreatePlayer, METH_VARARGS),
	METHOD(SetPlayerStat, METH_VARARGS),
	METHOD(GetPlayerStat, METH_VARARGS),
	METHOD(GameSelectPC, METH_VARARGS),
	METHOD(GameIsPCSelected, METH_VARARGS),
	METHOD(GameSelectPCSingle, METH_VARARGS),
	METHOD(GameGetSelectedPCSingle, METH_NOARGS),
	METHOD(GameGetFirstSelectedPC, METH_NOARGS),
	METHOD(GetPlayerPortrait, METH_VARARGS),
	METHOD(GetPlayerName, METH_VARARGS),
	METHOD(SetPlayerName, METH_VARARGS),
	METHOD(GetPCStats, METH_VARARGS),
	METHOD(FillPlayerInfo, METH_VARARGS),
	METHOD(SetSpellIcon, METH_VARARGS),
	METHOD(SetItemIcon, METH_VARARGS),
	METHOD(EnterStore, METH_VARARGS),
	METHOD(GetStoreName, METH_VARARGS),
	METHOD(GetStoreRoomPrices, METH_VARARGS),
	METHOD(InvalidateWindow, METH_VARARGS),
	METHOD(EnableCheatKeys, METH_VARARGS), 
	METHOD(UpdateMusicVolume, METH_NOARGS),
	METHOD(UpdateAmbientsVolume, METH_NOARGS),
	METHOD(GetCurrentArea, METH_NOARGS),
	METHOD(MoveToArea, METH_VARARGS),
	METHOD(GetMemorizableSpellsCount, METH_VARARGS),
	METHOD(SetMemorizableSpellsCount, METH_VARARGS),
	METHOD(GetKnownSpellsCount, METH_VARARGS),
	METHOD(GetKnownSpell, METH_VARARGS),
	METHOD(GetMemorizedSpellsCount, METH_VARARGS),
	METHOD(GetMemorizedSpell, METH_VARARGS),
	METHOD(GetSpell, METH_VARARGS),
	METHOD(MemorizeSpell, METH_VARARGS),
	METHOD(UnmemorizeSpell, METH_VARARGS),
	METHOD(GetSlotItem, METH_VARARGS),
	METHOD(GetItem, METH_VARARGS),
	METHOD(DragItem, METH_VARARGS),
	METHOD(DropDraggedItem, METH_VARARGS),
	METHOD(IsDraggingItem, METH_NOARGS),
	METHOD(GetSystemVariable, METH_VARARGS),

	// terminating entry	
	{NULL, NULL, 0, NULL}
};

void initGemRB()
{
	/*PyObject * m =*/ Py_InitModule( "GemRB", GemRBMethods );
}

GUIScript::GUIScript(void)
{
	pDict = NULL; //borrowed, but used outside a function
	pModule = NULL; //should decref it
	pMainDic = NULL; //borrowed, but used outside a function
}

GUIScript::~GUIScript(void)
{
	if (Py_IsInitialized()) {
		if(pModule) {
			Py_DECREF( pModule );
		}
		Py_Finalize();
	}
}

PyDoc_STRVAR( GemRB__doc,
"Module exposing GemRB data and engine internals\n\n"
"This module exposes to python GUIScripts GemRB engine data and internals."
"It's implemented in gemrb/plugins/GUIScript/GUIScript.cpp\n\n" );

/** Initialization Routine */

bool GUIScript::Init(void)
{
//this should be a file name to python, not a title!
//	Py_SetProgramName( "GemRB -- Python" );
	Py_Initialize();
	if (!Py_IsInitialized()) {
		return false;
	}
	PyObject* pGemRB = Py_InitModule3( "GemRB", GemRBMethods, GemRB__doc );
	if (!pGemRB) {
		return false;
	}
	/*pGemRB is a borrowed reference */
	//pGemRBDict = PyModule_GetDict( pGemRB );
	/*pGemRBDict is a borrowed reference, and we don't need it*/
	char string[256];
	// FIXME: crashes
	//if (PyRun_SimpleString( "import pdb" ) == -1) {
	//	return false;
	//}
	if (PyRun_SimpleString( "import sys" ) == -1) {
		PyRun_SimpleString( "pdb.pm()" );
		PyErr_Print();
		return false;
	}
	char path[_MAX_PATH];
	char path2[_MAX_PATH];

	strncpy (path, core->GUIScriptsPath, _MAX_PATH);
	PathAppend( path, "GUIScripts" );

	memcpy(path2, path,_MAX_PATH);
	PathAppend( path2, core->GameType );


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
		PyRun_SimpleString( "pdb.pm()" );
		PyErr_Print();
		return false;
	}
	sprintf( string, "sys.path.append(\"%s\")", path );
	if (PyRun_SimpleString( string ) == -1) {
		PyRun_SimpleString( "pdb.pm()" );
		PyErr_Print();
		return false;
	}
	if (PyRun_SimpleString( "import GemRB" ) == -1) {
		PyRun_SimpleString( "pdb.pm()" );
		PyErr_Print();
		return false;
	}
	if (PyRun_SimpleString( "from GUIDefines import *" ) == -1) {
		PyRun_SimpleString( "pdb.pm()" );
		PyErr_Print();
		return false;
	}
	PyObject *pMainMod = PyImport_AddModule( "__main__" );
	/* pMainMod is a borrowed reference */
	pMainDic = PyModule_GetDict( pMainMod );
	/* pMainDic is a borrowed reference */
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

bool GUIScript::RunFunction(const char* fname)
{
	if (!Py_IsInitialized()) {
		return false;
	}
	if (pDict == NULL) {
		return false;
	}

	PyObject* pFunc, * pArgs, * pValue;

	pFunc = PyDict_GetItemString( pDict, fname );
	/* pFunc: Borrowed reference */
	if (( !pFunc ) || ( !PyCallable_Check( pFunc ) )) {
		return false;
	}
	pArgs = NULL;
	pValue = PyObject_CallObject( pFunc, pArgs );
	if (pValue == NULL) {
		PyErr_Print();
		return false;
	}
	Py_DECREF( pValue );
	return true;
}

/** Exec a single String */
char* GUIScript::ExecString(const char* string)
{
	if (PyRun_SimpleString( string ) == -1) {
		PyErr_Print();
		// try with GemRB. prefix
		char * newstr = (char *) malloc( strlen(string) + 7 );
		strncpy(newstr, "GemRB.", 6);
		strcpy(newstr + 6, string);
		if (PyRun_SimpleString( newstr ) == -1) {
			PyErr_Print();
		}
		free( newstr );
	}
	return NULL;
}
