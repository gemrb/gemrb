#ifndef ANIMATION_H
#define ANIMATION_H

#include "Sprite2D.h"
#include "Region.h"
#include <vector>

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Animation
{
private:
	std::vector<unsigned short> indices;
	std::vector<Sprite2D*> frames;
	std::vector<int> link;
	unsigned int pos;
	unsigned long starttime;
public:
	bool free;
	int x,y;
	Region animArea;
	Animation(unsigned short * frames, int count);
	~Animation(void);
	void AddFrame(Sprite2D * frame, int index);
	Sprite2D * NextFrame(void);
	void release(void);
  /** Gets the i-th frame */
  Sprite2D * GetFrame(unsigned long i);
};

#endif
