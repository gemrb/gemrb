#include "GUIScriptCD.h"
#include "GUIScript.h"

GUIScriptCD::GUIScriptCD(void)
{
}

GUIScriptCD::~GUIScriptCD(void)
{
}

void * GUIScriptCD::Create(void)
{
	return new GUIScript();
}

const char * GUIScriptCD::ClassName(void)
{
	return "GUIScript";
}

SClass_ID GUIScriptCD::SuperClassID(void)
{
	return IE_GUI_SCRIPT_CLASS_ID;
}

Class_ID GUIScriptCD::ClassID(void)
{
	return Class_ID(0x715adf34, 0x110641ad);
}
