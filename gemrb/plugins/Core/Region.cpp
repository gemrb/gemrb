#include "../../includes/win32def.h"
#include "Region.h"

Region::Region(void)
{
	x = y = w = h = 0;
DEBUG=0;
}

Region::~Region(void)
{
}

Region::Region(Region & rgn)
{
	x = rgn.x;
	y = rgn.y;
	w = rgn.w;
	h = rgn.h;
DEBUG=rgn.DEBUG;
}

Region::Region(const Region & rgn)
{
  x = rgn.x;
  y = rgn.y;
  w = rgn.w;
  h = rgn.h;
DEBUG=rgn.DEBUG;
}

Region & Region::operator=(Region & rgn)
{
	x = rgn.x;
	y = rgn.y;
	w = rgn.w;
	h = rgn.h;
DEBUG=rgn.DEBUG;
	return *this;
}

bool Region::operator==(Region & rgn)
{
	if((x == rgn.x) && (y == rgn.y) && (w == rgn.w) && (h == rgn.h))
		return true;
	return false;
}

bool Region::operator!=(Region & rgn)
{
	if((x != rgn.x) || (y != rgn.y) || (w != rgn.w) || (h != rgn.h))
		return true;
	return false;
}

Region::Region(int x, int y, int w, int h, int debug)
{
	this->x = x;
	this->y = y;
	this->w = w;
	this->h = h;
this->DEBUG=DEBUG;
}
