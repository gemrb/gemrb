#include "../../includes/win32def.h"
#include "CharAnimations.h"

CharAnimations::CharAnimations(char * BaseResRef, unsigned long AniType)
{
	int len = strlen(BaseResRef);
	ResRef = (char*)malloc(len+1);
	memcpy(ResRef, BaseResRef, len+1);
	SupportedAnims = AniType;
	LoadedFlag = 0;
}

CharAnimations::~CharAnimations(void)
{
	free(ResRef);
}

Animation * CharAnimations::GetAnimation(unsigned char AnimID, unsigned char Orient)
{
	//TODO: Implement Auto Resource Loading
	return NULL;
}
