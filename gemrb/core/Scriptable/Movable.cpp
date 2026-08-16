// SPDX-FileCopyrightText: 2024 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Movable.h"

#include "ie_stats.h"

#include "Actor.h"
#include "Interface.h"
#include "Map.h"
#include "PathFinderScheduler.h"

#include "GUI/GameControl.h"
#include "GameScript/GSUtils.h"
#include "GameScript/GameScript.h"

namespace GemRB {

// Diagnostics helper: who is this, and what tick is it
static std::string MoveTag(const Movable* who)
{
	return fmt::format("[t={}] {}", core->GetGame() ? core->GetGame()->Ticks : 0, who->GetScriptName());
}

Point Movable::GetMostLikelyPosition() const
{
	// what matters here is whether there is a path to extrapolate along, not the request stage:
	// with FindPathScheduled the actor may still be walking a previous path - see MovementState
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

void Movable::SetStanceDirect(unsigned int arg)
{
	StanceID = static_cast<unsigned char>(arg);
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
		SetStanceDirect(IE_ANI_AWAKE);
		Log(ERROR, "Movable", "Tried to set invalid stance id({})", arg);
		return;
	}

	Actor* caster = Scriptable::As<Actor>(this);
	if (StanceID == IE_ANI_CONJURE && StanceID != arg) {
		if (caster && caster->castingSound) {
			caster->castingSound->Stop();
			caster->castingSound.reset();
		}
	}

	SetStanceDirect(arg);

	if (StanceID == IE_ANI_ATTACK) {
		// Set stance to a random attack animation
		int random = RAND(0, 99);
		if (random < AttackMovements[0]) {
			SetStanceDirect(IE_ANI_ATTACK_BACKSLASH);
		} else if (random < AttackMovements[0] + AttackMovements[1]) {
			SetStanceDirect(IE_ANI_ATTACK_SLASH);
		} else {
			SetStanceDirect(IE_ANI_ATTACK_JAB);
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
	// Only start a line move when nothing else is in progress; the Moving state set below is what
	// makes this guard reject re-entry while the line is still being walked.
	if (HasMovementInProgress() || !steps) {
		return;
	}
	// DoStep takes care of stopping on walls if necessary
	path.AppendStep(area->GetLineEnd(Pos, steps, orient));
	// the path is built here rather than requested from the pathfinder, so the state has to be
	// advanced by hand - DoStep ignores a path while the state is NoMovement
	SetMovementState(MovementState::Moving);
}

orient_t Movable::GetNextFace() const
{
	// slow turning
	if (timeStartStep == core->GetGame()->Ticks) {
		return Orientation;
	}
	return GemRB::GetNextFace(Orientation, NewOrientation);
}

void Movable::SetMovementState(const MovementState InNewMovementState)
{
	if (movementState == InNewMovementState) {
		return;
	}
	movementState = InNewMovementState;
}

FindPathRequestPriority Movable::GetFindPathRequestPriority() const
{
	return FindPathRequestPriority::Normal;
}

void Movable::Backoff()
{
	SetStanceDirect(IE_ANI_READY);
	if (InternalFlags & IF_RUNNING) {
		randomBackoff = RAND(MAX_PATH_TRIES * 2 / 3, MAX_PATH_TRIES * 4 / 3);
	} else {
		randomBackoff = RAND(MAX_PATH_TRIES, MAX_PATH_TRIES * 2);
	}
}

Movable::~Movable()
{
	if (!pathRequestId.IsNull()) {
		PathFinderScheduler::CancelPath(pathRequestId);
	}
}

void Movable::BumpAway()
{
	area->ClearSearchMapFor(this);
	if (!IsBumped()) oldPos = Pos;
	bumped = true;
	bumpBackTries = 0;
	const Point beforeBump = Pos;
	area->AdjustPositionNavmap(Pos);
	LogDebugPathfinder("Movable::BumpAway", "{}: bumped away {} -> {} (oldPos={}, landed on flags={})",
			   MoveTag(this), beforeBump, Pos, oldPos, uint8_t(area->GetBlocked(Pos)));
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
	if (!HasMovementInProgress()) {
		if (IsBumped()) {
			BumpBack();
		}
		return;
	}
	if (!time) time = core->GetGame()->Ticks;
	if (!walkScale) {
		// zero speed: no movement
		SetStanceDirect(IE_ANI_READY);
		timeStartStep = time;
		return;
	}
	if (time <= timeStartStep) {
		return;
	}

	if (GetMovementState() == MovementState::FindPathScheduled) {
		if (PathFinderScheduler::IsPathCalculated(pathRequestId)) {
			LogDebugPathfinder("Movable::DoStep", "{}: request ID={} completed, taking calculated path from the scheduler",
					   MoveTag(this), pathRequestId.GetId());
			Path newPath = PathFinderScheduler::TakeCalculatedPath(pathRequestId);
			const FindPathRequest pathRequest = PathFinderScheduler::TakeCompletedRequest(pathRequestId);
			OnPathCalculated(std::move(newPath), pathRequest);
		} else if (!PathFinderScheduler::IsRequestLive(pathRequestId)) {
			LogDebugPathfinder("Movable::DoStep", "{}: request ID={} vanished from the scheduler without a result",
					   MoveTag(this), pathRequestId.GetId());
			// The request left the scheduler without producing a result, so nothing will ever
			// answer it. Resolve the state by hand: WalkTo() refuses to reissue while a request is
			// in flight, so staying here would strand this actor for the rest of the game.
			pathRequestId = FindPathRequestId::NullId();
			SetMovementState(path ? MovementState::Moving : MovementState::NoMovement);
		}
		// Not ready yet: fall through and keep walking the path we already hold, if any. The round
		// trip costs a tick or two - two Sync() cycles, unless DrainCompletedPathsEarly() catches
		// the result mid-tick and saves one - and standing still for that window would show up as
		// a visible actor stall, so an actor never goes pathless while repathing. With no previous
		// path the `!path` check below still parks the actor until the first result lands.
	}

	if (!path) {
		// the pathless window: this is the frame budget the async round trip actually costs
		return;
	}

	const PathNode& step = path.GetCurrentStep();
	assert(!step.point.IsZero());

	Point nmptStep = step.point;
	float_t dx = nmptStep.x - Pos.x;
	float_t dy = nmptStep.y - Pos.y;
	PathFinder::NormalizeDeltas(dx, dy, float_t(gamedata->GetStepTime()) / float_t(walkScale));
	if (dx == 0 && dy == 0) {
		// probably shouldn't happen, but it does when running bg2's cut28a set of cutscenes
		LogDebugPathfinder("Movable::DoStep", "{}: ZERO-DELTA ABANDON at Pos={} step={} step {}/{} dest={}",
				   MoveTag(this), Pos, nmptStep, path.currentStep, path.Size(), Destination);
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
		// the cut-off should be max 1 foot, so attacking with close-ranged weapons is unlikely to stop approaching too soon
		// attacking actions already take weapon range into account when
		// triggering movement, so this here does not mean we go needlessly close
		if (path.Size() == 1 && WithinPersonalRange(this, nmptStep, 1)) {
			LogDebugPathfinder("Movable::DoStep", "{}: NEAR-GOAL ABANDON at Pos={} step={} dest={} blocker={} at {}",
					   MoveTag(this), Pos, nmptStep, Destination,
					   actorInTheWay->GetScriptName(), actorInTheWay->Pos);
			ClearPath(true);
			NewOrientation = Orientation;
			// Do not call ReleaseCurrentAction() since other actions
			// than MoveToPoint can cause movement
			Log(DEBUG, "PathFinderWIP", "Abandoning because I'm close to the goal");
			pathAbandoned = true;
			return;
		}
		if (actor && actor->ValidTarget(GA_CAN_BUMP) && actorInTheWay->ValidTarget(GA_ONLY_BUMPABLE)) {
			LogDebugPathfinder("Movable::DoStep", "{}: bumping {} away from {} (I'm at {}, step {}/{})",
					   MoveTag(this), actorInTheWay->GetScriptName(), actorInTheWay->Pos,
					   Pos, path.currentStep, path.Size());
			actorInTheWay->BumpAway();
		} else {
			LogDebugPathfinder("Movable::DoStep", "{}: backing off, {} at {} blocks me at {} (step {}/{})",
					   MoveTag(this), actorInTheWay->GetScriptName(), actorInTheWay->Pos,
					   Pos, path.currentStep, path.Size());
			Backoff();
			return;
		}
	}
	// Stop if there's a door in the way
	const Point wallProbe = Pos + Point(dx, dy);
	if (blocksSearch && !core->InCutSceneMode() && bool(area->GetBlocked(wallProbe) & PathMapFlags::SIDEWALL)) {
		LogDebugPathfinder("Movable::DoStep", "{}: WALL ABANDON at Pos={} SM={} probe={} probeFlags={} stepTarget={} "
						      "delta=({},{}) step {}/{} dest={} bumped={} probeRadiusFlags={} posFlags={}",
				   MoveTag(this), Pos, fmt::format("({},{})", SMPos.x, SMPos.y), wallProbe,
				   uint8_t(area->GetBlocked(wallProbe)), nmptStep, dx, dy,
				   path.currentStep, path.Size(), Destination, bumped,
				   uint8_t(area->GetBlockedInRadius(wallProbe, circleSize)),
				   uint8_t(area->GetBlocked(Pos)));
		Log(DEBUG, "PathFinder", "Abandoning because I'm in front of a wall");
		ClearPath(true);
		ReleaseCurrentAction(); // otherwise MoveToPoint and others that keep retrying will loop
		NewOrientation = Orientation;
		return;
	}
	if (blocksSearch) {
		area->ClearSearchMapFor(this);
	}
	SetStanceDirect(IE_ANI_WALK);
	if (InternalFlags & IF_RUNNING) {
		SetStanceDirect(IE_ANI_RUN);
	}
	SetPos(NavmapPoint(Pos.x + dx, Pos.y + dy));
	oldPos = Pos;
	if (actor && blocksSearch) {
		auto flag = actor->IsInExtendedParty() ? PathMapFlags::PC : PathMapFlags::NPC;
		area->tileProps.PaintSearchMap(SMPos, circleSize, flag);
	}

	SetOrientation(step.orient, false);
	timeStartStep = time;
	if (Pos == nmptStep) {
		path.nodes[path.currentStep].waypoint = false;
		++path.currentStep;
		if (path.currentStep >= path.Size()) {
			// Walked the path out. A request already in flight is the *continuation* of this
			// journey - when chasing a moving target the path just finished aimed at where the
			// target used to be, and the pending one aims at where it is now - so it must
			// survive. ClearPath() cancels it unconditionally, which would drop the actor to
			// NoMovement and reset its stance, leaving it standing short of the target until
			// something happened to issue a fresh request. Just drop the spent path instead and
			// stay in FindPathScheduled; the `!path` check above parks the actor for the frame
			// or two until the result lands, and an empty result still resolves to NoMovement
			// through OnPathCalculated.
			if (GetMovementState() == MovementState::FindPathScheduled) {
				path.Clear();
			} else {
				ClearPath(true);
			}
			NewOrientation = Orientation;
			pathfindingDistance = circleSize;
		}
	}
}

void Movable::AddWayPoint(const Point& Des)
{
	// A waypoint extends a path that is already being walked; with nothing to extend this degrades
	// to a plain WalkTo. The test is on the path rather than on the movement state: an actor in
	// FindPathScheduled may well be walking a previous path (see MovementState), and that path
	// extends just as safely - testing the state would silently drop the waypoint for the two
	// frames a re-path is in flight. A non-empty path is also what makes the last-node access
	// below safe.
	if (!path) {
		// A waypoint is a new order, not the retry of a failed one, so it must not be answered by a
		// stale PathSearchFailed: WalkTo() would consume that and file nothing, and GameScript's
		// AddWayPoint releases its action without ever testing InMove(), so the waypoint would be
		// dropped for good. Discard the pending verdict and let the search actually run.
		if (GetMovementState() == MovementState::PathSearchFailed) {
			SetMovementState(MovementState::NoMovement);
		}
		WalkTo(Des);
		return;
	}
	Destination = Des;

	const size_t steps = path.Size();
	const PathNode& lastStep = path.nodes[steps - 1];

	ScheduleFindPath(FindPathRequestType::AddWaypoint, lastStep.point, Des, 0);
}

void Movable::ScheduleFindPath(
	const FindPathRequestType InRequestType, const Point& InSource, const Point& InDestination, const int InMinDistance,
	Map* InOptionalMap, const int InOptionalAdditionalFlags)
{
	// first cancel a path if any was issued before
	if (!pathRequestId.IsNull()) {
		PathFinderScheduler::CancelPath(pathRequestId);
	}

	// prepare parameters for the request; the actor data is read here, on the main thread,
	// because the request itself only ever carries our pointer identity
	const Actor* self = Scriptable::As<Actor>(this);
	Map* requestMap = InOptionalMap ? InOptionalMap : area;
	const int actorSpeed = self ? self->GetSpeed() : 0;
	bool canRePathIgnoringActors = false;
	int pathfindingFlags = PF_SIGHT | InOptionalAdditionalFlags;
	if (InRequestType == FindPathRequestType::WalkTo) {
		canRePathIgnoringActors = self && self->ValidTarget(GA_CAN_BUMP);
		pathfindingFlags |= PF_ACTORS_ARE_BLOCKING;
	}

	// prepare request
	FindPathRequest pathfindingRequest;
	pathfindingRequest.PutBasicRequestData(
		InRequestType,
		GetFindPathRequestPriority(),
		this,
		GetScriptName());

	pathfindingRequest.PutPathData(
		InSource,
		InDestination,
		requestMap,
		pathfindingFlags);

	pathfindingRequest.PutActorData(
		SMPos,
		circleSize,
		static_cast<unsigned int>(InMinDistance),
		actorSpeed,
		canRePathIgnoringActors,
		BlocksSearchMap());

	// note appropriate movement state and submit the request
	SetMovementState(MovementState::FindPathScheduled);
	pathRequestId = PathFinderScheduler::RequestPath(std::move(pathfindingRequest));
	LogDebugPathfinder("Movable::ScheduleFindPath", "{}: filed request ID={} type={} from {} to {} minDist={} flags={} canRepathIgnoringActors={}",
			   MoveTag(this), pathRequestId.GetId(), int(InRequestType), InSource, InDestination,
			   InMinDistance, pathfindingFlags, canRePathIgnoringActors);
}

void Movable::OnPathCalculated(Path&& newPath, const FindPathRequest& pathRequest)
{
	LogDebugPathfinder("Movable::OnPathCalculated", "{}: request ID={} type={} came back with {} nodes (first={}, last={}), "
							"I'm at {}, requested source={} dest={}",
			   MoveTag(this), pathRequestId.GetId(), int(pathRequest.requestType), newPath.Size(),
			   newPath ? newPath.GetStep(0).point : Point(), newPath ? newPath.nodes.back().point : Point(),
			   Pos, pathRequest.source, pathRequest.destination);

	PathFinderScheduler::RemoveFoundPath(pathRequestId);
	pathRequestId = FindPathRequestId::NullId();

	switch (pathRequest.requestType) {
		case FindPathRequestType::WalkTo:
			// intentional fallthrough, mostly common implementation
		case FindPathRequestType::WalkToFromNewPath:
			if (newPath && newPath != path) {
				ClearPath(false);
				path = std::move(newPath);
				SetMovementState(MovementState::Moving);
				HandleAnkhegStance(false);
				return;
			}

			pathfindingDistance = std::max(circleSize, static_cast<int>(pathRequest.minDistance));
			// if we had an old path and new path was not correct, restore
			// the movement_state based on the currently held old path
			if (!path) {
				// the caller that ordered this move has to learn the search failed.
				// Park the dead end in the state until then, or the move actions
				// will re-file the same hopeless request forever.
				SetMovementState(MovementState::PathSearchFailed);
				lastFailedDestination = pathRequest.destination;
				// Count the failed try here rather than where the request was issued: the search
				// completes asynchronously, so failure is only known once the result arrives.
				// WalkToFromNewPath marks a request as originating from Actor::NewPath, which is
				// what MAX_PATH_TRIES caps.
				if (pathRequest.requestType == FindPathRequestType::WalkToFromNewPath) {
					IncrementPathTries();
				}
			} else {
				SetMovementState(MovementState::Moving);
			}
			break;
		case FindPathRequestType::AddWaypoint:
			// if the waypoint is too close to the current position, no path is generated
			if (!newPath) {
				// AddWayPoint() only files an `AddWaypoint` path request, while a path is being walked (with
				// no path it degrades to WalkTo). Setting here `NoMovement` unconditionally would strand an actor
				// still holding its original path.
				SetMovementState(path ? MovementState::Moving : MovementState::NoMovement);
				return;
			}
			if (path) {
				const size_t steps = path.Size();
				PathNode& lastStep = path.nodes[steps - 1];
				lastStep.waypoint = true;
			}
			path.AppendPath(newPath);
			SetMovementState(MovementState::Moving);
			break;
		case FindPathRequestType::RunAway:
			path = std::move(newPath);
			HandleAnkhegStance(false);
			SetMovementState(path ? MovementState::Moving : MovementState::NoMovement);
			break;
	}
}

// This function is called at each tick if an actor is following another actor
// Therefore it's rate-limited to avoid actors being stuck as they keep pathfinding
void Movable::WalkTo(const Point& Des, int distance, FindPathRequestType InRequestType)
{
	// A request is already in flight - let it finish, never reissue on top of it.
	// ScheduleFindPath() cancels the pending request, and CancelPath() also erases an
	// already-computed path from FoundPaths, while the pathfinder's round trip is up to two
	// Sync() cycles: a request filed in frame N is dispatched by that frame's Sync(), drained
	// into FoundPaths by frame N+1's, and first visible to DoStep() in frame N+2 - one frame
	// sooner when DrainCompletedPathsEarly() picks the result up mid-tick. Since WalkTo() runs
	// before DoStep() within a frame (Map::UpdateScripts), reissuing on any interval at or below
	// that latency destroys every result one call before it would be consumed, and the actor
	// never moves at all. The wait is bounded by requestExpirationFrames, or by DoStep()'s
	// IsRequestLive() check if the request left the scheduler without producing a result.
	if (GetMovementState() == MovementState::FindPathScheduled) {
		LogDebugPathfinder("Movable::WalkTo", "{}: skipped, request {} still in flight (dest={})",
				   MoveTag(this), pathRequestId.GetId(), Des);
		return;
	}

	// Only rate-limit an actor that is actually walking. Gating on `path` instead would also catch
	// an actor whose re-path is in flight, since it keeps walking the path it still holds.
	if (GetMovementState() == MovementState::Moving && prevTicks && Ticks < prevTicks + 2) {
		return;
	}

	// Report a search that already came back empty for this exact destination. The move actions
	// give up by testing InMove() right after WalkTo(), which the synchronous pathfinder could
	// answer within the call; here the answer is the state left behind by OnPathCalculated() a few
	// frames ago. Drop to NoMovement without filing anything, and that InMove() test - false for
	// both idle states - fires exactly as it used to. Consuming the state keeps it one-shot, so a
	// later order for the same spot still gets a fresh search.
	// A different destination is not covered by the failure and falls through to a real search.
	if (GetMovementState() == MovementState::PathSearchFailed) {
		SetMovementState(MovementState::NoMovement);
		if (Des == lastFailedDestination) {
			LogDebugPathfinder("Movable::WalkTo", "{}: reporting earlier search failure for dest={}, filing nothing",
					   MoveTag(this), Des);
			Destination = Des;
			return;
		}
	}

	const Actor* actor = Scriptable::As<Actor>(this);

	prevTicks = Ticks;
	Destination = Des;
	if (pathAbandoned) {
		LogDebugPathfinder("Movable::WalkTo", "{}: refusing dest={}, path was abandoned last step", MoveTag(this), Des);
		Log(DEBUG, "WalkTo", "{}: Path was just abandoned", fmt::WideToChar { actor->GetShortName() });
		ClearPath(true);
		return;
	}

	if (Pos.x / 16 == Des.x / 16 && Pos.y / 12 == Des.y / 12) {
		ClearPath(true);
		SetStance(IE_ANI_HEAD_TURN);
		return;
	}

	ScheduleFindPath(InRequestType, Pos, Des, distance);
}

void Movable::RunAwayFrom(const Point& Source, int PathLength, bool noBackAway)
{
	ClearPath(true);
	area->RunAway(Pos, Source, PathLength, !noBackAway, As<Actor>());
}

void Movable::RandomWalk(bool can_stop, bool run)
{
	// Only start a random walk when nothing else is in progress; the Moving state set at the end
	// is what makes this guard reject re-entry while the step is still being walked.
	if (HasMovementInProgress()) {
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

	PathNode randomStep;
	const bool foundStep = area->RandomWalk(Pos, circleSize, maxWalkDistance ? maxWalkDistance : 5, As<Actor>(), randomStep);
	if (BlocksSearchMap()) {
		area->BlockSearchMapFor(this);
	}
	if (foundStep) {
		Destination = randomStep.point;
		path.PrependStep(std::move(randomStep)); // start or end doesn't matter, since the path is currently empty
		// the path is built here rather than requested from the pathfinder, so the state has to be
		// advanced by hand - DoStep ignores a path while the state is NoMovement
		SetMovementState(MovementState::Moving);
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
	if (HasMovementInProgress()) {
		LogDebugPathfinder("Movable::ClearPath", "{}: dropping movement (state={}, {} path nodes left, request ID={}, at {} heading for {})",
				   MoveTag(this), int(GetMovementState()), path.Size(), pathRequestId.GetId(), Pos, Destination);
	}
	pathAbandoned = false;

	if (resetDestination) {
		// this is to make sure attackers come to us
		// make sure ClearPath doesn't screw Destination (in the rare cases Destination
		// is set before ClearPath
		Destination = Pos;

		if (StanceID == IE_ANI_WALK || StanceID == IE_ANI_RUN) {
			SetStanceDirect(IE_ANI_AWAKE);
		}
		HandleAnkhegStance(true);
		InternalFlags &= ~IF_NORETICLE;
	}
	path.Clear();
	SetMovementState(MovementState::NoMovement);
	PathFinderScheduler::CancelPath(pathRequestId);
	pathRequestId = FindPathRequestId::NullId();
	// don't call ReleaseCurrentAction
}

// (un)hide ankhegs when they stop/start moving
void Movable::HandleAnkhegStance(bool emerge)
{
	const Actor* actor = As<Actor>();
	int nextStance = emerge ? IE_ANI_EMERGE : IE_ANI_HIDE;
	// Gated on the path, not on the movement state. Both callers run before the state is settled:
	// OnPathCalculated's RunAway branch assigns `path` and calls us while still in
	// FindPathScheduled, and ClearPath calls us before it clears the path and drops to NoMovement.
	// A state test is therefore false exactly when the stance needs changing.
	if (actor && path && StanceID != nextStance && actor->GetAnims()->GetAnimType() == IE_ANI_TWO_PIECE) {
		SetStance(nextStance);
		SetWait(15); // both stances have 15 frames, at 15 fps
	}
}

}
