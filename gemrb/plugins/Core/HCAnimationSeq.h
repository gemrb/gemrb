#ifndef HCANIMATIONSEQ_H
#define HCANIMATIONSEQ_H

#include "Plugin.h"
#include "Actor.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT  HCAnimationSeq : public Plugin
{
public:
	HCAnimationSeq(void);
	virtual ~HCAnimationSeq(void);
	virtual void GetCharAnimations(Actor * actor) = 0;
};

#endif
