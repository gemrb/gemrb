#ifndef GUISCRIPTCD_H
#define GUISCRIPTCD_H

#include "../Core/ClassDesc.h"

class GUIScriptCD : public ClassDesc
{
public:
	GUIScriptCD(void);
	~GUIScriptCD(void);
	void * Create(void);
	const char * ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
};
static GUIScriptCD GuiScriptCD;

#endif
