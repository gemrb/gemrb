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

/**
 * @file MoviePlayer.h
 * Declares MoviePlayer class, abstract loader and player for videos
 * @author The GemRB Project
 */


#ifndef MOVIEPLAYER_H
#define MOVIEPLAYER_H

#include "globals.h"
#include "win32def.h"

#include "Resource.h"

#include "GUI/TextSystem/Font.h"
#include "GUI/Window.h"
#include "System/String.h"
#include "Video.h"

namespace GemRB {

/**
 * @class MoviePlayer
 * Abstract loader and player for videos
 */

class GEM_EXPORT MoviePlayer : public Resource {
public:
	static const TypeID ID;

	class SubtitleSet {
		Font* font;
		Palette* palette;
	public:
		SubtitleSet(Font* fnt, Palette* pal = NULL) {
			font = fnt;
			palette = pal;
		}

		virtual const String& SubtitleAtFrame(size_t) const = 0;

		const Font* getFont() const { return font; }
		Palette* getPalette() const {
			if (!palette) {
				return font->GetPalette().get();
			}
			return palette;
		}
	};

private:
	bool isPlaying;
	const SubtitleSet* subtitles;

protected:
	// NOTE: make sure any new movie plugins set these!
	Video::BufferFormat movieFormat;
	Size movieSize;
	size_t framePos;

protected:
	void DisplaySubtitle(const String& sub);
	void PresentMovie(const Region&, Video::BufferFormat fmt);

	virtual bool DecodeFrame(VideoBuffer&) = 0;

public:
	MoviePlayer(void);
	virtual ~MoviePlayer(void);

	Size Dimensions() { return movieSize; }
	void Play(Window* win);
	virtual void Stop();

	void SetSubtitles(const SubtitleSet& subs);
};

class MoviePlayerControls : public View {
	MoviePlayer& player;

private:
	// currently dont have any real controls
	void DrawSelf(Region /*drawFrame*/, const Region& /*clip*/) {}

public:
	MoviePlayerControls(MoviePlayer& player)
	: View(Region(Point(), player.Dimensions())), player(player) {}

	bool OnKeyPress(const KeyboardEvent& /*Key*/, unsigned short /*Mod*/) {
		player.Stop();
		return true;
	}

	void OnMouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/) {
		player.Stop();
	}
};

}

#endif
