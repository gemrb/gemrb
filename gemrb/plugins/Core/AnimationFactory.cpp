#include "../../includes/win32def.h"
#include "AnimationFactory.h"

AnimationFactory::AnimationFactory(const char * ResRef) : FactoryObject(ResRef, IE_BAM_CLASS_ID)
{
	FLTable = NULL;
}

AnimationFactory::~AnimationFactory(void)
{
	if(FLTable)
		free(FLTable);
}

void AnimationFactory::AddFrame(Sprite2D * frame, unsigned short index)
{
	frames.push_back(frame);
	links.push_back(index);
}

void AnimationFactory::AddCycle(CycleEntry cycle)
{
	cycles.push_back(cycle);
}

void AnimationFactory::LoadFLT(unsigned short * buffer, int count)
{
	if(FLTable)
		free(FLTable);
	FLTable = (unsigned short*)malloc(count*sizeof(unsigned short));
	memcpy(FLTable, buffer, count*sizeof(unsigned short));
}

Animation * AnimationFactory::GetCycle(unsigned char cycle)
{
	if(cycle > cycles.size())
		return NULL;
	int ff = cycles[cycle].FirstFrame, lf = ff + cycles[cycle].FramesCount;
	Animation * anim = new Animation(&FLTable[ff], cycles[cycle].FramesCount);
	for(int i = ff; i < lf; i++) {
		for(unsigned int l = 0; l < frames.size(); l++) {
			if(links[l] == FLTable[i]) {
				anim->AddFrame(frames[l], FLTable[i]);
				break;
			}
		}
	}
	anim->x = 0;
	anim->y = 0;
	return anim;
}
/** No descriptions */
Sprite2D * AnimationFactory::GetFrame(unsigned short index)
{
	if(index >= frames.size())
		return NULL;
	return frames[index];
}
