#ifndef INTERFACEDESC_H
#define INTERFACEDESC_H

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT InterfaceDesc
{
public:
	InterfaceDesc(void);
	~InterfaceDesc(void);
};

#endif
