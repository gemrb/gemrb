#ifndef IMAGEMGR_H
#define IMAGEMGR_H

#include "Plugin.h"
#include "DataStream.h"
#include "Sprite2D.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT ImageMgr : public Plugin
{
public:
	ImageMgr(void);
	virtual ~ImageMgr(void);
	virtual bool Open(DataStream * stream, bool autoFree = false) = 0;
	virtual Sprite2D * GetImage() = 0;
  /** No descriptions */
  virtual void GetPalette(int index, int colors, Color * pal) = 0;
};

#endif
