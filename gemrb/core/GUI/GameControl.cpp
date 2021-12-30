/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2003 The GemRB Project
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
 */

#include "GUI/GameControl.h"

#include "strrefs.h"

#include "CharAnimations.h"
#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "GUIScriptInterface.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "KeyMap.h"
#include "PathFinder.h"
#include "ScriptEngine.h"
#include "TileMap.h"
#include "Video/Video.h"
#include "damages.h"
#include "ie_cursors.h"
#include "opcode_params.h"
#include "GameScript/GSUtils.h"
#include "GUI/EventMgr.h"
#include "GUI/TextArea.h"
#include "GUI/Window.h"
#include "RNG.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"

#include <array>
#include <cmath>

namespace GemRB {

constexpr uint8_t FORMATIONSIZE = 10;
using formation_t = std::array<Point, FORMATIONSIZE>;

class Formations {
	std::vector<formation_t> formations;
	
	Formations() noexcept {
		//TODO:
		//There could be a custom formation which is saved in the save game
		//alternatively, all formations could be saved in some compatible way
		//so it doesn't cause problems with the original engine
		AutoTable tab = gamedata->LoadTable("formatio");
		if (!tab) {
			// fallback
			formations.emplace_back();
			return;
		}
		auto formationcount = tab->GetRowCount();
		formations.resize(formationcount);
		for (unsigned int i = 0; i < formationcount; i++) {
			for (uint8_t j = 0; j < FORMATIONSIZE; j++) {
				Point p(tab->QueryFieldSigned<int>(i, j*2), tab->QueryFieldSigned<int>(i, j*2+1));
				formations[i][j] = p;
			}
		}
	}
public:
	static const formation_t& GetFormation(size_t formation) noexcept {
		static const Formations formations;
		formation = std::min(formation, formations.formations.size() - 1);
		return formations.formations[formation];
	}
};

static const ResRef TestSpell = "SPWI207";

uint32_t GameControl::DebugFlags = 0;
uint8_t GameControl::DebugPropVal = 0;

static Actor* GetMainSelectedActor()
{
	auto actor = core->GetFirstSelectedPC(false);
	if (actor) return actor;
	return core->GetFirstSelectedActor();
}

//If one of the actors has tracking on, the gamecontrol needs to display
//arrow markers on the edges to point at detected monsters
//tracterID is the tracker actor's global ID
//distance is the detection distance
void GameControl::SetTracker(const Actor *actor, ieDword dist)
{
	trackerID = actor->GetGlobalID();
	distance = dist;
}

GameControl::GameControl(const Region& frame)
: View(frame)
{
	ieDword tmp = 0;
	core->GetDictionary()->Lookup("Always Run", tmp);
	AlwaysRun = tmp != 0;

	ResetTargetMode();
	SetCursor(nullptr);

	tmp = 0;
	core->GetDictionary()->Lookup("Center",tmp);
	if (tmp) {
		ScreenFlags |= SF_ALWAYSCENTER;
	}
	// the game always starts paused so nothing happens till we are ready
	dialoghandler = new DialogHandler();

	EventMgr::EventCallback cb = METHOD_CALLBACK(&GameControl::OnGlobalMouseMove, this);
	eventMonitors[0] = EventMgr::RegisterEventMonitor(cb, Event::MouseMoveMask);
	EventMgr::EventCallback cb2 = METHOD_CALLBACK(&GameControl::DispatchEvent, this);
	eventMonitors[1] = EventMgr::RegisterEventMonitor(cb2, Event::KeyDownMask);
}

GameControl::~GameControl()
{
	for (unsigned long eventMonitor : eventMonitors) {
		EventMgr::UnRegisterEventMonitor(eventMonitor);
	}

	delete dialoghandler;
}

//returns a single point offset for a formation
//formation: the formation type
//pos: the actor's slot ID
Point GameControl::GetFormationOffset(size_t formation, uint8_t pos) const
{
	pos = std::min<uint8_t>(pos, FORMATIONSIZE - 1);
	return Formations::GetFormation(formation)[pos];
}

Point GameControl::GetFormationPoint(const Point& origin, size_t pos, double angle, const std::vector<Point>& exclude) const
{
	Point vec;
	
	const Game* game = core->GetGame();
	const Map* area = game->GetCurrentArea();
	assert(area);

	static constexpr int radius = 36 / 2; // 36 diameter is copied from make_formation.py
	const auto& formationid = game->GetFormation();
	const auto& formation = Formations::GetFormation(formationid);
	
	Point stepVec;
	int direction = (pos % 2 == 0) ? 1 : -1;

	/* Correct for the innate orientation of the points.  */
	angle += M_PI_2;

	if (pos < FORMATIONSIZE) {
		// calculate new coordinates by rotating formation around (0,0)
		vec = RotatePoint(formation[pos], angle);
		stepVec.y = radius;
	} else {
		
		// create a line formation perpendicular to the formation start point and beginning at the last point
		// of the formation table. Alternate between +90 degrees and -90 degrees to keep it balanced
		// the formation table is created along the x axis starting at (0,0)
		Point p = formation[FORMATIONSIZE - 1];
		vec = RotatePoint(p, angle);
		stepVec.x = radius * direction;
	}
	
	vec.y *= 0.75f; // isometric projection
	Point dest = vec + origin;
	int step = 0;
	constexpr int maxStep = 4;
	double stepAngle = 0.0;
	const Point& start = vec;
	
	auto NextDest = [&]() {
		// adjust the point if the actor cant get to `dest`
		// we do this by sweeping an M_PI arc a `radius` (stepVec) away from the point
		// and oriented according to `direction`
		// if nothing is found, reset to `start` and increase the `stepVec` and sweep again
		// each incremental sweep step the `stepAngle` increment shrinks because we have more area to fit
		// if nothing is found after `maxStep` sweeps we just give up and leave it to the path finder to work out
		
		// FIXME: we should precalculate these into a table and use step as an index
		// there is a precission/rounding problem here with comparing against M_PI
		// and we may not use one of the arc end points
		stepAngle += (M_PI_4 / (step + 1)) * direction;
		if (stepAngle > M_PI || stepAngle < -M_PI) {
			++step;
			stepAngle = 0.0;
			if (stepVec.y != 0) {
				stepVec.y += radius;
			} else {
				stepVec.x += radius * direction;
			}
		}
		
		Point rotated = RotatePoint(stepVec, angle + stepAngle);
		rotated.y *= 0.75f; // isometric projection
		return origin + start + rotated;
	};

	while (step < maxStep) {
		auto it = std::find_if(exclude.begin(), exclude.end(), [&](const Point& p) {
			// look for points within some radius
			return p.isWithinRadius(radius, dest);
		});

		if (it != exclude.end()) {
			dest = NextDest();
			continue;
		}
		
		if (area->IsExplored(dest) && !(area->GetBlocked(dest) & (PathMapFlags::PASSABLE | PathMapFlags::ACTOR))) {
			dest = NextDest();
			continue;
		}
		
		break;
	}
	
	if (step == maxStep) {
		// we never found a suitable point
		// to garauntee a point that is reachable just fall back to origin
		// let the pathfinder sort it out
		return origin;
	}
	
	return dest;
}

GameControl::FormationPoints GameControl::GetFormationPoints(const Point& origin, const std::vector<Actor*>& actors,
															 double angle) const
{
	FormationPoints formation;
	for (size_t i = 0; i < actors.size(); ++i) {
		formation.emplace_back(GetFormationPoint(origin, i, angle, formation));
	}
	return formation;
}

void GameControl::DrawFormation(const std::vector<Actor*>& actors, const Point& formationPoint, double angle) const
{
	std::vector<Point> formationPoints = GetFormationPoints(formationPoint, actors, angle);
	for (size_t i = 0; i < actors.size(); ++i) {
		DrawTargetReticle(actors[i], formationPoints[i] - vpOrigin);
	}
}

void GameControl::ClearMouseState()
{
	isSelectionRect = false;
	isFormationRotation = false;
	
	SetCursor(NULL);
}

// generate an action to do the actual movement
// only PST supports RunToPoint
void GameControl::CreateMovement(Actor *actor, const Point &p, bool append, bool tryToRun) const
{
	Action *action = NULL;
	tryToRun |= AlwaysRun;
	
	if (append) {
		action = GenerateAction(fmt::format("AddWayPoint([{}.{}])", p.x, p.y));
		assert(action);
	} else {
		//try running (in PST) only if not encumbered
		if (tryToRun && CanRun(actor)) {
			action = GenerateAction(fmt::format("RunToPoint([{}.{}])", p.x, p.y));
		}
		
		// check again because GenerateAction can fail (non PST)
		if (!action) {
			action = GenerateAction(fmt::format("MoveToPoint([{}.{}])", p.x, p.y));
		}
	}

	actor->CommandActor(action, !append);
	actor->Destination = p; // just to force target reticle drawing if paused
}

// can we handle it (no movement impairments)?
bool GameControl::CanRun(const Actor *actor) const
{
	if (!actor) return false;
	if (actor->GetEncumbranceFactor(true) != 1) {
		return false;
	}
	return true;
}

bool GameControl::ShouldRun(const Actor *actor) const
{
	return CanRun(actor) && AlwaysRun;
}

// Draws arrow markers along the edge of the game window
void GameControl::DrawArrowMarker(const Point& p, const Color& color) const
{
	const WindowManager* wm = core->GetWindowManager();
	auto lock = wm->DrawHUD();

	const Region& bounds = Viewport();
	if (bounds.PointInside(p)) return;

	orient_t dir = GetOrient(p, bounds.Center());
	Holder<Sprite2D> arrow = core->GetScrollCursorSprite(dir, 0);
	
	const Point& dp = bounds.Intercept(p) - bounds.origin;
	core->GetVideoDriver()->BlitGameSprite(arrow, dp, BlitFlags::COLOR_MOD | BlitFlags::BLENDED, color);
}

void GameControl::DrawTargetReticle(uint16_t size, const Color& color, const Point& p) const
{
	uint8_t offset = GlobalColorCycle.Step() >> 1;
	Point offsetH = Point(offset, 0);
	Point offsetV = Point(0, offset);
	
	/* segments should not go outside selection radius */
	uint16_t xradius = (size * 4) - 5;
	uint16_t yradius = (size * 3) - 5;
	const Size s(xradius * 2, yradius * 2);
	const Region r(p - s.Center(), s);

	std::vector<Point> points = PlotEllipse(r);
	assert(points.size() % 4 == 0);
	
	// a line bisecting the ellipse diagonally
	const Point adj(size + 1, 0);
	Point b1 = r.origin - adj;
	Point b2 = r.Maximum() + adj;
	
	Video* video = core->GetVideoDriver();
	
	size_t i = 0;
	// points are ordered per quadrant, so we can process 4 each iteration
	// at first, 2 points will be the left segment and 2 will be the right segment
	for (; i < points.size(); i += 4) {
		// each point is in a different quadrant
		const Point& q1 = points[i];
		const Point& q2 = points[i + 1];
		const Point& q3 = points[i + 2];
		const Point& q4 = points[i + 3];
		
		if (left(b1, b2, q1)) {
			// remaining points are top and bottom segments
			break;
		}
		
		video->DrawPoint(q1 + offsetH, color);
		video->DrawPoint(q2 - offsetH, color);
		video->DrawPoint(q3 - offsetH, color);
		video->DrawPoint(q4 + offsetH, color);
	}
	
	assert(i < points.size() - 4);
	
	// the current points are the ends of the side segments
	video->DrawLine(points[i++] + offsetH, p + offsetH, color);   // begin right segment
	video->DrawLine(points[i++] - offsetH, p - offsetH, color);   // begin left segment
	video->DrawLine(points[i++] - offsetH, p - offsetH, color);   // end left segment
	video->DrawLine(points[i++] + offsetH, p + offsetH, color);   // end right segment
	
	b1 = r.origin + adj;
	b2 = r.Maximum() - adj;
	
	// skip the void between segments
	for (; i < points.size(); i += 4) {
		if (left(b1, b2, points[i])) {
			break;
		}
	}
	
	// the current points are the ends of the top/bottom segments
	video->DrawLine(points[i++] + offsetV, p + offsetV, color); // begin top segement
	video->DrawLine(points[i++] + offsetV, p + offsetV, color); // end top segment
	video->DrawLine(points[i++] - offsetV, p - offsetV, color); // begin bottom segment
	video->DrawLine(points[i++] - offsetV, p - offsetV, color); // end bottom segment
	
	// remaining points are top/bottom segments
	for (; i < points.size(); i += 4) {
		const Point& q1 = points[i];
		const Point& q2 = points[i + 1];
		const Point& q3 = points[i + 2];
		const Point& q4 = points[i + 3];
		
		video->DrawPoint(q1 + offsetV, color);
		video->DrawPoint(q2 + offsetV, color);
		video->DrawPoint(q3 - offsetV, color);
		video->DrawPoint(q4 - offsetV, color);
	}
}

void GameControl::DrawTargetReticle(const Movable* target, const Point& p) const
{
	int size = target->CircleSize2Radius();
	const Color& green = target->selectedColor;
	const Color& color = (target->Over) ? GlobalColorCycle.Blend(target->overColor, green) : green;

	DrawTargetReticle(size, color, p);
}
	
void GameControl::WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	UpdateCursor();

	bool update_scripts = !(DialogueFlags & DF_FREEZE_SCRIPTS);
	
	// handle keeping the actor in the spotlight, but only when unpaused
	if ((ScreenFlags & SF_ALWAYSCENTER) && update_scripts) {
		const Actor *star = core->GetFirstSelectedActor();
		if (star) {
			vpVector = star->Pos - vpOrigin - Point(frame.w / 2, frame.h / 2);
		}
	}

	if (!vpVector.IsZero() && MoveViewportTo(vpOrigin + vpVector, false)) {
		if ((Flags() & IgnoreEvents) == 0 && core->GetMouseScrollSpeed() && (ScreenFlags & SF_ALWAYSCENTER) == 0) {
			orient_t orient = GetOrient(vpVector, Point());
			// set these cursors on game window so they are universal
			window->SetCursor(core->GetScrollCursorSprite(orient, numScrollCursor));

			numScrollCursor = (numScrollCursor + 1) % 15;
		}
	} else if (!window->IsDisabled()) {
		window->SetCursor(NULL);
	}

	const Map* area = CurrentArea();
	assert(area);

	int flags = GA_NO_DEAD|GA_NO_UNSCHEDULED|GA_SELECT|GA_NO_ENEMY|GA_NO_NEUTRAL;
	auto ab = area->GetActorsInRect(SelectionRect(), flags);

	std::vector<Actor*>::iterator it = highlighted.begin();
	for (; it != highlighted.end(); ++it) {
		Actor* act = *it;
		act->SetOver(false);
	}

	highlighted.clear();
	for (Actor* actor : ab) {
		if (actor->GetStat(IE_EA) > EA_CONTROLLABLE) continue;
		actor->SetOver(true);
		highlighted.push_back(actor);
	}
}

/** Draws the Control on the Output Display */
void GameControl::DrawSelf(const Region& screen, const Region& /*clip*/)
{
	const Game* game = core->GetGame();
	Map *area = game->GetCurrentArea();

	// FIXME: some of this should happen during mouse events
	// setup outlines
	InfoPoint *i;
	for (size_t idx = 0; (i = area->TMap->GetInfoPoint(idx)); idx++) {
		i->Highlight = false;
		if (i->VisibleTrap(0)) {
			if (overInfoPoint == i && target_mode) {
				i->outlineColor = ColorGreen;
			} else {
				i->outlineColor = ColorRed;
			}
			i->Highlight = true;
			continue;
		}
	}

	// FIXME: some of this should happen during mouse events
	Door *d;
	for (size_t idx = 0; (d = area->TMap->GetDoor(idx)); idx++) {
		d->Highlight = false;
		if (d->Flags & DOOR_HIDDEN) {
			continue;
		}

		if (d->Flags & DOOR_SECRET) {
			if (d->Flags & DOOR_FOUND) {
				d->Highlight = true;
				d->outlineColor = displaymsg->GetColor(GUIColors::HIDDENDOOR);
			} else {
				continue;
			}
		}

		if (overDoor == d) {
			d->Highlight = true;
			if (target_mode) {
				if (d->Visible() && (d->VisibleTrap(0) || (d->Flags & DOOR_LOCKED))) {
					// only highlight targettable doors
					d->outlineColor = ColorGreen;
				}
			} else if (!(d->Flags & DOOR_SECRET)) {
				// mouse over, not in target mode, no secret door
				d->outlineColor = ColorCyan;
			}
		}

		// traps always take precedence
		if (d->VisibleTrap(0)) {
			d->Highlight = true;
			d->outlineColor = ColorRed;
		}
	}

	// FIXME: some of this should happen during mouse events
	Container *c;
	for (unsigned int idx = 0; (c = area->TMap->GetContainer(idx)); ++idx) {
		c->Highlight = false;
		if (c->Flags & CONT_DISABLED) {
			continue;
		}

		if (overContainer == c) {
			c->Highlight = true;
			if (target_mode) {
				if (c->Flags & CONT_LOCKED) {
					c->outlineColor = ColorGreen;
				}
			} else {
				c->outlineColor = displaymsg->GetColor(GUIColors::HOVERCONTAINER);
			}
		}

		// traps always take precedence
		if (c->VisibleTrap(0)) {
			c->Highlight = true;
			c->outlineColor = ColorRed; // traps
		}
	}

	//drawmap should be here so it updates fog of war
	area->DrawMap(Viewport(), core->GetFogRenderer(), DebugFlags);

	if (trackerID) {
		const Actor *actor = area->GetActorByGlobalID(trackerID);

		if (actor) {
			std::vector<Actor*> monsters = area->GetAllActorsInRadius(actor->Pos, GA_NO_DEAD|GA_NO_LOS|GA_NO_UNSCHEDULED, distance);
			for (const auto& monster : monsters) {
				if (monster->IsPartyMember()) continue;
				if (monster->GetStat(IE_NOTRACKING)) continue;
				DrawArrowMarker(monster->Pos, ColorBlack);
			}
		} else {
			trackerID = 0;
		}
	}

	if (lastActorID) {
		const Actor* actor = GetLastActor();
		if (actor) {
			DrawArrowMarker(actor->Pos, ColorGreen);
		}
	}

	Video* video = core->GetVideoDriver();
	// Draw selection rect
	if (isSelectionRect) {
		Region r = SelectionRect();
		r.x -= vpOrigin.x;
		r.y -= vpOrigin.y;
		video->DrawRect(r, ColorGreen, false );
	}

	const Point& gameMousePos = GameMousePos();
	// draw reticles
	if (isFormationRotation) {
		double angle = AngleFromPoints(gameMousePos, gameClickPoint);
		DrawFormation(game->selected, gameClickPoint, angle);
	} else {
		for (const auto& selectee : game->selected) {
			assert(selectee);
			if (selectee->ShouldDrawReticle()) {
				DrawTargetReticle(selectee, selectee->Destination - vpOrigin);
			}
		}
	}

	// Draw path
	if (drawPath) {
		PathListNode* node = drawPath;
		while (true) {
			Point p = Map::ConvertCoordFromTile(node->point) + Point(8, 6);
			if (!node->Parent) {
				video->DrawCircle( p, 2, ColorRed );
			} else {
				Point old = Map::ConvertCoordFromTile(node->Parent->point) + Point(8, 6);
				video->DrawLine(old, p, ColorGreen);
			}
			if (!node->Next) {
				video->DrawCircle( p, 2, ColorGreen );
				break;
			}
			node = node->Next;
		}
	}

	if (core->HasFeature(GFFlags::ONSCREEN_TEXT) && !DisplayText.empty()) {
		Font::PrintColors colors = { displaymsg->GetColor(GUIColors::FLOAT_TXT_INFO), ColorBlack };
		core->GetTextFont()->Print(screen, DisplayText, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE, colors);
		if (!(DialogueFlags & DF_FREEZE_SCRIPTS)) {
			// just replicating original engine behaviour
			if (DisplayTextTime == 0) {
				SetDisplayText(L"", 0);
			} else {
				DisplayTextTime--;
			}
		}
	}
}

// this existly only so tab can be handled
// it's used both for tooltips everywhere and hp display on game control
bool GameControl::DispatchEvent(const Event& event) const
{
	if (!window || window->IsDisabled() || (Flags()&IgnoreEvents)) {
		return false;
	}

	if (event.keyboard.keycode == GEM_TAB) {
		const Game *game = core->GetGame();
		// show partymember hp/maxhp as overhead text
		for (int pm=0; pm < game->GetPartySize(false); pm++) {
			Actor *pc = game->GetPC(pm, true);
			if (!pc) continue;
			pc->DisplayHeadHPRatio();
		}
		return true;
	} else if (event.keyboard.keycode == GEM_ESCAPE) {
		core->ResetActionBar();
		core->SetEventFlag(EF_RESETTARGET);
	}
	return false;
}

/** Key Press Event */
bool GameControl::OnKeyPress(const KeyboardEvent& Key, unsigned short mod)
{
	unsigned int i, pc;
	Game* game = core->GetGame();

	KeyboardKey keycode = Key.keycode;
	if (mod) {
		switch (keycode) {
			case GEM_ALT:
				DebugFlags |= DEBUG_SHOW_CONTAINERS|DEBUG_SHOW_DOORS;
				break;
			default:
				// the random bitshift is to skip checking hotkeys with mods
				// eg. ctrl-j should be ignored for keymap.ini handling and
				// passed straight on
				if (!core->GetKeyMap()->ResolveKey(Key.keycode, mod<<20)) {
					game->SendHotKey(towupper(Key.character));
					return View::OnKeyPress(Key, mod);
				}
				break;
		}
	} else {
			switch (keycode) {
				case GEM_UP:
				case GEM_DOWN:
				case GEM_LEFT:
				case GEM_RIGHT:
					{
						ieDword keyScrollSpd = 64;
						core->GetDictionary()->Lookup("Keyboard Scroll Speed", keyScrollSpd);
						if (keycode >= GEM_UP) {
							int v = (keycode == GEM_UP) ? -1 : 1;
							Scroll( Point(0, keyScrollSpd * v) );
						} else {
							int v = (keycode == GEM_LEFT) ? -1 : 1;
							Scroll( Point(keyScrollSpd * v, 0) );
						}
					}
					break;
				#ifdef ANDROID
				case 'c': // show containers in ANDROID, GEM_ALT is not possible to use

					DebugFlags |= DEBUG_SHOW_CONTAINERS|DEBUG_SHOW_DOORS;
					break;
				#endif
				case GEM_TAB: // show partymember hp/maxhp as overhead text
				// fallthrough
				case GEM_ESCAPE: // redraw actionbar
					// do nothing; these are handled in DispatchEvent due to tab having two functions
					break;
				case '0':
					game->SelectActor( NULL, false, SELECT_NORMAL );
					i = game->GetPartySize(false)/2+1;
					while(i--) {
						SelectActor(i, true);
					}
					break;
				case '-':
					game->SelectActor( NULL, true, SELECT_NORMAL );
					i = game->GetPartySize(false)/2+1;
					while(i--) {
						SelectActor(i, false);
					}
					break;
				case '=':
					SelectActor(-1);
					break;
				case '7': // 1 & 2
				case '8': // 3 & 4
				case '9': // 5 & 6
					// We do not handle the range 1..6, these are handled as hotkeys
					// for the portrait buttons, so that they remain working when the
					// inventory screen is open.
					game->SelectActor( NULL, false, SELECT_NORMAL );
					i = game->GetPartySize(false);
					pc = 2*(keycode - '6')-1;
					if (pc >= i) {
						SelectActor(i, true);
						break;
					}
					SelectActor(pc, true);
					SelectActor(pc+1, true);
					break;
				default:
					if (!core->GetKeyMap()->ResolveKey(Key.keycode, 0)) {
						game->SendHotKey(towupper(Key.character));
						return View::OnKeyPress(Key, 0);
					}
					break;
			}
	}
	return true;
}

//Select (or deselect) a new actor (or actors)
void GameControl::SelectActor(int whom, int type)
{
	Game* game = core->GetGame();
	if (whom==-1) {
		game->SelectActor( NULL, true, SELECT_NORMAL );
		return;
	}

	/* doesn't fall through here */
	Actor* actor = game->FindPC( whom );
	if (!actor)
		return;

	if (type==0) {
		game->SelectActor( actor, false, SELECT_NORMAL );
		return;
	}
	if (type==1) {
		game->SelectActor( actor, true, SELECT_NORMAL );
		return;
	}

	bool was_selected = actor->IsSelected();
	if (game->SelectActor( actor, true, SELECT_REPLACE )) {
		if (was_selected || (ScreenFlags & SF_ALWAYSCENTER)) {
			ScreenFlags |= SF_CENTERONACTOR;
		}
	}
}

//Effect for the ctrl-r cheatkey (resurrect)
static EffectRef heal_ref = { "CurrentHPModifier", -1 };
static EffectRef damage_ref = { "Damage", -1 };

/** Key Release Event */
bool GameControl::OnKeyRelease(const KeyboardEvent& Key, unsigned short Mod)
{
	Point gameMousePos = GameMousePos();
	//cheatkeys with ctrl-
	if (Mod & GEM_MOD_CTRL) {
		if (!core->CheatEnabled()) {
			return false;
		}
		Game* game = core->GetGame();
		Map* area = game->GetCurrentArea( );
		if (!area)
			return false;
		Actor *lastActor = area->GetActorByGlobalID(lastActorID);
		switch (Key.character) {
			case 'a': //switches through the avatar animations
				if (lastActor) {
					lastActor->GetNextAnimation();
				}
				break;
			// b
			case 'c': //force cast a hardcoded spell
				//caster is the last selected actor
				//target is the door/actor currently under the pointer
				if (!game->selected.empty()) {
					Actor *src = game->selected[0];
					Scriptable *target = lastActor;
					if (overDoor) {
						target = overDoor;
					}
					if (target) {
						src->SetSpellResRef(TestSpell);
						src->CastSpell(target, false);
						if (src->LastSpellTarget) {
							src->CastSpellEnd(0, false);
						} else {
							src->CastSpellPointEnd(0, false);
						}
					}
				}
				break;
			case 'd': //detect a trap or door
				if (overInfoPoint) {
					overInfoPoint->DetectTrap(256, lastActorID);
				}
				if (overContainer) {
					overContainer->DetectTrap(256, lastActorID);
				}
				if (overDoor) {
					overDoor->TryDetectSecret(256, lastActorID);
					overDoor->DetectTrap(256, lastActorID);
				}
				break;
			case 'e':// reverses pc order (useful for parties bigger than 6)
				game->ReversePCs();
				break;
			// f
			case 'g'://shows loaded areas and other game information
				Log(DEBUG, "Game", "{}", game->dump());
				break;
			// h
			case 'i'://interact trigger (from the original game)
				if (!lastActor) {
					lastActor = area->GetActor( gameMousePos, GA_DEFAULT);
				}
				if (lastActor && !(lastActor->GetStat(IE_MC_FLAGS)&MC_EXPORTABLE)) {
					int size = game->GetPartySize(true);
					if (size < 2 || lastActor->GetCurrentArea() != game->GetCurrentArea()) break;
					for (int i = core->Roll(1, size, 0); i < 2*size; i++) {
						const Actor *target = game->GetPC(i % size, true);
						if (target == lastActor) continue;
						if (target->GetStat(IE_MC_FLAGS) & MC_EXPORTABLE) continue; //not NPC
						lastActor->HandleInteractV1(target);
						break;
					}
				}
				break;
			case 'j': //teleports the selected actors
				for (Actor *selectee : game->selected) {
					selectee->ClearActions();
					MoveBetweenAreasCore(selectee, core->GetGame()->CurrentArea, gameMousePos, -1, true);
				}
				break;
			case 'k': //kicks out actor
				if (lastActor && lastActor->InParty) {
					lastActor->Stop();
					lastActor->AddAction( GenerateAction("LeaveParty()") );
				}
				break;
			case 'l': //play an animation (vvc/bam) over an actor
				//the original engine was able to swap through all animations
				if (lastActor) {
					lastActor->AddAnimation(ResRef("S056ICBL"), 0, 0, 0);
				}
				break;
			case 'M':
				if (!lastActor) {
					lastActor = area->GetActor( gameMousePos, GA_DEFAULT);
				}
				if (!lastActor) {
					// ValidTarget never returns immobile targets, making debugging a nightmare
					// so if we don't have an actor, we make really really sure by checking manually
					unsigned int count = area->GetActorCount(true);
					while (count--) {
						const Actor *actor = area->GetActor(count, true);
						if (actor->IsOver(gameMousePos)) {
							actor->GetAnims()->DebugDump();
						}
					}
				}
				if (lastActor) {
					lastActor->GetAnims()->DebugDump();
					break;
				}
				break;
			case 'm': //prints a debug dump (ctrl-m in the original game too)
				if (!lastActor) {
					lastActor = area->GetActor( gameMousePos, GA_DEFAULT);
				}
				if (!lastActor) {
					// ValidTarget never returns immobile targets, making debugging a nightmare
					// so if we don't have an actor, we make really really sure by checking manually
					unsigned int count = area->GetActorCount(true);
					while (count--) {
						const Actor *actor = area->GetActor(count, true);
						if (actor->IsOver(gameMousePos)) {
							actor->dump();
						}
					}
				}
				if (lastActor) {
					lastActor->dump();
					break;
				}
				if (overDoor) {
					overDoor->dump();
					break;
				}
				if (overContainer) {
					overContainer->dump();
					break;
				}
				if (overInfoPoint) {
					overInfoPoint->dump();
					break;
				}
				core->GetGame()->GetCurrentArea()->dump(false);
				break;
			case 'n': //prints a list of all the live actors in the area
				core->GetGame()->GetCurrentArea()->dump(true);
				break;
			// o
			case 'p': //center on actor
				ScreenFlags|=SF_CENTERONACTOR;
				ScreenFlags^=SF_ALWAYSCENTER;
				break;
			case 'q': //joins actor to the party
				if (lastActor && !lastActor->InParty) {
					lastActor->Stop();
					lastActor->AddAction( GenerateAction("JoinParty()") );
				}
				break;
			case 'r'://resurrects actor
				if (!lastActor) {
					lastActor = area->GetActor( gameMousePos, GA_DEFAULT);
				}
				if (lastActor) {
					Effect *fx = EffectQueue::CreateEffect(heal_ref, lastActor->GetStat(IE_MAXHITPOINTS), 0x30001, FX_DURATION_INSTANT_PERMANENT);
					if (fx) {
						core->ApplyEffect(fx, lastActor, lastActor);
					}
				}
				break;
			case 's': //switches through the stance animations
				if (lastActor) {
					lastActor->GetNextStance();
				}
				break;
			case 't': // advances time by 1 hour
				game->AdvanceTime(core->Time.hour_size);
				//refresh gui here once we got it
				break;
			case 'u': // dump GameScript GLOBAL vars
				core->GetGame()->locals->DebugDump();
				break;
			case 'U': // dump death vars
				core->GetGame()->kaputz->DebugDump();
				break;
			case 'V': // dump GemRB vars like the game ini settings
				core->GetDictionary()->DebugDump();
				break;
			case 'v': //marks some of the map visited (random vision distance)
				area->ExploreMapChunk( gameMousePos, RAND(0,29), 1 );
				break;
			case 'w': // consolidates found ground piles under the pointed pc
				area->MoveVisibleGroundPiles(gameMousePos);
				break;
			case 'x': // shows coordinates on the map
				Log(MESSAGE, "GameControl", "Position: {} [{}.{}]", area->GetScriptName(), gameMousePos.x, gameMousePos.y);
				break;
			case 'Y': // damages all enemies by 300 (resistances apply)
				// mwahaha!
				{
				int i = area->GetActorCount(false);
				while(i--) {
					Actor *victim = area->GetActor(i, false);
					if (victim->Modified[IE_EA] == EA_ENEMY) {
						Effect *newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_MAGIC<<16, FX_DURATION_INSTANT_PERMANENT);
						core->ApplyEffect(newfx, victim, victim);
					}
				}
				}
				// fallthrough
			case 'y': //kills actor
				if (lastActor) {
					//using action so the actor is killed
					//correctly (synchronisation)
					lastActor->Stop();

					Effect *newfx;
					newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_MAGIC<<16, FX_DURATION_INSTANT_PERMANENT);
					core->ApplyEffect(newfx, lastActor, lastActor);
					if (! (lastActor->GetInternalFlag() & IF_REALLYDIED)) {
						newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_ACID<<16, FX_DURATION_INSTANT_PERMANENT);
						core->ApplyEffect(newfx, lastActor, lastActor);
						newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_CRUSHING<<16, FX_DURATION_INSTANT_PERMANENT);
						core->ApplyEffect(newfx, lastActor, lastActor);
					}
				} else if (overContainer) {
					overContainer->SetContainerLocked(false);
				} else if (overDoor) {
					overDoor->SetDoorLocked(0,0);
				}
				break;
			case 'z': //shift through the avatar animations backward
				if (lastActor) {
					lastActor->GetPrevAnimation();
				}
				break;
			case '1': //change paperdoll armour level
				if (! lastActor)
					break;
				lastActor->NewStat(IE_ARMOR_TYPE,1,MOD_ADDITIVE);
				break;
			case '4': //show all traps and infopoints
				DebugFlags ^= DEBUG_SHOW_INFOPOINTS;
				Log(MESSAGE, "GameControl", "Show traps and infopoints {}", DebugFlags & DEBUG_SHOW_INFOPOINTS ? "ON" : "OFF");
				break;
			case '5':
				{
					constexpr int flagCnt = 6;
					static uint32_t wallFlags[flagCnt]{
						0,
						DEBUG_SHOW_WALLS_ALL,
						DEBUG_SHOW_DOORS_SECRET,
						DEBUG_SHOW_DOORS_DISABLED,
						DEBUG_SHOW_WALLS,
						DEBUG_SHOW_WALLS_ANIM_COVER
					};
					static uint32_t flagIdx = 0;
					DebugFlags &= ~DEBUG_SHOW_WALLS_ALL;
					DebugFlags |= wallFlags[flagIdx++];
					flagIdx = flagIdx % flagCnt;
				}
				break;
			case '6': //toggle between lightmap/heightmap/material/search
				{
					constexpr int flagCnt = 5;
					static uint32_t flags[flagCnt]{
						0,
						DEBUG_SHOW_SEARCHMAP,
						DEBUG_SHOW_MATERIALMAP,
						DEBUG_SHOW_HEIGHTMAP,
						DEBUG_SHOW_LIGHTMAP,
					};
					constexpr uint32_t mask = (DEBUG_SHOW_LIGHTMAP | DEBUG_SHOW_HEIGHTMAP
											   | DEBUG_SHOW_MATERIALMAP | DEBUG_SHOW_SEARCHMAP);
					
					static uint32_t flagIdx = 0;
					DebugFlags &= ~mask;
					DebugFlags |= flags[flagIdx++];
					flagIdx = flagIdx % flagCnt;
					
					if (DebugFlags & mask) {
						// fog interferese with debugging the map
						// you can manually reenable with ctrl+7
						DebugFlags |= DEBUG_SHOW_FOG_ALL;
					} else {
						DebugFlags &= ~DEBUG_SHOW_FOG_ALL;
						DebugPropVal = 0;
					}
				}
				break;
			case '7': //toggles fog of war
				{
					constexpr int flagCnt = 4;
					static uint32_t fogFlags[flagCnt]{
						0,
						DEBUG_SHOW_FOG_ALL,
						DEBUG_SHOW_FOG_INVISIBLE,
						DEBUG_SHOW_FOG_UNEXPLORED
					};
					static uint32_t flagIdx = 0;
					
					DebugFlags &= ~DEBUG_SHOW_FOG_ALL;
					DebugFlags |= fogFlags[flagIdx++];
					flagIdx = flagIdx % flagCnt;
				}
				break;
		}
		return true; //return from cheatkeys
	}
	const Game* game = core->GetGame();
	switch (Key.keycode) {
//FIXME: move these to guiscript
		case ' ': //soft pause
			core->TogglePause();
			break;
		case GEM_ALT: //alt key (shows containers)
#ifdef ANDROID
		case 'c': // show containers in ANDROID, GEM_ALT is not possible to use
#endif
			DebugFlags &= ~(DEBUG_SHOW_CONTAINERS|DEBUG_SHOW_DOORS);
			break;
		case GEM_TAB: // remove overhead partymember hp/maxhp
			for (int pm = 0; pm < game->GetPartySize(false); pm++) {
				Actor *pc = game->GetPC(pm, true);
				if (!pc) continue;
				pc->overHead.Display(false, 0);
			}
			break;
		default:
			return false;
	}
	return true;
}

String GameControl::TooltipText() const {
	const Map* area = CurrentArea();
	if (area == NULL) {
		return View::TooltipText();
	}

	const Point& gameMousePos = GameMousePos();
	if (!area->IsVisible(gameMousePos)) {
		return View::TooltipText();
	}

	const Actor* actor = area->GetActor(gameMousePos, GA_NO_DEAD|GA_NO_UNSCHEDULED);
	if (actor == NULL) {
		return View::TooltipText();
	}

	static String tip; // only one game control and we return a const& so cant be temporary.
	// pst ignores TalkCount
	if (core->HasFeature(GFFlags::PST_STATE_FLAGS)) {
		tip = actor->GetName();
	} else {
		tip = actor->GetDefaultName();
	}

	int hp = actor->GetStat(IE_HITPOINTS);
	int maxhp = actor->GetStat(IE_MAXHITPOINTS);

	if (actor->InParty) {
		if (core->HasFeature(GFFlags::ONSCREEN_TEXT)) {
			tip += L": ";
		} else {
			tip += L"\n";
		}

		if (actor->HasVisibleHP()) {
			tip += fmt::format(L"{}/{}", hp, maxhp);
		} else {
			tip += L"?";
		}
	} else {
		// a guess at a neutral check
		bool enemy = actor->GetStat(IE_EA) != EA_NEUTRAL;
		// test for an injured string being present for this game
		ieStrRef strref = DisplayMessage::GetStringReference(HCStrings::Uninjured);
		if (enemy && strref != ieStrRef::INVALID) {
			// non-neutral, not in party: display injured string
			// these boundaries are just a guess
			HCStrings strIdx = HCStrings::Injured4;
			if (hp == maxhp) {
				strIdx = HCStrings::Uninjured;
			} else if (hp > (maxhp*3)/4) {
				strIdx = HCStrings::Injured1;
			} else if (hp > maxhp/2) {
				strIdx = HCStrings::Injured2;
			} else if (hp > maxhp/3) {
				strIdx = HCStrings::Injured3;
			}
			strref = DisplayMessage::GetStringReference(strIdx);
			String injuredstring = core->GetString(strref, STRING_FLAGS::NONE);
			tip += L"\n" + injuredstring;
		}
	}

	return tip;
}

Holder<Sprite2D> GameControl::GetTargetActionCursor() const
{
	int curIdx = -1;
	switch(target_mode) {
		case TARGET_MODE_TALK:
			curIdx = IE_CURSOR_TALK;
			break;
		case TARGET_MODE_ATTACK:
			curIdx = IE_CURSOR_ATTACK;
			break;
		case TARGET_MODE_CAST:
			curIdx = IE_CURSOR_CAST;
			break;
		case TARGET_MODE_DEFEND:
			curIdx = IE_CURSOR_DEFEND;
			break;
		case TARGET_MODE_PICK:
			curIdx = IE_CURSOR_PICK;
			break;
	}
	if (curIdx != -1) {
		return core->Cursors[curIdx];
	}
	return nullptr;
}

Holder<Sprite2D> GameControl::Cursor() const
{
	Holder<Sprite2D> cursor = View::Cursor();
	if (cursor == NULL && lastCursor != IE_CURSOR_INVALID) {
		int idx = lastCursor & ~IE_CURSOR_GRAY;
		if (EventMgr::MouseDown()) {
			++idx;
		}
		cursor = core->Cursors[idx];
	}
	return cursor;
}

/** Mouse Over Event */
bool GameControl::OnMouseOver(const MouseEvent& /*me*/)
{
	const Map* area = CurrentArea();
	if (area == NULL) {
		return false;
	}

	Actor *lastActor = area->GetActorByGlobalID(lastActorID);
	if (lastActor) {
		lastActor->SetOver( false );
	}

	Point gameMousePos = GameMousePos();
	// let us target party members even if they are invisible
	lastActor = area->GetActor(gameMousePos, GA_NO_DEAD|GA_NO_UNSCHEDULED);
	if (lastActor && lastActor->Modified[IE_EA] >= EA_CONTROLLED) {
		if (!lastActor->ValidTarget(target_types) || !area->IsVisible(gameMousePos)) {
			lastActor = NULL;
		}
	}

	if ((target_types & GA_NO_SELF) && lastActor ) {
		if (lastActor == core->GetFirstSelectedActor()) {
			lastActor=NULL;
		}
	}

	if (lastActor && lastActor->GetStat(IE_NOCIRCLE)) {
		lastActor = nullptr;
	}

	SetLastActor(lastActor);

	return true;
}

void GameControl::UpdateCursor()
{
	const Map *area = CurrentArea();
	if (area == NULL) {
		lastCursor = IE_CURSOR_BLOCKED;
		return;
	}

	Point gameMousePos = GameMousePos();
	int nextCursor = area->GetCursor( gameMousePos );
	//make the invisible area really invisible
	if (nextCursor == IE_CURSOR_INVALID) {
		lastCursor = IE_CURSOR_BLOCKED;
		return;
	}
	
	if (overDoor) {
		overDoor->Highlight = false;
	}
	if (overContainer) {
		overContainer->Highlight = false;
	}
	
	overDoor = area->TMap->GetDoor(gameMousePos);
	// ignore infopoints and containers beneath doors
	if (overDoor) {
		if (overDoor->Visible()) {
			nextCursor = overDoor->GetCursor(target_mode, lastCursor);
		} else {
			overDoor = nullptr;
		}
	} else {
		overInfoPoint = area->TMap->GetInfoPoint(gameMousePos, false);
		if (overInfoPoint) {
			nextCursor = overInfoPoint->GetCursor(target_mode);
		}
		// recheck in case the position was different, resulting in a new isVisible check
		if (nextCursor == IE_CURSOR_INVALID) {
			lastCursor = IE_CURSOR_BLOCKED;
			return;
		}

		// don't allow summons to try travelling (alone), since it causes tons of loading
		if (nextCursor == IE_CURSOR_TRAVEL && core->GetGame()->OnlyNPCsSelected()) {
			lastCursor = IE_CURSOR_BLOCKED;
			return;
		}

		overContainer = area->TMap->GetContainer( gameMousePos );
	}

	if (overContainer) {
		nextCursor = overContainer->GetCursor(target_mode, lastCursor);
	}
	// recheck in case the position was different, resulting in a new isVisible check
	// fixes bg2 long block door in ar0801 above vamp beds, crashing on mouseover (too big)
	if (nextCursor == IE_CURSOR_INVALID) {
		lastCursor = IE_CURSOR_BLOCKED;
		return;
	}

	const Actor *lastActor = area->GetActorByGlobalID(lastActorID);
	if (lastActor) {
		// don't change the cursor for birds
		if (lastActor->GetStat(IE_DONOTJUMP) == DNJ_BIRD) return;

		ieDword type = lastActor->GetStat(IE_EA);
		if (type >= EA_EVILCUTOFF || type == EA_GOODBUTRED) {
			nextCursor = IE_CURSOR_ATTACK;
		} else if ( type > EA_CHARMED ) {
			if (lastActor->GetStat(IE_NOCIRCLE)) return;
			nextCursor = IE_CURSOR_TALK;
			//don't let the pc to talk to frozen/stoned creatures
			ieDword state = lastActor->GetStat(IE_STATE_ID);
			if (state & (STATE_CANTMOVE^STATE_SLEEP)) {
				nextCursor |= IE_CURSOR_GRAY;
			}
		} else {
			nextCursor = IE_CURSOR_NORMAL;
		}
	}

	if (target_mode == TARGET_MODE_TALK) {
		nextCursor = IE_CURSOR_TALK;
		if (!lastActor) {
			nextCursor |= IE_CURSOR_GRAY;
		} else {
			//don't let the pc to talk to frozen/stoned creatures
			ieDword state = lastActor->GetStat(IE_STATE_ID);
			if (state & (STATE_CANTMOVE^STATE_SLEEP)) {
				nextCursor |= IE_CURSOR_GRAY;
			}
		}
	} else if (target_mode == TARGET_MODE_ATTACK) {
		nextCursor = IE_CURSOR_ATTACK;
		if (!overDoor && !lastActor && !overContainer) {
			nextCursor |= IE_CURSOR_GRAY;
		}
	} else if (target_mode == TARGET_MODE_CAST) {
		nextCursor = IE_CURSOR_CAST;
		//point is always valid
		if (!(target_types & GA_POINT) && !lastActor) {
			nextCursor |= IE_CURSOR_GRAY;
		}
	} else if (target_mode == TARGET_MODE_DEFEND) {
		nextCursor = IE_CURSOR_DEFEND;
		if(!lastActor) {
			nextCursor |= IE_CURSOR_GRAY;
		}
	} else if (target_mode == TARGET_MODE_PICK) {
		if (lastActor) {
			nextCursor = IE_CURSOR_PICK;
		} else {
			if (!overContainer && !overDoor && !overInfoPoint) {
				nextCursor = IE_CURSOR_STEALTH|IE_CURSOR_GRAY;
			}
		}
	}

	if (nextCursor >= 0) {
		lastCursor = nextCursor ;
	}
}

bool GameControl::IsDisabledCursor() const
{
	bool isDisabled = View::IsDisabledCursor();
	if (lastCursor != IE_CURSOR_INVALID)
		isDisabled |= bool(lastCursor&IE_CURSOR_GRAY);

	return isDisabled;
}

void GameControl::DebugPaint(const Point& p, bool sample) const noexcept
{
	if (DebugFlags & (DEBUG_SHOW_SEARCHMAP | DEBUG_SHOW_MATERIALMAP | DEBUG_SHOW_HEIGHTMAP | DEBUG_SHOW_LIGHTMAP)) {
		Map* map = CurrentArea();
		Point tile = Map::ConvertCoordToTile(p);
		TileProps::Property prop = TileProps::Property::SEARCH_MAP;
		if (DebugFlags & DEBUG_SHOW_MATERIALMAP) {
			prop = TileProps::Property::MATERIAL;
		} else if (DebugFlags & DEBUG_SHOW_HEIGHTMAP) {
			prop = TileProps::Property::ELEVATION;
		} else if (DebugFlags & DEBUG_SHOW_LIGHTMAP) {
			prop = TileProps::Property::LIGHTING;
		}
		
		if (sample) {
			DebugPropVal = map->tileProps.QueryTileProp(tile, prop);
		} else {
			map->tileProps.SetTileProp(tile, prop, DebugPropVal);
		}
	}
}

bool GameControl::OnMouseDrag(const MouseEvent& me)
{
	if (EventMgr::ModState(GEM_MOD_CTRL)) {
		Point p = ConvertPointFromScreen(me.Pos());
		DebugPaint(p + vpOrigin, false);
		return true;
	}

	if (me.ButtonState(GEM_MB_MIDDLE)) {
		Scroll(me.Delta());
		return true;
	}
	
	if (me.ButtonState(GEM_MB_MENU)) {
		InitFormation(gameClickPoint);
		return true;
	}

	if (target_mode != TARGET_MODE_NONE) {
		// we are in a target mode; nothing here applies
		return true;
	}

	if (overDoor || overContainer || overInfoPoint) {
		return true;
	}

	if (me.ButtonState(GEM_MB_ACTION) && !isFormationRotation) {
		isSelectionRect = true;
		SetCursor(core->Cursors[IE_CURSOR_PRESSED]);
	}

	return true;
}

bool GameControl::OnTouchDown(const TouchEvent& te, unsigned short mod)
{
	if (EventMgr::NumFingersDown() == 2) {
		// container highlights
		DebugFlags |= DEBUG_SHOW_CONTAINERS|DEBUG_SHOW_DOORS;
	}

	// TODO: check pressure to distinguish between tooltip and HP modes
	if (View::OnTouchDown(te, mod)) {
		if (te.numFingers == 1) {
			screenMousePos = te.Pos();

			// if an actor is being touched show HP
			Actor* actor = GetLastActor();
			if (actor) {
				actor->DisplayHeadHPRatio();
			}
		}
		return true;
	}
	return false;
}

bool GameControl::OnTouchUp(const TouchEvent& te, unsigned short mod)
{
	if (EventMgr::ModState(GEM_MOD_ALT) == false) {
		DebugFlags &= ~(DEBUG_SHOW_CONTAINERS|DEBUG_SHOW_DOORS);
	}

	return View::OnTouchUp(te, mod);
}

bool GameControl::OnTouchGesture(const GestureEvent& gesture)
{
	if (gesture.numFingers == 1) {
		if (target_mode != TARGET_MODE_NONE) {
			// we are in a target mode; nothing here applies
			return true;
		}

		screenMousePos = gesture.Pos();
		isSelectionRect = true;
	} else if (gesture.numFingers == 2) {
		if (gesture.dTheta < -0.2 || gesture.dTheta > 0.2) { // TODO: actually figure out a good number
			if (EventMgr::ModState(GEM_MOD_ALT) == false) {
				DebugFlags &= ~(DEBUG_SHOW_CONTAINERS|DEBUG_SHOW_DOORS);
			}

			isSelectionRect = false;

			if (core->GetGame()->selected.size() <= 1) {
				isFormationRotation = false;
			} else {
				screenMousePos = gesture.fingers[1].Pos();
				InitFormation(screenMousePos);
			}
		} else { // scroll viewport
			MoveViewportTo( vpOrigin - gesture.Delta(), false );
		}
	} else if (gesture.numFingers == 3) { // keyboard/console
		Video* video = core->GetVideoDriver();

		enum class SWIPE { DOWN = -1, NONE = 0, UP = 1 };
		SWIPE swipe = SWIPE::NONE;
		if (gesture.deltaY < -EventMgr::mouseDragRadius) {
			swipe = SWIPE::UP;
		} else if (gesture.deltaY > EventMgr::mouseDragRadius) {
			swipe = SWIPE::DOWN;
		}

		Window* consoleWin = GemRB::GetWindow(0, "WIN_CON");
		assert(consoleWin);

		// swipe up to show the keyboard
		// if the kwyboard is showing swipe up to access console
		// swipe down to hide both keyboard and console
		switch (swipe) {
			case SWIPE::DOWN:
				consoleWin->Close();
				video->StopTextInput();
				consoleWin->Close();
				break;
			case SWIPE::UP:
				if (video->InTextInput()) {
					consoleWin->Focus();
				}
				video->StartTextInput();
				break;
			case SWIPE::NONE:
				break;
		}
	}
	return true;
}

Point GameControl::GameMousePos() const
{
	return vpOrigin + ConvertPointFromScreen(screenMousePos);
}

bool GameControl::OnGlobalMouseMove(const Event& e)
{
	// we are using the window->IsDisabled on purpose
	// to avoid bugs, we are disabling the window when we open one of the "top window"s
	// GC->IsDisabled is for other uses
	if (!window || window->IsDisabled() || (Flags()&IgnoreEvents)) {
		return false;
	}
	
	if (e.mouse.ButtonState(GEM_MB_MIDDLE)) {
		// if we are panning the map don't scroll from being at the edge
		vpVector.reset();
		return false;
	}
	
#define SCROLL_AREA_WIDTH 5
	Region mask = frame;
	mask.x += SCROLL_AREA_WIDTH;
	mask.y += SCROLL_AREA_WIDTH;
	mask.w -= SCROLL_AREA_WIDTH*2;
	mask.h -= SCROLL_AREA_WIDTH*2;
#undef SCROLL_AREA_WIDTH

	screenMousePos = e.mouse.Pos();
	Point mp = ConvertPointFromScreen(screenMousePos);
	int mousescrollspd = core->GetMouseScrollSpeed();

	if (mp.x < mask.x) {
		vpVector.x = -mousescrollspd;
	} else if (mp.x > mask.x + mask.w) {
		vpVector.x = mousescrollspd;
	} else {
		vpVector.x = 0;
	}

	if (mp.y < mask.y) {
		vpVector.y = -mousescrollspd;
	} else if (mp.y > mask.y + mask.h) {
		vpVector.y = mousescrollspd;
	} else {
		vpVector.y = 0;
	}
	
	if (!vpVector.IsZero()) {
		// cancel any scripted moves
		// we are not in dialog or cutscene mode anymore
		// and the user is attempting to move the viewport
		core->timer.SetMoveViewPort(vpOrigin, 0, false);
	}
	
	return true;
}

void GameControl::MoveViewportUnlockedTo(Point p, bool center)
{
	Point half(frame.w / 2, frame.h / 2);
	if (center) {
		p -= half;
	}
	
	core->GetAudioDrv()->UpdateListenerPos(p + half);
	vpOrigin = p;
}

bool GameControl::MoveViewportTo(Point p, bool center, int speed)
{
	const Map* area = CurrentArea();
	bool canMove = area != NULL;

	if (updateVPTimer && speed) {
		updateVPTimer = false;
		core->timer.SetMoveViewPort(p, speed, center);
	} else if (canMove && p != vpOrigin) {
		updateVPTimer = true;

		Size mapsize = area->GetSize();

		if (center) {
			p.x -= frame.w/2;
			p.y -= frame.h/2;
		}

		// TODO: make the overflow more dynamic
		if (frame.w >= mapsize.w + 64) {
			p.x = (mapsize.w - frame.w)/2;
			canMove = false;
		} else if (p.x + frame.w >= mapsize.w + 64) {
			p.x = mapsize.w - frame.w + 64;
			canMove = false;
		} else if (p.x < -64) {
			p.x = -64;
			canMove = false;
		}

		int mwinh = 0;
		const TextArea* mta = core->GetMessageTextArea();
		if (mta) {
			mwinh = mta->GetWindow()->Frame().h;
		}

		constexpr int padding = 50;
		if (frame.h >= mapsize.h + mwinh + padding) {
			p.y = (mapsize.h - frame.h)/2 + padding;
			canMove = false;
		} else if (p.y + frame.h >= mapsize.h + mwinh + padding) {
			p.y = mapsize.h - frame.h + mwinh + padding;
			canMove = false;
		} else if (p.y < 0) {
			p.y = 0;
			canMove = false;
		}

		MoveViewportUnlockedTo(p, false); // we already handled centering
	} else {
		updateVPTimer = true;
		canMove = (p != vpOrigin);
	}

	return canMove;
}

Region GameControl::Viewport() const
{
	return Region(vpOrigin, frame.size);
}

//generate action code for source actor to try to attack a target
void GameControl::TryToAttack(Actor *source, const Actor *tgt) const
{
	if (source->GetStat(IE_SEX) == SEX_ILLUSION) return;
	source->CommandActor(GenerateActionDirect( "NIDSpecial3()", tgt));
}

//generate action code for source actor to try to defend a target
void GameControl::TryToDefend(Actor *source, const Actor *tgt) const
{
	source->SetModal(MS_NONE);
	source->CommandActor(GenerateActionDirect( "NIDSpecial4()", tgt));
}

// generate action code for source actor to try to pick pockets of a target (if an actor)
// else if door/container try to pick a lock/disable trap
// The -1 flag is a placeholder for dynamic target IDs
void GameControl::TryToPick(Actor *source, const Scriptable *tgt) const
{
	source->SetModal(MS_NONE);
	std::string cmdString;
	cmdString.reserve(20);
	switch (tgt->Type) {
		case ST_ACTOR:
			cmdString = "PickPockets([-1])";
			break;
		case ST_DOOR:
		case ST_CONTAINER:
			if (((const Highlightable *) tgt)->Trapped && ((const Highlightable *) tgt)->TrapDetected) {
				cmdString = "RemoveTraps([-1])";
			} else {
				cmdString = "PickLock([-1])";
			}
			break;
		default:
			Log(ERROR, "GameControl", "Invalid pick target of type {}", tgt->Type);
			return;
	}
	source->CommandActor(GenerateActionDirect(std::move(cmdString), tgt));
}

//generate action code for source actor to try to disable trap (only trap type active regions)
void GameControl::TryToDisarm(Actor *source, const InfoPoint *tgt) const
{
	if (tgt->Type!=ST_PROXIMITY) return;

	source->SetModal(MS_NONE);
	source->CommandActor(GenerateActionDirect( "RemoveTraps([-1])", tgt ));
}

//generate action code for source actor to use item/cast spell on a point
void GameControl::TryToCast(Actor *source, const Point &tgt)
{
	if ((target_types&GA_POINT) == false) {
		return; // not allowed to target point
	}

	if (!spellCount) {
		ResetTargetMode();
		return; // not casting or using an own item
	}
	source->Stop();

	spellCount--;
	std::string tmp;
	tmp.reserve(30);
	if (spellOrItem>=0) {
		if (spellIndex<0) {
			tmp = "SpellPointNoDec(\"\",[0.0])";
		} else {
			tmp = "SpellPoint(\"\",[0.0])";
		}
	} else {
		//using item on target
		tmp = "UseItemPoint(\"\",[0,0],0)";
	}
	Action* action = GenerateAction(std::move(tmp));
	action->pointParameter=tgt;
	if (spellOrItem>=0) {
		if (spellIndex<0) {
			action->resref0Parameter = spellName;
		} else {
			const CREMemorizedSpell *si;
			//spell casting at target
			si = source->spellbook.GetMemorizedSpell(spellOrItem, spellSlot, spellIndex);
			if (!si) {
				ResetTargetMode();
				delete action;
				return;
			}
			action->resref0Parameter = si->SpellResRef;
		}
	} else {
		action->int0Parameter = spellSlot;
		action->int1Parameter = spellIndex;
		action->int2Parameter = UI_SILENT;
		//for multi-shot items like BG wand of lightning
		if (spellCount) {
			action->int2Parameter |= UI_NOAURA|UI_NOCHARGE;
		}
	}
	source->AddAction( action );
	if (!spellCount) {
		ResetTargetMode();
	}
}

//generate action code for source actor to use item/cast spell on another actor
void GameControl::TryToCast(Actor *source, const Actor *tgt)
{
	// pst has no aura pollution
	bool aural = true;
	if (spellCount >= 1000) {
		spellCount -= 1000;
		aural = false;
	}

	if (!spellCount) {
		ResetTargetMode();
		return; //not casting or using an own item
	}
	source->Stop();

	// cannot target spells on invisible or sanctuaried creatures
	// invisible actors are invisible, so this is usually impossible by itself, but improved invisibility changes that
	if (source != tgt && tgt->Untargetable(spellName)) {
		displaymsg->DisplayConstantStringName(HCStrings::NoSeeNoCast, GUIColors::RED, source);
		ResetTargetMode();
		return;
	}

	spellCount--;
	std::string tmp;
	tmp.reserve(20);
	if (spellOrItem>=0) {
		if (spellIndex<0) {
			tmp = "NIDSpecial7()";
		} else {
			tmp = "NIDSpecial6()";
		}
	} else {
		//using item on target
		tmp = "NIDSpecial5()";
	}
	Action* action = GenerateActionDirect(std::move(tmp), tgt);
	if (spellOrItem>=0) {
		if (spellIndex<0) {
			action->resref0Parameter = spellName;
		} else {
			const CREMemorizedSpell *si;
			//spell casting at target
			si = source->spellbook.GetMemorizedSpell(spellOrItem, spellSlot, spellIndex);
			if (!si) {
				ResetTargetMode();
				delete action;
				return;
			}
			action->resref0Parameter = si->SpellResRef;
		}
	} else {
		action->int0Parameter = spellSlot;
		action->int1Parameter = spellIndex;
		action->int2Parameter = UI_SILENT;
		if (!aural) {
			action->int2Parameter |= UI_NOAURA;
		}
		//for multi-shot items like BG wand of lightning
		if (spellCount) {
			action->int2Parameter |= UI_NOAURA|UI_NOCHARGE;
		}
	}
	source->AddAction( action );
	if (!spellCount) {
		ResetTargetMode();
	}
}

//generate action code for source actor to use talk to target actor
void GameControl::TryToTalk(Actor *source, const Actor *tgt) const
{
	if (source->GetStat(IE_SEX) == SEX_ILLUSION) return;
	//Nidspecial1 is just an unused action existing in all games
	//(non interactive demo)
	//i found no fitting action which would emulate this kind of
	//dialog initation
	source->SetModal(MS_NONE);
	dialoghandler->SetTarget(tgt); //this is a hack, but not so deadly
	source->CommandActor(GenerateActionDirect( "NIDSpecial1()", tgt));
}

//generate action code for actor appropriate for the target mode when the target is a container
void GameControl::HandleContainer(Container *container, Actor *actor)
{
	if (actor->GetStat(IE_SEX) == SEX_ILLUSION) return;
	//container is disabled, it should not react
	if (container->Flags & CONT_DISABLED) {
		return;
	}

	if ((target_mode == TARGET_MODE_CAST) && spellCount) {
		//we'll get the container back from the coordinates
		TryToCast(actor, container->Pos);
		//Do not reset target_mode, TryToCast does it for us!!
		return;
	}

	core->SetEventFlag(EF_RESETTARGET);

	if (target_mode == TARGET_MODE_ATTACK) {
		std::string Tmp = fmt::format("BashDoor(\"{}\")", container->GetScriptName());
		actor->CommandActor(GenerateAction(std::move(Tmp)));
		return;
	}

	if (target_mode == TARGET_MODE_PICK) {
		TryToPick(actor, container);
		return;
	}

	container->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
	core->SetCurrentContainer( actor, container);
	actor->CommandActor(GenerateAction("UseContainer()"));
}

//generate action code for actor appropriate for the target mode when the target is a door
void GameControl::HandleDoor(Door *door, Actor *actor)
{
	if (actor->GetStat(IE_SEX) == SEX_ILLUSION) return;
	if ((target_mode == TARGET_MODE_CAST) && spellCount) {
		//we'll get the door back from the coordinates
		const Point *p = door->toOpen;
		const Point *otherp = door->toOpen+1;
		if (Distance(*p,actor)>Distance(*otherp,actor)) {
			p=otherp;
		}
		TryToCast(actor, *p);
		return;
	}

	core->SetEventFlag(EF_RESETTARGET);

	if (target_mode == TARGET_MODE_ATTACK) {
		std::string Tmp = fmt::format("BashDoor(\"{}\")", door->GetScriptName());
		actor->CommandActor(GenerateAction(std::move(Tmp)));
		return;
	}

	if (target_mode == TARGET_MODE_PICK) {
		TryToPick(actor, door);
		return;
	}

	door->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
	actor->TargetDoor = door->GetGlobalID();
	// internal gemrb toggle door action hack - should we use UseDoor instead?
	actor->CommandActor(GenerateAction("NIDSpecial9()"));
}

//generate action code for actor appropriate for the target mode when the target is an active region (infopoint, trap or travel)
bool GameControl::HandleActiveRegion(InfoPoint *trap, Actor * actor, const Point& p)
{
	if (actor->GetStat(IE_SEX) == SEX_ILLUSION) return false;
	if ((target_mode == TARGET_MODE_CAST) && spellCount) {
		//we'll get the active region from the coordinates (if needed)
		TryToCast(actor, p);
		//don't bother with this region further
		return true;
	}
	if (target_mode == TARGET_MODE_PICK) {
		TryToDisarm(actor, trap);
		return true;
	}

	switch(trap->Type) {
		case ST_TRAVEL:
			trap->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
			actor->LastMarked = trap->GetGlobalID();
			//clear the go closer flag
			trap->GetCurrentArea()->LastGoCloser = 0;
			return false;
		case ST_TRIGGER:
			// pst, eg. ar1500
			if (!trap->GetDialog().IsEmpty()) {
				trap->AddAction(GenerateAction("Dialogue([PC])"));
				return true;
			}

			// always display overhead text; totsc's ar0511 library relies on it
			DisplayString(trap);

			//the importer shouldn't load the script
			//if it is unallowed anyway (though
			//deactivated scripts could be reactivated)
			//only the 'trapped' flag should be honoured
			//there. Here we have to check on the
			//reset trap and deactivated flags
			if (trap->Scripts[0]) {
				if (!(trap->Flags & TRAP_DEACTIVATED) && !(GetDialogueFlags() & DF_FREEZE_SCRIPTS)) {
					trap->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
					actor->LastMarked = trap->GetGlobalID();
					//directly feeding the event, even if there are actions in the queue
					//trap->Scripts[0]->Update();
					// FIXME
					trap->ExecuteScript(1);
					trap->ProcessActions();
				}
			}

			// bleh, pst has this infopoint that needs to trigger, but covers two others that also do
			if (trap->Flags & TRAP_SILENT) {
				// recheck if there are other infopoints at this position
				const Map* map = trap->GetCurrentArea();
				InfoPoint* ip2 = map->TMap->GetInfoPoint(p, true);
				DisplayString(ip2);
			}

			if (trap->GetUsePoint() ) {
				std::string Tmp = fmt::format("TriggerWalkTo(\"{}\")", trap->GetScriptName());
				actor->CommandActor(GenerateAction(std::move(Tmp)));
				return true;
			}
			return true;
		default:;
	}
	return false;
}

// Calculate the angle between a clicked point and the first selected character,
// so that we can set a sensible orientation for the formation.
void GameControl::InitFormation(const Point &clickPoint)
{
	// Of course single actors don't get rotated, but we need to ensure
	// isFormationRotation is set in all cases where we initiate movement,
	// since OnMouseUp tests for it.
	if (isFormationRotation || core->GetGame()->selected.empty()) {
		return;
	}

	const Actor *selectedActor = GetMainSelectedActor();

	isFormationRotation = true;
	formationBaseAngle = AngleFromPoints(clickPoint, selectedActor->Pos);
	SetCursor(core->Cursors[IE_CURSOR_USE]);
}

void GameControl::TryDefaultTalk() const
{
	Actor* targetActor = GetLastActor();
	if (targetActor) {
		targetActor->PlaySelectionSound(true);
	}
}

/** Mouse Button Down */
bool GameControl::OnMouseDown(const MouseEvent& me, unsigned short Mod)
{
	if (Mod & GEM_MOD_CTRL) { // debug paint mode
		return true;
	}

	Point p = ConvertPointFromScreen(me.Pos());
	gameClickPoint = p + vpOrigin;

	switch(me.button) {
	case GEM_MB_MENU: //right click.
		if (core->HasFeature(GFFlags::HAS_FLOAT_MENU) && !Mod) {
			core->GetGUIScriptEngine()->RunFunction("GUICommon", "OpenFloatMenuWindow", p, false);
		} else {
			TryDefaultTalk();
		}
		break;
	case GEM_MB_ACTION:
		// PST uses alt + left click for formation rotation
		// is there any harm in this being true in all games?
		if (me.repeats != 2 && EventMgr::ModState(GEM_MOD_ALT)) {
			InitFormation(gameClickPoint);
		}

		break;
	}
	return true;
}

// list of allowed area and exit combinations in pst that trigger worldmap travel
static const ResRefMap<std::vector<ResRef>> pstWMapExits {
	{"ar0100", {"to0300", "to0200", "to0101"}},
	{"ar0101", {"to0100"}},
	{"ar0200", {"to0100", "to0301", "to0400"}},
	{"ar0300", {"to0100", "to0301", "to0400"}},
	{"ar0301", {"to0200", "to0300"}},
	{"ar0400", {"to0200", "to0300"}},
	{"ar0500", {"to0405", "to0600"}},
	{"ar0600", {"to0500"}}
};

// pst: determine if we need to trigger worldmap travel, since it had it's own system
// eg. it doesn't use the searchmap for this in ar0500 when travelling globally
// has to be a plain travel region and on the whitelist
bool GameControl::ShouldTriggerWorldMap(const Actor *pc) const
{
	if (!core->HasFeature(GFFlags::TEAM_MOVEMENT)) return false;

	bool keyAreaVisited = CheckVariable(pc, "AR0500_Visited", "GLOBAL") == 1;
	if (!keyAreaVisited) return false;

	bool teamMoved = (pc->GetInternalFlag() & IF_USEEXIT) && overInfoPoint && overInfoPoint->Type == ST_TRAVEL;
	if (!teamMoved) return false;

	teamMoved = false;
	auto wmapExits = pstWMapExits.find(pc->GetCurrentArea()->GetScriptRef());
	if (wmapExits != pstWMapExits.end()) {
		for (const auto& exit : wmapExits->second) {
			if (exit == overInfoPoint->GetScriptName()) {
				teamMoved = true;
				break;
			}
		}
	}

	return teamMoved;
}

/** Mouse Button Up */
bool GameControl::OnMouseUp(const MouseEvent& me, unsigned short Mod)
{
	if (Mod & GEM_MOD_CTRL) {
		Point p = ConvertPointFromScreen(me.Pos());
		DebugPaint(p + vpOrigin, me.button == GEM_MB_MENU);
		return true;
	}
	
	//heh, i found no better place
	core->CloseCurrentContainer();

	Point p = ConvertPointFromScreen(me.Pos()) + vpOrigin;
	bool isDoubleClick = me.repeats == 2;
	bool tryToRun = isDoubleClick;
	if (core->HasFeature(GFFlags::HAS_FLOAT_MENU)) {
		tryToRun |= Mod & GEM_MOD_SHIFT;
	}

	// right click
	if (me.button == GEM_MB_MENU) {
		ieDword actionLevel;
		core->GetDictionary()->Lookup("ActionLevel", actionLevel);
		if (target_mode != TARGET_MODE_NONE || actionLevel) {
			if (!core->HasFeature(GFFlags::HAS_FLOAT_MENU)) {
				SetTargetMode(TARGET_MODE_NONE);
			}
			// update the action bar
			core->GetDictionary()->SetAt("ActionLevel", 0, false);
			core->SetEventFlag(EF_ACTION);
			ClearMouseState();
			return true;
		} else {
			p = gameClickPoint;
		}
	} else if (me.button == GEM_MB_MIDDLE) {
		// do nothing, so middle button panning doesn't trigger a move
		return true;
	} else {
		// any other button behaves as left click (scrollwhell buttons are mouse wheel events now)
		if (isDoubleClick)
			MoveViewportTo(p, true);

		// handle actions
		if (target_mode == TARGET_MODE_NONE && lastActorID) {
			switch (lastCursor & ~IE_CURSOR_GRAY) {
				case IE_CURSOR_TALK:
					SetTargetMode(TARGET_MODE_TALK);
					break;
				case IE_CURSOR_ATTACK:
					SetTargetMode(TARGET_MODE_ATTACK);
					break;
				case IE_CURSOR_CAST:
					SetTargetMode(TARGET_MODE_CAST);
					break;
				case IE_CURSOR_DEFEND:
					SetTargetMode(TARGET_MODE_DEFEND);
					break;
				case IE_CURSOR_PICK:
					SetTargetMode(TARGET_MODE_PICK);
					break;
				default: break;
			}
		}

		if (target_mode == TARGET_MODE_NONE && (isSelectionRect || lastActorID)) {
			MakeSelection(Mod & GEM_MOD_SHIFT);
			ClearMouseState();
			return true;
		}

		if (lastCursor == IE_CURSOR_BLOCKED) {
			// don't allow travel if the destination is actually blocked
			return false;
		}
		
		if (overContainer || overDoor || (overInfoPoint && overInfoPoint->Type==ST_TRAVEL && target_mode == TARGET_MODE_NONE)) {
			// move to the object before trying to interact with it
			Actor* mainActor = GetMainSelectedActor();
			if (mainActor && overContainer) {
				CreateMovement(mainActor, p, false, tryToRun); // let one actor handle loot and containers
			} else {
				CommandSelectedMovement(p, false, tryToRun);
			}
		}
		
		if (target_mode != TARGET_MODE_NONE || overInfoPoint || overContainer || overDoor) {
			PerformSelectedAction(p);
			ClearMouseState();
			return true;
		}

		// Ensure that left-click movement also orients the formation
		// in the direction of movement.
		InitFormation(p);
	}

	// handle movement/travel, but not if we just opened the float window
	if ((!core->HasFeature(GFFlags::HAS_FLOAT_MENU) || me.button != GEM_MB_MENU) && lastCursor != IE_CURSOR_BLOCKED && lastCursor != IE_CURSOR_NORMAL) {
		// pst has different mod keys
		int modKey = GEM_MOD_SHIFT;
		if (core->HasFeature(GFFlags::HAS_FLOAT_MENU)) modKey = GEM_MOD_CTRL;
		CommandSelectedMovement(p, Mod & modKey, tryToRun);
	}
	ClearMouseState();
	return true;
}

void GameControl::PerformSelectedAction(const Point& p)
{
	// TODO: consolidate the 'over' members into a single Scriptable*
	// then we simply switch on its type

	const Game* game = core->GetGame();
	const Map* area = game->GetCurrentArea();
	Actor* targetActor = area->GetActor(p, target_types & ~GA_NO_HIDDEN);
	if (targetActor && targetActor->GetStat(IE_NOCIRCLE) == 0) {
		PerformActionOn(targetActor);
		return;
	}

	Actor* selectedActor = GetMainSelectedActor();
	if (!selectedActor) {
		return;
	}

	//add a check if you don't want some random monster handle doors and such
	if (target_mode == TARGET_MODE_CAST) {
		//the player is using an item or spell on the ground
		TryToCast(selectedActor, p);
	} else if (overDoor) {
		HandleDoor(overDoor, selectedActor);
	} else if (overContainer) {
		HandleContainer(overContainer, selectedActor);
	} else if (overInfoPoint) {
		if (overInfoPoint->Type==ST_TRAVEL && target_mode == TARGET_MODE_NONE) {
			ieDword exitID = overInfoPoint->GetGlobalID();
			if (core->HasFeature(GFFlags::TEAM_MOVEMENT)) {
				// pst forces everyone to travel (eg. ar0201 outside_portal)
				int i = game->GetPartySize(false);
				while(i--) {
					game->GetPC(i, false)->UseExit(exitID);
				}
			} else {
				size_t i = game->selected.size();
				while(i--) {
					game->selected[i]->UseExit(exitID);
				}
			}
			CommandSelectedMovement(p);
		}
		if (HandleActiveRegion(overInfoPoint, selectedActor, p)) {
			core->SetEventFlag(EF_RESETTARGET);
		}
	}
}

void GameControl::CommandSelectedMovement(const Point& p, bool append, bool tryToRun) const
{
	const Game* game = core->GetGame();

	// construct a sorted party
	std::vector<Actor *> party;
	// first, from the actual party
	int max = game->GetPartySize(false);
	for (int idx = 1; idx <= max; idx++) {
		Actor *act = game->FindPC(idx);
		assert(act);
		if (act->IsSelected()) {
			party.push_back(act);
		}
	}
	// then summons etc.
	for (Actor *selected : game->selected) {
		if (!selected->InParty) {
			party.push_back(selected);
		}
	}
	
	if (party.empty())
		return;

	double angle = isFormationRotation ? AngleFromPoints(GameMousePos(), p) : formationBaseAngle;
	bool doWorldMap = ShouldTriggerWorldMap(party[0]);
	
	std::vector<Point> formationPoints = GetFormationPoints(p, party, angle);
	for (size_t i = 0; i < party.size(); i++) {
		Actor *actor = party[i];
		// don't stop the party if we're just trying to add a waypoint
		if (!append) {
			actor->Stop();
		}
		
		if (party.size() > 1) {
			CreateMovement(actor, formationPoints[i], append, tryToRun);
		} else {
			CreateMovement(actor, p, append, tryToRun);
		}
		
		// don't trigger the travel region, so everyone can bunch up there and NIDSpecial2 can take over
		if (doWorldMap) actor->SetInternalFlag(IF_PST_WMAPPING, BitOp::OR);
	}

	// p is a searchmap travel region or a plain travel region in pst (matching several other criteria)
	if (party[0]->GetCurrentArea()->GetCursor(p) == IE_CURSOR_TRAVEL || doWorldMap) {
		party[0]->AddAction(GenerateAction("NIDSpecial2()"));
	}
}
bool GameControl::OnMouseWheelScroll(const Point& delta)
{
	// Game coordinates start at the top left to the bottom right
	// so we need to invert the 'y' axis
	Point d = delta;
	d.y *= -1;
	Scroll(d);
	return true;
}

bool GameControl::OnControllerButtonDown(const ControllerEvent& ce)
{
	switch (ce.button) {
		case CONTROLLER_BUTTON_Y:
			return !core->GetGUIScriptEngine()->RunFunction("GUIINV", "ToggleInventoryWindow", false).IsNull();
		case CONTROLLER_BUTTON_X:
			return !core->GetGUIScriptEngine()->RunFunction("GUIMA", "ToggleMapWindow", false).IsNull();
		case CONTROLLER_BUTTON_BACK:
			core->SetEventFlag(EF_ACTION|EF_RESETTARGET);
			return true;
		default:
			return View::OnControllerButtonDown(ce);
	}
}

void GameControl::Scroll(const Point& amt)
{
	MoveViewportTo(vpOrigin + amt, false);
}

void GameControl::PerformActionOn(Actor *actor)
{
	const Game* game = core->GetGame();

	//determining the type of the clicked actor
	ieDword type = actor->GetStat(IE_EA);
	if (type >= EA_EVILCUTOFF || type == EA_GOODBUTRED) {
		type = ACT_ATTACK; //hostile
	} else if (type > EA_CHARMED) {
		type = ACT_TALK; //neutral
	} else {
		type = ACT_NONE; //party
	}

	if (target_mode == TARGET_MODE_ATTACK) {
		type = ACT_ATTACK;
	} else if (target_mode == TARGET_MODE_TALK) {
		type = ACT_TALK;
	} else if (target_mode == TARGET_MODE_CAST) {
		type = ACT_CAST;
	} else if (target_mode == TARGET_MODE_DEFEND) {
		type = ACT_DEFEND;
	} else if (target_mode == TARGET_MODE_PICK) {
		type = ACT_THIEVING;
	}

	if (type != ACT_NONE && !actor->ValidTarget(target_types)) {
		return;
	}

	//we shouldn't zero this for two reasons in case of spell or item
	//1. there could be multiple targets
	//2. the target mode is important
	if (!(target_mode == TARGET_MODE_CAST) || !spellCount) {
		ResetTargetMode();
	}

	switch (type) {
		case ACT_NONE: //none
			if (!actor->ValidTarget(GA_SELECT)) {
				return;
			}

			if (actor->InParty)
				SelectActor( actor->InParty );
			else if (actor->GetStat(IE_EA) <= EA_CHARMED) {
				/*let's select charmed/summoned creatures
				EA_CHARMED is the maximum value known atm*/
				core->GetGame()->SelectActor(actor, true, SELECT_REPLACE);
			}
			break;
		case ACT_TALK:
			if (!actor->ValidTarget(GA_TALK)) {
				return;
			}

			//talk (first selected talks)
			if (!game->selected.empty()) {
				//if we are in PST modify this to NO!
				Actor *source;
				if (core->HasFeature(GFFlags::PROTAGONIST_TALKS) ) {
					source = game->GetPC(0, false); //protagonist
				} else {
					source = core->GetFirstSelectedPC(false);
				}
				// only party members can start conversations
				if (source) {
					TryToTalk(source, actor);
				}
			}
			break;
		case ACT_ATTACK:
			//all of them attacks the red circled actor
			for (Actor *selectee : game->selected) {
				TryToAttack(selectee, actor);
			}
			break;
		case ACT_CAST: //cast on target or use item on target
			if (game->selected.size()==1) {
				Actor *source = core->GetFirstSelectedActor();
				if (source) {
					TryToCast(source, actor);
				}
			}
			break;
		case ACT_DEFEND:
			for (Actor *selectee : game->selected) {
				TryToDefend(selectee, actor);
			}
			break;
		case ACT_THIEVING:
			if (game->selected.size()==1) {
				Actor *source = core->GetFirstSelectedActor();
				if (source) {
					TryToPick(source, actor);
				}
			}
			break;
	}
}

//sets target mode, and resets the cursor
void GameControl::SetTargetMode(int mode) {
	target_mode = mode;
}

void GameControl::ResetTargetMode() {
	target_types = GA_NO_DEAD|GA_NO_HIDDEN|GA_NO_UNSCHEDULED;
	SetTargetMode(TARGET_MODE_NONE);
}

void GameControl::UpdateTargetMode() {
	SetTargetMode(target_mode);
}

Region GameControl::SelectionRect() const
{
	Point pos = GameMousePos();
	if (isSelectionRect) {
		return Region::RegionFromPoints(pos, gameClickPoint);
	}
	return Region(pos.x, pos.y, 1, 1);
}

void GameControl::MakeSelection(bool extend)
{
	Game* game = core->GetGame();

	if (!extend && !highlighted.empty()) {
		game->SelectActor( NULL, false, SELECT_NORMAL );
	}

	std::vector<Actor*>::iterator it = highlighted.begin();
	for (; it != highlighted.end(); ++it) {
		Actor* act = *it;
		act->SetOver(false);
		game->SelectActor(act, true, SELECT_NORMAL);
	}
}

void GameControl::SetCutSceneMode(bool active)
{
	WindowManager* wm = core->GetWindowManager();
	if (active) {
		ScreenFlags |= SF_CUTSCENE;
		vpVector.reset();
		wm->SetCursorFeedback(WindowManager::MOUSE_NONE);
	} else {
		ScreenFlags &= ~SF_CUTSCENE;
		wm->SetCursorFeedback(WindowManager::CursorFeedback(core->config.MouseFeedback));
	}
	SetFlags(IgnoreEvents, (active || InDialog()) ? BitOp::OR : BitOp::NAND);
}

//Create an overhead text over a scriptable target
//Multiple texts are possible, as this code copies the text to a new object
void GameControl::DisplayString(Scriptable* target) const
{
	if (!target || target->overHead.Empty() || target->overHead.IsDisplaying()) {
		return;
	}

	// add as a "subtitle" to the main message window
	ieDword tmp = 0;
	core->GetDictionary()->Lookup("Duplicate Floating Text", tmp);
	if (tmp) {
		displaymsg->DisplayString(target->overHead.GetText());
	}
	target->overHead.Display(true, 0);
}

/** changes displayed map to the currently selected PC */
void GameControl::ChangeMap(const Actor *pc, bool forced)
{
	//swap in the area of the actor
	Game* game = core->GetGame();
	if (forced || (pc && pc->Area != game->CurrentArea)) {
		// disable so that drawing and events dispatched doesn't happen while there is not an area
		// we are single threaded, but game loading runs its own event loop which will cause events/drawing to come in
		SetDisabled(true);
		ClearMouseState();

		dialoghandler->EndDialog();
		overInfoPoint = NULL;
		overContainer = NULL;
		overDoor = NULL;
		/*this is loadmap, because we need the index, not the pointer*/
		if (pc) {
			game->GetMap(pc->Area, true);
		} else {
			ResRef oldMaster = game->LastMasterArea; // only update it for party travel
			game->GetMap(game->CurrentArea, true);
			game->LastMasterArea = oldMaster;
		}

		if (!core->InCutSceneMode()) {
			// don't interfere with any scripted moves of the viewport
			// checking core->timer->ViewportIsMoving() is not enough
			ScreenFlags |= SF_CENTERONACTOR;
		}
		
		SetDisabled(false);

		if (window) {
			window->Focus();
		}
	}
	//center on first selected actor
	if (pc && (ScreenFlags&SF_CENTERONACTOR)) {
		MoveViewportTo( pc->Pos, true );
		ScreenFlags&=~SF_CENTERONACTOR;
	}
}

void GameControl::FlagsChanged(unsigned int /*oldflags*/)
{
	if (Flags()&IgnoreEvents) {
		ClearMouseState();
		vpVector.reset();
	}
}

bool GameControl::SetScreenFlags(unsigned int value, BitOp mode)
{
	return SetBits(ScreenFlags, value, mode);
}

void GameControl::SetDialogueFlags(unsigned int value, BitOp mode)
{
	SetBits(DialogueFlags, value, mode);
	SetFlags(IgnoreEvents, (DialogueFlags&DF_IN_DIALOG || ScreenFlags&SF_CUTSCENE) ? BitOp::OR : BitOp::NAND);
}

Map* GameControl::CurrentArea() const
{
	const Game *game = core->GetGame();
	if (game) {
		return game->GetCurrentArea();
	}
	return NULL;
}

Actor *GameControl::GetLastActor() const
{
	Actor* actor = nullptr;
	const Map* area = CurrentArea();
	if (area) {
		actor = area->GetActorByGlobalID(lastActorID);
	}
	return actor;
}

void GameControl::SetLastActor(Actor* lastActor)
{
	if (lastActorID) {
		const Map* area = CurrentArea();
		if (area == NULL) {
			return;
		}

		Actor* current = area->GetActorByGlobalID(lastActorID);
		if (current)
			current->SetOver(false);
		lastActorID = 0;
	}

	if (lastActor) {
		lastActorID = lastActor->GetGlobalID();
		lastActor->SetOver(true);
	}
}

//Set up an item use which needs targeting
//Slot is an inventory slot
//header is the used item extended header
//u is the user
//target type is a bunch of GetActor flags that alter valid targets
//cnt is the number of different targets (usually 1)
void GameControl::SetupItemUse(int slot, size_t header, Actor *u, int targettype, int cnt)
{
	spellName.Reset();
	spellOrItem = -1;
	spellUser = u;
	spellSlot = slot;
	spellIndex = static_cast<int>(header);
	//item use also uses the casting icon, this might be changed in some custom game?
	SetTargetMode(TARGET_MODE_CAST);
	target_types = targettype;
	spellCount = cnt;
}

//Set up spell casting which needs targeting
//type is the spell's type
//level is the caster level
//idx is the spell's number
//u is the caster
//target type is a bunch of GetActor flags that alter valid targets
//cnt is the number of different targets (usually 1)
void GameControl::SetupCasting(const ResRef& spellname, int type, int level, int idx, Actor *u, int targettype, int cnt)
{
	spellName = spellname;
	spellOrItem = type;
	spellUser = u;
	spellSlot = level;
	spellIndex = idx;
	SetTargetMode(TARGET_MODE_CAST);
	target_types = targettype;
	spellCount = cnt;
}

void GameControl::SetDisplayText(const String& text, unsigned int time)
{
	DisplayTextTime = time;
	DisplayText = text;
}

void GameControl::SetDisplayText(HCStrings text, unsigned int time)
{
	SetDisplayText(core->GetString(DisplayMessage::GetStringReference(text), STRING_FLAGS::NONE), time);
}

void GameControl::ToggleAlwaysRun()
{
	AlwaysRun = !AlwaysRun;
	core->GetDictionary()->SetAt("Always Run", AlwaysRun);
}

int GameControl::GetOverheadOffset() const
{
	const Actor* actor = GetLastActor();
	if (actor) {
		return actor->overHead.GetHeightOffset();
	}
	return 0;
}

}
