#ifndef ANIMATIONFACTORY_H
#define ANIMATIONFACTORY_H

#include "FactoryObject.h"
#include "../../includes/globals.h"
#include "Animation.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT AnimationFactory : public FactoryObject
{
private:
	std::vector<unsigned short> links;
	std::vector<Sprite2D*> frames;
	std::vector<CycleEntry> cycles;
	unsigned short * FLTable;	// Frame Lookup Table
public:
	AnimationFactory(const char * ResRef);
	~AnimationFactory(void);
	void AddFrame(Sprite2D * frame, unsigned short index);
	void AddCycle(CycleEntry cycle);
	void LoadFLT(unsigned short * buffer, int count);
	Animation * GetCycle(unsigned char cycle);
  /** No descriptions */
  Sprite2D * GetFrame(unsigned short index);
};

#endif
