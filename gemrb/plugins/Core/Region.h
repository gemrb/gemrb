#ifndef REGION_H
#define REGION_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Region
{
public:
	Region(void);
	~Region(void);
	int x;
	int y;
	int w;
	int h;
	Region(const Region & rgn);
	Region(Region & rgn);
	Region & operator=(Region & rgn);
	bool operator==(Region & rgn);
	bool operator!=(Region & rgn);
	Region(int x, int y, int w, int h);
};

#endif
