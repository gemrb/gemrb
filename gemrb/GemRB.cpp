// GemRB.cpp : Defines the entry point for the application.
//

#include "stdio.h"
#include "plugins/Core/Interface.h"
#include "plugins/Core/Video.h"
#include "plugins/Core/ResourceMgr.h"
#include "plugins/Core/MapMgr.h"
#include "plugins/Core/ImageMgr.h"
#include "plugins/Core/AnimationMgr.h"
#include "plugins/Core/WindowMgr.h"

#ifndef WIN32
extern Interface * core;
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
  /*
  DataStream * bamFile = core->GetResourceMgr()->GetResource("TOOLTIP\0", IE_BAM_CLASS_ID);
  if(bamFile == NULL) {
    delete(core);
    return -1;
  }
  printf("BAM File Retrieved\n");
  AnimationMgr * fontloader = (AnimationMgr*)core->GetInterface(IE_BAM_CLASS_ID);
  if(fontloader == NULL) {
    printf("Cannot get an AnimationMgr\n");
    delete(bamFile);
    delete(core);
    return -1;
  }
  fontloader->Open(bamFile, true);
  printf("AnimationManager Obtained\n");
  Font * fnt = fontloader->GetFont();*/
	/*char ResRef[8] = {0};
	printf("Please Insert a AREA Name to Display: ");
	scanf("%8s", &ResRef);*/
	//DataStream * arefile = core->GetResourceMgr()->GetResource(/*"GUIAPB\0\0"*/ResRef, IE_ARE_CLASS_ID);
	/*if(arefile == NULL) {
		printf("Resource not found. (Try Writing the name Upper Case)\n");
		delete(core);
		return 0;
	}
	MapMgr * mm = (MapMgr*)core->GetInterface(IE_ARE_CLASS_ID);
	mm->Open(arefile);
	Map * map = mm->GetMap();*/
	//core->GetVideoDriver()->MoveViewportTo(3741,2801);
	//Region rgn(0,0,200,200);
	//Color rcol = {0,0xff,0,0};
	WindowMgr * wmgr = (WindowMgr*)core->GetInterface(IE_CHU_CLASS_ID);
	if(wmgr == NULL) {
		printf("No WindowMgr present...");
		delete(core);
		return -1;
	}
	DataStream * chu = core->GetResourceMgr()->GetResource("GUIOPT\0", IE_CHU_CLASS_ID);
	if(chu == NULL) {
		printf("No CHUFile present...");
		delete(core);
		return -1;
	}
	wmgr->Open(chu, true);
	Window * win[3];
	win[0] = wmgr->GetWindow(6);
	win[1] = wmgr->GetWindow(0);
	win[2] = wmgr->GetWindow(1);
	//Window * win2 = wmgr->GetWindow(1);
	//Window * win3 = wmgr->GetWindow(2);
	Font * fps = core->GetFont("NORMAL\0");
	char fpsstring[_MAX_PATH];
	Color fpscolor = {0xff,0xff,0xff,0x00}, fpsblack = {0x00,0x00,0x00,0x00};
	int frame = 0, time, timebase = 0;
	double frames = 0.0;
#ifndef WIN32
	struct timeval tv;
#endif
	do {
    //fnt->Print(rgn, (unsigned char*)("La Agile Volpe Bruna Salta Sopra Il Cane Pigro."), 79, 0);
    //core->GetVideoDriver()->DrawRect(rgn, rcol);
		//map->DrawMap();
		//win->DrawWindow();
		//win2->DrawWindow();
		//win3->DrawWindow();
		for(int i = 0; i < 3; i++) {
			win[i]->DrawWindow();
		}
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
		fps->Print(Region(0,0,100,20), (unsigned char *)fpsstring, &fpscolor, &fpsblack, IE_FONT_ALIGN_LEFT | IE_FONT_ALIGN_MIDDLE,true);
	} while(core->GetVideoDriver()->SwapBuffers() == GEM_OK);
	//core->GetVideoDriver()->FreeSprite(img);
	//core->FreeInterface(mm);
	core->FreeInterface(wmgr);
	for(int i = 0; i < 3; i++) {
		win[i]->release();
	}
	delete(core);
	return 0;
}
