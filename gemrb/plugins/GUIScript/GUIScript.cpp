#include "../../includes/win32def.h"
#include "GUIScript.h"
#include "../Core/Interface.h"

#ifdef _DEBUG
#undef _DEBUG
#ifdef __cplusplus
extern "C" {
#endif
#include "Python.h"
#ifdef __cplusplus
}
#endif
#define _DEBUG
#else
#ifndef WIN32
#include "/usr/local/include/python2.3/Python.h"
#else
#include "Python.h"
#endif
#endif

#include "../Core/Label.h"

static PyObject * GemRB_LoadWindowPack(PyObject *self, PyObject *args)
{
	char *string;

	if(!PyArg_ParseTuple(args, "s", &string)) {
		printMessage("GUIScript", "Syntax Error: Expected String\n", LIGHT_RED);
		return NULL;
	}
	
	DataStream * stream = core->GetResourceMgr()->GetResource(string, IE_CHU_CLASS_ID);
	if(stream == NULL) {
		printMessage("GUIScript", "Error: Cannot find ", LIGHT_RED);
		printf("%s.CHU\n", string);
		return NULL;
	}
	if(!core->GetWindowMgr()->Open(stream, true)) {
		printMessage("GUIScript", "Error: Cannot Load ", LIGHT_RED);
		printf("%s.CHU\n", string);
		return NULL;
	}

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_LoadWindow(PyObject *self, PyObject *args)
{
	int WindowID;

	if(!PyArg_ParseTuple(args, "i", &WindowID)) {
		printMessage("GUIScript", "Syntax Error: Expected an Unsigned Short Value\n", LIGHT_RED);
		return NULL;
	}
	
	int ret = core->LoadWindow(WindowID);
	if(ret == -1)
		return NULL;
	
	return Py_BuildValue("i", ret);
}

static PyObject * GemRB_LoadTable(PyObject *self, PyObject *args)
{
	char *string;

	if(!PyArg_ParseTuple(args, "s", &string)) {
		printMessage("GUIScript", "Syntax Error: Expected String\n", LIGHT_RED);
		return NULL;
	}
	
	int ind = core->LoadTable(string);
	if(ind == -1)
	{
		printMessage("GUIScript","Can't find resource\n",LIGHT_RED);
		return NULL;
	}

	return Py_BuildValue("i", ind);
}

static PyObject * GemRB_UnLoadTable(PyObject *self, PyObject *args)
{
	int ti;

	if(!PyArg_ParseTuple(args, "i", &ti)) {
		printMessage("GUIScript", "Syntax Error: Expected Integer\n", LIGHT_RED);
		return NULL;
	}
	
	int ind = core->DelTable(ti);
	if(ind == -1)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_GetTable(PyObject *self, PyObject *args)
{
	char *string;

	if(!PyArg_ParseTuple(args, "s", &string)) {
		printMessage("GUIScript", "Syntax Error: Expected String\n", LIGHT_RED);
		return NULL;
	}

	int ind = core->GetIndex(string);
	if(ind == -1)
		return NULL;

	return Py_BuildValue("i", ind);
}

static PyObject * GemRB_GetTableValue(PyObject *self, PyObject *args)
{
	PyObject *ti, *row, *col;

	if(PyArg_UnpackTuple(args, "ref", 3, 3, &ti, &row, &col)) {
		if(!PyObject_TypeCheck(ti, &PyInt_Type)) {
			printMessage("GUIScript", "Syntax Error: GetTableValue(Table, RowIndex/RowString, ColIndex/ColString)\n", LIGHT_RED);
			return NULL;
		}
		int TableIndex = PyInt_AsLong(ti);
		if((!PyObject_TypeCheck(row, &PyInt_Type)) && (!PyObject_TypeCheck(row, &PyString_Type))) {
			printMessage("GUIScript", "Syntax Error: GetTableValue(Table, RowIndex/RowString, ColIndex/ColString)\n", LIGHT_RED);
			return NULL;
		}
		if((!PyObject_TypeCheck(col, &PyInt_Type)) && (!PyObject_TypeCheck(col, &PyString_Type))) {
			printMessage("GUIScript", "Syntax Error: GetTableValue(Table, RowIndex/RowString, ColIndex/ColString)\n", LIGHT_RED);
			return NULL;
		}
		if(PyObject_TypeCheck(row, &PyInt_Type) && (!PyObject_TypeCheck(col, &PyInt_Type))) {
			printMessage("GUIScript", "Type Error: RowIndex/RowString and ColIndex/ColString must be the same type\n", LIGHT_RED);
			return NULL;
		}
		if(PyObject_TypeCheck(row, &PyString_Type) && (!PyObject_TypeCheck(col, &PyString_Type))) {
			printMessage("GUIScript", "Type Error: RowIndex/RowString and ColIndex/ColString must be the same type\n", LIGHT_RED);
			return NULL;
		}
		if(PyObject_TypeCheck(row, &PyString_Type)) {
			char * rows = PyString_AsString(row);
			char * cols = PyString_AsString(col);
			TableMgr * tm = core->GetTable(TableIndex);
			char * ret = tm->QueryField(rows, cols);
			if(ret == NULL)
				return NULL;
			if((ret[0] >= '0') && (ret[0] <= '9')) {
				if(ret[1] == 'x') {
					int val;
					sscanf(ret, "0x%x", &val);
					return Py_BuildValue("i", val);
				}
				else
					return Py_BuildValue("i", atoi(ret));
			}
			return Py_BuildValue("s", ret);
		}
		else {
			int rowi = PyInt_AsLong(row);
			int coli = PyInt_AsLong(col);
			TableMgr * tm = core->GetTable(TableIndex);
			char * ret = tm->QueryField(rowi, coli);
			if(ret == NULL)
				return NULL;
			if((ret[0] >= '0') && (ret[0] <= '9')) {
				if(ret[1] == 'x') {
					int val;
					sscanf(ret, "0x%x", &val);
					return Py_BuildValue("i", val);
				}
				else
					return Py_BuildValue("i", atoi(ret));
			}
			return Py_BuildValue("s", ret);
		}
	}
	
	return NULL;
}

static PyObject * GemRB_GetTableRowName(PyObject *self, PyObject *args)
{
	int ti, row;

	if(!PyArg_ParseTuple(args, "ii", &ti, &row)) {
		printMessage("GUIScript", "Syntax Error: GetTableRowName(TableIndex, RowIndex)\n", LIGHT_RED);
		return NULL;
	}
	
	TableMgr * tm = core->GetTable(ti);
	if(tm == NULL)
		return NULL;
	const char * str = tm->GetRowName(row);
	if(str == NULL)
		return NULL;

	return Py_BuildValue("s", str);
}

static PyObject * GemRB_GetTableRowCount(PyObject *self, PyObject *args)
{
	int ti;

	if(!PyArg_ParseTuple(args, "i", &ti)) {
		printMessage("GUIScript", "Syntax Error: Expected Integer\n", LIGHT_RED);
		return NULL;
	}
	
	TableMgr * tm = core->GetTable(ti);
	if(tm == NULL)
		return NULL;

	return Py_BuildValue("i", tm->GetRowCount());
}

static PyObject * GemRB_LoadSymbol(PyObject *self, PyObject *args)
{
	char *string;

	if(!PyArg_ParseTuple(args, "s", &string)) {
		printMessage("GUIScript", "Syntax Error: Expected String\n", LIGHT_RED);
		return NULL;
	}
	
	int ind = core->LoadSymbol(string);
	if(ind == -1)
		return NULL;

	return Py_BuildValue("i", ind);
}

static PyObject * GemRB_UnLoadSymbol(PyObject *self, PyObject *args)
{
	int si;

	if(!PyArg_ParseTuple(args, "i", &si)) {
		printMessage("GUIScript", "Syntax Error: Expected Integer\n", LIGHT_RED);
		return NULL;
	}
	
	int ind = core->DelSymbol(si);
	if(ind == -1)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_GetSymbol(PyObject *self, PyObject *args)
{
	char *string;

	if(!PyArg_ParseTuple(args, "s", &string)) {
		printMessage("GUIScript", "Syntax Error: Expected String\n", LIGHT_RED);
		return NULL;
	}

	int ind = core->GetSymbolIndex(string);
	if(ind == -1)
		return NULL;

	return Py_BuildValue("i", ind);
}

static PyObject * GemRB_GetSymbolValue(PyObject *self, PyObject *args)
{
	PyObject *si, *sym;

	if(PyArg_UnpackTuple(args, "ref", 2, 2, &si, &sym)) {
		if(!PyObject_TypeCheck(si, &PyInt_Type)) {
			printMessage("GUIScript", "Syntax Error: GetTableValue(Table, RowIndex/RowString, ColIndex/ColString)\n", LIGHT_RED);
			return NULL;
		}
		int SymbolIndex = PyInt_AsLong(si);
		if((PyObject_TypeCheck(sym, &PyInt_Type)) || (PyObject_TypeCheck(sym, &PyString_Type))) {
			if(PyObject_TypeCheck(sym, &PyString_Type)) {
				char * syms = PyString_AsString(sym);
				SymbolMgr * sm = core->GetSymbol(SymbolIndex);
				long val = sm->GetValue(syms);
				return Py_BuildValue("l", val);
			}
			else {
				int symi = PyInt_AsLong(sym);
				SymbolMgr * sm = core->GetSymbol(SymbolIndex);
				const char * str = sm->GetValue(symi);
				return Py_BuildValue("s", str);
			}	
		}
		printMessage("GUIScript", "Type Error: GetSymbolValue(SymbolTable, StringVal/IntVal)\n", LIGHT_RED);
		return NULL;
	}
	
	return NULL;
}

static PyObject * GemRB_GetControl(PyObject *self, PyObject *args)
{
	int WindowIndex, ControlID;

	if(!PyArg_ParseTuple(args, "ii", &WindowIndex, &ControlID)) {
		printMessage("GUIScript", "Syntax Error: GetControl(unsigned short WindowIndex, unsigned long ControlID)\n", LIGHT_RED);
		return NULL;
	}
	
	int ret = core->GetControl(WindowIndex, ControlID);
	if(ret == -1)
		return NULL;
	
	return Py_BuildValue("i", ret);
}

static PyObject * GemRB_SetText(PyObject *self, PyObject *args)
{
	PyObject *wi, *ci, *str;
	int WindowIndex, ControlIndex, StrRef;
	char * string;
	int ret;

	if(PyArg_UnpackTuple(args, "ref", 3, 3, &wi, &ci, &str)) {
		if(!PyObject_TypeCheck(wi, &PyInt_Type) || !PyObject_TypeCheck(ci, &PyInt_Type) || (!PyObject_TypeCheck(str, &PyString_Type) && !PyObject_TypeCheck(str, &PyInt_Type))) {
			printMessage("GUIScript", "Syntax Error: SetText(WindowIndex, ControlIndex, String|Strref, [row])\n", LIGHT_RED);
			return NULL;
		}

		WindowIndex = PyInt_AsLong(wi);
		ControlIndex = PyInt_AsLong(ci);
		if(PyObject_TypeCheck(str, &PyString_Type)) {
			string = PyString_AsString(str);
			if(string == NULL)
				return NULL;
			ret = core->SetText(WindowIndex, ControlIndex, string);
			if(ret == -1)
				return NULL;
		}
		else {
			StrRef = PyInt_AsLong(str);
			char * str = core->GetString(StrRef);
			ret = core->SetText(WindowIndex, ControlIndex, str);
			if(ret == -1) {
				free(str);
				return NULL;
			}
			free(str);
		}
	}
	else
		return NULL;
	
	return Py_BuildValue("i", ret);
}

static PyObject * GemRB_TextAreaAppend(PyObject *self, PyObject *args)
{
	PyObject *wi, *ci, *str, *row=NULL;
	int WindowIndex, ControlIndex, StrRef, Row;
	char * string;
	int ret;

	if(PyArg_UnpackTuple(args, "ref", 3, 4, &wi, &ci, &str, &row)) {
		if(!PyObject_TypeCheck(wi, &PyInt_Type) || !PyObject_TypeCheck(ci, &PyInt_Type) || (!PyObject_TypeCheck(str, &PyString_Type) && !PyObject_TypeCheck(str, &PyInt_Type))) {
			printMessage("GUIScript", "Syntax Error: TextAreaAppend(unsigned short WindowIndex, unsigned short ControlIndex, char * string)\n", LIGHT_RED);
			return NULL;
		}
		WindowIndex = PyInt_AsLong(wi);
		ControlIndex = PyInt_AsLong(ci);
		Window * win = core->GetWindow(WindowIndex);
		if(!win)
			return NULL;
		Control * ctrl = win->GetControl(ControlIndex);
		if(!ctrl)
			return NULL;
		if(ctrl->ControlType != 5)
			return NULL;
		TextArea * ta = (TextArea*)ctrl;
		if(row) {
			if(!PyObject_TypeCheck(row,&PyInt_Type) )
			{
				printMessage("GUIScript", "Syntax Error: SetText row must be integer\n", LIGHT_RED);
				return NULL;
			}
			Row = PyInt_AsLong(row);
			if(Row>ta->GetRowCount()-1)
				Row=-1;
		}
		else Row = ta->GetRowCount()-1;
		if(PyObject_TypeCheck(str, &PyString_Type)) {
			string = PyString_AsString(str);
			if(string == NULL)
				return NULL;
			ret = ta->AppendText(string, Row);
		}
		else {
			StrRef = PyInt_AsLong(str);
			char * str = core->GetString(StrRef);
			ret = ta->AppendText(str, Row);
			free(str);
		}
	}
	else
		return NULL;
	
	return Py_BuildValue("i", ret);
}

static PyObject * GemRB_SetVisible(PyObject *self, PyObject *args)
{
	int WindowIndex;
	int visible;

	if(!PyArg_ParseTuple(args, "ii", &WindowIndex, &visible)) {
		printMessage("GUIScript", "Syntax Error: SetVisible(unsigned short WindowIndex, int visible)\n", LIGHT_RED);
		return NULL;
	}
	
	int ret = core->SetVisible(WindowIndex, visible);
	if(ret == -1)
		return NULL;
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_ShowModal(PyObject *self, PyObject *args)
{
	int WindowIndex;

	if(!PyArg_ParseTuple(args, "i", &WindowIndex)) {
		printMessage("GUIScript", "Syntax Error: Expected a window index\n", LIGHT_RED);
		return NULL;
	}
	
	int ret = core->ShowModal(WindowIndex);
	if(ret == -1)
		return NULL;
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SetEvent(PyObject *self, PyObject *args)
{
	int WindowIndex, ControlIndex;
	int event;
	char *funcName;

	if(!PyArg_ParseTuple(args, "iiis", &WindowIndex, &ControlIndex, &event, &funcName)) {
		printMessage("GUIScript", "Syntax Error: SetEvent(WindowIndex, ControlIndex, EventMask, FunctionName)\n", LIGHT_RED);
		return NULL;
	}

	int ret = core->SetEvent(WindowIndex, ControlIndex, event, funcName);
	if(ret == -1)
		return NULL;
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SetNextScript(PyObject *self, PyObject *args)
{
	char *funcName;

	if(!PyArg_ParseTuple(args, "s", &funcName)) {
		printMessage("GUIScript", "Syntax Error: Expected a script name\n", LIGHT_RED);
		return NULL;
	}

	strcpy(core->NextScript, funcName);
	core->ChangeScript = true;
		
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SetControlStatus(PyObject *self, PyObject *args)
{
	int WindowIndex, ControlIndex;
	int status;

	if(!PyArg_ParseTuple(args, "iii", &WindowIndex, &ControlIndex, &status)) {
		printMessage("GUIScript", "Syntax Error: SetControlStatus(WindowIndex, ControlIndex, status)\n", LIGHT_RED);
		return NULL;
	}

	
	int ret = core->SetControlStatus(WindowIndex, ControlIndex, status);
	if(ret == -1)
		return NULL;
		
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SetVarAssoc(PyObject *self, PyObject *args)
{
	int WindowIndex, ControlIndex;
	unsigned long Value;
	char* VarName;

	if(!PyArg_ParseTuple(args, "iisl", &WindowIndex, &ControlIndex, &VarName, &Value)) {
		printMessage("GUIScript", "Syntax Error: SetVarAssoc(WindowIndex, ControlIndex, VariableName, LongValue)\n", LIGHT_RED);
		return NULL;
	}

	Window * win = core->GetWindow(WindowIndex);
	if(win == NULL)
		return NULL;

	Control * ctrl = win->GetControl(ControlIndex);
	if(ctrl == NULL)
		return NULL;

	strncpy(ctrl->VarName, VarName, MAX_VARIABLE_LENGTH);
	ctrl->Value=Value;
	/** setting the correct state for this control */
	/** it is possible to set up a default value, if Lookup returns false, use it */
	Value=0;
	core->GetDictionary()->Lookup(VarName, Value);
	win->RedrawControls(VarName, Value);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_UnloadWindow(PyObject *self, PyObject *args)
{
	int WindowIndex;

	if(!PyArg_ParseTuple(args, "i", &WindowIndex)) {
		printMessage("GUIScript", "Syntax Error: UnloadWindow(WindowIndex)\n", LIGHT_RED);
		return NULL;
	}
	
	int ret = core->DelWindow(WindowIndex);
	if(ret == -1)
		return NULL;
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_CreateLabel(PyObject *self, PyObject *args)
{
	int WindowIndex, ControlID, x, y, w, h, align;
	char *font, *text;

	if(!PyArg_ParseTuple(args, "iiiiiissi", &WindowIndex, &ControlID, &x, &y, &w, &h, &font, &text, &align)) {
		printMessage("GUIScript", "Syntax Error: CreateLabel(WindowIndex, ControlIndex, x, y, w, h, font, text, align)\n", LIGHT_RED);
		return NULL;
	}
	
	Window * win = core->GetWindow(WindowIndex);
	if(win == NULL)
		return NULL;
	Label * lbl = new Label(4096, core->GetFont(font));
	lbl->XPos = x;
	lbl->YPos = y;
	lbl->Width = w;
	lbl->Height = h;
	lbl->ControlID = ControlID;
	lbl->ControlType = 6;
	lbl->Owner = win;
	lbl->SetText(text);
	lbl->SetAlignment(align);
	win->AddControl(lbl);
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SetButtonFlags(PyObject *self, PyObject *args)
{
	int WindowIndex, ControlIndex, Flags, Operation;

	if(!PyArg_ParseTuple(args, "iiii", &WindowIndex, &ControlIndex, &Flags, &Operation)) {
		printMessage("GUIScript", "Syntax Error: SetButtonFlags(WindowIndex, ControlIndex, Flags, Operation)\n", LIGHT_RED);
		return NULL;
	}
	if(Operation<0 || Operation>2)
	{
		printMessage("GUIScript","Syntax Error: SetButtonFlags operation must be 0-2\n", LIGHT_RED);
		return NULL;
	}

	Window * win = core->GetWindow(WindowIndex);
	if(win == NULL)
		return NULL;

	Control * ctrl = win->GetControl(ControlIndex);
	if(ctrl == NULL)
		return NULL;

	if(ctrl->ControlType != 0)
		return NULL;

	Button * btn = (Button*)ctrl;
	btn->SetFlags(Flags, Operation);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SetButtonState(PyObject *self, PyObject *args)
{
	int WindowIndex, ControlIndex, state;

	if(!PyArg_ParseTuple(args, "iii", &WindowIndex, &ControlIndex, &state)) {
		printMessage("GUIScript", "Syntax Error: SetButtonState(WindowIndex, ControlIndex, State)\n", LIGHT_RED);
		return NULL;
	}

	Window * win = core->GetWindow(WindowIndex);
	if(win == NULL)
		return NULL;

	Control * ctrl = win->GetControl(ControlIndex);
	if(ctrl == NULL)
		return NULL;

	if(ctrl->ControlType != 0)
		return NULL;

	Button * btn = (Button*)ctrl;
	btn->SetState(state);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SetButtonPicture(PyObject *self, PyObject *args)
{
	int WindowIndex, ControlIndex;
	char * ResRef;

	if(!PyArg_ParseTuple(args, "iis", &WindowIndex, &ControlIndex, &ResRef)) {
		printMessage("GUIScript", "Syntax Error: SetButtonPicture(WindowIndex, ControlIndex, PictureResRef)\n", LIGHT_RED);
		return NULL;
	}

	Window * win = core->GetWindow(WindowIndex);
	if(win == NULL)
		return NULL;

	Control * ctrl = win->GetControl(ControlIndex);
	if(ctrl == NULL)
		return NULL;

	if(ctrl->ControlType != 0)
		return NULL;

	DataStream * str = core->GetResourceMgr()->GetResource(ResRef, IE_BMP_CLASS_ID);
	if(str == NULL)
		return NULL;
	ImageMgr * im = (ImageMgr*)core->GetInterface(IE_BMP_CLASS_ID);
	if(im == NULL) {
		delete (str);
		return NULL;
	}

	if(!im->Open(str, true)) {
		delete(str);
		core->FreeInterface(im);
		return NULL;
	}

	Sprite2D * Picture = im->GetImage();
	if(Picture == NULL) {
		delete(str);
		core->FreeInterface(im);
		return NULL;
	}

	Button * btn = (Button*)ctrl;
	btn->SetPicture(Picture);

	core->FreeInterface(im);

	Py_INCREF(Py_None);
	return Py_None;
}
static PyObject * GemRB_PlaySound(PyObject *self, PyObject *args)
{
	char* ResRef;

	if(!PyArg_ParseTuple(args, "s", &ResRef)) {
		printMessage("GUIScript", "Syntax Error: PlaySound(SoundResource)\n", LIGHT_RED);
		return NULL;
	}
	
	int ret = core->GetSoundMgr()->Play(ResRef);
	if(!ret)
		return NULL;
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_Quit(PyObject *self, PyObject *args)
{
	bool ret = core->Quit();
	if(!ret)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_LoadMusicPL(PyObject *self, PyObject *args)
{
	char* ResRef;

	if(!PyArg_ParseTuple(args, "s", &ResRef)) {
		printMessage("GUIScript", "Syntax Error: PlayMusicPL(MusicPlayListResource)\n", LIGHT_RED);
		return NULL;
	}
	
	bool ret = core->GetMusicMgr()->OpenPlaylist(ResRef);
	if(!ret)
		return NULL;

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_StartPL(PyObject *self, PyObject *args)
{	
	core->GetMusicMgr()->Start();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SoftEndPL(PyObject *self, PyObject *args)
{	
	core->GetMusicMgr()->End();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_HardEndPL(PyObject *self, PyObject *args)
{	
	core->GetMusicMgr()->HardEnd();

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SetToken(PyObject *self, PyObject *args)
{	
	char* Variable;
	char* value;

	if(!PyArg_ParseTuple(args, "ss", &Variable, &value)) {
		printMessage("GUIScript", "Syntax Error: SetVar(VariableName, Value)\n", LIGHT_RED);
		return NULL;
	}

	char *newvalue = (char *) malloc(strlen(value)+1);  //duplicating the string
	strcpy(newvalue, value);
	core->GetTokenDictionary()->SetAt(Variable, newvalue);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_SetVar(PyObject *self, PyObject *args)
{	
	char* Variable;
	unsigned long value;

	if(!PyArg_ParseTuple(args, "sl", &Variable, &value)) {
		printMessage("GUIScript", "Syntax Error: SetVar(VariableName, Value)\n", LIGHT_RED);
		return NULL;
	}

	core->GetDictionary()->SetAt(Variable, value);

	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_GetToken(PyObject *self, PyObject *args)
{	
	char* Variable;
	char* value;

	if(!PyArg_ParseTuple(args, "s", &Variable)) {
		printMessage("GUIScript", "Syntax Error: GetToken(VariableName)\n", LIGHT_RED);
		return NULL;
	}

//trying to cheat the pointer out from the Dictionary without allocating it
//BuildValue will allocate its own area anyway
	if(!core->GetTokenDictionary()->Lookup(Variable, (unsigned long &) value))
		return Py_BuildValue("s", "");

	return Py_BuildValue("s", value);
}

static PyObject * GemRB_GetVar(PyObject *self, PyObject *args)
{	
	char* Variable;
	unsigned long value;

	if(!PyArg_ParseTuple(args, "s", &Variable)) {
		printMessage("GUIScript", "Syntax Error: GetVar(VariableName)\n", LIGHT_RED);
		return NULL;
	}

	if(!core->GetDictionary()->Lookup(Variable, value))
		return Py_BuildValue("l", (unsigned long) 0);

	return Py_BuildValue("l", value);
}

static PyMethodDef GemRBMethods[] = {
    {"LoadWindowPack", GemRB_LoadWindowPack, METH_VARARGS,
     "Loads a WindowPack into the Window Manager Module."},

	{"LoadWindow", GemRB_LoadWindow, METH_VARARGS,
     "Returns a Window."},

 	{"LoadTable", GemRB_LoadTable, METH_VARARGS,
     "Loads a 2DA Table."},

	{"UnLoadTable", GemRB_UnLoadTable, METH_VARARGS,
     "UnLoads a 2DA Table."},

	{"GetTable", GemRB_GetTable, METH_VARARGS,
     "Returns a Loaded 2DA Table."},

	{"GetTableValue", GemRB_GetTableValue, METH_VARARGS,
     "Returns a field of a 2DA Table."},

	{"GetTableRowName", GemRB_GetTableRowName, METH_VARARGS,
     "Returns the Name of a Row in a 2DA Table."},

 	{"GetTableRowCount", GemRB_GetTableRowCount, METH_VARARGS,
     "Returns the number of rows in a 2DA Table."},

  	{"LoadSymbol", GemRB_LoadSymbol, METH_VARARGS,
     "Loads a IDS Symbol Table."},

	{"UnLoadSymbol", GemRB_UnLoadSymbol, METH_VARARGS,
     "UnLoads a IDS Symbol Table."},

	{"GetSymbol", GemRB_GetSymbol, METH_VARARGS,
     "Returns a Loaded IDS Symbol Table."},

	{"GetSymbolValue", GemRB_GetSymbolValue, METH_VARARGS,
     "Returns a field of a IDS Symbol Table."},

	{"GetControl", GemRB_GetControl, METH_VARARGS,
     "Returns a control in a Window."},

	{"SetText", GemRB_SetText, METH_VARARGS,
     "Sets the Text of a control in a Window."},

	{"TextAreaAppend", GemRB_TextAreaAppend, METH_VARARGS,
     "Appends the Text to the TextArea Control in the Window."},

	{"SetVisible", GemRB_SetVisible, METH_VARARGS,
     "Sets the Visibility Flag of a Window."},

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

	{"SetButtonFlags", GemRB_SetButtonFlags, METH_VARARGS,
     "Sets the Display Flags of a Button."},

	{"SetButtonState", GemRB_SetButtonState, METH_VARARGS,
     "Sets the state of a Button Control."},

	{"SetButtonPicture", GemRB_SetButtonPicture, METH_VARARGS,
     "Sets the Picture of a Button Control."},

	{"PlaySound", GemRB_PlaySound, METH_VARARGS,
     "Plays a Sound."},

	{"LoadMusicPL", GemRB_LoadMusicPL, METH_VARARGS,
     "Loads a Music PlayList."},

	{"StartPL", GemRB_StartPL, METH_NOARGS,
     "Starts a Music Playlist."},

	{"SoftEndPL", GemRB_SoftEndPL, METH_NOARGS,
     "Ends a Music Playlist softly."},

 	{"HardEndPL", GemRB_HardEndPL, METH_NOARGS,
     "Ends a Music Playlist immediately."},

	{"Quit", GemRB_Quit, METH_NOARGS,
     "Quits GemRB."},

 	{"GetVar", GemRB_GetVar, METH_VARARGS,
     "Get a Variable value from the Global Dictionary."},

 	{"SetVar", GemRB_SetVar, METH_VARARGS,
     "Set/Create a Variable in the Global Dictionary."},

 	{"GetToken", GemRB_GetToken, METH_VARARGS,
     "Get a Variable value from the Token Dictionary."},

 	{"SetToken", GemRB_SetToken, METH_VARARGS,
     "Set/Create a Variable in the Token Dictionary."},

    {NULL, NULL, 0, NULL}
};

void initGemRB() {
	PyObject * m = Py_InitModule("GemRB", GemRBMethods);
	
}

GUIScript::GUIScript(void)
{
	pDict = NULL;
	pModule = NULL;
	pName = NULL;
}

GUIScript::~GUIScript(void)
{
	if(Py_IsInitialized())
		Py_Finalize();
}

/** Initialization Routine */
bool GUIScript::Init(void)
{
	Py_SetProgramName("GemRB -- Python");
	Py_Initialize();
	if(!Py_IsInitialized())
		return false;
	pGemRB = Py_InitModule("GemRB", GemRBMethods);
	if(!pGemRB)
		return false;
	pGemRBDict = PyModule_GetDict(pGemRB);
	char string[256];
	if(PyRun_SimpleString("import sys")==-1) {
		PyRun_SimpleString("pdb.pm()");
		PyErr_Print();
		return false;
	}
	char path[_MAX_PATH];
	#ifdef WIN32
	int len = strlen(core->GemRBPath);
	int p = 0;
	for(int i = 0; i < len; i++) {
		if(core->GemRBPath[i] == '\\') {
			path[p] = '//';
		}
		else
			path[p] = core->GUIScriptsPath[i];
		p++;
	}
	path[p] = 0;
	sprintf(string, "sys.path.append('%sGUIScripts')", path);
	#else
	sprintf(string, "sys.path.append('%sGUIScripts')", core->GUIScriptsPath);
	#endif
	if(PyRun_SimpleString(string)==-1){
		PyRun_SimpleString("pdb.pm()");
		PyErr_Print();
		return false;
	}
	if(PyRun_SimpleString("import GemRB")==-1){
		PyRun_SimpleString("pdb.pm()");
		PyErr_Print();
		return false;
	}
	strcpy(path, core->GUIScriptsPath);
	strcat(path, "GUIScripts");
	strcat(path, SPathDelimiter);
	strcat(path, "GUIDefines.py");
	FILE * config = fopen(path, "rb");
	if(!config)
		return false;
	while(true) {
		int ret = fscanf(config, "%[^\r\n]\r\n", path);
		if(ret != 1)
			break;
		int obj = PyRun_SimpleString(path);
		if(obj == -1) {
			PyErr_Print();
			return false;
		}
	}
	fclose(config);
	PyObject * mainmod = PyImport_AddModule("__main__");
	maindic = PyModule_GetDict(mainmod);
	return true;
}

bool GUIScript::LoadScript(const char * filename)
{  
	if(!Py_IsInitialized())
		return false;
	printMessage("GUIScript", "Loading Script ", WHITE);
	printf("%s...", filename);
	
	char path[_MAX_PATH];
	strcpy(path, filename);

    pName = PyString_FromString(filename);
    /* Error checking of pName left out */
	if(pName == NULL) {
		printStatus("ERROR", LIGHT_RED);
		return false;
	}

    pModule = PyImport_Import(pName);
    Py_DECREF(pName);
    
    if (pModule != NULL) {
			pDict = PyModule_GetDict(pModule);
		if(PyDict_Merge(pDict, maindic, false) == -1)
			return false;
        /* pDict is a borrowed reference */
    }
	else {
		PyErr_Print();
		printStatus("ERROR", LIGHT_RED);
		return false;
	}
    printStatus("OK", LIGHT_GREEN);
    return true;
}

bool GUIScript::RunFunction(const char * fname)
{
	if(!Py_IsInitialized())
		return false;
	if(pDict == NULL)
		return false;

	PyObject *pFunc, *pArgs, *pValue;


	pFunc = PyDict_GetItemString(pDict, fname);
    /* pFun: Borrowed reference */
	if((!pFunc) || (!PyCallable_Check(pFunc)))
		return false;
	pArgs = NULL;
	pValue = PyObject_CallObject(pFunc, pArgs);
	if(pValue == NULL) {
		PyErr_Print();
		return false;
	}
	Py_DECREF(pValue);
	return true;
}

/** Exec a single String */
char * GUIScript::ExecString(const char * string)
{
	if(PyRun_SimpleString(string)==-1) {
		PyErr_Print();
	}
	return NULL;
}
