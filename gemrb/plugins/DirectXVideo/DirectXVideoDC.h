#ifndef SDLVIDEODC_H
#define SDLVIDEODC_H

#include "../../includes/globals.h"
#include "../Core/ClassDesc.h"

class DirectXVideoDC : public ClassDesc
{
public:
	DirectXVideoDC(void);
	~DirectXVideoDC(void);
	void * Create(void);
	const char * ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
};
static DirectXVideoDC DirectXVideoCD;

#endif
