// 2DAImporter.cpp : Defines the entry point for the DLL application.
//
#include "../../includes/globals.h"
#include "2DAImpCD.h"

#ifdef WIN32
#define GEM_EXPORT_DLL __declspec(dllexport)
#else
#define GEM_EXPORT_DLL
#endif

#ifdef WIN32
#include <windows.h>

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
    return TRUE;
}

#endif

GEM_EXPORT_DLL int LibNumberClasses() 
{ 
	return 1; 
}

GEM_EXPORT_DLL ClassDesc *LibClassDesc(int i) {
	switch(i) {
		case 0: 
			return &p2DAImpCD;
		default: 
			return 0;
	}
}

GEM_EXPORT_DLL const char *LibDescription() {
	return "2DA File Importer";
}

GEM_EXPORT_DLL unsigned long LibVersion() 
{ 
	return VERSION_GEMRB; 
}
