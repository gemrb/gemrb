#ifndef SDLVIDEODC_H
#define SDLVIDEODC_H

#include "../../includes/globals.h"
#include "../Core/ClassDesc.h"

class SDLVideoDC : public ClassDesc
{
public:
	SDLVideoDC(void);
	~SDLVideoDC(void);
	void * Create(void);
	const char * ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
};
static SDLVideoDC SDLVideoCD;

#endif
