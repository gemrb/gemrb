#include "../../includes/win32def.h"
#include "Animation.h"
#include "Interface.h"
#ifndef WIN32
#include <sys/time.h>
#else
#include <windows.h>
#endif

extern Interface * core;

Animation::Animation(unsigned short * frames, int count)
{
	for(int i = 0; i < count; i++)
		indices.push_back(frames[i]);
	pos = 0;
	starttime = 0;
	x = 0;
	y = 0;
	free = true;
	ChangePalette = true;
}

Animation::~Animation(void)
{
	if(!free)
		return;
	for(unsigned int i = 0; i < frames.size(); i++) {
		core->GetVideoDriver()->FreeSprite(frames[i]);
	}
}

void Animation::AddFrame(Sprite2D * frame, int index)
{
	frames.push_back(frame);
	link.push_back(index);
	int x = -frame->XPos;
	int y = -frame->YPos;
	int w = frame->Width-frame->XPos;
	int h = frame->Height-frame->YPos;
	if(x < animArea.x)
		animArea.x = x;
	if(y < animArea.y)
		y = y;
	if(w > animArea.w)
		animArea.w = w;
	if(h > animArea.h)
		animArea.h = h;
}

Sprite2D * Animation::NextFrame(void)
{
	if(starttime == 0) {
#ifdef WIN32
		starttime = GetTickCount();
#else
		struct timeval tv;
		gettimeofday(&tv, NULL);
		starttime = (tv.tv_usec/1000) + (tv.tv_sec*1000);
#endif
	}
	Sprite2D * ret = NULL;
	for(unsigned int i = 0; i < link.size(); i++) {
		if(link[i] == indices[pos]) {
			ret = frames[i];
			break;
		}
	}
#ifdef WIN32
	pos = (((GetTickCount() - starttime) % (132*indices.size())) / 132);
#else
	struct timeval tv;
	unsigned long time;
	gettimeofday(&tv, NULL);
	time = (tv.tv_usec/1000) + (tv.tv_sec*1000);
	pos = (((time - starttime) % (132*indices.size())) / 132);
	/*pos++;
	if(pos >= indices.size())
	pos = 0;*/
#endif
	return ret;
}


void Animation::release(void)
{
	delete this;
}
/** Gets the i-th frame */
Sprite2D * Animation::GetFrame(unsigned long i)
{
	if(i >= frames.size())
		return NULL;
	return frames[i];
}

void Animation::SetPalette(Color * Palette)
{
	for(int i = 0; i < frames.size(); i++) {
		core->GetVideoDriver()->SetPalette(frames[i], Palette);
	}
}
