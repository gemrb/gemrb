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

#include "GUI/Label.h"
#include "Interface.h"

namespace GemRB {

const TypeID MoviePlayer::ID = { "MoviePlayer" };

MoviePlayer::MoviePlayer(void)
{
	framePos = 0;
	subtitles = NULL;
}

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

void MoviePlayer::Play(Window* win)
{
	assert(win);
	Video* video = core->GetVideoDriver();

	win->SetFlags(Window::Borderless, OP_OR);
	win->SetPosition(Window::PosCentered);
	MoviePlayerControls* mpc = new MoviePlayerControls(*this);
	win->AddSubviewInFrontOfView(mpc);

	VideoBuffer* vb = video->CreateBuffer(win->Frame(), movieFormat);

	// currently, our MoviePlayer implementation takes over the entire screen
	// not only that but the Play method blocks until movie is done/stopped.
	win->Focus(); // we bypass the WindowManager for drawing, but for event handling we need this
	isPlaying = true;
	do {
		// taking over the application runloop...
		video->PushDrawingBuffer(vb);
		if (DecodeFrame(*vb)) {
			if (subtitles) {
				subtitles->RenderInRegion(win->Frame(), framePos);
			}
			// we could draw all the windows if we wanted to be able to have videos that aren't fullscreen
			// However, since we completely block the normal game loop we will bypass WindowManager drawing
			//WindowManager::DefaultWindowManager().DrawWindows();

			//win.Draw(); // if we had any playback controls we would need to draw them
		} else {
			Stop(); // error / end
		}
		// TODO: pass movie fps (and remove the cap from within the movie decoders)
	} while ((video->SwapBuffers(0) == GEM_OK) && isPlaying);

	delete win->View::RemoveSubview(mpc);
	video->DestroyBuffer(vb);
}

void MoviePlayer::Stop()
{
	isPlaying = false;
}

}
