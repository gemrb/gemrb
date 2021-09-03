/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2006 The GemRB Project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 *
 */

#include "Particles.h"

#include "CharAnimations.h"
#include "Game.h"
#include "Interface.h"
#include "TableMgr.h"
#include "Video/Video.h"

namespace GemRB {

Color sparkcolors[MAX_SPARK_COLOR][MAX_SPARK_PHASE];
bool inited = false;

static const int spark_color_indices[MAX_SPARK_COLOR] = {12, 5, 0, 6, 1, 8, 2, 7, 9, 3, 4, 10, 11};

static void TranslateColor(const char *value, Color &color)
{
	//if not RGB then try to interpret it as a dword
	if (strnicmp(value, "RGB(", 4) != 0) {
		uint32_t c = strtounsigned<uint32_t>(value);
		color = Color::FromABGR(c);
	} else {
		sscanf(value+4,"%hhu,%hhu,%hhu)", &color.r, &color.g, &color.b);
	}
}

static void InitSparks()
{
	AutoTable tab = gamedata->LoadTable("sprklclr");
	if (!tab)
		return;

	memset(sparkcolors,0,sizeof(sparkcolors));
	for (int i = 0; i < MAX_SPARK_COLOR; i++) {
		for (int j = 0; j < MAX_SPARK_PHASE; j++) {
			sparkcolors[i][j].a=0xff;
		}
	}
	int i = tab->GetRowCount();
	if (i>MAX_SPARK_COLOR) {
		i = MAX_SPARK_COLOR;
	}
	while (i--) {
		for (int j = 0; j < MAX_SPARK_PHASE; j++) {
			int idx;

			if (i < MAX_SPARK_COLOR) {
				idx = spark_color_indices[i];
			} else {
				idx = i;
			}
			const char *value = tab->QueryField(idx,j);
			TranslateColor(value, sparkcolors[i][j]);
		}
	}
	inited = true;
}

Particles::Particles(int s)
{
	points.resize(s);
	/*
	for (int i=0;i<MAX_SPARK_PHASE;i++) {
		bitmap[i]=NULL;
	}
	*/
	if (!inited) {
		InitSparks();
	}
	size = last_insert = s;
}

void Particles::SetBitmap(unsigned int FragAnimID)
{
	//int i;

	fragments = GemRB::make_unique<CharAnimations>(FragAnimID, 0);
/*
	for (i=0;i<MAX_SPARK_PHASE;i++) {
		delete( bitmap[i] );
	}

	AnimationFactory* af = ( AnimationFactory* )
		gamedata->GetFactoryResource( BAM, IE_BAM_CLASS_ID );

	if (af == NULL) {
		return;
	}

	for (i=0;i<MAX_SPARK_PHASE; i++) {
		bitmap[i] = af->GetCycle( i );
	}

*/
}

bool Particles::AddNew(const Point &point)
{
	int st;

	switch(path)
	{
	case SP_PATH_EXPL:
		st = pos.h+last_insert%15;
		break;
	case SP_PATH_RAIN:
	case SP_PATH_FLIT:
		st = core->Roll(3,5,MAX_SPARK_PHASE)<<4;
		break;
	case SP_PATH_FOUNT:
		st =(MAX_SPARK_PHASE + 2*pos.h);
		break;
	case SP_PATH_FALL:
	default:
		st =(MAX_SPARK_PHASE + pos.h)<<4;
		break;
	}
	int i = last_insert;
	while (i--) {
		if (points[i].state == -1) {
			points[i].state = st;
			points[i].pos = point;
			last_insert = i;
			return false;
		}
	}
	i = size;
	while (i--!=last_insert) {
		if (points[i].state == -1) {
			points[i].state = st;
			points[i].pos = point;
			last_insert = i;
			return false;
		}
	}
	return true;
}

void Particles::Draw(Point p)
{
	Video *video=core->GetVideoDriver();
	const Game *game = core->GetGame();

	if (owner) {
		p.x-=pos.x;
		p.y-=pos.y;
	}
	int i = size;
	while (i--) {
		if (points[i].state == -1) {
			continue;
		}
		int state;

		switch(path) {
		case SP_PATH_FLIT:
		case SP_PATH_RAIN:
			state = points[i].state>>4;
			break;
		default:
			state = points[i].state;
			break;
		}

		int length; //used only for raindrops
		if (state>=MAX_SPARK_PHASE) {
			constexpr int maxDropLength = 6;
			length = maxDropLength - abs(state - MAX_SPARK_PHASE - maxDropLength);
			state = 0;
		} else {
			state=MAX_SPARK_PHASE-state-1;
			length=0;
		}
		Color clr = sparkcolors[color][state];
		switch (type) {
		case SP_TYPE_BITMAP:
			/*
			if (bitmap[state]) {
				Holder<Sprite2D> frame = bitmap[state]->GetFrame(points[i].state&255);
				video->BlitGameSprite(frame,
					points[i].pos.x+screen.x,
					points[i].pos.y+screen.y, 0, clr,
					NULL, NULL, &screen);
			}
			*/
			if (fragments) {
				//IE_ANI_CAST stance has a simple looping animation
				Animation** anims = fragments->GetAnimation( IE_ANI_CAST, i );
				if (!anims) {
					break;
				}

				const Animation* anim = anims[0];
				Holder<Sprite2D> nextFrame = anim->GetFrame(anim->GetCurrentFrameIndex());

				BlitFlags flags = BlitFlags::NONE;
				if (game) game->ApplyGlobalTint(clr, flags);

				video->BlitGameSpriteWithPalette(nextFrame, fragments->GetPartPalette(0),
													points[i].pos - p, flags, clr);
			}
			break;
		case SP_TYPE_CIRCLE:
			video->DrawCircle (points[i].pos - p, 2, clr);
			break;
		case SP_TYPE_POINT:
		default:
			video->DrawPoint(points[i].pos - p, clr);
			break;
		// this is more like a raindrop
		case SP_TYPE_LINE:
			if (length) {
				video->DrawLine (points[i].pos - p, points[i].pos - p + Point((i&1), length), clr);
			}
			break;
		}
	}
}

void Particles::AddParticles(int count)
{
	while (count--) {
		Point p;

		switch (path) {
		case SP_PATH_EXPL:
			p.x = pos.w/2+core->Roll(1,pos.w/2,pos.w/4);
			p.y = pos.h/2+(last_insert&7);
			break;
		case SP_PATH_FALL:
		default:
			p.x = core->Roll(1,pos.w,0);
			p.y = core->Roll(1,pos.h/2,0);
			break;
		case SP_PATH_RAIN:
		case SP_PATH_FLIT:
			p.x = core->Roll(1,pos.w,0);
			p.y = core->Roll(1,pos.h,0);
			break;
		case SP_PATH_FOUNT:
			p.x = core->Roll(1,pos.w/2,pos.w/4);
			p.y = core->Roll(1,pos.h/2,0);
			break;
		}
		if (AddNew(p) ) {
			break;
	 	}
	}
}

int Particles::Update()
{
	int drawn=false;

	if (phase==P_EMPTY) {
		return drawn;
	}

	if (timetolive) {
		if (timetolive<core->GetGame()->GameTime) {
			spawn_type = SP_SPAWN_NONE;
			phase = P_FADE;
		}
	}

	int grow;
	switch(spawn_type) {
	case SP_SPAWN_NONE:
		grow = 0;
		break;
	case SP_SPAWN_FULL:
		grow = size;
		spawn_type=SP_SPAWN_NONE;
		break;
	case SP_SPAWN_SOME:
	default:
		grow = size/10;
	}
	for (int i = 0; i < size; i++) {
		if (points[i].state==-1) {
			continue;
		}
		drawn=true;
		if (!points[i].state) {
			grow++;
		}
		points[i].state--;

		switch (path) {
		case SP_PATH_FALL:
			points[i].pos.y+=3+((i>>2)&3);
			points[i].pos.y%=pos.h;
			break;
		case SP_PATH_RAIN:
			points[i].pos.x+=pos.w+(i&1);
			points[i].pos.x%=pos.w;
			points[i].pos.y+=3+((i>>2)&3);
			points[i].pos.y%=pos.h;
			break;
		case SP_PATH_FLIT:
			if (points[i].state<=MAX_SPARK_PHASE<<4) {
				break;
			}
			points[i].pos.x+=core->Roll(1,3,pos.w-2);
			points[i].pos.x%=pos.w;
			points[i].pos.y+=(i&3)+1;
			break;
		case SP_PATH_EXPL:
			points[i].pos.y+=1;
			break;
		case SP_PATH_FOUNT:
			if (points[i].state<=MAX_SPARK_PHASE) {
				break;
			}
			if (points[i].state<(MAX_SPARK_PHASE+pos.h)) {
				if ( (points[i].state&7) == 7) {
					points[i].pos.x+=(i&3)-1;
				}
				points[i].pos.y+=2;
			} else {
				if ( (points[i].state&7) == 7) {
					points[i].pos.x+=(i&3)-1;
				}
				points[i].pos.y-=2;
			}
			break;
		}
	}
	if (phase==P_GROW) {
		AddParticles(grow);
		drawn=true;
	}
	if (!drawn) {
		phase = P_EMPTY;
	}
	return drawn;
}

}
