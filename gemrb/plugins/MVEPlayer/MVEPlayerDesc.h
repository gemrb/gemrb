#ifndef MVEPLAYERDESC_H
#define MVEPLAYERDESC_H

#include "../../includes/globals.h"
#include "../Core/ClassDesc.h"

class MVEPlayerDesc : public ClassDesc
{
public:
	MVEPlayerDesc(void);
	~MVEPlayerDesc(void);
	void * Create(void);
	const char* ClassName(void);
	SClass_ID SuperClassID(void);
	Class_ID ClassID(void);
	const char* InternalName(void);
};
static MVEPlayerDesc MVEPlayCD;

#endif
