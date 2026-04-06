// SPDX-FileCopyrightText: 2003 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

/**
 * @file MoviePlayer.h
 * Declares MoviePlayer class, abstract loader and player for videos
 * @author The GemRB Project
 */


#ifndef MOVIEPLAYER_H
#define MOVIEPLAYER_H

#include "Resource.h"

#include "GUI/TextSystem/Font.h"
#include "GUI/View.h"
#include "Strings/String.h"
#include "Video/Video.h"

#include <chrono>

namespace GemRB {

class Window;
struct KeyboardEvent;

/**
 * @class MoviePlayer
 * Abstract loader and player for videos
 */

class GEM_EXPORT MoviePlayer : public Resource {
public:
	using microseconds = std::chrono::microseconds;

	static const TypeID ID;

	class SubtitleSet {
		Color col;
		Holder<Font> font;

	public:
		explicit SubtitleSet(Holder<Font> fnt, Color col = ColorWhite)
			: col(col), font(std::move(fnt))
		{
			assert(font);
		}

		virtual ~SubtitleSet() noexcept = default;

		virtual size_t NextSubtitleFrame() const = 0;
		virtual const String& SubtitleAtFrame(size_t) const = 0;

		void RenderInBuffer(VideoBuffer& buffer, size_t frame) const
		{
			if (frame >= NextSubtitleFrame()) {
				buffer.Clear();
				const String& str = SubtitleAtFrame(frame);
				Region rect(Point(), buffer.Size());
				font->Print(rect, str, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE, { col, ColorBlack });
			}
		}
	};

private:
	bool isPlaying = false;
	bool showSubtitles = false;
	std::unique_ptr<SubtitleSet> subtitles;

protected:
	// NOTE: make sure any new movie plugins set these!
	Video::BufferFormat movieFormat = Video::BufferFormat::DISPLAY;
	Size movieSize;
	size_t framePos = 0;

	microseconds lastTime = microseconds(0);

	microseconds frame_wait = microseconds(0);
	unsigned int video_frameskip = 0;
	unsigned int video_skippedframes = 0;

protected:
	microseconds get_current_time() const;
	void timer_start();
	void timer_wait(microseconds frameWait);

	virtual bool DecodeFrame(VideoBuffer&) = 0;

public:
	MoviePlayer() noexcept {};

	Size Dimensions() const { return movieSize; }
	void Play(Window* win);
	virtual void Stop();

	void SetSubtitles(std::unique_ptr<SubtitleSet> subs);
	void EnableSubtitles(bool set);
	bool SubtitlesEnabled() const;
	bool IsPlaying() const;
};

class MoviePlayerControls : public View {
	MoviePlayer& player;

private:
	// currently dont have any real controls
	void DrawSelf(const Region& /*drawFrame*/, const Region& /*clip*/) override {}

	bool OnKeyPress(const KeyboardEvent& Key, unsigned short /*Mod*/) override;

	bool OnMouseDown(const MouseEvent& /*me*/, unsigned short /*Mod*/) override
	{
		player.Stop();
		return true;
	}

public:
	explicit MoviePlayerControls(MoviePlayer& player)
		: View(Region(Point(), player.Dimensions())), player(player) {}
};

}

#endif
