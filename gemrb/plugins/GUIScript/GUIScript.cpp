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
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/plugins/GUIScript/GUIScript.cpp,v 1.130 2004/02/29 19:32:35 edheldil Exp $
 *
 */

#include "GUIScript.h"
#include "../Core/Interface.h"
#include "../Core/Map.h"
#ifdef WIN32
#ifdef _DEBUG
#undef _DEBUG
#include "Python.h"
#define _DEBUG
#else
#include "Python.h"
#endif
#else
#include "Python.h"
#endif
#include "../Core/Label.h"
#include "../Core/AnimationMgr.h"
#include "../Core/GameControl.h"
#include "../Core/MapMgr.h"
#include "../Core/WorldMapMgr.h"
#include "../Core/SpellMgr.h"
#include "../Core/ItemMgr.h"
#include "../Core/StoreMgr.h"

inline bool valid_number(const char* string, long& val)
{
	char* endpr;

	val = strtol( string, &endpr, 0 );
	return ( const char * ) endpr != string;
}

static PyObject* GemRB_UnhideGUI(PyObject*, PyObject* args)
{
	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );
	if (gc->ControlType != IE_GUI_GAMECONTROL) {
		return NULL;
	}
	gc->UnhideGUI();
	gc->SetCutSceneMode( false );
	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_HideGUI(PyObject*, PyObject* args)
{
	GameControl* gc = ( GameControl* ) core->GetWindow( 0 )->GetControl( 0 );
	if (gc->ControlType != IE_GUI_GAMECONTROL) {
		return NULL;
	}
	gc->HideGUI();

	Py_INCREF( Py_None );
	return Py_None;
}

GameControl* StartGameControl()
{
	int count = core->GetWindowMgr()->GetWindowsCount();
	for (int i = 0; i < count; i++) {
		core->DelWindow( i );
	}
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
	gamewin->SetFocused( gc );
	core->SetVisible( 0, 1 );
	if (core->GetGUIScriptEngine()->LoadScript( "MessageWindow" )) {
		core->GetGUIScriptEngine()->RunFunction( "OnLoad" );
		unsigned long index;
		gc->UnhideGUI();
	}
	if (core->ConsolePopped) {
		core->PopupConsole();
	}

	return gc;
}

static PyObject* GemRB_LoadGame(PyObject*, PyObject* args)
{
	int GameIndex;

	if (!PyArg_ParseTuple( args, "i", &GameIndex )) {
		printMessage( "GUIScript", "Syntax Error: LoadGame(Index)\n",
			LIGHT_RED );
		return NULL;
	}
	core->LoadGame( GameIndex );
	GameControl* gc = StartGameControl();
	gc->SetCurrentArea( 0 );
	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_EnterGame(PyObject*, PyObject* args)
{
	core->LoadGame( -1 );
	GameControl* gc = StartGameControl();
	// 0 - single player, 1 - tutorial, 2 - multiplayer
	unsigned long playmode = 0;
	core->GetDictionary()->Lookup( "PlayMode", playmode );
	playmode *= 3;
	int start = core->LoadTable( "STARTARE" );
	TableMgr* tm = core->GetTable( start );
	char* StartArea = tm->QueryField( playmode );
	int startX = atoi( tm->QueryField( playmode + 1 ) );
	int startY = atoi( tm->QueryField( playmode + 2 ) );
	gc->SetCurrentArea( 0 );
	core->GetVideoDriver()->MoveViewportTo( startX, startY );
	core->EnterActors( StartArea );
	core->DelTable( start );
	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_MoveTAText(PyObject * /*self*/, PyObject* args)
{
	int srcWin, srcCtrl, dstWin, dstCtrl;

	if (!PyArg_ParseTuple( args, "iiii", &srcWin, &srcCtrl, &dstWin, &dstCtrl )) {
		printMessage( "GUIScript",
			"Syntax Error: MoveTAText(srcWin, srcCtrl, dstWin, dstCtrl)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* SrcWin = core->GetWindow( srcWin );
	if (!SrcWin) {
		return NULL;
	}
	TextArea* SrcTA = ( TextArea* ) SrcWin->GetControl( srcCtrl );
	if (!SrcTA) {
		return NULL;
	}
	if (SrcTA->ControlType != IE_GUI_TEXTAREA) {
		return NULL;
	}

	Window* DstWin = core->GetWindow( dstWin );
	if (!DstWin) {
		return NULL;
	}
	TextArea* DstTA = ( TextArea* ) DstWin->GetControl( dstCtrl );
	if (!DstTA) {
		return NULL;
	}
	if (DstTA->ControlType != IE_GUI_TEXTAREA) {
		return NULL;
	}

	SrcTA->CopyTo( DstTA );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetTAAutoScroll(PyObject * /*self*/, PyObject* args)
{
	int wi, ci, state;

	if (!PyArg_ParseTuple( args, "iii", &wi, &ci, &state )) {
		printMessage( "GUIScript",
			"Syntax Error: SetTAAutoScroll(WindowIndex, ControlIndex, State)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( wi );
	if (!win) {
		return NULL;
	}
	TextArea* ta = ( TextArea* ) win->GetControl( ci );
	if (!ta) {
		return NULL;
	}
	if (ta->ControlType != IE_GUI_TEXTAREA) {
		return NULL;
	}
	ta->AutoScroll = state;

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_StatComment(PyObject * /*self*/, PyObject* args)
{
	int Strref, X, Y;
	PyObject* ret;

	if (!PyArg_ParseTuple( args, "iii", &Strref, &X, &Y )) {
		printMessage( "GUIScript",
			"Syntax Error: StatComment(Strref, X, Y)\n", LIGHT_RED );
		return NULL;
	}
	char* text = core->GetString( Strref );
	char* newtext = ( char* ) malloc( strlen( text ) + 12 );
	//this could be DANGEROUS
	sprintf( newtext, text, X, Y );
	free( text );
	ret = Py_BuildValue( "s", newtext );
	free( newtext );
	return ret;
}

static PyObject* GemRB_EndCutSceneMode(PyObject * /*self*/, PyObject* args)
{
	core->SetCutSceneMode( false );
	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_LoadWindowPack(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		printMessage( "GUIScript", "Syntax Error: Expected String\n",
			LIGHT_RED );
		return NULL;
	}

	if (!core->LoadWindowPack( string )) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_LoadWindow(PyObject * /*self*/, PyObject* args)
{
	int WindowID;

	if (!PyArg_ParseTuple( args, "i", &WindowID )) {
		printMessage( "GUIScript",
			"Syntax Error: Expected an Unsigned Short Value\n", LIGHT_RED );
		return NULL;
	}

	int ret = core->LoadWindow( WindowID );
	if (ret == -1) {
		return NULL;
	}

	return Py_BuildValue( "i", ret );
}

static PyObject* GemRB_SetWindowSize(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, Width, Height;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &Width, &Height )) {
		printMessage( "GUIScript",
			"Syntax Error: SetWindowSize(WindowIndex, Width, Height)\n",
			LIGHT_RED );
		return NULL;
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

static PyObject* GemRB_EnableCheatKeys(PyObject * /*self*/, PyObject* args)
{
	int Flag;

	if (!PyArg_ParseTuple( args, "i", &Flag )) {
		printMessage( "GUIScript", "Syntax Error: EnableCheatKeys(flag)\n",
			LIGHT_RED );
		return NULL;
	}

	core->EnableCheatKeys( Flag );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetWindowPicture(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;
	char* MosResRef;

	if (!PyArg_ParseTuple( args, "is", &WindowIndex, &MosResRef )) {
		printMessage( "GUIScript",
			"Syntax Error: SetWindowPicture(WindowIndex, MosResRef)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (!win) {
		return NULL;
	}

	DataStream* bkgr = core->GetResourceMgr()->GetResource( MosResRef,
												IE_MOS_CLASS_ID );
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

static PyObject* GemRB_SetWindowPos(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, X, Y;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &X, &Y )) {
		printMessage( "GUIScript",
			"Syntax Error: SetWindowPos(WindowIndex, X, Y)\n", LIGHT_RED );
		return NULL;
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

static PyObject* GemRB_LoadTable(PyObject * /*self*/, PyObject* args)
{
	char* string;
	int noerror = 0;

	if (!PyArg_ParseTuple( args, "s|i", &string, &noerror )) {
		printMessage( "GUIScript", "Syntax Error: Expected String\n",
			LIGHT_RED );
		return NULL;
	}

	int ind = core->LoadTable( string );
	if (!noerror && ind == -1) {
		printMessage( "GUIScript", "Can't find resource\n", LIGHT_RED );
		return NULL;
	}

	return Py_BuildValue( "i", ind );
}

static PyObject* GemRB_UnloadTable(PyObject * /*self*/, PyObject* args)
{
	int ti;

	if (!PyArg_ParseTuple( args, "i", &ti )) {
		printMessage( "GUIScript", "Syntax Error: Expected Integer\n",
			LIGHT_RED );
		return NULL;
	}

	int ind = core->DelTable( ti );
	if (ind == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_GetTable(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		printMessage( "GUIScript", "Syntax Error: Expected String\n",
			LIGHT_RED );
		return NULL;
	}

	int ind = core->GetIndex( string );
	if (ind == -1) {
		return NULL;
	}

	return Py_BuildValue( "i", ind );
}

static PyObject* GemRB_GetTableValue(PyObject * /*self*/, PyObject* args)
{
	PyObject* ti, * row, * col;

	if (PyArg_UnpackTuple( args, "ref", 3, 3, &ti, &row, &col )) {
		if (!PyObject_TypeCheck( ti, &PyInt_Type )) {
			printMessage( "GUIScript",
				"Syntax Error: GetTableValue(Table, RowIndex/RowString, ColIndex/ColString)\n",
				LIGHT_RED );
			return NULL;
		}
		int TableIndex = PyInt_AsLong( ti );
		if (( !PyObject_TypeCheck( row, &PyInt_Type ) ) &&
			( !PyObject_TypeCheck( row, &PyString_Type ) )) {
			printMessage( "GUIScript",
				"Syntax Error: GetTableValue(Table, RowIndex/RowString, ColIndex/ColString)\n",
				LIGHT_RED );
			return NULL;
		}
		if (( !PyObject_TypeCheck( col, &PyInt_Type ) ) &&
			( !PyObject_TypeCheck( col, &PyString_Type ) )) {
			printMessage( "GUIScript",
				"Syntax Error: GetTableValue(Table, RowIndex/RowString, ColIndex/ColString)\n",
				LIGHT_RED );
			return NULL;
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

static PyObject* GemRB_FindTableValue(PyObject * /*self*/, PyObject* args)
{
	int ti, col, row;
	unsigned long Value;

	if (!PyArg_ParseTuple( args, "iil", &ti, &col, &Value )) {
		printMessage( "GUIScript",
			"Syntax Error: FindTableValue(TableIndex, ColumnIndex, Value)\n",
			LIGHT_RED );
		return NULL;
	}

	TableMgr* tm = core->GetTable( ti );
	if (tm == NULL) {
		return NULL;
	}
	for (row = 0; row < tm->GetRowCount(); row++) {
		char* ret = tm->QueryField( row, col );
		long val;
		if (valid_number( ret, val ) && Value == val)
			return Py_BuildValue( "i", row );
	}
	return Py_BuildValue( "i", -1 ); //row not found
}

static PyObject* GemRB_GetTableRowIndex(PyObject * /*self*/, PyObject* args)
{
	int ti;
	char* rowname;

	if (!PyArg_ParseTuple( args, "is", &ti, &rowname )) {
		printMessage( "GUIScript",
			"Syntax Error: GetTableRowIndex(TableIndex, RowName)\n",
			LIGHT_RED );
		return NULL;
	}

	TableMgr* tm = core->GetTable( ti );
	if (tm == NULL) {
		return NULL;
	}
	int row = tm->GetRowIndex( rowname );
	//no error if the row doesn't exist
	return Py_BuildValue( "i", row );
}
static PyObject* GemRB_GetTableRowName(PyObject * /*self*/, PyObject* args)
{
	int ti, row;

	if (!PyArg_ParseTuple( args, "ii", &ti, &row )) {
		printMessage( "GUIScript",
			"Syntax Error: GetTableRowName(TableIndex, RowIndex)\n",
			LIGHT_RED );
		return NULL;
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

static PyObject* GemRB_GetTableRowCount(PyObject * /*self*/, PyObject* args)
{
	int ti;

	if (!PyArg_ParseTuple( args, "i", &ti )) {
		printMessage( "GUIScript", "Syntax Error: Expected Integer\n",
			LIGHT_RED );
		return NULL;
	}

	TableMgr* tm = core->GetTable( ti );
	if (tm == NULL) {
		return NULL;
	}

	return Py_BuildValue( "i", tm->GetRowCount() );
}

static PyObject* GemRB_LoadSymbol(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		printMessage( "GUIScript", "Syntax Error: Expected String\n",
			LIGHT_RED );
		return NULL;
	}

	int ind = core->LoadSymbol( string );
	if (ind == -1) {
		return NULL;
	}

	return Py_BuildValue( "i", ind );
}

static PyObject* GemRB_UnloadSymbol(PyObject * /*self*/, PyObject* args)
{
	int si;

	if (!PyArg_ParseTuple( args, "i", &si )) {
		printMessage( "GUIScript",
			"Syntax Error:UnloadSymbol Expected Integer\n", LIGHT_RED );
		return NULL;
	}

	int ind = core->DelSymbol( si );
	if (ind == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_GetSymbol(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		printMessage( "GUIScript", "Syntax Error: Expected String\n",
			LIGHT_RED );
		return NULL;
	}

	int ind = core->GetSymbolIndex( string );
	if (ind == -1) {
		return NULL;
	}

	return Py_BuildValue( "i", ind );
}

static PyObject* GemRB_GetSymbolValue(PyObject * /*self*/, PyObject* args)
{
	PyObject* si, * sym;

	if (PyArg_UnpackTuple( args, "ref", 2, 2, &si, &sym )) {
		if (!PyObject_TypeCheck( si, &PyInt_Type )) {
			printMessage( "GUIScript",
				"Syntax Error: GetSymbolValue(Table, RowIndex/RowString, ColIndex/ColString)\n",
				LIGHT_RED );
			return NULL;
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
	printMessage( "GUIScript",
		"Type Error: GetSymbolValue(SymbolTable, StringVal/IntVal)\n",
		LIGHT_RED );
	return NULL;
}

static PyObject* GemRB_GetControl(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlID )) {
		printMessage( "GUIScript",
			"Syntax Error: GetControl(unsigned short WindowIndex, unsigned long ControlID)\n",
			LIGHT_RED );
		return NULL;
	}

	int ret = core->GetControl( WindowIndex, ControlID );
	if (ret == -1) {
		return NULL;
	}

	return Py_BuildValue( "i", ret );
}

static PyObject* GemRB_QueryText(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlIndex )) {
		printMessage( "GUIScript",
			"Syntax Error: QueryText(unsigned short WindowIndex, unsigned long ControlIndex)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_EDIT) {
		return NULL;
	}

	return Py_BuildValue( "s", ( ( TextEdit * ) ctrl )->QueryText() );
}

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
			printMessage( "GUIScript",
				"Syntax Error: SetText(WindowIndex, ControlIndex, String|Strref, [row])\n",
				LIGHT_RED );
			return NULL;
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

static PyObject* GemRB_TextAreaAppend(PyObject * /*self*/, PyObject* args)
{
	PyObject* wi, * ci, * str, * row = NULL;
	int WindowIndex, ControlIndex, StrRef, Row;
	char* string;
	int ret;

	if (PyArg_UnpackTuple( args, "ref", 3, 4, &wi, &ci, &str, &row )) {
		if (!PyObject_TypeCheck( wi, &PyInt_Type ) ||
			!PyObject_TypeCheck( ci,
														&PyInt_Type ) ||
			( !PyObject_TypeCheck( str, &PyString_Type ) &&
			!PyObject_TypeCheck( str,
																&PyInt_Type ) )) {
			printMessage( "GUIScript",
				"Syntax Error: TextAreaAppend(unsigned short WindowIndex, unsigned short ControlIndex, char * string)\n",
				LIGHT_RED );
			return NULL;
		}
		WindowIndex = PyInt_AsLong( wi );
		ControlIndex = PyInt_AsLong( ci );
		Window* win = core->GetWindow( WindowIndex );
		if (!win)
			return NULL;
		Control* ctrl = win->GetControl( ControlIndex );
		if (!ctrl)
			return NULL;
		if (ctrl->ControlType != IE_GUI_TEXTAREA)
			return NULL;
		TextArea* ta = ( TextArea* ) ctrl;
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

static PyObject* GemRB_SetVisible(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;
	int visible;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &visible )) {
		printMessage( "GUIScript",
			"Syntax Error: SetVisible(unsigned short WindowIndex, int visible)\n",
			LIGHT_RED );
		return NULL;
	}

	int ret = core->SetVisible( WindowIndex, visible );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

//useful only for ToB and HoW, sets masterscript/worldmap name
PyObject* GemRB_SetMasterScript(PyObject * /*self*/, PyObject* args)
{
	char* script;
	char* worldmap;

	if (!PyArg_ParseTuple( args, "ss", &script, &worldmap )) {
		printMessage( "GUIScript",
			"Syntax Error: SetMasterScript expects 2 ResRefs\n", LIGHT_RED );
		return NULL;
	}
	strncpy( core->GlobalScript, script, 8 );
	strncpy( core->GlobalMap, worldmap, 8 );
	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_ShowModal(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;

	if (!PyArg_ParseTuple( args, "i", &WindowIndex )) {
		printMessage( "GUIScript", "Syntax Error: Expected a window index\n",
			LIGHT_RED );
		return NULL;
	}

	int ret = core->ShowModal( WindowIndex );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetEvent(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int event;
	char* funcName;

	if (!PyArg_ParseTuple( args, "iiis", &WindowIndex, &ControlIndex, &event,
			&funcName )) {
		printMessage( "GUIScript",
			"Syntax Error: SetEvent(WindowIndex, ControlIndex, EventMask, FunctionName)\n",
			LIGHT_RED );
		return NULL;
	}

	int ret = core->SetEvent( WindowIndex, ControlIndex, event, funcName );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetNextScript(PyObject * /*self*/, PyObject* args)
{
	char* funcName;

	if (!PyArg_ParseTuple( args, "s", &funcName )) {
		printMessage( "GUIScript",
			"Syntax Error: SetNextScript expects a script name\n", LIGHT_RED );
		return NULL;
	}

	strcpy( core->NextScript, funcName );
	core->ChangeScript = true;

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetControlStatus(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	int status;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &status )) {
		printMessage( "GUIScript",
			"Syntax Error: SetControlStatus(WindowIndex, ControlIndex, status)\n",
			LIGHT_RED );
		return NULL;
	}


	int ret = core->SetControlStatus( WindowIndex, ControlIndex, status );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetVarAssoc(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	unsigned long Value;
	char* VarName;

	if (!PyArg_ParseTuple( args, "iisl", &WindowIndex, &ControlIndex,
			&VarName, &Value )) {
		printMessage( "GUIScript",
			"Syntax Error: SetVarAssoc(WindowIndex, ControlIndex, VariableName, LongValue)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	strncpy( ctrl->VarName, VarName, MAX_VARIABLE_LENGTH );
	ctrl->Value = Value;
	/** setting the correct state for this control */
	/** it is possible to set up a default value, if Lookup returns false, use it */
	Value = 0;
	core->GetDictionary()->Lookup( VarName, Value );
	win->RedrawControls( VarName, Value );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_UnloadWindow(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;

	if (!PyArg_ParseTuple( args, "i", &WindowIndex )) {
		printMessage( "GUIScript",
			"Syntax Error: UnloadWindow(WindowIndex)\n", LIGHT_RED );
		return NULL;
	}

	int ret = core->DelWindow( WindowIndex );
	if (ret == -1) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_InvalidateWindow(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex;

	if (!PyArg_ParseTuple( args, "i", &WindowIndex )) {
		printMessage( "GUIScript",
			"Syntax Error: InvalidateWindow(WindowIndex)\n", LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (!win) {
		return NULL;
	}
	win->Invalidate();

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_CreateLabel(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h, align;
	char* font, * text;

	if (!PyArg_ParseTuple( args, "iiiiiissi", &WindowIndex, &ControlID, &x,
			&y, &w, &h, &font, &text, &align )) {
		printMessage( "GUIScript",
			"Syntax Error: CreateLabel(WindowIndex, ControlIndex, x, y, w, h, font, text, align)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}
	Label* lbl = new Label( 4096, core->GetFont( font ) );
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

static PyObject* GemRB_SetLabelTextColor(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, r, g, b;

	if (!PyArg_ParseTuple( args, "iiiii", &WindowIndex, &ControlIndex, &r, &g,
			&b )) {
		printMessage( "GUIScript",
			"Syntax Error: SetLabelTextColor(WindowIndex, ControlIndex, red, green, blue)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (!ctrl) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_LABEL) {
		return NULL;
	}

	Label* lab = ( Label* ) ctrl;
	Color fore = {r,g, b, 0}, back = {0, 0, 0, 0};
	lab->SetColor( fore, back );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_CreateButton(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlID, x, y, w, h;

	if (!PyArg_ParseTuple( args, "iiiiii", &WindowIndex, &ControlID, &x, &y,
			&w, &h )) {
		printMessage( "GUIScript",
			"Syntax Error: CreateButton(WindowIndex, ControlIndex, x, y, w, h)\n",
			LIGHT_RED );
		return NULL;
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

static PyObject* GemRB_SetButtonSprites(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, cycle, unpressed, pressed, selected,
		disabled;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iisiiiii", &WindowIndex, &ControlIndex,
			&ResRef, &cycle, &unpressed, &pressed, &selected, &disabled )) {
		printMessage( "GUIScript",
			"Syntax Error: SetButtonSprites(WindowIndex, ControlIndex, ResRef, Cycle, UnpressedFrame, PressedFrame, SelectedFrame, DisabledFrame)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (!ctrl) {
		return NULL;
	}

	if (ctrl->ControlType != 0) {
		return NULL;
	}

	Button* btn = ( Button* ) ctrl;

	if (ResRef[0] == 0) {
		btn->SetImage( IE_GUI_BUTTON_UNPRESSED, 0 );
		btn->SetImage( IE_GUI_BUTTON_PRESSED, 0 );
		btn->SetImage( IE_GUI_BUTTON_SELECTED, 0 );
		btn->SetImage( IE_GUI_BUTTON_DISABLED, 0 );
		Py_INCREF( Py_None );
		return Py_None;
	}

	AnimationFactory* bam = ( AnimationFactory* )
		core->GetResourceMgr()->GetFactoryResource( ResRef,
									IE_BAM_CLASS_ID );
	if (!bam) {
		printMessage( "GUIScript", "Error: %s.BAM not Found\n", LIGHT_RED );
		return NULL;
	}

	Animation* ani = bam->GetCycle( cycle );

	btn->SetImage( IE_GUI_BUTTON_UNPRESSED, ani->GetFrame( unpressed ) );
	btn->SetImage( IE_GUI_BUTTON_PRESSED, ani->GetFrame( pressed ) );
	btn->SetImage( IE_GUI_BUTTON_SELECTED, ani->GetFrame( selected ) );
	btn->SetImage( IE_GUI_BUTTON_DISABLED, ani->GetFrame( disabled ) );
	ani->free = false;

	delete( ani );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetControlPos(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, X, Y;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &X, &Y )) {
		printMessage( "GUIScript",
			"Syntax Error: SetControlPos(WindowIndex, ControlIndex, X, Y)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (!win) {
		return NULL;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (!ctrl) {
		return NULL;
	}

	ctrl->XPos = X;
	ctrl->YPos = Y;

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetControlSize(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Width, Height;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &Width,
			&Height )) {
		printMessage( "GUIScript",
			"Syntax Error: SetControlPos(WindowIndex, ControlIndex, Width, Height)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (!win) {
		return NULL;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (!ctrl) {
		return NULL;
	}

	ctrl->Width = Width;
	ctrl->Height = Height;

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetLabelUseRGB(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, status;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &status )) {
		printMessage( "GUIScript",
			"Syntax Error: UnloadWindow(WindowIndex)\n", LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (!win) {
		return NULL;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (!ctrl) {
		return NULL;
	}
	if (ctrl->ControlType != IE_GUI_LABEL) {
		return NULL;
	}
	Label* lab = ( Label* ) ctrl;
	lab->useRGB = status;

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetTextAreaSelectable(PyObject * /*self*/,
	PyObject* args)
{
	int WindowIndex, ControlIndex, Flag;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &Flag )) {
		printMessage( "GUIScript",
			"Syntax Error: SetTextAreaSelectable(WindowIndex, ControlIndex, Flag)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_TEXTAREA) //TextArea
	{
		return NULL;
	}

	TextArea* ta = ( TextArea* ) ctrl;
	ta->SetSelectable( !!Flag );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetButtonFlags(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, Flags, Operation;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex, &Flags,
			&Operation )) {
		printMessage( "GUIScript",
			"Syntax Error: SetButtonFlags(WindowIndex, ControlIndex, Flags, Operation)\n",
			LIGHT_RED );
		return NULL;
	}
	if (Operation< 0 || Operation>2) {
		printMessage( "GUIScript",
			"Syntax Error: SetButtonFlags operation must be 0-2\n", LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		return NULL;
	}

	Button* btn = ( Button* ) ctrl;
	btn->SetFlags( Flags, Operation );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetButtonState(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, state;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex, &state )) {
		printMessage( "GUIScript",
			"Syntax Error: SetButtonState(WindowIndex, ControlIndex, State)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		return NULL;
	}

	Button* btn = ( Button* ) ctrl;
	btn->SetState( state );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetButtonPicture(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex, &ResRef )) {
		printMessage( "GUIScript",
			"Syntax Error: SetButtonPicture(WindowIndex, ControlIndex, PictureResRef)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		Button* btn = ( Button* ) ctrl;
		btn->SetPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

	DataStream* str = core->GetResourceMgr()->GetResource( ResRef,
												IE_BMP_CLASS_ID );
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

	Button* btn = ( Button* ) ctrl;
	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetButtonMOS(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex, &ResRef )) {
		printMessage( "GUIScript",
			"Syntax Error: SetButtonMOS(WindowIndex, ControlIndex, MOSResRef)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		Button* btn = ( Button* ) ctrl;
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

	Button* btn = ( Button* ) ctrl;
	btn->SetFlags( 0x82, OP_OR );
	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetButtonPLT(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, col1, col2, col3, col4, col5, col6, col7,
		col8;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iisiiiiiiii", &WindowIndex, &ControlIndex,
			&ResRef, &col1, &col2, &col3, &col4, &col5, &col6, &col7, &col8 )) {
		printMessage( "GUIScript",
			"Syntax Error: SetButtonPLT(WindowIndex, ControlIndex, PLTResRef, col1, col2, col3, col4, col5, col6, col7, col8)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		Button* btn = ( Button* ) ctrl;
		btn->SetPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

	DataStream* str = core->GetResourceMgr()->GetResource( ResRef,
												IE_PLT_CLASS_ID );
	if (str == NULL) {
		return NULL;
	}
	ImageMgr* im = ( ImageMgr* ) core->GetInterface( IE_PLT_CLASS_ID );
	if (im == NULL) {
		delete ( str );
		return NULL;
	}

	if (!im->Open( str, true )) {
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
		core->FreeInterface( im );
		return NULL;
	}

	Button* btn = ( Button* ) ctrl;
	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetButtonBAM(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, CycleIndex, FrameIndex, col1;
	char* ResRef;

	if (!PyArg_ParseTuple( args, "iisiii", &WindowIndex, &ControlIndex,
			&ResRef, &CycleIndex, &FrameIndex, &col1 )) {
		printMessage( "GUIScript",
			"Syntax Error: SetButtonBAM(WindowIndex, ControlIndex, BAMResRef, CycleIndex, FrameIndex, col1)\n",
			LIGHT_RED );
		return NULL;
	}

	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		return NULL;
	}

	if (ResRef[0] == 0) {
		Button* btn = ( Button* ) ctrl;
		btn->SetPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

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

	Color* pal = core->GetPalette( col1, 12 );
	Color* orgpal = core->GetVideoDriver()->GetPalette( Picture );
	memcpy( &orgpal[4], pal, 12 * sizeof( Color ) );
	core->GetVideoDriver()->SetPalette( Picture, orgpal );
	free( pal );
	free( orgpal );

	Button* btn = ( Button* ) ctrl;
	btn->SetPicture( Picture );

	core->FreeInterface( am );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_PlaySound(PyObject * /*self*/, PyObject* args)
{
	char* ResRef;

	if (!PyArg_ParseTuple( args, "s", &ResRef )) {
		printMessage( "GUIScript", "Syntax Error: PlaySound(SoundResource)\n",
			LIGHT_RED );
		return NULL;
	}

	int ret = core->GetSoundMgr()->Play( ResRef );
	if (!ret) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_DrawWindows(PyObject * /*self*/, PyObject * /*args*/)
{
	core->DrawWindows();

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_Quit(PyObject * /*self*/, PyObject * /*args*/)
{
	bool ret = core->Quit();
	if (!ret) {
		return NULL;
	}

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_LoadMusicPL(PyObject * /*self*/, PyObject* args)
{
	char* ResRef;
	int HardEnd = 0;

	if (!PyArg_ParseTuple( args, "s|i", &ResRef, &HardEnd )) {
		printMessage( "GUIScript",
			"Syntax Error: LoadMusicPL(MusicPlayListResource)\n", LIGHT_RED );
		return NULL;
	}

	core->GetMusicMgr()->SwitchPlayList( ResRef, HardEnd );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SoftEndPL(PyObject * /*self*/, PyObject * /*args*/)
{
	core->GetMusicMgr()->End();

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_HardEndPL(PyObject * /*self*/, PyObject * /*args*/)
{
	core->GetMusicMgr()->HardEnd();

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetToken(PyObject * /*self*/, PyObject* args)
{
	char* Variable;
	char* value;

	if (!PyArg_ParseTuple( args, "ss", &Variable, &value )) {
		printMessage( "GUIScript",
			"Syntax Error: SetVar(VariableName, Value)\n", LIGHT_RED );
		return NULL;
	}

	char* newvalue = ( char* ) malloc( strlen( value ) + 1 );  //duplicating the string
	strcpy( newvalue, value );
	core->GetTokenDictionary()->SetAt( Variable, newvalue );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetVar(PyObject * /*self*/, PyObject* args)
{
	char* Variable;
	unsigned long value;

	if (!PyArg_ParseTuple( args, "sl", &Variable, &value )) {
		printMessage( "GUIScript",
			"Syntax Error: SetVar(VariableName, Value)\n", LIGHT_RED );
		return NULL;
	}

	core->GetDictionary()->SetAt( Variable, value );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_GetToken(PyObject * /*self*/, PyObject* args)
{
	char* Variable;
	char* value;

	if (!PyArg_ParseTuple( args, "s", &Variable )) {
		printMessage( "GUIScript", "Syntax Error: GetToken(VariableName)\n",
			LIGHT_RED );
		return NULL;
	}

	//trying to cheat the pointer out from the Dictionary without allocating it
	//BuildValue will allocate its own area anyway
	if (!core->GetTokenDictionary()->Lookup( Variable,
										( unsigned long & ) value )) {
		return Py_BuildValue( "s", "" );
	}

	return Py_BuildValue( "s", value );
}

static PyObject* GemRB_GetVar(PyObject * /*self*/, PyObject* args)
{
	char* Variable;
	unsigned long value;

	if (!PyArg_ParseTuple( args, "s", &Variable )) {
		printMessage( "GUIScript", "Syntax Error: GetVar(VariableName)\n",
			LIGHT_RED );
		return NULL;
	}

	if (!core->GetDictionary()->Lookup( Variable, value )) {
		return Py_BuildValue( "l", ( unsigned long ) 0 );
	}

	return Py_BuildValue( "l", value );
}

static PyObject * GemRB_ReplaceVarsInText (PyObject * /*self*/, PyObject *args)
{
        ieStrRef  Strref;
	char* Variable;
	unsigned long value;
	//PyDict_Type reqtype;
	PyObject  *dict;

	//char newtext[4096];

	if(!PyArg_ParseTuple(args, "iO!", &Strref, &PyDict_Type, &dict)) {
		printMessage("GUIScript", "Syntax Error: ReplaceVarsInText(StrRef)\n", LIGHT_RED);
		return NULL;
	}

	char newtext[4096] = "";
	char *src = core->GetString (Strref);
	//char *src = "Page <PAGE> in <NUMPAGES> of <jshd> kjj <<ok> on <<XYX>> fif <ABC";
	char *dest = newtext;
	char *next;
	int  len;

	while (1) {
	  next = strchr (src, '<');
	  if (! next) {
	    strcpy (dest, src);
	    break;
	  }

	  len = next - src;
	  memcpy (dest, src, len);
	  src += len;
	  dest += len;

	  len = strspn (src + 1, "ABCDEFGHIJKLMNOPQRSTUVWXYZ_");

	  if (len == 0 || *(src + len + 1) != '>') {
	    memcpy (dest, src, len + 1);
	    src += len + 1;
	    dest += len + 1;
	    continue;
	  }

	  // do replace
	  memcpy (dest, src + 1, len);
	  dest[len] = 0;

	  //printf ("REPLACE: %s\n", dest);

	  /*
	    if(!core->GetDictionary()->Lookup(Variable, value))
	    return Py_BuildValue("l", (unsigned long) 0);
	  */

	  PyObject *res = PyDict_GetItemString(dict, dest);
	  if (! res) {
	    dest += len;
	  }
	  else {
	    strcpy (dest, PyString_AsString (res));
	    dest += strlen (dest);
	  }
	  src += len + 2;
	}

	//printf ("RES: %s\n", newtext);

	return Py_BuildValue("s", newtext);
}

static PyObject* GemRB_PlayMovie(PyObject * /*self*/, PyObject* args)
{
	char* string;

	if (!PyArg_ParseTuple( args, "s", &string )) {
		printMessage( "GUIScript", "Syntax Error: Expected String\n",
			LIGHT_RED );
		return NULL;
	}

	int ind = core->PlayMovie( string );
	if (ind == -1) {
		return NULL;
	}

	return Py_BuildValue( "i", ind );
}

static PyObject* GemRB_GetSaveGameCount(PyObject * /*self*/,
	PyObject * /*args*/)
{
	return Py_BuildValue( "i",
			core->GetSaveGameIterator()->GetSaveGameCount() );
}

static PyObject* GemRB_DeleteSaveGame(PyObject * /*self*/, PyObject* args)
{
	int SlotCount;

	if (!PyArg_ParseTuple( args, "i", &SlotCount )) {
		printMessage( "GUIScript",
			"Syntax Error: DeleteSaveGame(SlotCount)\n", LIGHT_RED );
		return NULL;
	}
	core->GetSaveGameIterator()->DeleteSaveGame( SlotCount );
	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_GetSaveGameAttrib(PyObject * /*self*/, PyObject* args)
{
	int Type, SlotCount;

	if (!PyArg_ParseTuple( args, "ii", &Type, &SlotCount )) {
		printMessage( "GUIScript",
			"Syntax Error: GetSaveGameAttrib(Type, SlotCount)\n", LIGHT_RED );
		return NULL;
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

static PyObject* GemRB_SetSaveGamePortrait(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, SaveSlotCount, PCSlotCount;

	if (!PyArg_ParseTuple( args, "iiii", &WindowIndex, &ControlIndex,
			&SaveSlotCount, &PCSlotCount )) {
		printMessage( "GUIScript",
			"Syntax Error: SetSaveGameAreaPortrait(WindowID, ControlID, SaveSlotCount, PCSlotCount)\n",
			LIGHT_RED );
		return NULL;
	}
	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		return NULL;
	}

	SaveGame* sg = core->GetSaveGameIterator()->GetSaveGame( SaveSlotCount );
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}
	if (sg->GetPortraitCount() <= PCSlotCount) {
		Button* btn = ( Button* ) ctrl;
		btn->SetPicture( NULL );
		Py_INCREF( Py_None );
		return Py_None;
	}

	DataStream* str = sg->GetPortrait( PCSlotCount );
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

	Button* btn = ( Button* ) ctrl;
	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetSaveGamePreview(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex, SlotCount;

	if (!PyArg_ParseTuple( args, "iii", &WindowIndex, &ControlIndex,
			&SlotCount )) {
		printMessage( "GUIScript",
			"Syntax Error: SetSaveGameAreaPreview(WindowID, ControlID, SlotCount)\n",
			LIGHT_RED );
		return NULL;
	}
	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		return NULL;
	}

	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		return NULL;
	}

	SaveGame* sg = core->GetSaveGameIterator()->GetSaveGame( SlotCount );
	if (sg == NULL) {
		printMessage( "GUIScript", "Can't find savegame\n", LIGHT_RED );
		return NULL;
	}
	DataStream* str = sg->GetScreen();
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

	Button* btn = ( Button* ) ctrl;
	btn->SetPicture( Picture );

	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_Roll(PyObject * /*self*/, PyObject* args)
{
	int Dice, Size, Add;

	if (!PyArg_ParseTuple( args, "iii", &Dice, &Size, &Add )) {
		printMessage( "GUIScript", "Syntax Error: Roll(Dice, Size, Add)\n",
			LIGHT_RED );
		return NULL;
	}
	return Py_BuildValue( "i", core->Roll( Dice, Size, Add ) );
}

static PyObject* GemRB_GetCharSounds(PyObject * /*self*/, PyObject* args)
{
	int wi, ci;

	if (!PyArg_ParseTuple( args, "ii", &wi, &ci )) {
		printMessage( "GUIScript",
			"Syntax Error: GetCharSounds(WindowID, ControlID)\n", LIGHT_RED );
	}
	Window* win = core->GetWindow( wi );
	if (!win) {
		return NULL;
	}
	TextArea* ta = ( TextArea* ) win->GetControl( ci );
	if (!ta) {
		return NULL;
	}
	if (ta->ControlType != IE_GUI_TEXTAREA) {
		return NULL;
	}
	return Py_BuildValue( "i", core->GetCharSounds( ta ) );
}

static PyObject* GemRB_ActorGetSmallPortrait(PyObject * /*self*/, PyObject* args)
{
	int index;

	if (!PyArg_ParseTuple( args, "i", &index )) {
		printMessage( "GUIScript",
			"Syntax Error: ActorGetSmallPortrait(index)\n", LIGHT_RED );
	}

	printf ("game: %p\n", core->GetGame ());
	Actor * actor = core->GetGame ()->GetPC (index);
	printf ("actor: %p\n", actor);
	return Py_BuildValue( "s", actor->GetPortrait (0));
}


static PyObject* GemRB_GetPartySize(PyObject * /*self*/, PyObject * /*args*/)
{
	return Py_BuildValue( "i", core->GetPartySize() );
}

static PyObject* GemRB_GetINIPartyCount(PyObject * /*self*/,
	PyObject * /*args*/)
{
	if (!core->GetPartyINI()) {
		return NULL;
	}
	return Py_BuildValue( "i", core->GetPartyINI()->GetTagsCount() );
}

static PyObject* GemRB_GetINIPartyKey(PyObject * /*self*/, PyObject* args)
{
	char* Tag, * Key, * Default;
	if (!PyArg_ParseTuple( args, "sss", &Tag, &Key, &Default )) {
		printMessage( "GUIScript",
			"Syntax Error: GetINIPartyKey(Tag, Key, Default)\n", LIGHT_RED );
		return NULL;
	}
	if (!core->GetPartyINI()) {
		return NULL;
	}
	return Py_BuildValue( "s",
			core->GetPartyINI()->GetKeyAsString( Tag, Key, Default ) );
}

static PyObject* GemRB_CreatePlayer(PyObject * /*self*/, PyObject* args)
{
	char* CreResRef;
	int PlayerSlot, Slot;

	if (!PyArg_ParseTuple( args, "si", &CreResRef, &PlayerSlot )) {
		printMessage( "GUIScript",
			"Syntax Error: CreatePlayer(ResRef, Slot)\n", LIGHT_RED );
		return NULL;
	}
	//PlayerSlot is zero based, if not, remove the +1
	Slot = ( PlayerSlot & 0x7fff ) + 1; 
	if (PlayerSlot & 0x8000) {
		PlayerSlot = core->FindPlayer( Slot );
		if (PlayerSlot < 0) {
			PlayerSlot = core->LoadCreature( CreResRef, Slot );
		}
	} else {
		PlayerSlot = core->FindPlayer( PlayerSlot );
		if (PlayerSlot >= 0) {
			printMessage( "GUIScript", "Slot is already filled!\n", LIGHT_RED );
			return NULL;
		}
		PlayerSlot = core->LoadCreature( CreResRef, Slot ); //inparty flag
	}
	if (PlayerSlot < 0) {
		printMessage( "GUIScript", "Not found!\n", LIGHT_RED );
		return NULL;
	}
	return Py_BuildValue( "i", PlayerSlot );
}

static PyObject* GemRB_GetPlayerStat(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, StatID, StatValue;

	if (!PyArg_ParseTuple( args, "ii", &PlayerSlot, &StatID )) {
		printMessage( "GUIScript", "Syntax Error: GetPlayerStat(Slot, ID)\n",
			LIGHT_RED );
		return NULL;
	}
	/*
		PlayerSlot = core->FindPlayer(PlayerSlot);
		if(PlayerSlot<0)
			return NULL;
	*/
	//returning the modified stat
	StatValue = core->GetCreatureStat( PlayerSlot, StatID, 1 );
	return Py_BuildValue( "i", StatValue );
}

static PyObject* GemRB_SetPlayerStat(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot, StatID, StatValue;

	if (!PyArg_ParseTuple( args, "iii", &PlayerSlot, &StatID, &StatValue )) {
		printMessage( "GUIScript",
			"Syntax Error: SetPlayerStat(Slot, ID, Value)\n", LIGHT_RED );
		return NULL;
	}
	/*
		PlayerSlot = core->FindPlayer(PlayerSlot);
		if(PlayerSlot<0) {
			printMessage("GUIScript","SetPlayerStat: Can't find actor\n",LIGHT_RED);
			return NULL;
		}
	*/
	//Setting the creature's base stat, which gets saved (0)
	if (!core->SetCreatureStat( PlayerSlot, StatID, StatValue, 0 )) {
		return NULL;
	}
	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_FillPlayerInfo(PyObject * /*self*/, PyObject* args)
{
	int PlayerSlot;

	if (!PyArg_ParseTuple( args, "i", &PlayerSlot )) {
		printMessage( "GUIScript", "Syntax Error: FillPlayerInfo(Slot)\n",
			LIGHT_RED );
		return NULL;
	}
	/*
	PlayerSlot = core->FindPlayer(PlayerSlot);
	if(PlayerSlot<0)
	return NULL;
	*/
	// here comes some code to transfer icon/name to the PC sheet
	//
	//
	Actor* MyActor = core->GetActor( PlayerSlot );
	if (!MyActor) {
		return NULL;
	}
	char* poi;
	unsigned long PortraitIndex;
	if (core->GetDictionary()->Lookup( "PortraitIndex", PortraitIndex )) {
		int table = core->LoadTable( "pictures" );
		TableMgr* tm = core->GetTable( table );
		poi = tm->GetRowName( PortraitIndex );
		MyActor->SetPortrait( poi );
	}
	int mastertable = core->LoadTable( "avprefix" );
	TableMgr* mtm = core->GetTable( mastertable );
	int count = mtm->GetRowCount();
	if (count< 1 || count>8) {
		printMessage( "GUIScript", "Table is invalid.\n", LIGHT_RED );
		return NULL;
	}
	poi = mtm->QueryField( 0 );
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
	//MyActor->SetAnimationID( AnimID );
	//setting PST's starting stance to 18
	poi = mtm->QueryField( 0, 1 );
	if (*poi != '*') {
		MyActor->AnimID = atoi( poi );
	}
	core->DelTable( mastertable );
	// 0 - single player, 1 - tutorial, 2 - multiplayer
	unsigned long playmode = 0;
	core->GetDictionary()->Lookup( "PlayMode", playmode );
	playmode *= 2;
	int saindex = core->LoadTable( "STARTPOS" );
	TableMgr* strta = core->GetTable( saindex );
	MyActor->XPos = MyActor->XDes = atoi( strta->QueryField( playmode,
													PlayerSlot ) );
	MyActor->YPos = MyActor->YDes = atoi( strta->QueryField( playmode + 1,
													PlayerSlot ) );
	MyActor->SetOver( false );
	//core->GetGame()->SetPC(MyActor);
	core->DelTable( saindex );
	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetWorldMapImage(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;

	if (!PyArg_ParseTuple( args, "ii", &WindowIndex, &ControlIndex )) {
		printMessage( "GUIScript",
			"Syntax Error: SetSaveWorldMapImage(WindowID, ControlID)\n",
			LIGHT_RED );
		return NULL;
	}
	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		printMessage( "GUIScript", "Runtime Error: Window not found\n",
			LIGHT_RED );
		return NULL;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		printMessage( "GUIScript", "Runtime Error: Control not found\n",
			LIGHT_RED );
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		printMessage( "GUIScript",
			"Runtime Error: SetSaveWorldMapImage() - control must be a button\n",
			LIGHT_RED );
		return NULL;
	}

	// FIXME!!!
	DataStream* str = core->GetResourceMgr()->GetResource( core->GlobalMap,
												IE_WMP_CLASS_ID );
	WorldMapMgr* im = ( WorldMapMgr* ) core->GetInterface( IE_WMP_CLASS_ID );
	if (im == NULL) {
		delete ( str );
		return NULL;
	}
	if (!im->Open( str, true )) {
		core->FreeInterface( im );
		return NULL;
	}

	// FIXME - should use some already allocated in core
	WorldMap* worldmap = im->GetWorldMap( 0 );
	if (worldmap == NULL) {
		core->FreeInterface( im );
		return NULL;
	}

	Button* btn = ( Button* ) ctrl;
	btn->SetFlags( 0x82, OP_OR );
	btn->SetPicture( worldmap->MapMOS );
	// FIXME: there's a leak, with MapMOS, I am afraid
	delete worldmap;
	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetSpellIcon(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char* SpellResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex,
			&SpellResRef )) {
		printMessage( "GUIScript",
			"Syntax Error: SetSpellIcon(WindowID, ControlID, SpellName)\n",
			LIGHT_RED );
		return NULL;
	}
	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		printMessage( "GUIScript", "Runtime Error: Window not found\n",
			LIGHT_RED );
		return NULL;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		printMessage( "GUIScript", "Runtime Error: Control not found\n",
			LIGHT_RED );
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		printMessage( "GUIScript",
			"Runtime Error: SetSpellIcon() - control must be a button\n",
			LIGHT_RED );
		return NULL;
	}

	// FIXME!!!
	DataStream* str = core->GetResourceMgr()->GetResource( SpellResRef,
												IE_SPL_CLASS_ID );
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

	Button* btn = ( Button* ) ctrl;
	btn->SetImage( IE_GUI_BUTTON_UNPRESSED,
			spell->SpellIconBAM->GetFrame( 0 ) );
	btn->SetImage( IE_GUI_BUTTON_PRESSED, spell->SpellIconBAM->GetFrame( 1 ) );
	delete spell;
	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_SetItemIcon(PyObject * /*self*/, PyObject* args)
{
	int WindowIndex, ControlIndex;
	char* ItemResRef;

	if (!PyArg_ParseTuple( args, "iis", &WindowIndex, &ControlIndex,
			&ItemResRef )) {
		printMessage( "GUIScript",
			"Syntax Error: SetItemIcon(WindowID, ControlID, ItemName)\n",
			LIGHT_RED );
		return NULL;
	}
	Window* win = core->GetWindow( WindowIndex );
	if (win == NULL) {
		printMessage( "GUIScript", "Runtime Error: Window not found\n",
			LIGHT_RED );
		return NULL;
	}
	Control* ctrl = win->GetControl( ControlIndex );
	if (ctrl == NULL) {
		printMessage( "GUIScript", "Runtime Error: Control not found\n",
			LIGHT_RED );
		return NULL;
	}

	if (ctrl->ControlType != IE_GUI_BUTTON) {
		printMessage( "GUIScript",
			"Runtime Error: SetItemIcon() - control must be a button\n",
			LIGHT_RED );
		return NULL;
	}

	// FIXME!!!
	DataStream* str = core->GetResourceMgr()->GetResource( ItemResRef,
												IE_ITM_CLASS_ID );
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

	Button* btn = ( Button* ) ctrl;
	btn->SetImage( IE_GUI_BUTTON_UNPRESSED, item->ItemIconBAM->GetFrame( 0 ) );
	delete item;
	core->FreeInterface( im );

	Py_INCREF( Py_None );
	return Py_None;
}

// FIXME: ugly, should be in core
Store* store;

static PyObject* GemRB_EnterStore(PyObject * /*self*/, PyObject* args)
{
	char* StoreResRef;

	if (!PyArg_ParseTuple( args, "s", &StoreResRef )) {
		printMessage( "GUIScript", "Syntax Error: EnterStore(StoreName)\n",
			LIGHT_RED );
		return NULL;
	}

	// FIXME!!!
	DataStream* str = core->GetResourceMgr()->GetResource( StoreResRef,
												IE_STO_CLASS_ID );
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

	//Button * btn = (Button*)ctrl;
	//btn->SetImage(IE_GUI_BUTTON_UNPRESSED, item->ItemIconBAM->GetFrame (0));
	//btn->SetImage(IE_GUI_BUTTON_PRESSED, item->ItemIconBAM->GetFrame (0));
	//delete store;
	core->FreeInterface( sm );

	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_GetStoreName(PyObject * /*self*/, PyObject* args)
{
	if (!PyArg_ParseTuple( args, "" )) {
		printMessage( "GUIScript", "Syntax Error: GetStoreName()\n",
			LIGHT_RED );
		return NULL;
	}

	return PyString_FromString( core->GetString( store->StoreName ) );
}

static PyObject* GemRB_GetStoreRoomPrices(PyObject * /*self*/, PyObject* args)
{
	if (!PyArg_ParseTuple( args, "" )) {
		printMessage( "GUIScript", "Syntax Error: GetStoreRoomPrices()\n",
			LIGHT_RED );
		return NULL;
	}

	PyObject* p = PyTuple_New( 4 );

	for (int i = 0; i < 4; i++) {
		PyTuple_SetItem( p, i, PyInt_FromLong( store->RoomPrices[i] ) );
	}

	return p;
}



static PyObject* GemRB_ExecuteString(PyObject * /*self*/, PyObject* args)
{
	char* String;

	if (!PyArg_ParseTuple( args, "s", &String )) {
		printMessage( "GUIScript", "Syntax Error: ExecuteString(String)\n",
			LIGHT_RED );
		return NULL;
	}
	GameScript::ExecuteString( core->GetGame()->GetMap( 0 ), String );
	Py_INCREF( Py_None );
	return Py_None;
}

static PyObject* GemRB_EvaluateString(PyObject * /*self*/, PyObject* args)
{
	char* String;

	if (!PyArg_ParseTuple( args, "s", &String )) {
		printMessage( "GUIScript", "Syntax Error: EvaluateString(String)\n",
			LIGHT_RED );
		return NULL;
	}
	if (GameScript::EvaluateString( core->GetGame()->GetMap( 0 ), String )) {
		printf( "%s returned True\n", String );
	} else {
		printf( "%s returned False\n", String );
	}
	Py_INCREF( Py_None );
	return Py_None;
}

static PyMethodDef GemRBMethods[] = {
	{"HideGUI", GemRB_HideGUI, METH_NOARGS,
	"Hides the Game GUI."}, {"UnhideGUI", GemRB_UnhideGUI, METH_NOARGS,
	"Shows the Game GUI."},
	{"SetTAAutoScroll", GemRB_SetTAAutoScroll, METH_VARARGS,
	"Sets the TextArea auto-scroll feature status."},
	{"MoveTAText", GemRB_MoveTAText, METH_VARARGS,
	"Copies a TextArea content to another."},
	{"ExecuteString", GemRB_ExecuteString, METH_VARARGS,
	"Executes an In-Game Script Action in the current Area Script Context"},
	{"EvaluateString", GemRB_EvaluateString, METH_VARARGS,
	"Evaluate an In-Game Script Trigger in the current Area Script Context"},
	{"LoadGame", GemRB_LoadGame, METH_VARARGS,
	"Loads and enters the Game."}, {"EnterGame", GemRB_EnterGame, METH_NOARGS,
	"Starts new game and enters it."},
	{"StatComment", GemRB_StatComment, METH_VARARGS,
	"Replaces values into an strref."},

	{"ReplaceVarsInText", GemRB_ReplaceVarsInText, METH_VARARGS,
	 "Replaces named vars into an strref."},
	{"ActorGetSmallPortrait", GemRB_ActorGetSmallPortrait, METH_VARARGS,
	 "Returns actor's small portrait resref."},

	{"EndCutSceneMode", GemRB_EndCutSceneMode, METH_NOARGS,
	"Exits the CutScene Mode."},
	{"GetPartySize", GemRB_GetPartySize, METH_NOARGS,
	"Returns the number of PCs."},
	{"GetINIPartyCount", GemRB_GetINIPartyCount, METH_NOARGS,
	"Returns the Number of Party defined in Party.ini (works only on IWD2)."},
	{"GetINIPartyKey", GemRB_GetINIPartyKey, METH_VARARGS,
	"Returns a Value from the Party.ini File (works only on IWD2)."},
	{"LoadWindowPack", GemRB_LoadWindowPack, METH_VARARGS,
	"Loads a WindowPack into the Window Manager Module."},
	{"LoadWindow", GemRB_LoadWindow, METH_VARARGS,
	"Returns a Window."}, {"SetWindowSize", GemRB_SetWindowSize, METH_VARARGS,
	"Resizes a Window."}, {"SetWindowPos", GemRB_SetWindowPos, METH_VARARGS,
	"Moves a Window."},
	{"SetWindowPicture", GemRB_SetWindowPicture, METH_VARARGS,
	"Changes the background of a Window."},
	{"LoadTable", GemRB_LoadTable, METH_VARARGS,
	"Loads a 2DA Table."}, {"UnloadTable", GemRB_UnloadTable, METH_VARARGS,
	"Unloads a 2DA Table."}, {"GetTable", GemRB_GetTable, METH_VARARGS,
	"Returns a Loaded 2DA Table."},
	{"GetTableValue", GemRB_GetTableValue, METH_VARARGS,
	"Returns a field of a 2DA Table."},
	{"FindTableValue", GemRB_FindTableValue, METH_VARARGS,
	"Returns the first rowcount of a field of a 2DA Table."},
	{"GetTableRowIndex", GemRB_GetTableRowIndex, METH_VARARGS,
	"Returns the Index of a Row in a 2DA Table."},
	{"GetTableRowName", GemRB_GetTableRowName, METH_VARARGS,
	"Returns the Name of a Row in a 2DA Table."},
	{"GetTableRowCount", GemRB_GetTableRowCount, METH_VARARGS,
	"Returns the number of rows in a 2DA Table."},
	{"LoadSymbol", GemRB_LoadSymbol, METH_VARARGS,
	"Loads a IDS Symbol Table."},
	{"UnloadSymbol", GemRB_UnloadSymbol, METH_VARARGS,
	"Unloads a IDS Symbol Table."},
	{"GetSymbol", GemRB_GetSymbol, METH_VARARGS,
	"Returns a Loaded IDS Symbol Table."},
	{"GetSymbolValue", GemRB_GetSymbolValue, METH_VARARGS,
	"Returns a field of a IDS Symbol Table."},
	{"GetControl", GemRB_GetControl, METH_VARARGS,
	"Returns a control in a Window."},
	{"SetText", GemRB_SetText, METH_VARARGS,
	"Sets the Text of a control in a Window."},
	{"QueryText", GemRB_QueryText, METH_VARARGS,
	"Returns the Text of a control in a Window."},
	{"TextAreaAppend", GemRB_TextAreaAppend, METH_VARARGS,
	"Appends the Text to the TextArea Control in the Window."},
	{"SetVisible", GemRB_SetVisible, METH_VARARGS,
	"Sets the Visibility Flag of a Window."},
	{"SetMasterScript", GemRB_SetMasterScript, METH_VARARGS,
	"Sets the worldmap and masterscript names."},
	{"ShowModal", GemRB_ShowModal, METH_VARARGS,
	"Show a Window on Screen setting the Modal Status."},
	{"SetEvent", GemRB_SetEvent, METH_VARARGS,
	"Sets an Event of a Control on a Window to a Script Defined Function."},
	{"SetNextScript", GemRB_SetNextScript, METH_VARARGS,
	"Sets the Next Script File to be loaded."},
	{"SetControlStatus", GemRB_SetControlStatus, METH_VARARGS,
	"Sets the status of a Control."},
	{"SetVarAssoc", GemRB_SetVarAssoc, METH_VARARGS,
	"Sets the name of the Variable associated with a control."},
	{"UnloadWindow", GemRB_UnloadWindow, METH_VARARGS,
	"Unloads a previously Loaded Window."},
	{"CreateLabel", GemRB_CreateLabel, METH_VARARGS,
	"Creates and Add a new Label to a Window."},
	{"SetLabelTextColor", GemRB_SetLabelTextColor, METH_VARARGS,
	"Sets the Text Color of a Label Control."},
	{"CreateButton", GemRB_CreateButton, METH_VARARGS,
	"Creates and Add a new Button to a Window."},
	{"SetButtonSprites", GemRB_SetButtonSprites, METH_VARARGS,
	"Sets a Button Sprites Images."},
	{"SetControlPos", GemRB_SetControlPos, METH_VARARGS,
	"Moves a Control."},
	{"SetControlSize", GemRB_SetControlSize, METH_VARARGS,
	"Resizes a Control."},
	{"SetTextAreaSelectable",GemRB_SetTextAreaSelectable, METH_VARARGS,
	"Sets the Selectable Flag of a TextArea."},
	{"SetButtonFlags", GemRB_SetButtonFlags, METH_VARARGS,
	"Sets the Display Flags of a Button."},
	{"SetButtonState", GemRB_SetButtonState, METH_VARARGS,
	"Sets the state of a Button Control."},
	{"SetButtonPicture", GemRB_SetButtonPicture, METH_VARARGS,
	"Sets the Picture of a Button Control from a BMP file."},
	{"SetButtonMOS", GemRB_SetButtonMOS, METH_VARARGS,
	"Sets the Picture of a Button Control from a MOS file."},
	{"SetButtonPLT", GemRB_SetButtonPLT, METH_VARARGS,
	"Sets the Picture of a Button Control from a PLT file."},
	{"SetButtonBAM", GemRB_SetButtonBAM, METH_VARARGS,
	"Sets the Picture of a Button Control from a BAM file."},
	{"SetLabelUseRGB", GemRB_SetLabelUseRGB, METH_VARARGS,
	"Tells a Label to use the RGB colors with the text."},
	{"PlaySound", GemRB_PlaySound, METH_VARARGS,
	"Plays a Sound."}, {"LoadMusicPL", GemRB_LoadMusicPL, METH_VARARGS,
	"Loads and starts a Music PlayList."},
	{"SoftEndPL", GemRB_SoftEndPL, METH_NOARGS,
	"Ends a Music Playlist softly."},
	{"HardEndPL", GemRB_HardEndPL, METH_NOARGS,
	"Ends a Music Playlist immediately."},
	{"DrawWindows", GemRB_DrawWindows, METH_NOARGS,
	"Refreshes the User Interface."}, {"Quit", GemRB_Quit, METH_NOARGS,
	"Quits GemRB."}, {"GetVar", GemRB_GetVar, METH_VARARGS,
	"Get a Variable value from the Global Dictionary."},
	{"SetVar", GemRB_SetVar, METH_VARARGS,
	"Set/Create a Variable in the Global Dictionary."},
	{"GetToken", GemRB_GetToken, METH_VARARGS,
	"Get a Variable value from the Token Dictionary."},
	{"SetToken", GemRB_SetToken, METH_VARARGS,
	"Set/Create a Variable in the Token Dictionary."},
	{"PlayMovie", GemRB_PlayMovie, METH_VARARGS,
	"Starts the Movie Player."}, {"Roll", GemRB_Roll, METH_VARARGS,
	"Calls traditional dice roll."},
	{"GetCharSounds", GemRB_GetCharSounds, METH_VARARGS,
	"Reads in the contents of the sounds subfolder."},
	{"GetSaveGameCount", GemRB_GetSaveGameCount, METH_VARARGS,
	"Returns the number of saved games."},
	{"DeleteSaveGame", GemRB_DeleteSaveGame, METH_VARARGS,
	"Deletes a saved game folder completely."},
	{"GetSaveGameAttrib", GemRB_GetSaveGameAttrib, METH_VARARGS,
	"Returns the name, path or prefix of the saved game."},
	{"SetSaveGamePreview", GemRB_SetSaveGamePreview, METH_VARARGS,
	"Sets a savegame area preview bmp onto a button as picture."},
	{"SetSaveGamePortrait", GemRB_SetSaveGamePortrait, METH_VARARGS,
	"Sets a savegame PC portrait bmp onto a button as picture."},
	{"CreatePlayer", GemRB_CreatePlayer, METH_VARARGS,
	"Creates a player slot."},
	{"SetPlayerStat", GemRB_SetPlayerStat, METH_VARARGS,
	"Changes a stat."}, {"GetPlayerStat", GemRB_GetPlayerStat, METH_VARARGS,
	"Queries a stat."}, {"FillPlayerInfo", GemRB_FillPlayerInfo, METH_VARARGS,
	"Fills basic character info, that is not stored in stats."},
	{"SetWorldMapImage", GemRB_SetWorldMapImage, METH_VARARGS,
	"FIXME: Set WM image ...."},
	{"SetSpellIcon", GemRB_SetSpellIcon, METH_VARARGS,
	"FIXME: temporary Set Spell icon image ...."},
	{"SetItemIcon", GemRB_SetItemIcon, METH_VARARGS,
	"FIXME: temporary Set Item icon image ...."},
	{"EnterStore", GemRB_EnterStore, METH_VARARGS,
	"FIXME: temporary EnterStore (StoreName) ...."},
	{"GetStoreName", GemRB_GetStoreName, METH_VARARGS,
	"FIXME: temporary GetStoreName () ...."},
	{"GetStoreRoomPrices", GemRB_GetStoreRoomPrices, METH_VARARGS,
	"FIXME: temporary GetStoreRoomPrices () ...."},
	{"InvalidateWindow", GemRB_InvalidateWindow, METH_VARARGS,
	"Invalidates the given Window."},
	{"EnableCheatKeys", GemRB_EnableCheatKeys, METH_VARARGS,
	"Sets CheatFlags."}, {NULL, NULL, 0, NULL}
};

void initGemRB()
{
	/*PyObject * m =*/ Py_InitModule( "GemRB", GemRBMethods );
}

GUIScript::GUIScript(void)
{
	pDict = NULL;
	pModule = NULL;
	pName = NULL;
}

GUIScript::~GUIScript(void)
{
	if (Py_IsInitialized()) {
		Py_Finalize();
	}
}

/** Initialization Routine */
bool GUIScript::Init(void)
{
	Py_SetProgramName( "GemRB -- Python" );
	Py_Initialize();
	if (!Py_IsInitialized()) {
		return false;
	}
	pGemRB = Py_InitModule( "GemRB", GemRBMethods );
	if (!pGemRB) {
		return false;
	}
	pGemRBDict = PyModule_GetDict( pGemRB );
	char string[256];
	if (PyRun_SimpleString( "import sys" ) == -1) {
		PyRun_SimpleString( "pdb.pm()" );
		PyErr_Print();
		return false;
	}
	char path[_MAX_PATH];
#ifdef WIN32
	int len = ( int ) strlen( core->GUIScriptsPath );
	int p = 0;
	for (int i = 0; i < len; i++) {
		if (core->GUIScriptsPath[i] == '\\') {
			path[p] = '/';
		} else
			path[p] = core->GUIScriptsPath[i];
		p++;
	}
	path[p] = 0;
	sprintf( string, "sys.path.append('%sGUIScripts/%s')", path,
		core->GameType );
#else
	sprintf( string, "sys.path.append('%sGUIScripts/%s')",
		core->GUIScriptsPath, core->GameType );
#endif
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
	strcpy( path, core->GUIScriptsPath );
	strcat( path, "GUIScripts" );
	strcat( path, SPathDelimiter );
	strcat( path, core->GameType );
	strcat( path, SPathDelimiter );
	strcat( path, "GUIDefines.py" );
	FILE* config = fopen( path, "rb" );
	if (!config) {
		return false;
	}
	while (true) {
		int ret = fscanf( config, "%[^\r\n]\r\n", path );
		if (ret != 1)
			break;
		int obj = PyRun_SimpleString( path );
		if (obj == -1) {
			PyErr_Print();
			return false;
		}
	}
	fclose( config );
	PyObject* mainmod = PyImport_AddModule( "__main__" );
	maindic = PyModule_GetDict( mainmod );
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

	pName = PyString_FromString( filename );
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
		if (PyDict_Merge( pDict, maindic, false ) == -1)
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

bool GUIScript::RunFunction(char* fname)
{
	if (!Py_IsInitialized()) {
		return false;
	}
	if (pDict == NULL) {
		return false;
	}

	PyObject* pFunc, * pArgs, * pValue;


	pFunc = PyDict_GetItemString( pDict, fname );
	/* pFun: Borrowed reference */
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
char* GUIScript::ExecString(char* string)
{
	if (PyRun_SimpleString( string ) == -1) {
		PyErr_Print();
	}
	return NULL;
}
