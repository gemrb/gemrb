#ifndef SPRITE2D_H
#define SPRITE2D_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Sprite2D
{
public:
	void * vptr;		//Pointer to the Driver Video Structure
	void * pixels;
	int XPos, YPos, Width, Height;
	Sprite2D(void);
	~Sprite2D(void);
	Sprite2D & operator=(Sprite2D & p);
	Sprite2D(Sprite2D & p);
};

#endif
