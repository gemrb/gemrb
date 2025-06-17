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

#ifndef MOVABLE_H
#define MOVABLE_H

#include "CharAnimations.h"
#include "Region.h"
#include "Selectable.h"

namespace GemRB {

class GEM_EXPORT Movable : public Selectable {
private: // these seem to be sensitive, so get protection
	unsigned char StanceID = 0;
	orient_t Orientation = S;
	orient_t NewOrientation = S;
	std::array<ieWord, 3> AttackMovements = { 100, 0, 0 };

	Path path; // whole path
	unsigned int prevTicks = 0;
	int bumpBackTries = 0;
	bool pathAbandoned = false;

protected:
	ieDword timeStartStep = 0;
	// the # of previous tries to pick up a new walkpath
	int pathTries = 0;
	int randomBackoff = 0;
	Point oldPos = Pos;
	bool bumped = false;
	int pathfindingDistance = circleSize;
	int randomWalkCounter = 0;

public:
	inline int GetRandomBackoff() const
	{
		return randomBackoff;
	}
	void Backoff();
	inline void DecreaseBackoff()
	{
		randomBackoff--;
	}
	using Selectable::Selectable;
	Movable(const Movable&) = delete;
	Movable& operator=(const Movable&) = delete;

	Point Destination = Pos;
	ResRef AreaName;
	Point HomeLocation; // spawnpoint, return here after rest
	ieWord maxWalkDistance = 0; // maximum random walk distance from home

public:
	inline void ImpedeBumping()
	{
		oldPos = Pos;
		bumped = false;
	}
	void BumpAway();
	void BumpBack();
	inline bool IsBumped() const { return bumped; }
	PathNode GetNextStep(int x) const;
	inline const Path& GetPath() const { return path; };
	inline int GetPathTries() const { return pathTries; }
	inline void IncrementPathTries() { pathTries++; }
	inline void ResetPathTries() { pathTries = 0; }
	// inliners to protect data consistency
	inline bool IsMoving() const
	{
		return (StanceID == IE_ANI_WALK || StanceID == IE_ANI_RUN);
	}

	orient_t GetNextFace() const;

	inline orient_t GetOrientation() const
	{
		return Orientation;
	}

	inline unsigned char GetStance() const
	{
		return StanceID;
	}

	void SetStance(unsigned int arg);
	void SetOrientation(orient_t value, bool slow);
	void SetOrientation(const Point& from, const Point& to, bool slow);
	void SetAttackMoveChances(const std::array<ieWord, 3>& amc);
	virtual void DoStep(unsigned int walkScale, ieDword time = 0);
	void AddWayPoint(const Point& Des);
	void RunAwayFrom(const Point& Des, int PathLength, bool noBackAway);
	void RandomWalk(bool can_stop, bool run);
	int GetRandomWalkCounter() const { return randomWalkCounter; };
	void MoveLine(int steps, orient_t Orient);
	void WalkTo(const Point& Des, int MinDistance = 0);
	void MoveTo(const Point& Des);
	void Stop(int flags = 0) override;
	void ClearPath(bool resetDestination = true);
	void HandleAnkhegStance(bool emerge);

	/* returns the most likely position of this actor */
	Point GetMostLikelyPosition() const;
	virtual bool BlocksSearchMap() const = 0;
};

}

#endif
