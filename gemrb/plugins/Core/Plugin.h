#ifndef PLUGIN_H
#define PLUGIN_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT Plugin
{
public:
	Plugin(void);
	~Plugin(void);
	virtual void release(void) = 0;
};

#endif
