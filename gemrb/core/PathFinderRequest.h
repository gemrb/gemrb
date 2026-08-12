// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef PATHFINDERREQUEST_H
#define PATHFINDERREQUEST_H

// The vocabulary types of a pathfinding request

#include "ie_types.h"

#include "Region.h"

#include <cstddef>
#include <cstdint>
#include <limits>

namespace GemRB {

class Map;
class Movable;

/**
 * The requesting actor's properties, as snapshotted when the request was made.
 */
struct ActorPathContext {
	unsigned int circleSize = 0;
	const Movable* identity = nullptr; // pointer identity only, never dereferenced
	int speed = 0;
	ieVariable scriptName;
};

/**
 *  Specifies the type of pathfinding operation to perform after the path is calculated.
 *  Different request types may affect how the path is computed, merged with existing paths,
 *  or applied to the actor's movement queue.
 */
enum class FindPathRequestType : uint8_t {
	WalkTo,
	WalkToFromNewPath,
	AddWaypoint,
	RunAway,
};

/**
 *  Defines scheduling priorities for pathfinding requests.
 *  Higher priority requests (lower numeric value) are processed before lower priority ones.
 *  The `Immediate` priority bypasses the request queues entirely and executes synchronously.
 */
enum class FindPathRequestPriority : uint8_t {
	Highest = 0, ///< For player's party - highest async priority
	High = 1, ///< For NPCs in combat
	Normal = 2, ///< For background NPCs, random walks, etc.
	Immediate = 3, ///< Special priority: forces immediate, synchronous calculation. Keep it last in enum.
};

/** Number of distinct priority queues used by the scheduler (excludes Immediate) */
constexpr size_t ScheduledQueuesPrioritiesCount = static_cast<size_t>(FindPathRequestPriority::Normal) + 1;

// The scheduler uses a priority's numeric value directly as an index into
// workerScheduledQueuesByPriority (see Sync() step 3), an unchecked raw-array subscript.
// Immediate is deliberately not counted: RequestPath() computes those synchronously and they
// never enter the queues, so Immediate sits exactly one past the end of the array.
static_assert(static_cast<size_t>(FindPathRequestPriority::Highest) == 0,
	      "queued priorities are used as array indices and must start at 0");
static_assert(static_cast<size_t>(FindPathRequestPriority::Immediate) == ScheduledQueuesPrioritiesCount,
	      "Immediate must be the last enumerator, directly after Normal - otherwise "
	      "ScheduledQueuesPrioritiesCount undercounts the queued priorities and Sync() "
	      "indexes workerScheduledQueuesByPriority out of bounds");


/**
 * Represents a request to find a path between two locations in the navigation system.
 * Encapsulates all necessary information for the pathfinding algorithm to compute a route,
 * including start and end positions, navigation constraints, and request-specific parameters.
 */
struct FindPathRequest {
	FindPathRequestPriority priority = FindPathRequestPriority::Normal;
	Point source;
	Point destination;
	SearchmapPoint actorSMPos; // actor's searchmap position, snapshotted at request time
	// The Movable that issued this request. Never dereferenced on worker threads: it is used
	// only for pointer identity comparisons (skipping the requester's own footprint when
	// repainting the searchmap, and recognising its own cell as bumpable). Actor is the only
	// Movable subclass, so comparisons against the const Actor* stored in ActorSearchMapData
	// and TraversabilityCellData are plain upcasts.
	Movable* instigatorIdentity = nullptr;
	ieVariable instigatorScriptName;
	Map* map = nullptr; // never dereferenced on worker threads, used in immediate calculation flow and for getting ID
	int actorCircleSize = 0; // actor's collision circle size, snapshotted at request time
	unsigned int minDistance = 0;
	int actorSpeed = 0; // actor's walk speed, snapshotted at request time
	int pathfindingFlags = 0;
	FindPathRequestType requestType = FindPathRequestType::WalkTo;
	bool canRePathIgnoringActors = false;
	bool blocksSearchMaps = false;

	/**
	 *  Sets the basic request metadata.
	 *  Builder method for setting request type, priority, and the instigating Movable.
	 *
	 *  @param inRequestType The type of pathfinding operation (WalkTo, RunAway, etc.)
	 *  @param inPriority The scheduling priority for this request
	 *  @param inInstigatorIdentity Pointer identity of the Movable that initiated the request
	 *  @param inInstigatorScriptName The requester's script name, for debug logging
	 *  @return Reference to this request for method chaining
	 */
	FindPathRequest& PutBasicRequestData(const FindPathRequestType inRequestType,
					     const FindPathRequestPriority inPriority,
					     Movable* inInstigatorIdentity,
					     const ieVariable& inInstigatorScriptName)
	{
		requestType = inRequestType;
		priority = inPriority;
		instigatorIdentity = inInstigatorIdentity;
		instigatorScriptName = inInstigatorScriptName;
		return *this;
	}

	/**
	 *  Sets the pathfinding spatial data.
	 *  Builder method for setting source, destination, map reference, and pathfinding flags.
	 *
	 *  @param inSource The starting position for pathfinding
	 *  @param inDestination The target position for pathfinding
	 *  @param inMap Pointer identity of the Map containing the path (not dereferenced on worker threads)
	 *  @param inPathfindingFlags Bitfield of pathfinding behavior flags
	 *  @return Reference to this request for method chaining
	 */
	FindPathRequest& PutPathData(
		const Point& inSource,
		const Point& inDestination,
		Map* inMap,
		const int inPathfindingFlags)
	{
		source = inSource;
		destination = inDestination;
		map = inMap;
		pathfindingFlags = inPathfindingFlags;
		return *this;
	}

	/**
	 *  Sets the actor-specific pathfinding data.
	 *  Builder method for setting collision properties, movement parameters,
	 *  and retry behavior flags. The requester's identity is set by PutBasicRequestData().
	 *
	 *  @param inActorSMPos Actor's searchmap position snapshot
	 *  @param inActorCircleSize Actor's collision circle size snapshot
	 *  @param inMinDistance Minimum acceptable distance from destination
	 *  @param inActorSpeed Actor's walk speed snapshot
	 *  @param inCanRePathIgnoringActors Whether pathfinding should retry ignoring actors if it fails
	 *  @param inBlocksSearchMaps Whether this actor blocks searchmaps
	 *  @return Reference to this request for method chaining
	 */
	FindPathRequest& PutActorData(const SearchmapPoint& inActorSMPos,
				      const int inActorCircleSize,
				      const unsigned int inMinDistance,
				      const int inActorSpeed,
				      const bool inCanRePathIgnoringActors,
				      const bool inBlocksSearchMaps)
	{
		actorSMPos = inActorSMPos;
		actorCircleSize = inActorCircleSize;
		minDistance = inMinDistance;
		actorSpeed = inActorSpeed;
		canRePathIgnoringActors = inCanRePathIgnoringActors;
		blocksSearchMaps = inBlocksSearchMaps;
		return *this;
	}
};

/**
 *  Unique identifier for pathfinding requests in the navigation system.
 *  Provides a lightweight, copyable handle to track and reference pathfinding operations
 *  across threads and scheduling queues.
 *  IDs are automatically generated in sequence and wrap around when reaching the maximum value.
 *  The identifier zero is reserved for the null request.
 *  Includes hash and equality functors for use with standard containers.
 */
class FindPathRequestId {
public:
	// don't allow to default-construct by the user
	FindPathRequestId() = delete;

	/**
	 *  Returns a special null/invalid request identifier.
	 *  Used to represent the absence of a valid request ID.
	 *
	 *  @return A FindPathRequestId with ID value 0, representing null
	 */
	static FindPathRequestId NullId()
	{
		static FindPathRequestId nullId { 0 };
		return nullId;
	}

	/**
	 *  Generates the next unique request identifier in sequence.
	 *  ID 0 is reserved for null, so valid IDs range from 1 to 65534.
	 *
	 *  @return A new unique FindPathRequestId
	 */
	static FindPathRequestId CreateNextId()
	{
		static uint16_t lastUsedID = 1;

		// Wrapping is accepted. A collision needs a request to still be live when the counter
		// laps back onto its ID, 65534 requests later. An ID does not stay live long: incoming
		// requests are cleared every Sync(), scheduled ones are dropped after
		// requestExpirationFrames, and a computed path is consumed on the next DoStep(). Each
		// Movable also holds only one request at a time - RequestPath() cancels the previous one
		// from the same instigator - so the issue rate is bounded by the actor count. Lapping
		// within that window would take on the order of a thousand actors requesting a path every
		// frame, sustained.
		// If it ever did happen, two live requests would share a key and lookups by that ID could
		// return the other request's data and this  would misroute an actor.
		// For IE games, it's not reachable in any circumstance.
		++lastUsedID;
		if (lastUsedID == std::numeric_limits<uint16_t>::max()) {
			lastUsedID = 1;
		}
		return FindPathRequestId { lastUsedID };
	}

	/**
	 *  Checks if this ID represents a null/invalid request.
	 *
	 *  @return True if this is the null ID (value 0), false otherwise
	 */
	bool IsNull() const
	{
		return GetId() == 0;
	}

	/**
	 *  Returns the raw numeric identifier value.
	 *
	 *  @return The uint16_t ID value
	 */
	uint16_t GetId() const
	{
		return id;
	}

	/**
	 *  Hash functor for use with std::unordered_map and std::unordered_set.
	 *  Allows FindPathRequestId to be used as a key in hash-based containers.
	 */
	struct Hash {
		std::size_t operator()(const FindPathRequestId& requestId) const
		{
			return requestId.GetId();
		}
	};

	bool operator==(const FindPathRequestId& other) const
	{
		return this->GetId() == other.GetId();
	}

	FindPathRequestId(const FindPathRequestId& other) = default;

	FindPathRequestId& operator=(const FindPathRequestId& other) = default;

private:
	explicit FindPathRequestId(const uint16_t inId)
		: id { inId }
	{
	}

	uint16_t id = 0;
};


}

#endif //PATHFINDERREQUEST_H
