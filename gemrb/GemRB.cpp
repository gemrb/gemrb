// GemRB.cpp : Defines the entry point for the application.
//

#define GEM_APP

#include "stdio.h"
#include "includes/win32def.h"
#include "plugins/Core/Interface.h"
#include "plugins/Core/Video.h"
#include "plugins/Core/ResourceMgr.h"
#include "plugins/Core/MapMgr.h"
#include "plugins/Core/ImageMgr.h"
#include "plugins/Core/AnimationMgr.h"
#include "plugins/Core/WindowMgr.h"

#ifndef WIN32
//extern Interface * core;
#include <sys/time.h>
#else
#include <windows.h>
#endif

int main(int argc, char ** argv)
{
	core = new Interface();
 	if(core->Init() == GEM_ERROR) {
		delete(core);
		return -1;
	}
	core->GetVideoDriver()->CreateDisplay(core->Width,core->Height,core->Bpp,core->FullScreen);
	Font * fps = core->GetFont("NORMAL\0");
	char fpsstring[_MAX_PATH];
	Color fpscolor = {0xff,0xff,0xff,0x00}, fpsblack = {0x00,0x00,0x00,0x00};
	Color * palette = core->GetVideoDriver()->CreatePalette(fpscolor, fpsblack);
	int frame = 0, time, timebase = 0;
	double frames = 0.0;
#ifndef WIN32
	struct timeval tv;
#endif
	do {
		if(core->ChangeScript) {
			core->GetGUIScriptEngine()->LoadScript(core->NextScript);
			core->ChangeScript = false;
			core->GetGUIScriptEngine()->RunFunction("OnLoad");
		}
		core->DrawWindows();
		frame++;
#ifndef WIN32
		gettimeofday(&tv, NULL);
		time = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#else
		time = GetTickCount();
#endif
		if(time - timebase > 1000) {
			frames = (frame*1000.0/(time-timebase));
			timebase = time;
			frame = 0;
		}
		sprintf(fpsstring, "%.3f fps", frames);
		fps->Print(Region(0,0,100,20), (unsigned char *)fpsstring, palette, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE,true);
	} while(core->GetVideoDriver()->SwapBuffers() == GEM_OK);
	delete(core);
	return 0;
}
