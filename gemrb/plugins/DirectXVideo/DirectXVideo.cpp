// SDLVideo.cpp : Defines the entry point for the DLL application.
//

#include "DirectXVideoDC.h"
#include "../../includes/globals.h"
#ifndef WIN32
#include <dlfcn.h>
#endif

#ifdef WIN32
#define GEM_EXPORT_DLL __declspec(dllexport)
#else
#define GEM_EXPORT_DLL
#endif

#ifdef WIN32
#include <windows.h>

extern HINSTANCE hInst;

BOOL APIENTRY DllMain( HANDLE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
	hInst = (HINSTANCE)hModule;
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
			return &DirectXVideoCD;
		default: 
			return 0;
	}
}

GEM_EXPORT_DLL const char *LibDescription() {
	return "DirectX Video Driver";
}

GEM_EXPORT_DLL unsigned long LibVersion() 
{ 
	return VERSION_GEMRB; 
}
