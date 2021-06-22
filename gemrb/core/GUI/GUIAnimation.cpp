/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2017 The GemRB Project
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
 */

#include "GUIAnimation.h"

#include "AnimationFactory.h"
#include "GUI/Button.h"
#include "Interface.h"
#include "RNG.h"

namespace GemRB {

// so far everything we need uses this cycle
static const unsigned long ColorCycleSteps[] = {6,4,2,0,2,4,6,8};
ColorCycle GlobalColorCycle(7);

void ColorCycle::AdvanceTime(tick_t time)
{
	step = ColorCycleSteps[(time >> speed) & 7];
}

Color ColorCycle::Blend(const Color& c1, const Color& c2) const
{
	Color mix;
	// we dont waste our time with the alpha component
	mix.a = c1.a;
	mix.r = (c1.r * step + c2.r * (8-step)) >> 3;
	mix.g = (c1.g * step + c2.g * (8-step)) >> 3;
	mix.b = (c1.b * step + c2.b * (8-step)) >> 3;
	return mix;
}

bool PointAnimation::HasEnded() const
{
	return current == end;
}
	
Point PointAnimation::GenerateNext(tick_t curTime)
{
	if (curTime < endtime) {
		int deltax = end.x - begin.x;
		int deltay = end.y - begin.y;
		unsigned long deltaT = endtime - begintime;
		Point p;
		p.x = deltax * double(curTime - begintime) / deltaT;
		p.y = deltay * double(curTime - begintime) / deltaT;

		return begin + p;
	} else {
		return end;
	}
}

Color ColorAnimation::GenerateNext(tick_t time)
{
	cycle.AdvanceTime(time - timeOffset);
	return cycle.Blend(end, begin);
}

bool ColorAnimation::HasEnded() const
{
	return (repeat) ? false : current == end;
}

ButtonAnimation::ButtonAnimation(Button* btn, const ResRef& ResRef, int Cycle)
{
	cycle = Cycle;

	bam = ( AnimationFactory* ) gamedata->GetFactoryResource( ResRef,
		IE_BAM_CLASS_ID, IE_NORMAL );

	if (! bam)
		return;

	button = btn;
	button->animation = this;
	time = GetTicks();
	UpdateAnimation(false);
}

//freeing the bitmaps only once, but using an intelligent algorithm
ButtonAnimation::~ButtonAnimation()
{
	//removing from timer first
	core->timer.RemoveAnimation(this);
}

bool ButtonAnimation::SameResource(const ResRef& resRef, int Cycle)
{
	if (!button) return false;
	if (!bam) return false;
	if (resRef != bam->ResRef) return false;
	int c = cycle;
	if (button->Flags()&IE_GUI_BUTTON_PLAYRANDOM) {
		c&=~1;
	}
	if (Cycle!=c) return false;
	return true;
}

void ButtonAnimation::UpdateAnimation(bool paused)
{
	if (paused && !(button->Flags() & IE_GUI_BUTTON_PLAYALWAYS)) {
		// try again later
		time = GetTicks() + 1;
		core->timer.AddAnimation(this);
		return;
	}

	tick_t delta = 0;
	if (button->Flags() & IE_GUI_BUTTON_PLAYRANDOM) {
		// simple Finite-State Machine
		if (anim_phase == 0) {
			frame = 0;
			anim_phase = 1;
			// note: the granularity of time should be
			// one of twenty values from [500, 10000]
			// but not the full range.
			delta = 500 + 500 * RAND(0, 19);
			cycle&=~1;
		} else if (anim_phase == 1) {
			if (!RAND(0,29)) {
				cycle|=1;
			}
			anim_phase = 2;
			delta = 100;
		} else {
			frame++;
			delta = 100;
		}
	} else {
		frame ++;
		if (has_palette) {
			delta = 100;  //hack for slower movement
		} else {
			delta = 15;
		}
	}

	if (UpdateAnimationSprite()) {
		time = GetTicks() + delta;
		core->timer.AddAnimation(this);
	}
}

bool ButtonAnimation::UpdateAnimationSprite() {
	Holder<Sprite2D> pic = bam->GetFrame((unsigned short) frame, (unsigned char) cycle);

	if (pic == NULL) {
		//stopping at end frame
		if (button->Flags() & IE_GUI_BUTTON_PLAYONCE) {
			core->timer.RemoveAnimation( this );
			button->SetAnimPicture( NULL );
			return false;
		}
		anim_phase = 0;
		frame = 0;
		pic = bam->GetFrame( 0, (unsigned char) cycle );
	}

	if (pic == NULL) {
		return true;
	}

	if (has_palette) {
		PaletteHolder palette = pic->GetPalette();
		palette->SetupPaperdollColours(colors, 0);
	}
	
	if (is_blended) {
		PaletteHolder palette = pic->GetPalette();
		palette->CreateShadedAlphaChannel();
	}

	button->SetAnimPicture( pic );
	return true;
}

void ButtonAnimation::SetPaletteGradients(ieDword *col)
{
	memcpy(colors, col, 8*sizeof(ieDword));
	has_palette = true;
	UpdateAnimationSprite();
}

void ButtonAnimation::SetBlend(bool b)
{
	is_blended = b;
}

}
