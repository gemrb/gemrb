#ifndef VIDEOMODE_H
#define VIDEOMODE_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT VideoMode
{
public:
	VideoMode(void);
	~VideoMode(void);
	VideoMode(const VideoMode & vm);
	VideoMode(int w, int h, int bpp, bool fs);

private:
	int Width;
	int Height;
	int bpp;
	bool fullscreen;
public:
	void SetWidth(int w);
	int GetWidth(void) const;
	void SetHeight(int h);
	int GetHeight(void) const;
	void SetBPP(int b);
	int GetBPP(void) const ;
	void SetFullScreen(bool fs);
	bool GetFullScreen(void) const;
	bool operator==(const VideoMode & cpt) const;
	VideoMode & operator=(const VideoMode & vm);
};

#endif
