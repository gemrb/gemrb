/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header: /data/gemrb/cvs2svn/gemrb/gemrb/gemrb/GemRB.cpp,v 1.21 2004/02/22 13:41:41 avenger_teambg Exp $
 *
 */

// GemRB.cpp : Defines the entry point for the application.


#define GEM_APP

#include "stdio.h"
#include "includes/win32def.h"
#include "plugins/Core/Interface.h"
#include "plugins/Core/Video.h"
#include "plugins/Core/ResourceMgr.h"
#include "plugins/Core/MapMgr.h"
#include "plugins/Core/ImageMgr.h"
#include "plugins/Core/DialogMgr.h"
#include "plugins/Core/WindowMgr.h"

#ifndef WIN32
#include <ctype.h>
#include <sys/time.h>
#include <dirent.h>
#else
#include <windows.h>
#endif

int main(int argc, char ** argv)
{
	core = new Interface(argc, argv);
 	if(core->Init() == GEM_ERROR) {
		delete(core);
		return -1;
	}
	core->GetVideoDriver()->CreateDisplay(core->Width,core->Height,core->Bpp,core->FullScreen);
	core->GetVideoDriver()->SetDisplayTitle(core->GameName, core->GameType);
	Font * fps = core->GetFont((unsigned int)0);
	char fpsstring[_MAX_PATH];
	Color fpscolor = {0xff,0xff,0xff,0x00}, fpsblack = {0x00,0x00,0x00,0x00};
	unsigned long frame = 0, time, timebase = 0;
	double frames = 0.0;
	Region bg(0,0,100,30);
	Color * palette = core->GetVideoDriver()->CreatePalette(fpscolor, fpsblack);
	do {
		if(core->ChangeScript) {
		        core->GetGUIScriptEngine()->LoadScript(core->NextScript);
			core->ChangeScript = false;
			core->GetGUIScriptEngine()->RunFunction("OnLoad");
		}
		core->DrawWindows();
		frame++;
		GetTime(time);
		if(time - timebase > 1000) {
			frames = (frame*1000.0/(time-timebase));
			timebase = time;
			frame = 0;
			sprintf(fpsstring, "%.3f fps", frames);
			core->GetVideoDriver()->DrawRect(bg, fpsblack);
			fps->Print(Region(0,0,100,20), (unsigned char *)fpsstring, palette, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE,true);
		}
	} while(core->GetVideoDriver()->SwapBuffers() == GEM_OK);
	delete(core);
	return 0;
}
