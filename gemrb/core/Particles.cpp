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
#include "Video.h"

namespace GemRB {

Color sparkcolors[MAX_SPARK_COLOR][MAX_SPARK_PHASE];
bool inited = false;

#define SPARK_COUNT 13

static int spark_color_indices[SPARK_COUNT]={12,5,0,6,1,8,2,7,9,3,4,10,11};

static void TranslateColor(const char *value, Color &color)
{
	int r = 0;
	int g = 0;
	int b = 0;
	//if not RGB then try to interpret it as a dword
	if (strnicmp(value,"RGB(",4)) {
		r = strtol(value,NULL,0);
		color.r = r&0xff;
		color.g = (r>>8)&0xff;
		color.b = (r>>16)&0xff;
		color.a = (r>>24)&0xff;
	}
	sscanf(value+4,"%d,%d,%d)", &r, &g, &b);
	color.r=r;
	color.g=g;
	color.b=b;
}

static void InitSparks()
{
	int i,j;
	AutoTable tab("sprklclr");
	if (!tab)
		return;

	memset(sparkcolors,0,sizeof(sparkcolors));
	for (i=0;i<MAX_SPARK_COLOR;i++) {
		for (j=0;j<MAX_SPARK_PHASE;j++) {
			sparkcolors[i][j].a=0xff;
		}
	}
	i = tab->GetRowCount();
	if (i>MAX_SPARK_COLOR) {
		i = MAX_SPARK_COLOR;
	}
	while (i--) {
		for (int j=0;j<MAX_SPARK_PHASE;j++) {
			int idx;

			if (i<SPARK_COUNT) {
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
	points = (Element *) malloc(s*sizeof(Element) );
	memset(points, -1, s*sizeof(Element) );
	/*
	for (int i=0;i<MAX_SPARK_PHASE;i++) {
		bitmap[i]=NULL;
	}
	*/
	fragments = NULL;
	if (!inited) {
		InitSparks();
	}
	size = last_insert = s;
	color = 0;
	phase = P_FADE;
	owner = NULL;
	type = SP_TYPE_POINT;
	path = SP_PATH_FALL;
	spawn_type = SP_SPAWN_NONE;
	timetolive = 0;
}

Particles::~Particles()
{
	if (points) {
		free(points);
	}
	/*
	for (int i=0;i<MAX_SPARK_PHASE;i++) {
		delete( bitmap[i]);
	}
	*/
	delete fragments;
}

void Particles::SetBitmap(unsigned int FragAnimID)
{
	//int i;

	delete fragments;

	fragments = new CharAnimations(FragAnimID, 0);
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

void Particles::Draw(const Region &screen)
{
	int length; //used only for raindrops

	Video *video=core->GetVideoDriver();
	Region region = video->GetViewport();
	if (owner) {
		region.x-=pos.x;
		region.y-=pos.y;
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

		if (state>=MAX_SPARK_PHASE) {
			length = 6-abs(state-MAX_SPARK_PHASE-6);
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
				Sprite2D *frame = bitmap[state]->GetFrame(points[i].state&255);
				video->BlitGameSprite(frame,
					points[i].pos.x+screen.x,
					points[i].pos.y+screen.y, 0, clr,
					NULL, NULL, &screen);
			}
			*/
			if (fragments) {
				//IE_ANI_CAST stance has a simple looping animation
				Animation** anims = fragments->GetAnimation( IE_ANI_CAST, i );
				if (anims) {
					Animation* anim = anims[0];
					Sprite2D* nextFrame = anim->GetFrame(anim->GetCurrentFrame());
					video->BlitGameSprite( nextFrame, points[i].pos.x - region.x, points[i].pos.y - region.y,
						0, clr, NULL, fragments->GetPartPalette(0), &screen);
				}
			}
			break;
		case SP_TYPE_CIRCLE:
			video->DrawCircle (points[i].pos.x-region.x,
				points[i].pos.y-region.y, 2, clr, true);
			break;
		case SP_TYPE_POINT:
		default:
			video->SetPixel (points[i].pos.x-region.x,
				points[i].pos.y-region.y, clr, true);
			break;
		// this is more like a raindrop
		case SP_TYPE_LINE:
			if (length) {
				video->DrawLine (points[i].pos.x+region.x,
					points[i].pos.y+region.y,
					points[i].pos.x+region.x+(i&1),
					points[i].pos.y+region.y+length, clr, true);
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
	int i;
	int grow;

	if (phase==P_EMPTY) {
		return drawn;
	}

	if (timetolive) {
		if (timetolive<core->GetGame()->GameTime) {
			spawn_type = SP_SPAWN_NONE;
			phase = P_FADE;
		}
	}

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
	for(i=0;i<size;i++) {
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
