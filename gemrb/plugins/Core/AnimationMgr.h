#ifndef ANIMATIONMGR_H
#define ANIMATIONMGR_H

#include "Plugin.h"
#include "../../includes/globals.h"
#include "Animation.h"
#include "AnimationFactory.h"
#include "Font.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT AnimationMgr : public Plugin
{
public:
	AnimationMgr(void);
	virtual ~AnimationMgr(void);
	virtual bool Open(DataStream * stream, bool autoFree = false) = 0;
	virtual Sprite2D * GetFrameFromCycle(unsigned char Cycle, unsigned short frame) = 0;
	virtual Animation * GetAnimation(unsigned char Cycle, int x, int y, unsigned char mode = IE_NORMAL) = 0;
	virtual AnimationFactory * GetAnimationFactory(const char * ResRef, unsigned char mode = IE_NORMAL) = 0;
	virtual Sprite2D * GetFrame(unsigned short findex, unsigned char mode = IE_NORMAL) = 0;
  /** This function will load the Animation as a Font */
  virtual Font * GetFont() = 0;
  /** Debug Function: Returns the Global Animation Palette as a Sprite2D Object.
If the Global Animation Palette is NULL, returns NULL. */
  virtual Sprite2D * GetPalette() = 0;
};

#endif
