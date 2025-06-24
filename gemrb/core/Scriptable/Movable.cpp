/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2024 The GemRB Project
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
 */

#include "Movable.h"

#include "ie_stats.h"

#include "Actor.h"
#include "Interface.h"
#include "Map.h"

#include "GUI/GameControl.h"
#include "GameScript/GSUtils.h"
#include "GameScript/GameScript.h"

namespace GemRB {

Point Movable::GetMostLikelyPosition() const
{
	if (!path) {
		return Pos;
	}

	// actually, sometimes middle path would be better, if
	// we stand in Destination already
	size_t halfway = path.Size() / 2;
	const PathNode& node = path.GetNextStep(halfway);
	if (!node.point.IsZero()) {
		return node.point + Point(8, 6);
	}
	return Destination;
}

void Movable::SetStance(unsigned int arg)
{
	// don't modify stance from dead back to anything if the actor is dead
	if ((StanceID == IE_ANI_TWITCH || StanceID == IE_ANI_DIE) && (arg != IE_ANI_TWITCH)) {
		if (GetInternalFlag() & IF_REALLYDIED) {
			Log(WARNING, "Movable", "Stance overridden by death");
			return;
		}
	}

	if (arg >= MAX_ANIMS) {
		StanceID = IE_ANI_AWAKE;
		Log(ERROR, "Movable", "Tried to set invalid stance id({})", arg);
		return;
	}

	Actor* caster = Scriptable::As<Actor>(this);
	if (StanceID == IE_ANI_CONJURE && StanceID != arg) {
		if (caster && caster->casting_sound) {
			caster->casting_sound->Stop();
			caster->casting_sound.reset();
		}
	}

	StanceID = (unsigned char) arg;

	if (StanceID == IE_ANI_ATTACK) {
		// Set stance to a random attack animation
		int random = RAND(0, 99);
		if (random < AttackMovements[0]) {
			StanceID = IE_ANI_ATTACK_BACKSLASH;
		} else if (random < AttackMovements[0] + AttackMovements[1]) {
			StanceID = IE_ANI_ATTACK_SLASH;
		} else {
			StanceID = IE_ANI_ATTACK_JAB;
		}
	}

	// this doesn't get hit on movement, since movement overrides the stance manually
	// but it is needed for the twang/clank when an actor stops moving
	// a lot of other stances would get skipped later, since we check we're out of combat
	if (caster) {
		caster->PlayArmorSound();
	}
}

void Movable::SetOrientation(orient_t value, bool slow)
{
	NewOrientation = value;
	if (NewOrientation != Orientation && Type == ST_ACTOR) {
		const Actor* actor = (Actor*) this;
		actor->PlayArmorSound();
	}
	if (!slow) {
		Orientation = NewOrientation;
	}
}

void Movable::SetOrientation(const Point& from, const Point& to, bool slow)
{
	SetOrientation(GetOrient(from, to), slow);
}

void Movable::SetAttackMoveChances(const std::array<ieWord, 3>& amc)
{
	AttackMovements = amc;
}

//this could be used for WingBuffet as well
void Movable::MoveLine(int steps, orient_t orient)
{
	if (path || !steps) {
		return;
	}
	// DoStep takes care of stopping on walls if necessary
	path.AppendStep(area->GetLineEnd(Pos, steps, orient));
}

orient_t Movable::GetNextFace() const
{
	// slow turning
	if (timeStartStep == core->GetGame()->Ticks) {
		return Orientation;
	}
	return GemRB::GetNextFace(Orientation, NewOrientation);
}

void Movable::Backoff()
{
	StanceID = IE_ANI_READY;
	if (InternalFlags & IF_RUNNING) {
		randomBackoff = RAND(MAX_PATH_TRIES * 2 / 3, MAX_PATH_TRIES * 4 / 3);
	} else {
		randomBackoff = RAND(MAX_PATH_TRIES, MAX_PATH_TRIES * 2);
	}
}

void Movable::BumpAway()
{
	area->ClearSearchMapFor(this);
	if (!IsBumped()) oldPos = Pos;
	bumped = true;
	bumpBackTries = 0;
	area->AdjustPositionNavmap(Pos);
}

void Movable::BumpBack()
{
	if (Type != ST_ACTOR) return;
	const Actor* actor = (const Actor*) this;
	area->ClearSearchMapFor(this);
	// is the spot free again?
	PathMapFlags oldPosBlockStatus = area->GetBlocked(oldPos);
	if (!!(oldPosBlockStatus & PathMapFlags::PASSABLE)) {
		bumped = false;
		MoveTo(oldPos);
		bumpBackTries = 0;
		return;
	}

	// Do bump back if the actor is "blocking" itself
	if ((oldPosBlockStatus & PathMapFlags::ACTOR) == PathMapFlags::ACTOR && area->GetActor(oldPos, GA_NO_DEAD | GA_NO_UNSCHEDULED) == actor) {
		bumped = false;
		MoveTo(oldPos);
		bumpBackTries = 0;
		return;
	}

	// no luck, try again
	area->BlockSearchMapFor(this);
	if (actor->GetStat(IE_EA) < EA_GOODCUTOFF) {
		bumpBackTries++;
		if (bumpBackTries > MAX_BUMP_BACK_TRIES && SquaredDistance(Pos, oldPos) < unsigned(circleSize * 32 * circleSize * 32)) {
			oldPos = Pos;
			bumped = false;
			bumpBackTries = 0;
			if (SquaredDistance(Pos, Destination) < unsigned(circleSize * 32 * circleSize * 32)) {
				ClearPath(true);
			}
		}
	}
}

// Takes care of movement and actor bumping, i.e. gently pushing blocking actors out of the way
// The movement logic is a proportional regulator: the displacement/movement vector has a
// fixed radius, based on actor walk speed, and its direction heads towards the next waypoint.
// The bumping logic checks if there would be a collision if the actor was to move according to this
// displacement vector and then, if that is the case, checks if that actor can be bumped
// In that case, it bumps it and goes on with its step, otherwise it either stops and waits
// for a random time (inspired by network media access control algorithms) or just stops if
// the goal is close enough.
void Movable::DoStep(unsigned int walkScale, ieDword time)
{
	// Only bump back if not moving
	// Actors can be bumped while moving if they are backing off
	if (!path) {
		if (IsBumped()) {
			BumpBack();
		}
		return;
	}
	if (!time) time = core->GetGame()->Ticks;
	if (!walkScale) {
		// zero speed: no movement
		StanceID = IE_ANI_READY;
		timeStartStep = time;
		return;
	}
	if (time <= timeStartStep) {
		return;
	}

	const PathNode& step = path.GetCurrentStep();
	assert(!step.point.IsZero());

	Point nmptStep = step.point;
	float_t dx = nmptStep.x - Pos.x;
	float_t dy = nmptStep.y - Pos.y;
	Map::NormalizeDeltas(dx, dy, float_t(gamedata->GetStepTime()) / float_t(walkScale));
	if (dx == 0 && dy == 0) {
		// probably shouldn't happen, but it does when running bg2's cut28a set of cutscenes
		ClearPath(true);
		Log(DEBUG, "PathFinderWIP", "Abandoning because I'm exactly at the goal");
		pathAbandoned = true;
		return;
	}

	// trying to move should break the current modal action if a special effect is in place
	Actor* actor = Scriptable::As<Actor>(this);
	static EffectRef fx_modal_movement_check_ref = { "ModalStateCheck", -1 };
	if (actor && actor->fxqueue.HasEffect(fx_modal_movement_check_ref)) {
		actor->SetModal(Modal::None);
	}

	Actor* actorInTheWay = nullptr;
	// We can't use GetActorInRadius because we want to only check directly along the way
	// and not be blocked by actors who are on the sides
	int collisionLookaheadRadius = ((circleSize < 3 ? 3 : circleSize) - 1) * 3;
	for (int r = collisionLookaheadRadius; r > 0; r--) {
		auto xCollision = Pos.x + dx * r;
		auto yCollision = Pos.y + dy * r; // NormalizeDeltas already adjusted dy for perspective
		Point nmptCollision(xCollision, yCollision);
		actorInTheWay = area->GetActor(nmptCollision, GA_NO_DEAD | GA_NO_UNSCHEDULED | GA_NO_SELF, this);
		if (actorInTheWay) break;
	}

	bool blocksSearch = BlocksSearchMap();
	if (actorInTheWay && blocksSearch && actorInTheWay->BlocksSearchMap()) {
		// Give up instead of bumping if you are close to the goal
		if (path.Size() == 1 && PersonalDistance(nmptStep, this) < MAX_OPERATING_DISTANCE) {
			ClearPath(true);
			NewOrientation = Orientation;
			// Do not call ReleaseCurrentAction() since other actions
			// than MoveToPoint can cause movement
			Log(DEBUG, "PathFinderWIP", "Abandoning because I'm close to the goal");
			pathAbandoned = true;
			return;
		}
		if (actor && actor->ValidTarget(GA_CAN_BUMP) && actorInTheWay->ValidTarget(GA_ONLY_BUMPABLE)) {
			actorInTheWay->BumpAway();
		} else {
			Backoff();
			return;
		}
	}
	// Stop if there's a door in the way
	if (blocksSearch && !core->InCutSceneMode() && bool(area->GetBlocked(Pos + Point(dx, dy)) & PathMapFlags::SIDEWALL)) {
		Log(DEBUG, "PathFinder", "Abandoning because I'm in front of a wall");
		ClearPath(true);
		ReleaseCurrentAction(); // otherwise MoveToPoint and others that keep retrying will loop
		NewOrientation = Orientation;
		return;
	}
	if (blocksSearch) {
		area->ClearSearchMapFor(this);
	}
	StanceID = IE_ANI_WALK;
	if (InternalFlags & IF_RUNNING) {
		StanceID = IE_ANI_RUN;
	}
	SetPos(NavmapPoint(Pos.x + dx, Pos.y + dy));
	oldPos = Pos;
	if (actor && blocksSearch) {
		auto flag = actor->IsPartyMember() ? PathMapFlags::PC : PathMapFlags::NPC;
		area->tileProps.PaintSearchMap(SMPos, circleSize, flag);
	}

	SetOrientation(step.orient, false);
	timeStartStep = time;
	if (Pos == nmptStep) {
		path.nodes[path.currentStep].waypoint = false;
		++path.currentStep;
		if (path.currentStep >= path.Size()) {
			ClearPath(true);
			NewOrientation = Orientation;
			pathfindingDistance = circleSize;
		}
	}
}

void Movable::AddWayPoint(const Point& Des)
{
	if (!path) {
		WalkTo(Des);
		return;
	}
	Destination = Des;

	size_t steps = path.Size();
	PathNode& lastStep = path.nodes[steps - 1];
	area->ClearSearchMapFor(this);
	Path path2 = area->FindPath(lastStep.point, Des, circleSize);
	// if the waypoint is too close to the current position, no path is generated
	if (!path2) {
		if (BlocksSearchMap()) {
			area->BlockSearchMapFor(this);
		}
		return;
	}
	lastStep.waypoint = true;
	path.AppendPath(path2);
}

// This function is called at each tick if an actor is following another actor
// Therefore it's rate-limited to avoid actors being stuck as they keep pathfinding
void Movable::WalkTo(const Point& Des, int distance)
{
	// Only rate-limit when moving
	if (path && prevTicks && Ticks < prevTicks + 2) {
		return;
	}

	const Actor* actor = Scriptable::As<Actor>(this);

	prevTicks = Ticks;
	Destination = Des;
	if (pathAbandoned) {
		Log(DEBUG, "WalkTo", "{}: Path was just abandoned", fmt::WideToChar { actor->GetShortName() });
		ClearPath(true);
		return;
	}

	if (Pos.x / 16 == Des.x / 16 && Pos.y / 12 == Des.y / 12) {
		ClearPath(true);
		SetStance(IE_ANI_HEAD_TURN);
		return;
	}

	if (BlocksSearchMap()) area->ClearSearchMapFor(this);
	Path newPath = area->FindPath(Pos, Des, circleSize, distance, PF_SIGHT | PF_ACTORS_ARE_BLOCKING, actor);
	if (!newPath && actor && actor->ValidTarget(GA_CAN_BUMP)) {
		Log(DEBUG, "WalkTo", "{} re-pathing ignoring actors", fmt::WideToChar { actor->GetShortName() });
		newPath = area->FindPath(Pos, Des, circleSize, distance, PF_SIGHT, actor);
	}

	if (newPath) {
		ClearPath(false);
		path = std::move(newPath);
		HandleAnkhegStance(false);
	} else {
		pathfindingDistance = std::max(circleSize, distance);
		if (BlocksSearchMap()) {
			area->BlockSearchMapFor(this);
		}
	}
}

void Movable::RunAwayFrom(const Point& Source, int PathLength, bool noBackAway)
{
	ClearPath(true);
	area->ClearSearchMapFor(this);
	path = area->RunAway(Pos, Source, PathLength, !noBackAway, As<Actor>());
	HandleAnkhegStance(false);
}

void Movable::RandomWalk(bool can_stop, bool run)
{
	if (path) {
		return;
	}
	// if not continuous random walk, then stops for a while
	if (can_stop) {
		Region vp = core->GetGameControl()->Viewport();
		if (!vp.PointInside(Pos)) {
			SetWait(core->Time.defaultTicksPerSec * core->Roll(1, 40, 0));
			return;
		}
		// a 50/50 chance to move or do a spin (including its own wait)
		if (RandomFlip()) {
			Action* me = ParamCopy(CurrentAction);
			Action* turnAction = GenerateAction("RandomTurn()");
			// only spin once before relinquishing control back
			turnAction->int0Parameter = 3;
			// remove and readd ourselves, so the turning gets a chance to run
			ReleaseCurrentAction();
			ClearPath(false);
			AddActionInFront(me);
			AddActionInFront(turnAction);
			return;
		}
	}

	// handle the RandomWalkTime variants, which only count moves
	if (CurrentAction->int0Parameter && !CurrentAction->int1Parameter) {
		// first run only
		CurrentAction->int1Parameter = 1;
		CurrentAction->int0Parameter++;
	}
	if (CurrentAction->int0Parameter) {
		CurrentAction->int0Parameter--;
	}
	if (CurrentAction->int1Parameter && !CurrentAction->int0Parameter) {
		ReleaseCurrentAction();
		return;
	}

	randomWalkCounter++;
	if (randomWalkCounter > MAX_RAND_WALK) {
		randomWalkCounter = 0;
		WalkTo(HomeLocation);
		return;
	}

	if (run) {
		InternalFlags |= IF_RUNNING;
	}

	if (BlocksSearchMap()) {
		area->ClearSearchMapFor(this);
	}

	// the 5th parameter is controlling the orientation of the actor
	// 0 - back away, 1 - face direction
	PathNode randomStep = area->RandomWalk(Pos, circleSize, maxWalkDistance ? maxWalkDistance : 5, As<Actor>());
	if (BlocksSearchMap()) {
		area->BlockSearchMapFor(this);
	}
	if (!randomStep.point.IsZero()) {
		Destination = randomStep.point;
		path.PrependStep(randomStep); // start or end doesn't matter, since the path is currently empty
	} else {
		randomWalkCounter = 0;
		WalkTo(HomeLocation);
		return;
	}
}

void Movable::MoveTo(const Point& Des)
{
	area->ClearSearchMapFor(this);
	SetPos(Des);
	oldPos = Des;
	Destination = Des;
	if (BlocksSearchMap()) {
		area->BlockSearchMapFor(this);
	}
}

void Movable::Stop(int flags)
{
	Scriptable::Stop(flags);
	ClearPath(true);
}

void Movable::ClearPath(bool resetDestination)
{
	pathAbandoned = false;

	if (resetDestination) {
		// this is to make sure attackers come to us
		// make sure ClearPath doesn't screw Destination (in the rare cases Destination
		// is set before ClearPath
		Destination = Pos;

		if (StanceID == IE_ANI_WALK || StanceID == IE_ANI_RUN) {
			StanceID = IE_ANI_AWAKE;
		}
		HandleAnkhegStance(true);
		InternalFlags &= ~IF_NORETICLE;
	}
	path.Clear();
	// don't call ReleaseCurrentAction
}

// (un)hide ankhegs when they stop/start moving
void Movable::HandleAnkhegStance(bool emerge)
{
	const Actor* actor = As<Actor>();
	int nextStance = emerge ? IE_ANI_EMERGE : IE_ANI_HIDE;
	if (actor && path && StanceID != nextStance && actor->GetAnims()->GetAnimType() == IE_ANI_TWO_PIECE) {
		SetStance(nextStance);
		SetWait(15); // both stances have 15 frames, at 15 fps
	}
}

}
