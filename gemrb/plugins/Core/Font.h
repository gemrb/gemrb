#ifndef FONT_H
#define FONT_H

#include "../../includes/globals.h"
#include <vector>

typedef struct StringList {
	Sprite2D *** strings;
	unsigned int  *  heights;
	int StringCount;
	int starty;
} StringList;

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Font
{
private:
	std::vector<Sprite2D*> chars;
	int maxHeight;
public:
	Font(void);
	~Font(void);
	void AddChar(Sprite2D * spr);
	void Print(Region rgn, unsigned char * string, Color *color, unsigned char Alignment, bool anchor = false);
	char ResRef[9];
private: // Private methods
  /** Calculate the Maximum Height of a Text Line */
  int PreCalcLineHeight(Region &rgn, unsigned char * string);
  /** PreCalculate for Printing */
  StringList Prepare(Region &rgn, unsigned char * string);
};

#endif
