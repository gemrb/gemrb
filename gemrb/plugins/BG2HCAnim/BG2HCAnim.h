#ifndef BG2HCANIM_H
#define BG2HCANIM_H
#include "../Core/HCAnimationSeq.h"

class BG2HCAnim : public HCAnimationSeq
{
public:
	BG2HCAnim(void);
	~BG2HCAnim(void);
	void GetCharAnimations(Actor * actor);
private:
	void LoadStaticAnims(const char * ResRef, Actor * actor);
	void LoadFullAnimLow(const char * ResRef, Actor * actor);
	void LoadMonsterHiRes(const char * ResRef, Actor * actor);
	void ParseStaticAnims(Actor* actor);
	void ParseSleeping(Actor* actor);
	void ParseFullAnimLow(Actor* actor);

	void LoadMonsterMidRes(const char * ResRef, Actor * actor);
	void LoadCritter(const char * ResRef, Actor * actor, bool bow);
public:
	void release(void)
	{
		delete this;
	}
};

#endif
