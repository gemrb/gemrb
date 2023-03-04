/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2023 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "OverHeadText.h"

#include "Interface.h"
#include "Scriptable.h"

#include "GUI/GameControl.h"

namespace GemRB {

const String& OverHeadText::GetText(size_t idx) const
{
	return messages[idx].text;
}

void OverHeadText::SetText(String newText, bool display, const Color& newColor)
{
	messages[0].pos.Invalidate();
	if (newText.empty()) {
		Display(false);
	} else {
		messages[0].text = std::move(newText);
		messages[0].color = newColor;
		Display(display);
	}
}

bool OverHeadText::Display(bool show, size_t idx)
{
	if (show) {
		isDisplaying = true;
		messages[idx].timeStartDisplaying = GetMilliseconds();
		return true;
	} else if (isDisplaying) {
		// is this the last displaying message?
		if (messages.size() == 1) {
			isDisplaying = false;
			messages[idx].timeStartDisplaying = 0;
		} else {
			messages.erase(messages.begin() + idx);
			bool show = false;
			for (const auto& msg : messages) {
				show = show || msg.timeStartDisplaying != 0;
			}
			if (!show) isDisplaying = false;
		}
		return true;
	}
	return false;
}

// 'fix' the current overhead text - follow owner's position
void OverHeadText::FixPos(const Point& pos, size_t idx)
{
	messages[idx].pos = pos;
}

int OverHeadText::GetHeightOffset() const
{
	int offset = 100;
	if (owner->Type == ST_ACTOR) {
		offset = static_cast<const Selectable*>(owner)->circleSize * 50;
	}

	return offset;
}

void OverHeadText::Draw()
{
	static constexpr int maxDelay = 6000;
	if (!isDisplaying)
		return;

	tick_t time = GetMilliseconds();
	const Color& textColor = messages[0].color == ColorBlack ? core->InfoTextColor : messages[0].color;
	Font::PrintColors color = { textColor, ColorBlack };

	time -= messages[0].timeStartDisplaying;
	if (time >= maxDelay) {
		Display(false);
		return;
	} else {
		time = (maxDelay - time) / 10;
		// rapid fade-out
		if (time < 256) {
			color.fg.a = static_cast<unsigned char>(255 - time);
		}
	}

	int cs = GetHeightOffset();
	Point p = messages[0].pos.IsInvalid() ? owner->Pos : messages[0].pos;
	Region vp = core->GetGameControl()->Viewport();
	Region rgn(p - Point(100, cs) - vp.origin, Size(200, 400));
	core->GetTextFont()->Print(rgn, messages[0].text, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_TOP, color);
}

}
