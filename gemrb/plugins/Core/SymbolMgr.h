#ifndef SYMBOLMGR_H
#define SYMBOLMGR_H

#define SYMBOL_VALUE_NOT_LOCATED -65535 // GetValue returns this if text is not found in arrays

#include "Plugin.h"
#include "DataStream.h"

#ifdef WIN32

#ifdef GEM_BUILD_DLL
#define GEM_EXPORT __declspec(dllexport)
#else
#define GEM_EXPORT __declspec(dllimport)
#endif

#else
#define GEM_EXPORT
#endif

class GEM_EXPORT SymbolMgr : public Plugin {
public:
	SymbolMgr(void);
	virtual ~SymbolMgr(void);
	virtual bool Open(DataStream* stream, bool autoFree = true) = 0;
	virtual int GetValue(const char* text) = 0;
	virtual char* GetValue(int val) = 0;
	virtual char* GetStringIndex(unsigned int Index) = 0;
	virtual int GetValueIndex(unsigned int Index) = 0;
	virtual int FindValue(int val) = 0;
	virtual int FindString(char *str, int len) = 0;
};

#endif
