#include "win32def.h"
#include "VideoMode.h"

VideoMode::VideoMode(void)
: Width(0)
, Height(0)
, bpp(0)
, fullscreen(false)
{
}

VideoMode::~VideoMode(void)
{
}

VideoMode::VideoMode(const VideoMode & vm)
{
	Width = vm.GetWidth();
	Height = vm.GetHeight();
	bpp = vm.GetBPP();
	fullscreen = vm.GetFullScreen();
}

VideoMode::VideoMode(int w, int h, int bpp, bool fs)
{
	Width = w;
	Height = h;
	this->bpp = bpp;
	fullscreen = fs;
}

void VideoMode::SetWidth(int w)
{
	Width = w;
}

int VideoMode::GetWidth(void) const
{
	return Width;
}

void VideoMode::SetHeight(int h)
{
	Height = h;
}

int VideoMode::GetHeight(void) const
{
	return Height;
}

void VideoMode::SetBPP(int b)
{
	bpp = b;
}

int VideoMode::GetBPP(void) const
{
	return bpp;
}

void VideoMode::SetFullScreen(bool fs)
{
	fullscreen = fs;
}

bool VideoMode::GetFullScreen(void) const
{
	return fullscreen;
}

bool VideoMode::operator==(const VideoMode & cpt) const
{
	if((Width == cpt.GetWidth()) && (Height == cpt.GetHeight()) && (bpp == cpt.GetBPP()) && (fullscreen == cpt.GetFullScreen()))
		return true;
	return false;
}

VideoMode & VideoMode::operator=(const VideoMode & vm)
{
	Width = vm.GetWidth();
	Height = vm.GetHeight();
	bpp = vm.GetBPP();
	fullscreen = vm.GetFullScreen();
	return *this;
}
