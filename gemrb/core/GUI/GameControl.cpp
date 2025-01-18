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

#include "ie_cursors.h"
#include "opcode_params.h"
#include "strrefs.h"

#include "CharAnimations.h"
#include "DialogHandler.h"
#include "DisplayMessage.h"
#include "GUIScriptInterface.h"
#include "Game.h"
#include "GameData.h"
#include "GlobalTimer.h"
#include "ImageMgr.h"
#include "Interface.h"
#include "KeyMap.h"
#include "PathFinder.h"
#include "RNG.h"
#include "ScriptEngine.h"
#include "TileMap.h"
#include "damages.h"

#include "GUI/Button.h"
#include "GUI/EventMgr.h"
#include "GUI/TextArea.h"
#include "GUI/WindowManager.h"
#include "GameScript/GSUtils.h"
#include "Scriptable/Container.h"
#include "Scriptable/Door.h"
#include "Scriptable/InfoPoint.h"
#include "Video/Video.h"
#include "fmt/ranges.h"

#include <array>
#include <cmath>

namespace GemRB {

constexpr uint8_t FORMATIONSIZE = 10;
using formation_t = std::array<Point, FORMATIONSIZE>;

class Formations {
	std::vector<formation_t> formations;

	Formations() noexcept
	{
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
				Point p(tab->QueryFieldSigned<int>(i, j * 2), tab->QueryFieldSigned<int>(i, j * 2 + 1));
				formations[i][j] = p;
			}
		}
	}

public:
	static const formation_t& GetFormation(size_t formation) noexcept
	{
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
void GameControl::SetTracker(const Actor* actor, ieDword dist)
{
	trackerID = actor->GetGlobalID();
	distance = dist;
}

GameControl::GameControl(const Region& frame)
	: View(frame)
{
	this->AlwaysRun = core->GetDictionary().Get("Always Run", 0);

	ResetTargetMode();
	SetCursor(nullptr);

	int lookup = core->GetDictionary().Get("Center", 0);
	if (lookup) {
		screenFlags.Set(ScreenFlags::AlwaysCenter);
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

Point GameControl::GetFormationPoint(const Point& origin, size_t pos, float_t angle, const std::vector<Point>& exclude) const
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
	float_t stepAngle = 0.0;
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
			return p.IsWithinRadius(radius, dest);
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
							     float_t angle) const
{
	FormationPoints formation;
	for (size_t i = 0; i < actors.size(); ++i) {
		formation.emplace_back(GetFormationPoint(origin, i, angle, formation));
	}
	return formation;
}

void GameControl::DrawFormation(const std::vector<Actor*>& actors, const Point& formationPoint, float_t angle) const
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

	SetCursor(nullptr);
}

// generate an action to do the actual movement
// only PST supports RunToPoint
void GameControl::CreateMovement(Actor* actor, const Point& p, bool append, bool tryToRun) const
{
	Action* action = nullptr;
	tryToRun = tryToRun || AlwaysRun;

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
			if (overMe && overMe->Type == ST_TRAVEL) {
				// gemrb extension to make travel through nearly blocked regions less painful
				action->int0Parameter = MAX_TRAVELING_DISTANCE / 10; // empirical
			}
		}
	}

	actor->CommandActor(action, !append);
	actor->Destination = p; // just to force target reticle drawing if paused
}

// can we handle it (no movement impairments)?
bool GameControl::CanRun(const Actor* actor) const
{
	if (!actor) return false;
	static bool hasRun = GenerateActionDirect("RunToPoint([0.0])", actor) != nullptr;
	if (!hasRun) return false;
	return actor->GetEncumbranceFactor(true) == 1;
}

bool GameControl::ShouldRun(const Actor* actor) const
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

	orient_t dir = GetOrient(bounds.Center(), p);
	Holder<Sprite2D> arrow = core->GetScrollCursorSprite(dir, 0);

	const Point& dp = bounds.Intercept(p) - bounds.origin;
	VideoDriver->BlitGameSprite(arrow, dp, BlitFlags::COLOR_MOD | BlitFlags::BLENDED, color);
}

void GameControl::DrawTargetReticle(uint16_t size, const Color& color, const Point& p, int radialOffset) const
{
	uint8_t offset = GlobalColorCycle.Step() >> 1;
	BasePoint offsetH = BasePoint(offset, 0);
	BasePoint offsetV = BasePoint(0, offset);

	/* segments should not go outside selection radius */
	uint16_t xradius = (size * 4) - 5 + radialOffset;
	uint16_t yradius = (size * 3) - 5 + radialOffset;
	const Size s(xradius * 2, yradius * 2);
	const Region r(p - s.Center(), s);

	std::vector<BasePoint> points = PlotEllipse(r);
	assert(points.size() % 4 == 0);

	// a line bisecting the ellipse diagonally
	const Point adj(size + 1, 0);
	BasePoint b1 = r.origin - adj;
	BasePoint b2 = r.Maximum() + adj;

	size_t i = 0;
	// points are ordered per quadrant, so we can process 4 each iteration
	// at first, 2 points will be the left segment and 2 will be the right segment
	for (; i < points.size(); i += 4) {
		// each point is in a different quadrant
		const BasePoint& q1 = points[i];
		const BasePoint& q2 = points[i + 1];
		const BasePoint& q3 = points[i + 2];
		const BasePoint& q4 = points[i + 3];

		if (left(b1, b2, q1)) {
			// remaining points are top and bottom segments
			break;
		}

		VideoDriver->DrawPoint(q1 + offsetH, color);
		VideoDriver->DrawPoint(q2 - offsetH, color);
		VideoDriver->DrawPoint(q3 - offsetH, color);
		VideoDriver->DrawPoint(q4 + offsetH, color);
	}

	assert(i < points.size() - 4);

	// the current points are the ends of the side segments
	BasePoint p2(p);
	VideoDriver->DrawLine(points[i++] + offsetH, p2 + offsetH, color); // begin right segment
	VideoDriver->DrawLine(points[i++] - offsetH, p2 - offsetH, color); // begin left segment
	VideoDriver->DrawLine(points[i++] - offsetH, p2 - offsetH, color); // end left segment
	VideoDriver->DrawLine(points[i++] + offsetH, p2 + offsetH, color); // end right segment

	b1 = r.origin + adj;
	b2 = r.Maximum() - adj;

	// skip the void between segments
	for (; i < points.size(); i += 4) {
		if (left(b1, b2, points[i])) {
			break;
		}
	}

	// the current points are the ends of the top/bottom segments
	VideoDriver->DrawLine(points[i++] + offsetV, p2 + offsetV, color); // begin top segment
	VideoDriver->DrawLine(points[i++] + offsetV, p2 + offsetV, color); // end top segment
	VideoDriver->DrawLine(points[i++] - offsetV, p2 - offsetV, color); // begin bottom segment
	VideoDriver->DrawLine(points[i++] - offsetV, p2 - offsetV, color); // end bottom segment

	// remaining points are top/bottom segments
	for (; i < points.size(); i += 4) {
		const BasePoint& q1 = points[i];
		const BasePoint& q2 = points[i + 1];
		const BasePoint& q3 = points[i + 2];
		const BasePoint& q4 = points[i + 3];

		VideoDriver->DrawPoint(q1 + offsetV, color);
		VideoDriver->DrawPoint(q2 + offsetV, color);
		VideoDriver->DrawPoint(q3 - offsetV, color);
		VideoDriver->DrawPoint(q4 - offsetV, color);
	}
}

void GameControl::DrawTargetReticle(const Movable* target, const Point& p, int offset) const
{
	int size = target->CircleSize2Radius();
	const Color& green = target->selectedColor;
	const Color& color = (target->Over) ? GlobalColorCycle.Blend(target->overColor, green) : green;

	DrawTargetReticle(size, color, p, offset);
}

void GameControl::WillDraw(const Region& /*drawFrame*/, const Region& /*clip*/)
{
	UpdateCursor();

	bool update_scripts = !(DialogueFlags & DF_FREEZE_SCRIPTS);

	// handle keeping the actor in the spotlight, but only when unpaused
	if (screenFlags.Test(ScreenFlags::AlwaysCenter) && update_scripts) {
		const Actor* star = core->GetFirstSelectedActor();
		if (star) {
			vpVector = star->Pos - vpOrigin - Point(frame.w / 2, frame.h / 2);
		}
	}

	if (!vpVector.IsZero() && MoveViewportTo(vpOrigin + vpVector, false)) {
		if ((Flags() & IgnoreEvents) == 0 && core->GetMouseScrollSpeed() && !screenFlags.Test(ScreenFlags::AlwaysCenter)) {
			orient_t orient = GetOrient(Point(), vpVector);
			// set these cursors on game window so they are universal
			window->SetCursor(core->GetScrollCursorSprite(orient, numScrollCursor));

			numScrollCursor = (numScrollCursor + 1) % 15;
		}
	} else if (!window->IsDisabled()) {
		window->SetCursor(nullptr);
	}

	const Map* area = CurrentArea();
	if (!area) return;

	int flags = GA_NO_DEAD | GA_NO_UNSCHEDULED | GA_SELECT | GA_NO_ENEMY | GA_NO_NEUTRAL;
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

// FIXME: some of this should happen during mouse events
void GameControl::OutlineInfoPoints() const
{
	const Game* game = core->GetGame();
	const Map* area = game->GetCurrentArea();

	for (const auto& infoPoint : area->TMap->GetInfoPoints()) {
		infoPoint->Highlight = false;

		if (infoPoint->VisibleTrap(0)) {
			if (overMe == infoPoint && targetMode != TargetMode::None) {
				infoPoint->outlineColor = displaymsg->GetColor(GUIColors::HOVERTARGETABLE);
			} else {
				infoPoint->outlineColor = displaymsg->GetColor(GUIColors::TRAPCOLOR);
			}
			infoPoint->Highlight = true;
			continue;
		}
	}
}

// FIXME: some of this should happen during mouse events
void GameControl::OutlineDoors() const
{
	const Game* game = core->GetGame();
	const Map* area = game->GetCurrentArea();

	for (const auto& door : area->TMap->GetDoors()) {
		door->Highlight = false;
		if (door->Flags & DOOR_HIDDEN) {
			continue;
		}

		if (door->Flags & DOOR_SECRET) {
			if (door->Flags & DOOR_FOUND) {
				door->Highlight = true;
				door->outlineColor = displaymsg->GetColor(GUIColors::HIDDENDOOR);
			} else {
				continue;
			}
		}

		// traps always take precedence
		if (door->VisibleTrap(0)) {
			door->Highlight = true;
			door->outlineColor = displaymsg->GetColor(GUIColors::TRAPCOLOR);
			continue;
		}

		if (overMe != door) continue;

		door->Highlight = true;
		if (targetMode != TargetMode::None) {
			if (door->Visible() && door->IsLocked()) {
				// only highlight targetable doors
				door->outlineColor = displaymsg->GetColor(GUIColors::HOVERTARGETABLE);
			}
		} else if (!(door->Flags & DOOR_SECRET)) {
			// mouse over, not in target mode, no secret door
			door->outlineColor = displaymsg->GetColor(GUIColors::HOVERDOOR);
		}
	}
}

// FIXME: some of this should happen during mouse events
void GameControl::OutlineContainers() const
{
	const Game* game = core->GetGame();
	const Map* area = game->GetCurrentArea();

	for (const auto& container : area->TMap->GetContainers()) {
		if (container->Flags & CONT_DISABLED) {
			continue;
		}

		if (overMe == container) {
			container->Highlight = true;
			if (targetMode == TargetMode::None) {
				container->outlineColor = displaymsg->GetColor(GUIColors::HOVERCONTAINER);
			} else if (container->IsLocked()) {
				container->outlineColor = displaymsg->GetColor(GUIColors::HOVERTARGETABLE);
			}
		}

		// traps always take precedence
		if (container->VisibleTrap(0)) {
			container->Highlight = true;
			container->outlineColor = displaymsg->GetColor(GUIColors::TRAPCOLOR);
		}
	}
}

void GameControl::DrawTrackingArrows()
{
	if (!trackerID) return;

	const Game* game = core->GetGame();
	Map* area = game->GetCurrentArea();
	const Actor* actor = area->GetActorByGlobalID(trackerID);
	if (actor) {
		std::vector<Actor*> monsters = area->GetAllActorsInRadius(actor->Pos, GA_NO_DEAD | GA_NO_LOS | GA_NO_UNSCHEDULED, distance);
		for (const auto& monster : monsters) {
			if (monster->IsPartyMember()) continue;
			if (monster->GetStat(IE_NOTRACKING)) continue;
			DrawArrowMarker(monster->Pos, ColorBlack);
		}
	} else {
		trackerID = 0;
	}
}

/** Draws the Control on the Output Display */
void GameControl::DrawSelf(const Region& screen, const Region& /*clip*/)
{
	const Game* game = core->GetGame();
	Map* area = game->GetCurrentArea();
	if (!area) return;

	OutlineInfoPoints();
	OutlineDoors();
	OutlineContainers();

	uint32_t tmpflags = DebugFlags;
	if (EventMgr::ModState(GEM_MOD_ALT)) {
		tmpflags |= DEBUG_SHOW_CONTAINERS | DEBUG_SHOW_DOORS;
	}
	//drawmap should be here so it updates fog of war
	area->DrawMap(Viewport(), core->GetFogRenderer(), tmpflags);

	DrawTrackingArrows();

	if (lastActorID && !(DialogueFlags & DF_FREEZE_SCRIPTS)) {
		const Actor* actor = GetLastActor();
		if (actor) {
			DrawArrowMarker(actor->Pos, ColorGreen);
		}
	}

	// Draw selection rect
	if (isSelectionRect) {
		Region r = SelectionRect();
		r.x -= vpOrigin.x;
		r.y -= vpOrigin.y;
		VideoDriver->DrawRect(r, ColorGreen, false);
	}

	if (core->HasFeature(GFFlags::ONSCREEN_TEXT) && !DisplayText.empty()) {
		Font::PrintColors colors = { displaymsg->GetColor(GUIColors::FLOAT_TXT_INFO), ColorBlack };
		core->GetTextFont()->Print(screen, DisplayText, IE_FONT_ALIGN_CENTER | IE_FONT_ALIGN_MIDDLE, colors);
		if (!(DialogueFlags & DF_FREEZE_SCRIPTS)) {
			// just replicating original engine behaviour
			if (DisplayTextTime == 0) {
				SetDisplayText(u"", 0);
			} else {
				DisplayTextTime--;
			}
		}
	}
}

void GameControl::DrawTargetReticles() const
{
	const Point& gameMousePos = GameMousePos();
	const Game* game = core->GetGame();
	if (isFormationRotation) {
		float_t angle = AngleFromPoints(gameMousePos, gameClickPoint);
		DrawFormation(game->selected, gameClickPoint, angle);
		return;
	}

	for (const auto& selectee : game->selected) {
		assert(selectee);
		if (!selectee->ShouldDrawReticle()) continue;

		// any waypoints to draw?
		const Path& path = selectee->GetPath();
		for (size_t i = 0; i < path.Size(); i++) {
			const PathNode& step = path.GetStep(i);
			if (!step.waypoint) continue;
			Point wp = step.point - vpOrigin;
			DrawTargetReticle(selectee, wp);
		}
		DrawTargetReticle(selectee, selectee->Destination - vpOrigin); // always draw last step
	}
}

// this existly only so tab can be handled
// it's used both for tooltips everywhere and hp display on game control
bool GameControl::DispatchEvent(const Event& event) const
{
	if (!window || window->IsDisabled() || (Flags() & IgnoreEvents)) {
		return false;
	}

	if (event.keyboard.keycode == GEM_TAB) {
		const Game* game = core->GetGame();
		// show partymember hp/maxhp as overhead text
		for (int pm = 0; pm < game->GetPartySize(false); pm++) {
			Actor* pc = game->GetPC(pm, true);
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
	int i;
	int pc;
	Game* game = core->GetGame();

	KeyboardKey keycode = Key.keycode;
	if (mod) {
		// the random bitshift is to skip checking hotkeys with mods
		// eg. ctrl-j should be ignored for keymap.ini handling and
		// passed straight on
		if (!core->GetKeyMap()->ResolveKey(Key.keycode, mod << 20)) {
			game->SendHotKey(towupper(Key.character));
			return View::OnKeyPress(Key, mod);
		}
		return true;
	}

	switch (keycode) {
		case GEM_UP:
		case GEM_DOWN:
		case GEM_LEFT:
		case GEM_RIGHT:
			{
				ieDword keyScrollSpd = core->GetDictionary().Get("Keyboard Scroll Speed", 64);

				if (keycode >= GEM_UP) {
					int v = (keycode == GEM_UP) ? -1 : 1;
					Scroll(Point(0, keyScrollSpd * v));
				} else {
					int v = (keycode == GEM_LEFT) ? -1 : 1;
					Scroll(Point(keyScrollSpd * v, 0));
				}
			}
			break;
		case GEM_TAB: // show partymember hp/maxhp as overhead text
		// fallthrough
		case GEM_ESCAPE: // redraw actionbar
			// do nothing; these are handled in DispatchEvent due to tab having two functions
			break;
		case '0':
			game->SelectActor(nullptr, false, SELECT_NORMAL);
			i = game->GetPartySize(false) / 2 + 1;
			while (i--) {
				SelectActor(i, true);
			}
			break;
		case '-':
			game->SelectActor(nullptr, true, SELECT_NORMAL);
			i = game->GetPartySize(false) / 2 + 1;
			while (i--) {
				SelectActor(i, false);
			}
			break;
		case ' ': //soft pause
			core->TogglePause();
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
			game->SelectActor(nullptr, false, SELECT_NORMAL);
			i = game->GetPartySize(false);
			pc = 2 * (keycode - '6') - 1;
			if (pc >= i) {
				SelectActor(i, true);
				break;
			}
			SelectActor(pc, true);
			SelectActor(pc + 1, true);
			break;
		default:
			if (!core->GetKeyMap()->ResolveKey(Key.keycode, 0)) {
				game->SendHotKey(towupper(Key.character));
				return View::OnKeyPress(Key, 0);
			}
			break;
	}
	return true;
}

//Select (or deselect) a new actor (or actors)
void GameControl::SelectActor(int whom, int type)
{
	Game* game = core->GetGame();
	if (whom == -1) {
		game->SelectActor(nullptr, true, SELECT_NORMAL);
		return;
	}

	/* doesn't fall through here */
	Actor* actor = game->FindPC(whom);
	if (!actor)
		return;

	if (type == 0) {
		game->SelectActor(actor, false, SELECT_NORMAL);
		return;
	}
	if (type == 1) {
		game->SelectActor(actor, true, SELECT_NORMAL);
		return;
	}

	bool was_selected = actor->IsSelected();
	if (game->SelectActor(actor, true, SELECT_REPLACE)) {
		if (was_selected || screenFlags.Test(ScreenFlags::AlwaysCenter)) {
			screenFlags.Set(ScreenFlags::CenterOnActor);
		}
	}
}

void GameControl::DumpActorInfo(ActorDump dump, const Map* area) const noexcept
{
	const Point gameMousePos = GameMousePos();
	const Actor* act = area->GetActorByGlobalID(lastActorID);
	if (!act) {
		act = area->GetActor(gameMousePos, GA_DEFAULT);
	}
	if (!act) {
		// ValidTarget never returns immobile targets, making debugging a nightmare
		// so if we don't have an actor, we make really really sure by checking manually
		unsigned int count = area->GetActorCount(true);
		while (count--) {
			const Actor* actor = area->GetActor(count, true);
			if (actor->IsOver(gameMousePos)) {
				act = actor;
				break;
			}
		}
	}

	if (act) {
		if (dump == ActorDump::Stats) {
			fmt::println("{}", act->dump());
		} else if (dump == ActorDump::Anims) {
			constexpr int width = 10;
			const CharAnimations* anims = act->GetAnims();
			fmt::println("{1:<{0}}: {2:#x}", width, "Anim ID", anims->GetAnimationID());
			fmt::println("{1:<{0}}: {2}", width, "BloodColor", anims->GetBloodColor());
			fmt::println("{1:<{0}}: {2:#x}", width, "Flags", anims->GetFlags());
		}
	}
}

template<typename CONTAIN>
static void PrintCollection(const char* name, const CONTAIN& container)
{
	fmt::println("{} ({}):\n{}", name, container.size(), fmt::join(container, "\n"));
}

//Effect for the ctrl-r cheatkey (resurrect)
static EffectRef heal_ref = { "CurrentHPModifier", -1 };
static EffectRef damage_ref = { "Damage", -1 };

/** Key Release Event */
bool GameControl::OnKeyRelease(const KeyboardEvent& Key, unsigned short Mod)
{
	Point gameMousePos = GameMousePos();
	Highlightable* over = Scriptable::As<Highlightable>(overMe);
	Game* game = core->GetGame();

	//cheatkeys with ctrl-
	if (Mod & GEM_MOD_CTRL) {
		if (!core->CheatEnabled()) {
			return false;
		}
		Map* area = game->GetCurrentArea();
		if (!area)
			return false;
		Actor* lastActor = area->GetActorByGlobalID(lastActorID);
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
					Actor* src = game->selected[0];
					Scriptable* target = lastActor;
					if (overMe) {
						target = overMe;
					}
					if (target) {
						src->SetSpellResRef(TestSpell);
						src->CastSpell(target, false);
						if (src->objects.LastSpellTarget) {
							src->CastSpellEnd(0, false);
						} else {
							src->CastSpellPointEnd(0, false);
						}
					}
				}
				break;
			case 'd': //detect a trap or door
				if (over) {
					if (overMe->Type == ST_DOOR) Scriptable::As<Door>(overMe)->TryDetectSecret(256, lastActorID);
					over->DetectTrap(256, lastActorID);
				}
				break;
			case 'e': // reverses pc order (useful for parties bigger than 6)
				game->ReversePCs();
				break;
			// f
			case 'g': //shows loaded areas and other game information
				fmt::println("{}", game->dump());
				break;
			// h
			case 'i': //interact trigger (from the original game)
				if (!lastActor) {
					lastActor = area->GetActor(gameMousePos, GA_DEFAULT);
				}
				if (lastActor && !(lastActor->GetStat(IE_MC_FLAGS) & MC_EXPORTABLE)) {
					int size = game->GetPartySize(true);
					if (size < 2 || lastActor->GetCurrentArea() != area) break;
					for (int i = core->Roll(1, size, 0); i < 2 * size; i++) {
						const Actor* target = game->GetPC(i % size, true);
						if (target == lastActor) continue;
						if (target->GetStat(IE_MC_FLAGS) & MC_EXPORTABLE) continue; //not NPC
						lastActor->HandleInteractV1(target);
						break;
					}
				}
				break;
			case 'j': //teleports the selected actors
				for (Actor* selectee : game->selected) {
					selectee->ClearActions();
					MoveBetweenAreasCore(selectee, core->GetGame()->CurrentArea, gameMousePos, -1, true);
				}
				break;
			case 'k': //kicks out actor
				if (lastActor && lastActor->InParty) {
					lastActor->Stop();
					lastActor->AddAction("LeaveParty()");
				}
				break;
			case 'l': //play an animation (vvc/bam) over an actor
				//the original engine was able to swap through all animations
				if (lastActor) {
					lastActor->AddAnimation(ResRef("S056ICBL"), 0, 0, 0);
				}
				break;
			case 'M':
				DumpActorInfo(ActorDump::Anims, area);
				FlushLogs();
				break;
			case 'm': //prints a debug dump (ctrl-m in the original game too)
				if (overMe && overMe->Type != ST_ACTOR) {
					fmt::println("{}", overMe->dump());
				} else if (lastActor) {
					DumpActorInfo(ActorDump::Stats, area);
				} else {
					area->dump(false);
				}
				FlushLogs();
				break;
			case 'n': //prints a list of all the live actors in the area
				area->dump(true);
				FlushLogs();
				break;
			// o
			case 'p': //center on actor
				screenFlags.Flip(ScreenFlags::CenterOnActor);
				screenFlags.Flip(ScreenFlags::AlwaysCenter);
				break;
			case 'q': //joins actor to the party
				if (lastActor && !lastActor->InParty) {
					lastActor->Stop();
					lastActor->AddAction("JoinParty()");
				}
				break;
			case 'r': //resurrects actor
				if (!lastActor) {
					lastActor = area->GetActor(gameMousePos, GA_DEFAULT);
				}
				if (lastActor) {
					Effect* fx = EffectQueue::CreateEffect(heal_ref, lastActor->GetStat(IE_MAXHITPOINTS), 0x30001, FX_DURATION_INSTANT_PERMANENT);
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
				PrintCollection("locals", core->GetGame()->locals);
				break;
			case 'U': // dump death vars
				PrintCollection("kaputz", core->GetGame()->kaputz);
				break;
			case 'V': // dump GemRB vars like the game ini settings
				PrintCollection("variables", core->GetDictionary());
				break;
			case 'v': //marks some of the map visited (random vision distance)
				area->ExploreMapChunk(SearchmapPoint(gameMousePos), RAND(0, 29), 1);
				break;
			case 'w': // consolidates found ground piles under the pointed pc
				area->MoveVisibleGroundPiles(gameMousePos);
				break;
			case 'x': // shows coordinates on the map
				fmt::println("{}: {}", area->GetScriptName(), gameMousePos);
				break;
			case 'Y': // damages all enemies by 300 (resistances apply)
				// mwahaha!
				{
					int i = area->GetActorCount(false);
					while (i--) {
						Actor* victim = area->GetActor(i, false);
						if (victim->Modified[IE_EA] == EA_ENEMY) {
							Effect* newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_MAGIC << 16, FX_DURATION_INSTANT_PERMANENT);
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

					Effect* newfx;
					newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_MAGIC << 16, FX_DURATION_INSTANT_PERMANENT);
					core->ApplyEffect(newfx, lastActor, lastActor);
					if (!(lastActor->GetInternalFlag() & IF_REALLYDIED)) {
						newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_ACID << 16, FX_DURATION_INSTANT_PERMANENT);
						core->ApplyEffect(newfx, lastActor, lastActor);
						newfx = EffectQueue::CreateEffect(damage_ref, 300, DAMAGE_CRUSHING << 16, FX_DURATION_INSTANT_PERMANENT);
						core->ApplyEffect(newfx, lastActor, lastActor);
					}
				} else if (!overMe) {
					break;
				} else if (overMe->Type == ST_CONTAINER) {
					Scriptable::As<Container>(overMe)->SetContainerLocked(false);
				} else if (overMe->Type == ST_DOOR) {
					Scriptable::As<Door>(overMe)->SetDoorLocked(0, 0);
				}
				break;
			case 'z': //shift through the avatar animations backward
				if (lastActor) {
					lastActor->GetPrevAnimation();
				}
				break;
			case '1': //change paperdoll armour level
				if (!lastActor)
					break;
				lastActor->NewStat(IE_ARMOR_TYPE, 1, MOD_ADDITIVE);
				break;
			case '4': //show all traps and infopoints
				DebugFlags ^= DEBUG_SHOW_INFOPOINTS;
				Log(MESSAGE, "GameControl", "Show traps and infopoints {}", DebugFlags & DEBUG_SHOW_INFOPOINTS ? "ON" : "OFF");
				break;
			case '5':
				{
					static std::array<uint32_t, 6> wallFlags {
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
					flagIdx = flagIdx % wallFlags.size();
				}
				break;
			case '6': //toggle between lightmap/heightmap/material/search
				{
					constexpr int flagCnt = 5;
					static uint32_t flags[flagCnt] {
						0,
						DEBUG_SHOW_SEARCHMAP,
						DEBUG_SHOW_MATERIALMAP,
						DEBUG_SHOW_HEIGHTMAP,
						DEBUG_SHOW_LIGHTMAP,
					};
					constexpr uint32_t mask = (DEBUG_SHOW_LIGHTMAP | DEBUG_SHOW_HEIGHTMAP | DEBUG_SHOW_MATERIALMAP | DEBUG_SHOW_SEARCHMAP);

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
					static uint32_t fogFlags[flagCnt] {
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

	switch (Key.keycode) {
			//FIXME: move these to guiscript
		case GEM_TAB: // remove overhead partymember hp/maxhp
			for (int pm = 0; pm < game->GetPartySize(false); pm++) {
				Actor* pc = game->GetPC(pm, true);
				if (!pc) continue;
				pc->overHead.Display(false, 0);
			}
			break;
		default:
			return false;
	}
	return true;
}

String GameControl::TooltipText() const
{
	const Map* area = CurrentArea();
	if (!area) {
		return View::TooltipText();
	}

	const Point& gameMousePos = GameMousePos();
	if (!area->IsVisible(gameMousePos)) {
		return View::TooltipText();
	}

	const Actor* actor = area->GetActor(gameMousePos, GA_NO_DEAD | GA_NO_UNSCHEDULED);
	if (!actor || actor->GetStat(IE_MC_FLAGS) & MC_NO_TOOLTIPS || (!actor->InParty && actor->IsInvisibleTo(nullptr))) {
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
			tip += u": ";
		} else {
			tip += u"\n";
		}

		if (actor->HasVisibleHP()) {
			tip += fmt::format(u"{}/{}", hp, maxhp);
		} else {
			tip += u"?";
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
			} else if (hp > (maxhp * 3) / 4) {
				strIdx = HCStrings::Injured1;
			} else if (hp > maxhp / 2) {
				strIdx = HCStrings::Injured2;
			} else if (hp > maxhp / 3) {
				strIdx = HCStrings::Injured3;
			}
			strref = DisplayMessage::GetStringReference(strIdx);
			String injuredstring = core->GetString(strref, STRING_FLAGS::NONE);
			tip += u"\n" + injuredstring;
		}
	}

	return tip;
}

Holder<Sprite2D> GameControl::GetTargetActionCursor(TargetMode mode)
{
	int curIdx = -1;
	switch (mode) {
		case TargetMode::Talk:
			curIdx = IE_CURSOR_TALK;
			break;
		case TargetMode::Attack:
			curIdx = IE_CURSOR_ATTACK;
			break;
		case TargetMode::Cast:
			curIdx = IE_CURSOR_CAST;
			break;
		case TargetMode::Defend:
			curIdx = IE_CURSOR_DEFEND;
			break;
		case TargetMode::Pick:
			curIdx = IE_CURSOR_PICK;
			break;
		default:
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
	if (!cursor && lastCursor != IE_CURSOR_INVALID) {
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
	if (!area) {
		return false;
	}

	Actor* lastActor = area->GetActorByGlobalID(lastActorID);

	// (un)highlight portrait border for pcs
	auto PulsateBorder = [&lastActor](bool enable) {
		if (!lastActor || !lastActor->InParty) return;

		const Window* win = GemRB::GetWindow(0, "PORTWIN");
		if (!win) return;

		Button* btn = GetControl<Button>(lastActor->InParty - 1, win);
		if (!btn) return;
		bool selected = lastActor->IsSelected();
		if (!selected) btn->EnableBorder(0, enable);
		btn->EnablePulsating(enable);
	};

	if (lastActor) {
		lastActor->SetOver(false);
		PulsateBorder(false);
	}

	Point gameMousePos = GameMousePos();
	// let us target party members even if they are invisible
	lastActor = area->GetActor(gameMousePos, GA_NO_DEAD | GA_NO_UNSCHEDULED);
	if (lastActor && lastActor->Modified[IE_EA] >= EA_CONTROLLED) {
		if (!lastActor->ValidTarget(target_types) || !area->IsVisible(gameMousePos)) {
			lastActor = nullptr;
		}
	}

	if ((target_types & GA_NO_SELF) && lastActor == core->GetFirstSelectedActor()) {
		lastActor = nullptr;
	}

	if (lastActor && lastActor->GetStat(IE_NOCIRCLE)) {
		lastActor = nullptr;
	}

	SetLastActor(lastActor);
	PulsateBorder(true);

	return true;
}

void GameControl::UpdateCursor()
{
	const Map* area = CurrentArea();
	if (!area) {
		lastCursor = IE_CURSOR_BLOCKED;
		return;
	}

	Point gameMousePos = GameMousePos();
	int nextCursor = area->GetCursor(gameMousePos);
	//make the invisible area really invisible
	if (nextCursor == IE_CURSOR_INVALID) {
		lastCursor = IE_CURSOR_BLOCKED;
		return;
	} else if (nextCursor == IE_CURSOR_BLOCKED) {
		// don't leak that an enemy is invisible and treat its space as passable
		// it's not necessarily lastActor, so we have to search again
		const Actor* actor = area->GetActor(gameMousePos, GA_NO_DEAD | GA_NO_UNSCHEDULED | GA_NO_ALLY);
		if (actor && actor->IsInvisibleTo(nullptr)) {
			lastCursor = IE_CURSOR_WALK;
			return;
		}
	}

	Door* overDoor = Scriptable::As<Door>(overMe);
	if (overDoor) {
		overDoor->Highlight = false;
	}
	Container* overContainer = Scriptable::As<Container>(overMe);
	if (overContainer) {
		overContainer->Highlight = false;
	}

	overMe = overDoor = area->TMap->GetDoor(gameMousePos);
	// ignore infopoints and containers beneath doors
	// pst mortuary door right above the starting position is a good test
	if (overDoor) {
		if (overDoor->Visible()) {
			nextCursor = overDoor->GetCursor(targetMode, lastCursor);
		} else {
			overMe = nullptr;
		}
	} else {
		InfoPoint* overInfoPoint = area->TMap->GetInfoPoint(gameMousePos, false);
		overMe = overInfoPoint;
		if (overInfoPoint) {
			nextCursor = overInfoPoint->GetCursor(targetMode);
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

		// let containers override infopoints if at the same location
		// needed in bg2 ar0809 or you can't get the loot on the altar
		// how ar3201 and a bunch in the pst mortuary (eg. near Ei-vene) show it as a usability win as well
		overContainer = area->TMap->GetContainer(gameMousePos);
		if (overContainer) {
			overMe = overContainer;
		}
	}

	if (overContainer) {
		nextCursor = overContainer->GetCursor(targetMode, lastCursor);
	}
	// recheck in case the position was different, resulting in a new isVisible check
	// fixes bg2 long block door in ar0801 above vamp beds, crashing on mouseover (too big)
	if (nextCursor == IE_CURSOR_INVALID) {
		lastCursor = IE_CURSOR_BLOCKED;
		return;
	}

	const Actor* lastActor = area->GetActorByGlobalID(lastActorID);
	if (lastActor) {
		// don't change the cursor for birds
		if (lastActor->GetStat(IE_DONOTJUMP) == DNJ_BIRD) return;

		ieDword type = lastActor->GetStat(IE_EA);
		if (type >= EA_EVILCUTOFF || type == EA_GOODBUTRED) {
			nextCursor = IE_CURSOR_ATTACK;
		} else if (type > EA_CHARMED) {
			if (lastActor->GetStat(IE_NOCIRCLE)) return;
			nextCursor = IE_CURSOR_TALK;
			//don't let the pc to talk to frozen/stoned creatures
			ieDword state = lastActor->GetStat(IE_STATE_ID);
			if (state & (STATE_CANTMOVE ^ STATE_SLEEP)) {
				nextCursor |= IE_CURSOR_GRAY;
			}
		} else {
			nextCursor = IE_CURSOR_NORMAL;
		}
	}

	if (targetMode == TargetMode::Talk) {
		nextCursor = IE_CURSOR_TALK;
		if (!lastActor) {
			nextCursor |= IE_CURSOR_GRAY;
		} else {
			//don't let the pc to talk to frozen/stoned creatures
			ieDword state = lastActor->GetStat(IE_STATE_ID);
			if (state & (STATE_CANTMOVE ^ STATE_SLEEP)) {
				nextCursor |= IE_CURSOR_GRAY;
			}
		}
	} else if (targetMode == TargetMode::Attack) {
		nextCursor = IE_CURSOR_ATTACK;
		if (!lastActor && (!overMe || overMe->Type <= ST_TRIGGER)) {
			nextCursor |= IE_CURSOR_GRAY;
		}
	} else if (targetMode == TargetMode::Cast) {
		nextCursor = IE_CURSOR_CAST;
		// point is always valid if accessible
		// knock ignores that
		bool blocked = bool(area->GetBlocked(gameMousePos) & (PathMapFlags::PASSABLE | PathMapFlags::TRAVEL | PathMapFlags::ACTOR));
		bool ignoreSM = gamedata->GetSpecialSpell(spellName) & SPEC_AREA;
		if (!ignoreSM && (!blocked || (!(target_types & GA_POINT) && !lastActor))) {
			nextCursor |= IE_CURSOR_GRAY;
		}
	} else if (targetMode == TargetMode::Defend) {
		nextCursor = IE_CURSOR_DEFEND;
		if (!lastActor) {
			nextCursor |= IE_CURSOR_GRAY;
		}
	} else if (targetMode == TargetMode::Pick) {
		if (lastActor) {
			nextCursor = IE_CURSOR_PICK;
		} else if (!overMe) {
			nextCursor = IE_CURSOR_STEALTH | IE_CURSOR_GRAY;
		}
	}

	if (nextCursor >= 0) {
		lastCursor = nextCursor;
	}
}

bool GameControl::IsDisabledCursor() const
{
	bool isDisabled = View::IsDisabledCursor();
	if (lastCursor != IE_CURSOR_INVALID)
		isDisabled |= bool(lastCursor & IE_CURSOR_GRAY);

	return isDisabled;
}

void GameControl::DebugPaint(const Point& p, bool sample) const noexcept
{
	if (DebugFlags & (DEBUG_SHOW_SEARCHMAP | DEBUG_SHOW_MATERIALMAP | DEBUG_SHOW_HEIGHTMAP | DEBUG_SHOW_LIGHTMAP)) {
		Map* map = CurrentArea();
		SearchmapPoint tile { p };
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
		InitFormation(gameClickPoint, true);
		return true;
	}

	if (targetMode != TargetMode::None) {
		// we are in a target mode; nothing here applies
		return true;
	}

	if (overMe) {
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
		DebugFlags |= DEBUG_SHOW_CONTAINERS | DEBUG_SHOW_DOORS;
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
		DebugFlags &= ~(DEBUG_SHOW_CONTAINERS | DEBUG_SHOW_DOORS);
	}

	return View::OnTouchUp(te, mod);
}

bool GameControl::OnTouchGesture(const GestureEvent& gesture)
{
	if (gesture.numFingers == 1) {
		if (targetMode != TargetMode::None) {
			// we are in a target mode; nothing here applies
			return true;
		}

		screenMousePos = gesture.Pos();
		isSelectionRect = true;
	} else if (gesture.numFingers == 2) {
		if (gesture.dTheta < -0.2 || gesture.dTheta > 0.2) { // TODO: actually figure out a good number
			if (EventMgr::ModState(GEM_MOD_ALT) == false) {
				DebugFlags &= ~(DEBUG_SHOW_CONTAINERS | DEBUG_SHOW_DOORS);
			}

			isSelectionRect = false;

			if (core->GetGame()->selected.size() <= 1) {
				isFormationRotation = false;
			} else {
				screenMousePos = gesture.fingers[1].Pos();
				InitFormation(screenMousePos, true);
			}
		} else { // scroll viewport
			MoveViewportTo(vpOrigin - gesture.Delta(), false);
		}
	} else if (gesture.numFingers == 3) { // keyboard/console
		enum class SWIPE { DOWN = -1,
				   NONE = 0,
				   UP = 1 };
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
				VideoDriver->StopTextInput();
				consoleWin->Close();
				break;
			case SWIPE::UP:
				if (VideoDriver->InTextInput()) {
					consoleWin->Focus();
				}
				VideoDriver->StartTextInput();
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
	if (!window || window->IsDisabled() || (Flags() & IgnoreEvents)) {
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
	mask.w -= SCROLL_AREA_WIDTH * 2;
	mask.h -= SCROLL_AREA_WIDTH * 2;
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
	bool canMove = area != nullptr;

	if (updateVPTimer && speed) {
		updateVPTimer = false;
		core->timer.SetMoveViewPort(p, speed, center);
	} else if (canMove && p != vpOrigin) {
		updateVPTimer = true;

		Size mapsize = area->GetSize();

		if (center) {
			p.x -= frame.w / 2;
			p.y -= frame.h / 2;
		}

		// TODO: make the overflow more dynamic
		if (frame.w >= mapsize.w + 64) {
			p.x = (mapsize.w - frame.w) / 2;
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
			p.y = (mapsize.h - frame.h) / 2 + padding;
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
void GameControl::TryToAttack(Actor* source, const Actor* tgt) const
{
	if (source->GetStat(IE_SEX) == SEX_ILLUSION) return;
	source->CommandActor(GenerateActionDirect("NIDSpecial3()", tgt));
}

//generate action code for source actor to try to defend a target
void GameControl::TryToDefend(Actor* source, const Actor* tgt) const
{
	source->SetModal(Modal::None);
	source->CommandActor(GenerateActionDirect("NIDSpecial4()", tgt));
}

// generate action code for source actor to try to pick pockets of a target (if an actor)
// else if door/container try to pick a lock/disable trap
// The -1 flag is a placeholder for dynamic target IDs
void GameControl::TryToPick(Actor* source, const Scriptable* tgt) const
{
	source->SetModal(Modal::None);
	std::string cmdString;
	cmdString.reserve(20);
	switch (tgt->Type) {
		case ST_ACTOR:
			cmdString = "PickPockets([-1])";
			break;
		case ST_DOOR:
		case ST_CONTAINER:
			if (((const Highlightable*) tgt)->Trapped && ((const Highlightable*) tgt)->TrapDetected) {
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
void GameControl::TryToDisarm(Actor* source, const InfoPoint* tgt) const
{
	if (tgt->Type != ST_PROXIMITY) return;

	source->SetModal(Modal::None);
	source->CommandActor(GenerateActionDirect("RemoveTraps([-1])", tgt));
}

//generate action code for source actor to use item/cast spell on a point
void GameControl::TryToCast(Actor* source, const Point& tgt)
{
	if ((target_types & GA_POINT) == false) {
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
	if (spellOrItem >= 0) {
		if (spellIndex < 0) {
			tmp = "SpellPointNoDec(\"\",[0.0])";
		} else {
			tmp = "SpellPoint(\"\",[0.0])";
		}
	} else {
		//using item on target
		tmp = "UseItemPoint(\"\",[0,0],0)";
	}
	Action* action = GenerateAction(std::move(tmp));
	action->pointParameter = tgt;
	if (spellOrItem >= 0) {
		if (spellIndex < 0) {
			action->resref0Parameter = spellName;
		} else {
			const CREMemorizedSpell* si;
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
			action->int2Parameter |= UI_NOAURA | UI_NOCHARGE;
		}
	}
	source->AddAction(action);
	if (!spellCount) {
		ResetTargetMode();
	}
}

//generate action code for source actor to use item/cast spell on another actor
void GameControl::TryToCast(Actor* source, const Actor* tgt)
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
	if (source != tgt && tgt->Untargetable(spellName, source)) {
		displaymsg->DisplayConstantStringName(HCStrings::NoSeeNoCast, GUIColors::RED, source);
		ResetTargetMode();
		return;
	}

	spellCount--;
	std::string tmp;
	tmp.reserve(20);
	if (spellOrItem >= 0) {
		if (spellIndex < 0) {
			tmp = "NIDSpecial7()";
		} else {
			tmp = "NIDSpecial6()";
		}
	} else {
		//using item on target
		tmp = "NIDSpecial5()";
	}
	Action* action = GenerateActionDirect(std::move(tmp), tgt);
	if (spellOrItem >= 0) {
		if (spellIndex < 0) {
			action->resref0Parameter = spellName;
		} else {
			const CREMemorizedSpell* si;
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
			action->int2Parameter |= UI_NOAURA | UI_NOCHARGE;
		}
	}
	source->AddAction(action);
	if (!spellCount) {
		ResetTargetMode();
	}
}

//generate action code for source actor to use talk to target actor
void GameControl::TryToTalk(Actor* source, const Actor* tgt) const
{
	if (source->GetStat(IE_SEX) == SEX_ILLUSION) return;
	//Nidspecial1 is just an unused action existing in all games
	//(non interactive demo)
	//i found no fitting action which would emulate this kind of
	// dialog initiation
	source->SetModal(Modal::None);
	dialoghandler->SetTarget(tgt); //this is a hack, but not so deadly
	source->CommandActor(GenerateActionDirect("NIDSpecial1()", tgt));
}

//generate action code for actor appropriate for the target mode when the target is a container
void GameControl::HandleContainer(Container* container, Actor* actor)
{
	if (actor->GetStat(IE_SEX) == SEX_ILLUSION) return;
	//container is disabled, it should not react
	if (container->Flags & CONT_DISABLED) {
		return;
	}

	if ((targetMode == TargetMode::Cast) && spellCount) {
		//we'll get the container back from the coordinates
		target_types |= GA_POINT;
		TryToCast(actor, container->Pos);
		//Do not reset target_mode, TryToCast does it for us!!
		return;
	}

	core->SetEventFlag(EF_RESETTARGET);

	if (targetMode == TargetMode::Attack) {
		std::string Tmp = fmt::format("BashDoor(\"{}\")", container->GetScriptName());
		actor->CommandActor(GenerateAction(std::move(Tmp)));
		return;
	}

	if (targetMode == TargetMode::Pick) {
		TryToPick(actor, container);
		return;
	}

	// familiars can not pick up items
	if (actor->GetBase(IE_EA) == EA_FAMILIAR) {
		displaymsg->DisplayConstantString(HCStrings::FamiliarNoHands, GUIColors::WHITE, actor);
		return;
	}

	container->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
	core->SetCurrentContainer(actor, container);
	actor->CommandActor(GenerateAction("UseContainer()"));
}

//generate action code for actor appropriate for the target mode when the target is a door
void GameControl::HandleDoor(Door* door, Actor* actor)
{
	if (actor->GetStat(IE_SEX) == SEX_ILLUSION) return;
	if ((targetMode == TargetMode::Cast) && spellCount) {
		//we'll get the door back from the coordinates
		unsigned int dist;
		const Point* p = door->GetClosestApproach(actor, dist);
		TryToCast(actor, *p);
		return;
	}

	core->SetEventFlag(EF_RESETTARGET);

	if (targetMode == TargetMode::Attack) {
		std::string Tmp = fmt::format("BashDoor(\"{}\")", door->GetScriptName());
		actor->CommandActor(GenerateAction(std::move(Tmp)));
		return;
	}

	if (targetMode == TargetMode::Pick) {
		TryToPick(actor, door);
		return;
	}

	door->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
	// internal gemrb toggle door action hack - should we use UseDoor instead?
	Action* toggle = GenerateAction("NIDSpecial9()");
	toggle->int0Parameter = door->GetGlobalID();
	actor->CommandActor(toggle);
}

//generate action code for actor appropriate for the target mode when the target is an active region (infopoint, trap or travel)
bool GameControl::HandleActiveRegion(InfoPoint* trap, Actor* actor, const Point& p)
{
	if (actor->GetStat(IE_SEX) == SEX_ILLUSION) return false;
	if ((targetMode == TargetMode::Cast) && spellCount) {
		//we'll get the active region from the coordinates (if needed)
		TryToCast(actor, p);
		//don't bother with this region further
		return true;
	}
	if (targetMode == TargetMode::Pick) {
		TryToDisarm(actor, trap);
		return true;
	}

	switch (trap->Type) {
		case ST_TRAVEL:
			trap->AddTrigger(TriggerEntry(trigger_clicked, actor->GetGlobalID()));
			actor->objects.LastMarked = trap->GetGlobalID();
			//clear the go closer flag
			trap->GetCurrentArea()->LastGoCloser = 0;
			return false;
		case ST_TRIGGER:
			// pst, eg. ar1500
			if (!trap->GetDialog().IsEmpty()) {
				trap->AddAction("Dialogue([PC])");
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
					actor->objects.LastMarked = trap->GetGlobalID();
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

			if (trap->GetUsePoint()) {
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
void GameControl::InitFormation(const Point& clickPoint, bool rotating)
{
	// Of course single actors don't get rotated, but we need to ensure
	// isFormationRotation is set in all cases where we initiate movement,
	// since OnMouseUp tests for it.
	if (isFormationRotation || core->GetGame()->selected.empty()) {
		return;
	}

	const Actor* selectedActor = GetMainSelectedActor();

	isFormationRotation = rotating;
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

	switch (me.button) {
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
				InitFormation(gameClickPoint, true);
			}

			break;
	}
	return true;
}

// list of allowed area and exit combinations in pst that trigger worldmap travel
static const ResRefMap<std::vector<ResRef>> pstWMapExits {
	{ "ar0100", { "to0300", "to0200", "to0101" } },
	{ "ar0101", { "to0100" } },
	{ "ar0200", { "to0100", "to0301", "to0400" } },
	{ "ar0300", { "to0100", "to0301", "to0400" } },
	{ "ar0301", { "to0200", "to0300" } },
	{ "ar0400", { "to0200", "to0300" } },
	{ "ar0500", { "to0405", "to0600" } },
	{ "ar0600", { "to0500" } }
};

// pst: determine if we need to trigger worldmap travel, since it had it's own system
// eg. it doesn't use the searchmap for this in ar0500 when travelling globally
// has to be a plain travel region and on the whitelist
bool GameControl::ShouldTriggerWorldMap(const Actor* pc) const
{
	if (!core->HasFeature(GFFlags::TEAM_MOVEMENT)) return false;

	bool keyAreaVisited = CheckVariable(pc, "AR0500_Visited", "GLOBAL") == 1;
	if (!keyAreaVisited) return false;

	bool teamMoved = (pc->GetInternalFlag() & IF_USEEXIT) && overMe && overMe->Type == ST_TRAVEL;
	if (!teamMoved) return false;

	teamMoved = false;
	auto wmapExits = pstWMapExits.find(pc->GetCurrentArea()->GetScriptRef());
	if (wmapExits != pstWMapExits.end()) {
		for (const auto& exit : wmapExits->second) {
			if (exit == overMe->GetScriptName()) {
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
		tryToRun = tryToRun || (Mod & GEM_MOD_SHIFT);
	}

	// right click
	if (me.button == GEM_MB_MENU) {
		ieDword actionLevel = core->GetDictionary().Get("ActionLevel", 0);

		if (targetMode != TargetMode::None || actionLevel) {
			if (!core->HasFeature(GFFlags::HAS_FLOAT_MENU)) {
				SetTargetMode(TargetMode::None);
			}
			// update the action bar
			core->GetDictionary().Set("ActionLevel", 0);
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
		if (targetMode == TargetMode::None && lastActorID) {
			switch (lastCursor & ~IE_CURSOR_GRAY) {
				case IE_CURSOR_TALK:
					SetTargetMode(TargetMode::Talk);
					break;
				case IE_CURSOR_ATTACK:
					SetTargetMode(TargetMode::Attack);
					break;
				case IE_CURSOR_CAST:
					SetTargetMode(TargetMode::Cast);
					break;
				case IE_CURSOR_DEFEND:
					SetTargetMode(TargetMode::Defend);
					break;
				case IE_CURSOR_PICK:
					SetTargetMode(TargetMode::Pick);
					break;
				default:
					break;
			}
		}

		if (targetMode == TargetMode::None && (isSelectionRect || lastActorID)) {
			MakeSelection(Mod & GEM_MOD_SHIFT);
			ClearMouseState();
			return true;
		}

		if (lastCursor == IE_CURSOR_BLOCKED) {
			// don't allow travel if the destination is actually blocked
			return false;
		}

		if (overMe && (overMe->Type == ST_DOOR || overMe->Type == ST_CONTAINER || (overMe->Type == ST_TRAVEL && targetMode == TargetMode::None))) {
			// move to the object before trying to interact with it
			Actor* mainActor = GetMainSelectedActor();
			if (mainActor && overMe->Type != ST_TRAVEL) {
				CreateMovement(mainActor, p, false, tryToRun); // let one actor handle doors, loot and containers
			} else {
				CommandSelectedMovement(p, overMe->Type != ST_TRAVEL, false, tryToRun);
			}
		}

		if (targetMode != TargetMode::None || (overMe && overMe->Type != ST_ACTOR)) {
			PerformSelectedAction(p);
			ClearMouseState();
			return true;
		}

		// Ensure that left-click movement also orients the formation
		// in the direction of movement.
		InitFormation(p, false);
	}

	// handle movement/travel, but not if we just opened the float window
	// or if it doesn't make sense due to the location
	bool saneCursor = lastCursor != IE_CURSOR_BLOCKED && lastCursor != IE_CURSOR_NORMAL && lastCursor != IE_CURSOR_TALK;
	if ((!core->HasFeature(GFFlags::HAS_FLOAT_MENU) || me.button != GEM_MB_MENU) && saneCursor) {
		// pst has different mod keys
		int modKey = GEM_MOD_SHIFT;
		if (core->HasFeature(GFFlags::HAS_FLOAT_MENU)) modKey = GEM_MOD_CTRL;
		CommandSelectedMovement(p, lastCursor != IE_CURSOR_TRAVEL, Mod & modKey, tryToRun);
	}
	ClearMouseState();
	return true;
}

void GameControl::PerformSelectedAction(const Point& p)
{
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
	if (targetMode == TargetMode::Cast && !(gamedata->GetSpecialSpell(spellName) & SPEC_AREA)) {
		//the player is using an item or spell on the ground
		target_types |= GA_POINT;
		TryToCast(selectedActor, p);
	} else if (!overMe) {
		return;
	} else if (overMe->Type == ST_DOOR) {
		HandleDoor(Scriptable::As<Door>(overMe), selectedActor);
	} else if (overMe->Type == ST_CONTAINER) {
		HandleContainer(Scriptable::As<Container>(overMe), selectedActor);
	} else if (overMe->Type == ST_TRAVEL && targetMode == TargetMode::None) {
		ieDword exitID = overMe->GetGlobalID();
		if (core->HasFeature(GFFlags::TEAM_MOVEMENT)) {
			// pst forces everyone to travel (eg. ar0201 outside_portal)
			int i = game->GetPartySize(false);
			while (i--) {
				game->GetPC(i, false)->UseExit(exitID);
			}
		} else {
			size_t i = game->selected.size();
			while (i--) {
				game->selected[i]->UseExit(exitID);
			}
		}
		if (HandleActiveRegion(Scriptable::As<InfoPoint>(overMe), selectedActor, p)) {
			core->SetEventFlag(EF_RESETTARGET);
		}
	} else if (overMe->Type == ST_TRAVEL || overMe->Type == ST_PROXIMITY || overMe->Type == ST_TRIGGER) {
		if (HandleActiveRegion(Scriptable::As<InfoPoint>(overMe), selectedActor, p)) {
			core->SetEventFlag(EF_RESETTARGET);
		}
	}
}

void GameControl::CommandSelectedMovement(const Point& p, bool formation, bool append, bool tryToRun) const
{
	const Game* game = core->GetGame();

	// construct a sorted party
	std::vector<Actor*> party;
	// first, from the actual party
	int max = game->GetPartySize(false);
	for (int idx = 1; idx <= max; idx++) {
		Actor* act = game->FindPC(idx);
		assert(act);
		if (act->IsSelected()) {
			party.push_back(act);
		}
	}
	// then summons etc.
	for (Actor* selected : game->selected) {
		if (!selected->InParty) {
			party.push_back(selected);
		}
	}

	if (party.empty())
		return;

	float_t angle = isFormationRotation ? AngleFromPoints(GameMousePos(), p) : formationBaseAngle;
	bool doWorldMap = ShouldTriggerWorldMap(party[0]);

	std::vector<Point> formationPoints = GetFormationPoints(p, party, angle);
	for (size_t i = 0; i < party.size(); i++) {
		Actor* actor = party[i];
		// don't stop the party if we're just trying to add a waypoint
		if (!append) {
			actor->Stop();
		}

		if (formation && party.size() > 1) {
			CreateMovement(actor, formationPoints[i], append, tryToRun);
		} else {
			CreateMovement(actor, p, append, tryToRun);
		}

		// don't trigger the travel region, so everyone can bunch up there and NIDSpecial2 can take over
		if (doWorldMap) actor->SetInternalFlag(IF_PST_WMAPPING, BitOp::OR);
	}

	// p is a searchmap travel region or a plain travel region in pst (matching several other criteria)
	if (party[0]->GetCurrentArea()->GetCursor(p) == IE_CURSOR_TRAVEL || doWorldMap) {
		party[0]->AddAction("NIDSpecial2()");
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
			core->SetEventFlag(EF_ACTION | EF_RESETTARGET);
			return true;
		default:
			return View::OnControllerButtonDown(ce);
	}
}

void GameControl::Scroll(const Point& amt)
{
	MoveViewportTo(vpOrigin + amt, false);
}

// only party members and familiars can start conversations from the GUI
static Actor* GetTalkInitiator()
{
	const Game* game = core->GetGame();
	Actor* source;
	if (core->HasFeature(GFFlags::PROTAGONIST_TALKS)) {
		source = game->GetPC(0, false); // protagonist
	} else {
		source = core->GetFirstSelectedPC(false);
		if (!source) {
			// check also for familiars
			for (auto& npc : game->selected) {
				if (npc->GetBase(IE_EA) == EA_FAMILIAR) {
					source = npc;
					break;
				}
			}
		}
	}
	return source;
}

void GameControl::PerformActionOn(Actor* actor)
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

	if (targetMode == TargetMode::Attack) {
		type = ACT_ATTACK;
	} else if (targetMode == TargetMode::Talk) {
		type = ACT_TALK;
	} else if (targetMode == TargetMode::Cast) {
		type = ACT_CAST;
	} else if (targetMode == TargetMode::Defend) {
		type = ACT_DEFEND;
	} else if (targetMode == TargetMode::Pick) {
		type = ACT_THIEVING;
	}

	if (type != ACT_NONE && !actor->ValidTarget(target_types)) {
		return;
	}

	//we shouldn't zero this for two reasons in case of spell or item
	//1. there could be multiple targets
	//2. the target mode is important
	if (targetMode != TargetMode::Cast || !spellCount) {
		ResetTargetMode();
	}

	switch (type) {
		case ACT_NONE: //none
			if (!actor->ValidTarget(GA_SELECT)) {
				return;
			}

			if (actor->InParty)
				SelectActor(actor->InParty);
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

			// talk (first selected talks)
			if (!game->selected.empty()) {
				Actor* source = GetTalkInitiator();
				if (source) {
					TryToTalk(source, actor);
				}
			}
			break;
		case ACT_ATTACK:
			//all of them attacks the red circled actor
			for (Actor* selectee : game->selected) {
				TryToAttack(selectee, actor);
			}
			break;
		case ACT_CAST: //cast on target or use item on target
			if (game->selected.size() == 1) {
				Actor* source = core->GetFirstSelectedActor();
				if (source) {
					TryToCast(source, actor);
				}
			}
			break;
		case ACT_DEFEND:
			for (Actor* selectee : game->selected) {
				TryToDefend(selectee, actor);
			}
			break;
		case ACT_THIEVING:
			if (game->selected.size() == 1) {
				Actor* source = core->GetFirstSelectedActor();
				if (source) {
					TryToPick(source, actor);
				}
			}
			break;
	}
}

//sets target mode, and resets the cursor
void GameControl::SetTargetMode(TargetMode mode)
{
	targetMode = mode;
	Window* win = GemRB::GetWindow(0, "PORTWIN");
	if (win) {
		win->SetCursor(GetTargetActionCursor(mode));
	}
}

void GameControl::ResetTargetMode()
{
	target_types = GA_NO_DEAD | GA_NO_HIDDEN | GA_NO_UNSCHEDULED;
	SetTargetMode(TargetMode::None);
}

void GameControl::UpdateTargetMode()
{
	SetTargetMode(targetMode);
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
		game->SelectActor(nullptr, false, SELECT_NORMAL);
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
		screenFlags.Set(ScreenFlags::Cutscene);
		vpVector.reset();
		wm->SetCursorFeedback(WindowManager::MOUSE_NONE);
	} else {
		screenFlags.Clear(ScreenFlags::Cutscene);
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
	auto lookup = core->GetDictionary().Get("Duplicate Floating Text", 0);
	if (lookup) {
		displaymsg->DisplayString(target->overHead.GetText());
	}
	target->overHead.Display(true, 0);
}

/** changes displayed map to the currently selected PC */
void GameControl::ChangeMap(const Actor* pc, bool forced)
{
	//swap in the area of the actor
	Game* game = core->GetGame();
	if (forced || (pc && pc->AreaName != game->CurrentArea)) {
		// disable so that drawing and events dispatched doesn't happen while there is not an area
		// we are single threaded, but game loading runs its own event loop which will cause events/drawing to come in
		SetDisabled(true);
		ClearMouseState();

		dialoghandler->EndDialog();
		overMe = nullptr;
		/*this is loadmap, because we need the index, not the pointer*/
		if (pc) {
			game->GetMap(pc->AreaName, true);
		} else {
			ResRef oldMaster = game->LastMasterArea; // only update it for party travel
			game->GetMap(game->CurrentArea, true);
			game->LastMasterArea = oldMaster;
		}

		if (!core->InCutSceneMode()) {
			// don't interfere with any scripted moves of the viewport
			// checking core->timer->ViewportIsMoving() is not enough
			screenFlags.Set(ScreenFlags::CenterOnActor);
		}

		SetDisabled(false);

		if (window) {
			window->Focus();
		}
	}
	//center on first selected actor
	if (pc && screenFlags.Test(ScreenFlags::CenterOnActor)) {
		MoveViewportTo(pc->Pos, true);
		screenFlags.Clear(ScreenFlags::CenterOnActor);
	}
}

void GameControl::FlagsChanged(unsigned int /*oldflags*/)
{
	if (Flags() & IgnoreEvents) {
		ClearMouseState();
		vpVector.reset();
	}
}

bool GameControl::SetScreenFlags(ScreenFlags value, BitOp mode)
{
	auto bits = screenFlags.to_ulong();
	bool toggled = SetBits(bits, (unsigned long) (1 << UnderType(value)), mode);
	screenFlags = EnumBitset<ScreenFlags>(bits);
	return toggled;
}

void GameControl::SetDialogueFlags(unsigned int value, BitOp mode)
{
	SetBits(DialogueFlags, value, mode);
	SetFlags(IgnoreEvents, (DialogueFlags & DF_IN_DIALOG || screenFlags.Test(ScreenFlags::Cutscene)) ? BitOp::OR : BitOp::NAND);
}

Map* GameControl::CurrentArea() const
{
	const Game* game = core->GetGame();
	if (game) {
		return game->GetCurrentArea();
	}
	return nullptr;
}

Actor* GameControl::GetLastActor() const
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
		if (!area) {
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

Scriptable* GameControl::GetHoverObject() const
{
	Scriptable* const lastActor = lastActorID != 0 ? GetLastActor() : nullptr;
	return lastActor != nullptr ? lastActor : overMe;
}

//Set up an item use which needs targeting
//Slot is an inventory slot
//header is the used item extended header
//u is the user
//target type is a bunch of GetActor flags that alter valid targets
//cnt is the number of different targets (usually 1)
void GameControl::SetupItemUse(int slot, size_t header, int targettype, int cnt)
{
	spellName.Reset();
	spellOrItem = -1;
	spellSlot = slot;
	spellIndex = static_cast<int>(header);
	//item use also uses the casting icon, this might be changed in some custom game?
	SetTargetMode(TargetMode::Cast);
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
void GameControl::SetupCasting(const ResRef& spellname, int type, int level, int idx, int targettype, int cnt)
{
	spellName = spellname;
	spellOrItem = type;
	spellSlot = level;
	spellIndex = idx;
	SetTargetMode(TargetMode::Cast);
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
	core->GetDictionary().Set("Always Run", AlwaysRun);
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
