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
	int WindowIndex, ControlIndex, StrRef;
	char * string;
	int ret;

	if(!PyArg_ParseTuple(args, "iis", &WindowIndex, &ControlIndex, &string)) {
		if(!PyArg_ParseTuple(args, "iii", &WindowIndex, &ControlIndex, &StrRef)) {
			printMessage("GUIScript", "Syntax Error: SetText(unsigned short WindowIndex, unsigned short ControlIndex, char * string)\n", LIGHT_RED);
			return NULL;
		}
		char * str = core->GetString(StrRef);
		ret = core->SetText(WindowIndex, ControlIndex, str);
		if(ret == -1) {
			free(str);
			return NULL;
		}
		free(str);

	}
	else {
		ret = core->SetText(WindowIndex, ControlIndex, string);
		if(ret == -1)
			return NULL;
	}
	
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
		printMessage("GUIScript", "Syntax Error: SetVisible(unsigned short WindowIndex, int visible)\n", LIGHT_RED);
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
		printMessage("GUIScript", "Syntax Error: SetVisible(unsigned short WindowIndex, int visible)\n", LIGHT_RED);
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
		printMessage("GUIScript", "Syntax Error: SetVisible(unsigned short WindowIndex, int visible)\n", LIGHT_RED);
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
		printMessage("GUIScript", "Syntax Error: SetVisible(unsigned short WindowIndex, int visible)\n", LIGHT_RED);
		return NULL;
	}

	
	int ret = core->SetControlStatus(WindowIndex, ControlIndex, status);
	if(ret == -1)
		return NULL;
		
	Py_INCREF(Py_None);
	return Py_None;
}

static PyObject * GemRB_UnloadWindow(PyObject *self, PyObject *args)
{
	int WindowIndex;

	if(!PyArg_ParseTuple(args, "i", &WindowIndex)) {
		printMessage("GUIScript", "Syntax Error: SetVisible(unsigned short WindowIndex, int visible)\n", LIGHT_RED);
		return NULL;
	}
	
	int ret = core->DelWindow(WindowIndex);
	if(ret == -1)
		return NULL;
	
	Py_INCREF(Py_None);
	return Py_None;
}

static PyMethodDef GemRBMethods[] = {
    {"LoadWindowPack", GemRB_LoadWindowPack, METH_VARARGS,
     "Loads a WindowPack into the Window Manager Module."},

	{"LoadWindow", GemRB_LoadWindow, METH_VARARGS,
     "Returns a Window."},

	{"GetControl", GemRB_GetControl, METH_VARARGS,
     "Returns a control in a Window."},

	{"SetText", GemRB_SetText, METH_VARARGS,
     "Sets the Text of a control in a Window."},

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

	{"UnloadWindow", GemRB_UnloadWindow, METH_VARARGS,
     "Unloads a previously Loaded Window."},

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
	if(pDict != NULL)
		Py_DECREF(pDict);
	if(pModule != NULL)
		Py_DECREF(pModule);
	if(pName != NULL)
		Py_DECREF(pName);
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
	#ifdef WIN32
	char path[_MAX_PATH];
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
	if(PyRun_SimpleString("global IE_BUTTON_ON_PRESS")==-1) {
		PyErr_Print();
		return false;
	}
	if(PyRun_SimpleString("IE_BUTTON_ON_PRESS=0x00000000") == -1) {
		PyErr_Print();
		return false;
	}
	return true;
}

bool GUIScript::LoadScript(const char * filename)
{  
	if(!Py_IsInitialized())
		return false;
	//if(pDict != NULL)
	//	Py_DECREF(pDict);
	//if(pModule != NULL)
	//	Py_DECREF(pModule);
	//if(pName != NULL)
	//	Py_DECREF(pName);
	printMessage("GUIScript", "Loading Script ", WHITE);
	printf("%s...", filename);
	
	char path[_MAX_PATH];
	strcpy(path, filename);
	//strcat(path, filename);

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
