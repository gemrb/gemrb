#include "../../includes/win32def.h"
#include "VideoModes.h"
#include "../../includes/errors.h"
#include <vector>

using namespace std;

VideoModes::VideoModes(void)
{
}

VideoModes::~VideoModes(void)
{
}

int VideoModes::AddVideoMode(int w, int h, int bpp, bool fs, bool checkUnique)
{
	VideoMode vm(w,h,bpp,fs);
	if(checkUnique) {
		for(unsigned long i = 0; i < modes.size(); i++) {
			if(modes[i] == vm)
				return GEM_ERROR;
		}
	}
	modes.push_back(vm);
	return GEM_OK;
}

int VideoModes::FindVideoMode(VideoMode &vm)
{
	for(unsigned long i = 0; i < modes.size(); i++) {
		if(modes[i] == vm)
			return i;
	}
	return GEM_ERROR;
}

void VideoModes::RemoveEntry(unsigned long n)
{
	if(n >= modes.size())
		return;
	vector<VideoMode>::iterator m = modes.begin();
	m+=n;
	modes.erase(m);
}

void VideoModes::Empty(void)
{
  vector<VideoMode>::iterator m;
  for(m = modes.begin(); m != modes.end(); ++m) {
	  modes.erase(m);
	}
}

VideoMode VideoModes::operator[](unsigned long n)
{
	if(n >= modes.size())
		return VideoMode();
	return modes[n];
}

int VideoModes::Count(void)
{
	return (int)modes.size();
}
