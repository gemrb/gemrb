#include "../../includes/win32def.h"
#include "CharAnimations.h"
#include "Interface.h"

extern Interface * core;

CharAnimations::CharAnimations(char * BaseResRef, unsigned char OrientCount, unsigned char MirrorType)
{
	int len = strlen(BaseResRef);
	ResRef = (char*)malloc(len+1);
	memcpy(ResRef, BaseResRef, len+1);
	this->OrientCount = OrientCount;
	this->MirrorType = MirrorType;
	LoadedFlag = 0;
}

CharAnimations::~CharAnimations(void)
{
	free(ResRef);
}
/*This is a simple Idea of how the animation are coded

If the Orientation Count is 9 (i.e. BG2/IWD2 Avatar animations)
the 1-7 frames are mirrored to create the 9-14 frames.

There are 4 Mirroring modes:

IE_ANI_CODE_MIRROR: The code automatically mirrors the needed frames 
(as in the example above)

IE_ANI_ONE_FILE: The whole animation is in one file, no mirroring needed.

IE_ANI_TWO_FILES: The whole animation is in 2 files. The East and West part are in 2 BAM Files.

IE_ANI_FOUR_FILES: The Animation is coded in Four Files. Probably it is an old Two File animation with
additional frames added in a second time.


 EAST PART       |       WEST PART
                 |
        NE  NNE  N  NNW  NW
     NE 006 007 008 009 010 NW
    ENE 005      |      011 WNW
      E 004     xxx     012 W
    ESE 003      |      013 WSW
     SE 002 001 000 015 014 SW
	    SE  SSE  S  SSW  SW
                 |
                 |

*/
Animation * CharAnimations::GetAnimation(unsigned char AnimID, unsigned char Orient)
{
	//TODO: Implement Auto Resource Loading
	if(Anims[AnimID][Orient]) {
		return Anims[AnimID][Orient];
	}
	char ResRef[9];
	unsigned char Cycle;
	GetAnimResRef(AnimID, Orient, ResRef, Cycle);
	DataStream * stream = core->GetResourceMgr()->GetResource(ResRef, IE_BAM_CLASS_ID);
	AnimationMgr * anim = core->GetInterface(IE_BAM_CLASS_ID);
	anim->Open(stream, true);
	Animation * a = anim->GetAnimation(Cycle, 0, 0, IE_NORMAL);
	switch(OrientCount) {
		case 5: 
		{
			if((Orient > 8) && (MirrorType == IE_ANI_CODE_MIRROR)) {
				core->GetVideoDriver()->MirrorAnimation(a);
			}
			if(Orient & 1)
				Orient--;
			Anims[AnimID][Orient] = a;
			Anims[AnimID][Orient+1] = a;
		}
		break;

		case 9:
		{
			if((Orient > 8) && (MirrorType == IE_ANI_CODE_MIRROR)) {
				core->GetVideoDriver()->MirrorAnimation(a);
			}
			Anims[AnimID][Orient] = a;
		}
		break;
	}
	return Anims[AnimID][Orient];
}
