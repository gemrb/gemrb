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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "MoviePlayer.h"

#include "Interface.h"

#include "GUI/Window.h"
#include "GUI/WindowManager.h"

#include <chrono>
#include <thread>

using namespace std::chrono;

namespace GemRB {

const TypeID MoviePlayer::ID = { "MoviePlayer" };

MoviePlayer::~MoviePlayer(void)
{
	Stop();
	delete subtitles;
}

void MoviePlayer::SetSubtitles(SubtitleSet* subs)
{
	delete subtitles;
	subtitles = subs;
}

void MoviePlayer::EnableSubtitles(bool set)
{
	showSubtitles = set;
}

bool MoviePlayer::SubtitlesEnabled() const
{
	return showSubtitles && subtitles;
}

void MoviePlayer::Play(Window* win)
{
	assert(win);

	MoviePlayerControls* mpc = new MoviePlayerControls(*this);
	mpc->SetFrameSize(win->Dimensions());
	win->AddSubviewInFrontOfView(mpc);

	// center over win
	const Region& winFrame = win->Frame();
	const Size& size = Dimensions();
	Point center(winFrame.w / 2 - size.w / 2, winFrame.h / 2 - size.h / 2);
	center = center + winFrame.origin;
	VideoBufferPtr subBuf = nullptr;
	VideoBufferPtr vb = VideoDriver->CreateBuffer(Region(center, size), movieFormat);

	if (subtitles) {
		// FIXME: arbitrary frame of my choosing, not sure there is a better method
		// this should probably at least be sized according to line height
		int y = std::min<int>(winFrame.h - center.y, winFrame.h - 50.0);
		Region subFrame(0, y, winFrame.w, 50.0);
		subBuf = VideoDriver->CreateBuffer(subFrame, Video::BufferFormat::DISPLAY_ALPHA);
	}

	// currently, our MoviePlayer implementation takes over the entire screen
	// not only that but the Play method blocks until movie is done/stopped.
	win->Focus(); // we bypass the WindowManager for drawing, but for event handling we need this
	isPlaying = true;
	do {
		// taking over the application runloop...

		// we could draw all the windows if we wanted to be able to have videos that aren't fullscreen
		// However, since we completely block the normal game loop we will bypass WindowManager drawing
		//WindowManager::DefaultWindowManager().DrawWindows();

		// first draw the window for play controls/subtitles
		//win->Draw();

		VideoDriver->PushDrawingBuffer(vb);
		if (DecodeFrame(*vb) == false) {
			Stop(); // error / end
		}

		if (subtitles && showSubtitles) {
			assert(subBuf);
			// we purposely draw on the window, which may be larger than the video
			VideoDriver->PushDrawingBuffer(subBuf);
			subtitles->RenderInBuffer(*subBuf, framePos);
		}
		// TODO: pass movie fps (and remove the cap from within the movie decoders)
	} while ((VideoDriver->SwapBuffers(0) == GEM_OK) && isPlaying);

	delete win->View::RemoveSubview(mpc);
}

void MoviePlayer::Stop()
{
	isPlaying = false;
	// update in case of early skipping
	core->GetWindowManager()->FadeColor.a = 0;
}

microseconds MoviePlayer::get_current_time() const
{
	return duration_cast<microseconds>(steady_clock::now().time_since_epoch());
}

void MoviePlayer::timer_start()
{
	lastTime = get_current_time();
}

void MoviePlayer::timer_wait(microseconds frameWait)
{
	auto time = get_current_time();

	while (time - lastTime > frameWait) {
		time -= frameWait;
		video_frameskip++;
	}

	microseconds to_sleep = frameWait - (time - lastTime);
	std::this_thread::sleep_for(to_sleep);

	timer_start();
}

bool MoviePlayerControls::OnKeyPress(const KeyboardEvent& Key, unsigned short /*Mod*/)
{
	KeyboardKey keycode = Key.keycode;
	switch (keycode) {
		case 's':
			player.EnableSubtitles(!player.SubtitlesEnabled());
			break;
		default:
			player.Stop();
			break;
	}

	return true;
}

}
