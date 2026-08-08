/* GemRB - Infinity Engine Emulator
 * Copyright (C) 2026 The GemRB Project
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
 *
 */
#ifndef PATHFINDERSCHEDULER_H
#define PATHFINDERSCHEDULER_H

#include "globals.h"

#include "PathFinder.h"
#include "PathFinderRequest.h"
#include "Region.h"
#include "TileProps.h"
#include "TraversabilityCache.h"

#include "Scriptable/Scriptable.h"

#include <array>
#include <atomic>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>
#include <vector>

namespace GemRB {
class Actor;
class Movable;


enum class PathFinderSchedulerMainThreadMode : uint8_t {
	Immediate,
	Queued,
};

/**
 *  Worker-thread facing data wrapper for pathfinding requests.
 *  Contains the core pathfinding request payload along with metadata for request lifecycle management,
 *  scheduling, and synchronization between worker threads and the main thread. Tracks request origin,
 *  map association, and availability status for processing.
 */
struct FindPathRequestWorkerData {
	FindPathRequest payload;
	int64_t originFrame = 0;
	ScriptID mapID = 0;
	bool taken = false;
};


/**
 *  Manages asynchronous pathfinding across multiple worker threads.
 *  Provides a scheduler interface for submitting pathfinding requests, tracking their status,
 *  and retrieving computed paths.
 *  Maintains separate queues for incoming requests, scheduled work, and completed paths, with thread-safe
 *  synchronization between the main thread and worker threads.
 *  Caches map state snapshots to ensure pathfinding calculations use consistent data without
 *  accessing live game state from worker threads.
 *
 *  All public methods must be called from the main thread.
 *
 *  SNAPSHOT ISOLATION
 *  Workers read snapshots, never live state. The main thread refreshes them once a frame.
 *  Pointers a worker holds are compared for identity, never dereferenced.
 *
 *  A REQUEST'S LIFE
 *  RequestPath() queues it and returns an ID. Sync() hands it to the workers. A worker computes
 *  it and publishes the result. Movable::DoStep() polls for it. Unanswered requests expire after
 *  requestExpirationFrames.
 *
 *  EXECUTION MODES, picked in Start() from PathfinderThreadsCount and PathfinderMainThreadMode:
 *  - N workers: the normal case.
 *  - 0 workers, "queued": Sync() runs the worker update itself.
 *  - 0 workers, "immediate": RequestPath() computes synchronously.
 *  The mode follows the number of workers actually spawned, so a failed thread start still lands
 *  in a working mode.
 *
 *  LOCKS, in acquisition order - scheduledAndCancelledQueuesMutex is always the outermost:
 *  - scheduledAndCancelledQueuesMutex: the work queues and the snapshots.
 *  - foundPathsMutexByWorker[i]: worker i's output slot.
 *  - newRequestsAvailableMutex: the wake-up condition variable.
 *
 *  The inner two are never held together, and neither is ever held while acquiring the queue
 *  mutex, so the order above is the only one that occurs and there is no cycle.
 *  The main thread has priority on the queue mutex; Sync() explains how.
 *
 *  More documentation you can find in:
 *  - Sync(): the per-frame handoff and the back-off protocol.
 *  - PathfinderThreadUpdate(): what a worker does, and where it locks.
 *  - DrainCompletedPathsEarly(): why results are collected twice per tick.
 *  - MovementState in Movable.h: how an actor's state tracks a request in flight.
 */
class PathFinderScheduler {
public:
	/**
	 *  Initializes and starts the pathfinder scheduler and worker threads.
	 *  Creates the configured number of pathfinding worker threads and puts them into their main loop.
	 *  If the scheduler is already initialized, this method returns immediately without effect.
	 *  If thread creation fails or no threads are configured, pathfinding falls back to execution
	 *  on the main thread.
	 *
	 *  @param InNumberOfRequestedWorkerThreads How many worker threads to spawn
	 *  @param InMainThreadMode Which mode will be used to calculate paths, if no worker threads will be spawned
	 */
	static void Start(uint16_t InNumberOfRequestedWorkerThreads, const std::string& InMainThreadMode);

	/**
	 *  Stops and shuts down all pathfinder worker threads and clears all internal state.
	 *  Signals all worker threads to stop, waits for them to join, and clears all pathfinding
	 *  queues, caches, and snapshots. After this call, the scheduler is uninitialized and must
	 *  be reinitialized before use. Thread-safe and blocks until all worker threads have terminated.
	 */
	static void Stop();

	/**
	 *	Synchronizes pathfinding state between worker threads and main thread.
	 *  Processes completed pathfinding results, handles incoming requests, and updates internal
	 *  state.
	 *  Must be called once per frame from the main thread. If no worker threads are configured,
	 *  executes pathfinding updates directly on the main thread. Increments the current sync frame
	 *  number and clears the incoming request queue after processing.
	 *
	 *  @param allMaps Vector of all active maps in the game
	 */
	static void Sync(const std::vector<Map*>& allMaps);

	/**
	 *  Submits a pathfinding request to the scheduler for processing.
	 *  If the request has immediate priority, the path is calculated synchronously and the result is delivered immediately.
	 *  For non-immediate requests, cancels any existing pending or scheduled requests for the same actor before queueing the new request.
	 *
	 *  @param request The pathfinding request: instigator identity, map, priority, start/end points and
	 *                 the snapshotted actor data the workers need.
	 *                 An asynchronous result is polled with IsPathCalculated()/TakeCalculatedPath(), while
	 *                 an immediate one is delivered by calling Movable::OnPathCalculated() directly from
	 *                 inside this function.
	 *  @return The unique identifier for the queued request, or a null ID if the request was processed
	 *          immediately - in that case the result has already been delivered and there is nothing to poll
	 */
	static FindPathRequestId RequestPath(FindPathRequest request);

	/**
	 *  Cancels a pathfinding request at any stage of its life.
	 *  Depending on the request's current stage:
	 *  - not yet dispatched: dropped from the incoming queue outright,
	 *  - scheduled, which includes one a worker is computing right now: queued for cancellation.
	 *    Sync() drops it from the worker-side queues, and a result published in the meantime is
	 *    discarded there too,
	 *  - already computed: its result is erased from foundPaths.
	 *
	 *
	 *  @param requestId The unique identifier of the request to cancel
	 *  @return True if the request was found in any of those three stages, false if the scheduler
	 *          does not know it - already consumed, expired, or never issued
	 */
	static bool CancelPath(const FindPathRequestId& requestId);

	/**
	 *  Checks if a pathfinding request has been completed and the result is available.
	 *
	 *  @param requestId The unique identifier of the request to check
	 *  @return True if the path has been calculated and is ready for retrieval, false otherwise
	 */
	static bool IsPathCalculated(const FindPathRequestId& requestId);

	/**
	 *  Checks whether a request is still known to the scheduler: waiting to be dispatched, sitting
	 *  in the scheduled queues, or with a result ready to be taken.
	 *  A request can leave the scheduler without ever producing a result - its map was deleted, it
	 *  was dropped because the map's snapshots had already gone, or it was never queued at all -
	 *  and a requester polling IsPathCalculated() alone would then wait on it forever.
	 *  Conservative by construction: the main-thread view of the scheduled queue is only rebuilt
	 *  once a frame, so this may report a request live slightly longer than it really is, never
	 *  the other way round.
	 *
	 *  @param requestId The unique identifier of the request to check
	 *  @return True while the request can still produce a result, false once it cannot
	 */
	static bool IsRequestLive(const FindPathRequestId& requestId);

	/**
	 *  Moves the computed path out of a completed pathfinding request.
	 *  The entry itself stays in foundPaths - only its contents are taken - so a second call for
	 *  the same ID yields an empty path, as does a call for an ID that has no result. Call
	 *  RemoveFoundPath() once the result has been consumed to drop the entry.
	 *
	 *  @param requestId The unique identifier of the request
	 *  @return The computed Path object for the request, or an empty Path
	 */
	static Path TakeCalculatedPath(const FindPathRequestId& requestId);

	/**
	 *  Moves the originating request data out of a completed pathfinding request.
	 *  Behaves like TakeCalculatedPath(): the entry stays in foundPaths and a second call yields
	 *  a default-constructed request. Takes a different part of the entry than
	 *  TakeCalculatedPath(), so the two are independent and may be called in either order.
	 *
	 *  @param requestId The unique identifier of the request
	 *  @return The FindPathRequest associated with the given ID, or a default-constructed one
	 */
	static FindPathRequest TakeCompletedRequest(const FindPathRequestId& requestId);

	/**
	 *  Removes a completed pathfinding result from the found paths queue.
	 *  This is what actually erases the entry that the Take... methods leave behind; call it
	 *  once the result has been consumed.
	 *
	 *  @param requestId The unique identifier of the request whose result should be removed
	 */
	static void RemoveFoundPath(const FindPathRequestId& requestId);


	/**
	 *  Must be called before deleting a Map. Acquires the pathfinder lock
	 *  and purges all references to the map from internal queues and caches,
	 *  ensuring no worker thread can access the map during or after deletion.
	 */
	static void OnMapDeletion(const Map* deletedMap);

	/**
	 *  Collects finished paths from the worker output slots at the head of the game tick.
	 *  Main thread only, called from the top of Game::UpdateScripts() before any actor is stepped.
	 *
	 *  Sync() collects them too, but it runs at the end of the tick, so a result published during
	 *  the tick waits there until the next one: Sync(N+1) collects it and DoStep() claims it in
	 *  tick N+2. Collecting here lets DoStep() claim it a tick earlier.
	 *
	 *  Takes only the per-worker slots locks, and only with try_lock, so workers are never blocked and
	 *  never pushed into their back-off path. Sync()'s reconciliation needs the queue mutex, so it
	 *  is deferred rather than done here - see earlyDrainedRequests.
	 */
	static void DrainCompletedPathsEarly();

private:
	/** Type alias for completed path results paired with their request metadata */
	using FoundPath_t = std::pair<Path, FindPathRequestWorkerData>;
	/** Type alias for scheduled request queues indexed by request ID */
	using ScheduledQueue_t = std::unordered_map<FindPathRequestId, FindPathRequestWorkerData, FindPathRequestId::Hash>;
	/** Type alias for found path result queues indexed by request ID */
	using FoundQueue_t = std::unordered_map<FindPathRequestId, FoundPath_t, FindPathRequestId::Hash>;
	/** Type alias for incoming request queues indexed by request ID */
	using IncomingQueue_t = std::unordered_map<FindPathRequestId, FindPathRequest, FindPathRequestId::Hash>;

	/** Number of frames after which a request is considered stale and discarded */
	constexpr static int64_t requestExpirationFrames = 60;

	// main thread facing members, no need to lock on them:
	/** Tracks whether the scheduler has been initialized and worker threads started */
	static bool isInitialized;
	/** Current frame number, incremented on each Sync call */
	static int64_t currentSyncFrameNumber;
	/** Frame number of the most recent pathfinding request submission */
	static int64_t lastRequestFrameNumber;
	/** Frame number of the most recent map state cache update */
	static int64_t lastCacheUpdateFrameNumber;
	/** Requests already moved into foundPaths by DrainCompletedPathsEarly(), still to be erased
	 *  from the worker-side scheduled queues. That erase needs the queue mutex, which the early
	 *  drain avoids taking, so Sync() step 1.5 does it instead. Main thread only. */
	static std::vector<FindPathRequestId> earlyDrainedRequests;
	/** Configurable number of worker threads to use for asynchronous pathfinding */
	static uint16_t numberOfRequestedWorkerThreads;
	/** How many worker threads was actually spawned? */
	static uint16_t numberOfSpawnedWorkerThreads;
	/** Mode in which we will execute path calculations on the main thread. */
	static PathFinderSchedulerMainThreadMode mainThreadMode;
	/** Flag indicating we should execute all requests immediately. */
	static bool isInMainThreadImmediateMode;
	/** Container of all active pathfinding worker threads */
	static std::vector<std::thread> workerThreads;

	// set of queues
	/** Main thread queue of completed paths ready for retrieval */
	static FoundQueue_t foundPaths;
	/** Main thread queue of newly submitted requests awaiting transfer to scheduled queue */
	static IncomingQueue_t incomingRequests;
	/** Main thread view of currently scheduled requests */
	static ScheduledQueue_t scheduledQueue;
	/** Main thread queue of request IDs marked for cancellation */
	static std::vector<FindPathRequestId> cancelledQueue;

	// cached state per map, keyed by map's global ID
	/** Pages' pool for traversabilityCacheData below. Facing only main thread. */
	static FixedSizePool<TraversabilityCache::Data_t::TPage_t> traversabilityCacheSnapshotAllocator;
	/** Main-thread instance per map; SyncFrom() updates it incrementally from the map's dirty set */
	static std::unordered_map<ScriptID, TraversabilityCache::Data_t> traversabilityCacheData;

	// The three snapshots below are handed to workers by shared handle rather than by value.
	// Sync() replaces the pointer when it refreshes a map, so a worker that grabbed the previous
	// handle keeps reading a stable, immutable snapshot for as long as it needs to. That lets a
	// worker leave the queue mutex holding only a pointer: the actor data is then read straight
	// through the handle, and the per-worker tileprops copy - needed because the worker paints the
	// requester's footprint into it - is made after the lock is released.

	/** Cached tile properties per map for worker thread access */
	static std::unordered_map<ScriptID, std::shared_ptr<const OwningTileProps>> tilePropsSnapshot;
	/** Cached actor positions and search map data per map for worker thread access */
	static std::unordered_map<ScriptID, std::shared_ptr<const std::vector<ActorSearchMapData>>> actorsSnapshot;
	/**
	 *  Immutable snapshot of the main-thread instance, which cannot be shared directly because
	 *  SyncFrom() mutates it in place. Taken lazily by the first worker to claim a request for the
	 *  map after a version bump, so the copy stays off the main thread and every other worker
	 *  claiming against the same version gets it for a refcount bump.
	 */
	static std::unordered_map<ScriptID, std::shared_ptr<const TraversabilityDataSnapshot>> traversabilityCacheDataSnapshot;
	/** Bumped by Sync() whenever the main-thread instance for a map was actually updated */
	static std::unordered_map<ScriptID, uint64_t> traversabilityCacheDataSnapshotVersion;

	// pathfinder threads facing members, need to synchronize them:

	// Both flags below are advisory hints and are accessed with std::memory_order_relaxed
	// throughout. No data is handed to the workers through them: everything a worker reads
	// (the scheduled/cancelled/found queues and the three snapshot maps) is protected by the
	// mutexes further down, and locking/unlocking those already provides the necessary
	// acquire/release ordering.

	/** Atomic flag signaling worker threads to terminate their main loop */
	static std::atomic<bool> shouldStop;

	/** Atomic flag signaling worker threads that main thread wants to sync now, so they back off for a while */
	static std::atomic<bool> mainThreadWantsSync;

	/** Flag indicating at least one scheduled request is still awaiting a worker; raised by Sync()
	 *  from the state of the scheduled queues and lowered by whichever worker finds nothing left to
	 *  take (protected by newRequestsAvailableMutex) */
	static bool newRequestsAvailable;

	// Completed paths are handed over through one output slot per worker.
	// A worker only ever locks its own slot, so publishing is free of worker-to-worker contention
	// (unlike claiming a request, which shares the queue mutex), and the main thread can drain the
	// slots with try_lock and simply skip any it can't get - those paths are delivered a frame later.
	// This is what keeps the publish path free of the main-thread back-off.
	//
	// Sized once in Start() and never resized while workers are running; index 0 is also used by
	// the main thread when it drives PathfinderThreadUpdate() itself (0 worker threads, "queued").
	/** Per-worker queues of completed paths pending transfer to main thread (each protected by its own foundPathsMutexByWorker entry) */
	static std::vector<FoundQueue_t> workerFoundPaths;
	/** Worker thread array of scheduled request queues, indexed by priority level (protected by scheduledAndCancelledQueuesMutex) */
	static std::array<ScheduledQueue_t, ScheduledQueuesPrioritiesCount> workerScheduledQueuesByPriority;
	/** Worker thread queue of cancelled request IDs pending processing (protected by scheduledAndCancelledQueuesMutex) */
	static std::vector<FindPathRequestId> workerCancelledQueue;

	/** Protects access to workerScheduledQueuesByPriority and workerCancelledQueue */
	static std::timed_mutex scheduledAndCancelledQueuesMutex;
	/** Protects access to the matching workerFoundPaths entry; only ever contended between
	 *  one worker and the main thread, never between two workers.
	 *  Lock ordering: a worker holds either scheduledAndCancelledQueuesMutex (while selecting a
	 *  request) or its own slot mutex (while publishing), never both, so there is no cycle with
	 *  the main thread taking scheduledAndCancelledQueuesMutex first and the slots after. */
	static std::vector<std::timed_mutex> foundPathsMutexByWorker;
	/** Protects access to newRequestsAvailable flag */
	static std::mutex newRequestsAvailableMutex;
	/** Condition variable for waking worker threads when new requests arrive */
	static std::condition_variable newRequestsAvailableSignal;

	/**
	 *  Main execution loop for pathfinding worker threads.
	 *  Continuously processes pathfinding requests until signaled to stop. Handles sleeping
	 *  when no work is available and wakes up on condition variable notification when new
	 *  requests arrive. Runs on each worker thread independently.
	 *
	 *  @param workerIdx Index of this worker, selecting which output slot it publishes into
	 */
	static void WorkerThreadMainLoop(size_t workerIdx);

	/**
	 *  Drains the scheduled queues on a worker thread.
	 *  Repeatedly selects the highest-priority request that is not yet taken, computes its path
	 *  from the thread-safe snapshots and publishes the result into this worker's output slot.
	 *  Returns once no untaken request is left - which is also where the worker clears
	 *  newRequestsAvailable so it can go back to sleep - or as soon as it has to yield to the
	 *  main thread, or when signalled to stop.
	 *
	 *  Publishing does not remove the request from the scheduled queues - Sync() reconciles the
	 *  slot against them, which is also where a cancellation that happened mid-computation is
	 *  detected. This keeps the worker off scheduledAndCancelledQueuesMutex once it holds a
	 *  finished path, so a result never has to be discarded to yield to the main thread.
	 *
	 *  @param workerIdx Index of this worker, selecting which output slot it publishes into
	 */
	static void PathfinderThreadUpdate(size_t workerIdx);

	/**
	 *  Calculates a path immediately on the main thread, bypassing all queues.
	 *  Used for immediate priority requests that require synchronous results. Prepares
	 *  necessary map state data and performs the pathfinding calculation directly.
	 *
	 *  @param currentRequestId The unique identifier for this request
	 *  @param InOutCurrentRequest The request data wrapper containing pathfinding parameters
	 *  @return The calculated Path, which may be empty if pathfinding failed
	 */
	static Path PerformImmediatePathCalculation(FindPathRequestId currentRequestId, FindPathRequestWorkerData& InOutCurrentRequest);

	/**
	 *  Executes the core pathfinding algorithm and handles retry logic.
	 *  Performs the pathfinding calculation using the provided map state snapshots.
	 *  If pathfinding fails and the request is eligible for retry (canRePathIgnoringActors),
	 *  attempts a second calculation ignoring actor positions.
	 *
	 *  @param currentTraversabilityCacheSnapshot Cached traversability data for the map
	 *  @param currentTileProps Cached tile properties for the map
	 *  @param currentRequestId The unique identifier for this request
	 *  @param InOutCurrentRequest The request data wrapper, potentially modified during processing
	 *  @return The calculated Path, which may be empty if pathfinding failed
	 */
	static Path PerformPathCalculation(
		const TraversabilityCache::Data_t& currentTraversabilityCacheSnapshot,
		const TileProps& currentTileProps, FindPathRequestId currentRequestId,
		FindPathRequestWorkerData& InOutCurrentRequest);

	/**
	 * Hands traversabilityCacheData's entry for selected map.
	 * Creates empty one on the pool if it is the first sync for that map.
	 */
	static TraversabilityCache::Data_t& GetOrCreateTraversabilityData(ScriptID mapID);
};
}

#endif
