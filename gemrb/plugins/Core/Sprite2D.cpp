#include "../../includes/win32def.h"
#include "Sprite2D.h"

Sprite2D::Sprite2D()
{
	this->XPos = this->YPos = 0;
}

Sprite2D::~Sprite2D(void)
{	
}

Sprite2D & Sprite2D::operator=(Sprite2D & p)
{
	vptr = p.vptr;
	pixels = p.pixels;
	XPos = p.XPos;
	YPos = p.YPos;
	return *this;
}

Sprite2D::Sprite2D(Sprite2D & p)
{
	vptr = p.vptr;
	pixels = p.pixels;
	XPos = p.XPos;
	YPos = p.YPos;
}
