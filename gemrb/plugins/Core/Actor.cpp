#include "../../includes/win32def.h"
#include "Actor.h"

Actor::Actor(void)
{
	anims = new CharAnimations();
	LongName = NULL;
	ShortName = NULL;
}

Actor::~Actor(void)
{
	if(anims)
		delete(anims);
	if(LongName)
		free(LongName);
	if(ShortName)
		free(ShortName);
}
