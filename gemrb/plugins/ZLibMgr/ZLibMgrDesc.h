#ifndef ZLIBMGRDESC_H
#define ZLIBMGRDESC_H

#include "../Core/ClassDesc.h"

class ZLibMgrDesc :
	public ClassDesc
{
public:
	ZLibMgrDesc(void);
	~ZLibMgrDesc(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
};
static ZLibMgrDesc ZLibCD;

#endif
