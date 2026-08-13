// SPDX-FileCopyrightText: 2026 Contributors to the GemRB project <https://gemrb.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PathFinderScheduler.h"

#include "Debug.h"
#include "GameData.h"
#include "Map.h"
#include "PathFinder.h"

#include "Logging/Logging.h"
#include "Scriptable/Actor.h"

namespace GemRB {

uint16_t PathFinderScheduler::numberOfRequestedWorkerThreads { 3 };
uint16_t PathFinderScheduler::numberOfSpawnedWorkerThreads { 0 };
PathFinderSchedulerMainThreadMode PathFinderScheduler::mainThreadMode { PathFinderSchedulerMainThreadMode::Immediate };
bool PathFinderScheduler::isInMainThreadImmediateMode { false };
bool PathFinderScheduler::isInitialized { false };

std::vector<std::thread> PathFinderScheduler::workerThreads;

PathFinderScheduler::FoundQueue_t PathFinderScheduler::foundPaths;
PathFinderScheduler::IncomingQueue_t PathFinderScheduler::incomingRequests;
PathFinderScheduler::ScheduledQueue_t PathFinderScheduler::scheduledQueue;
std::vector<FindPathRequestId> PathFinderScheduler::cancelledQueue;

FixedSizePool<TraversabilityCache::Data_t::TPage_t> PathFinderScheduler::traversabilityCacheSnapshotAllocator;
std::unordered_map<ScriptID, TraversabilityCache::Data_t> PathFinderScheduler::traversabilityCacheData;
std::unordered_map<ScriptID, uint64_t> PathFinderScheduler::traversabilityCacheDataSnapshotVersion;
std::unordered_map<ScriptID, std::shared_ptr<const TraversabilityDataSnapshot>> PathFinderScheduler::traversabilityCacheDataSnapshot;
std::unordered_map<ScriptID, std::shared_ptr<const OwningTileProps>> PathFinderScheduler::tilePropsSnapshot;
std::unordered_map<ScriptID, std::shared_ptr<const std::vector<ActorSearchMapData>>> PathFinderScheduler::actorsSnapshot;

std::atomic<bool> PathFinderScheduler::shouldStop { false };

std::atomic<bool> PathFinderScheduler::mainThreadWantsSync { false };

bool PathFinderScheduler::newRequestsAvailable { false };

std::vector<PathFinderScheduler::FoundQueue_t> PathFinderScheduler::workerFoundPaths;
std::array<PathFinderScheduler::ScheduledQueue_t, ScheduledQueuesPrioritiesCount> PathFinderScheduler::workerScheduledQueuesByPriority;
std::vector<FindPathRequestId> PathFinderScheduler::workerCancelledQueue;

std::timed_mutex PathFinderScheduler::scheduledAndCancelledQueuesMutex;
std::vector<std::timed_mutex> PathFinderScheduler::foundPathsMutexByWorker;
std::mutex PathFinderScheduler::newRequestsAvailableMutex;
std::condition_variable PathFinderScheduler::newRequestsAvailableSignal;

int64_t PathFinderScheduler::currentSyncFrameNumber { 0 };
int64_t PathFinderScheduler::lastRequestFrameNumber { 0 };
int64_t PathFinderScheduler::lastCacheUpdateFrameNumber { 0 };
std::vector<FindPathRequestId> PathFinderScheduler::earlyDrainedRequests;

TraversabilityCache::Data_t& PathFinderScheduler::GetOrCreateTraversabilityData(const ScriptID mapID)
{
	auto found = traversabilityCacheData.find(mapID);
	if (found == traversabilityCacheData.end()) {
		found = traversabilityCacheData.emplace(mapID, TraversabilityCache::Data_t(traversabilityCacheSnapshotAllocator)).first;
	}
	return found->second;
}

FindPathRequestId PathFinderScheduler::RequestPath(FindPathRequest request)
{
	// main thread
	lastRequestFrameNumber = currentSyncFrameNumber;

	// check if we had this actor in the incoming requests, if so - remove it
	for (auto it = incomingRequests.begin(); it != incomingRequests.end();) {
		const auto& scheduledRequest = it->second;
		if (scheduledRequest.instigatorIdentity == request.instigatorIdentity) {
			it = incomingRequests.erase(it);
		} else {
			++it;
		}
	}
	// check if we had already scheduled path for this actor and cancel it
	for (const auto& scheduledItem : scheduledQueue) {
		const auto& scheduledRequestID = scheduledItem.first;
		const auto& scheduledRequest = scheduledItem.second;
		if (scheduledRequest.payload.instigatorIdentity == request.instigatorIdentity) {
			cancelledQueue.push_back(scheduledRequestID);
		}
	}

	const auto requestId = FindPathRequestId::CreateNextId();

	const bool shouldCalculateImmediately = isInMainThreadImmediateMode || (request.priority == FindPathRequestPriority::Immediate);
	if (shouldCalculateImmediately) {
		if (request.map && request.instigatorIdentity) {
			request.map->UpdateTraversabilityCache();
			auto data = FindPathRequestWorkerData {
				request,
				currentSyncFrameNumber,
				request.map->GetGlobalID(),
				false
			};
			auto foundPath = PerformImmediatePathCalculation(
				requestId, data);
			request.instigatorIdentity->OnPathCalculated(std::move(foundPath), request);
		}
		return FindPathRequestId::NullId();
	}

	incomingRequests.emplace(requestId, request);
	return requestId;
}


void PathFinderScheduler::CancelPath(const FindPathRequestId& requestId)
{
	// main thread

	// check if we had this request in the incoming requests, if so - remove it
	const auto foundInIncomingRequests = incomingRequests.find(requestId);
	if (foundInIncomingRequests != incomingRequests.end()) {
		incomingRequests.erase(foundInIncomingRequests);
		return;
	}

	// check if we had already scheduled this path and cancel it
	const auto foundInScheduledQueue = scheduledQueue.find(requestId);
	if (foundInScheduledQueue != scheduledQueue.end()) {
		cancelledQueue.push_back(requestId);
		return;
	}

	// if we cancel already calculated path, remove it
	const auto foundInFoundPaths = foundPaths.find(requestId);
	if (foundInFoundPaths != foundPaths.end()) {
		foundPaths.erase(foundInFoundPaths);
	}
}

bool PathFinderScheduler::IsPathCalculated(const FindPathRequestId& requestId)
{
	// main thread
	return foundPaths.count(requestId) > 0;
}

bool PathFinderScheduler::IsRequestLive(const FindPathRequestId& requestId)
{
	// main thread
	return incomingRequests.count(requestId) > 0 ||
		scheduledQueue.count(requestId) > 0 ||
		foundPaths.count(requestId) > 0;
}

Path PathFinderScheduler::TakeCalculatedPath(const FindPathRequestId& requestId)
{
	// main thread

	const auto found = foundPaths.find(requestId);
	if (found != foundPaths.end()) {
		return std::move(found->second.first);
	}
	return Path {};
}

FindPathRequest PathFinderScheduler::TakeCompletedRequest(const FindPathRequestId& requestId)
{
	// main thread
	const auto found = foundPaths.find(requestId);
	if (found != foundPaths.end()) {
		return std::move(found->second.second.payload);
	}
	return FindPathRequest {};
}

void PathFinderScheduler::RemoveFoundPath(const FindPathRequestId& requestId)
{
	// main thread
	const auto found = foundPaths.find(requestId);
	if (found != foundPaths.end()) {
		foundPaths.erase(found);
	}
}

void PathFinderScheduler::Start(uint16_t InNumberOfRequestedWorkerThreads, const std::string& InMainThreadMode)
{
	// main thread
	if (isInitialized) {
		return;
	}

	Log(DEBUG, "PathfinderThreadUpdate", "[main] Starting pathfinder: numberOfRequestedWorkerThreads={}, mainThreadMode={}",
	    InNumberOfRequestedWorkerThreads, InMainThreadMode);

	isInitialized = true;
	shouldStop.store(false, std::memory_order_relaxed);
	newRequestsAvailable = false;
	numberOfRequestedWorkerThreads = InNumberOfRequestedWorkerThreads;

	if (InMainThreadMode == "immediate") {
		mainThreadMode = PathFinderSchedulerMainThreadMode::Immediate;
	} else if (InMainThreadMode == "queued") {
		mainThreadMode = PathFinderSchedulerMainThreadMode::Queued;
	}

	// One output slot per worker, plus a slot 0 that is always present: the main thread uses it
	// when it drives PathfinderThreadUpdate() itself (0 worker threads, "queued" mode).
	// Allocated up front, off the requested count, so the slots are in place before any worker
	// can touch them, and never resized while workers are running.
	const size_t workerSlotCount = std::max<size_t>(1, numberOfRequestedWorkerThreads);
	workerFoundPaths = std::vector<FoundQueue_t>(workerSlotCount);
	foundPathsMutexByWorker = std::vector<std::timed_mutex>(workerSlotCount);

	numberOfSpawnedWorkerThreads = 0;
	try {
		for (uint16_t i = 0; i < numberOfRequestedWorkerThreads; ++i) {
			LogDebugPathfinder("PathfinderThreadUpdate", "[main] Starting pathfinder thread #{}", i);
			workerThreads.emplace_back(WorkerThreadMainLoop, static_cast<size_t>(i));
			++numberOfSpawnedWorkerThreads;
		}
	} catch (std::system_error& e) {
		Log(ERROR, "PathfinderThreadUpdate", "[main] During creation of pathfinder threads, an exception was thrown: {}. Successfully started {} pathfinder thread(s).",
		    e.what(), numberOfSpawnedWorkerThreads);
	}

	if (numberOfSpawnedWorkerThreads == 0) {
		// this is the case if we failed to start any thread, but also if there is enforced 0 pathfinding threads from
		// the config file
		Log(DEBUG, "PathfinderThreadUpdate", "[main] Not starting any new pathfinder thread, pathfinding will be executed from the main thread.");
	}

	isInMainThreadImmediateMode = numberOfSpawnedWorkerThreads == 0 && mainThreadMode == PathFinderSchedulerMainThreadMode::Immediate;
}

void PathFinderScheduler::Stop()
{
	// main thread

	// signal to workers they should stop their main loop
	{ // newRequestsAvailableLock scope
		std::unique_lock<std::mutex> newRequestsAvailableLock { newRequestsAvailableMutex };
		shouldStop.store(true, std::memory_order_relaxed);
		newRequestsAvailableSignal.notify_all(); // wake up all sleeping working threads
	}

	// join all worker threads
	for (size_t i = 0; i < workerThreads.size(); ++i) {
		LogDebugPathfinder("PathfinderThreadUpdate", "[main] Stopping pathfinder thread #{}", i);
		workerThreads[i].join();
	}

	// clear the state; no need to lock on shared queues, no worker thread is alive now
	workerThreads.clear();

	workerCancelledQueue.clear();
	for (auto& foundPathsSlot : workerFoundPaths) {
		foundPathsSlot.clear();
	}
	for (size_t queueIdx = 0; queueIdx < ScheduledQueuesPrioritiesCount; ++queueIdx) {
		workerScheduledQueuesByPriority[queueIdx].clear();
	}
	incomingRequests.clear();
	cancelledQueue.clear();
	foundPaths.clear();
	scheduledQueue.clear();
	traversabilityCacheData.clear();
	traversabilityCacheDataSnapshotVersion.clear();
	traversabilityCacheDataSnapshot.clear();
	tilePropsSnapshot.clear();
	actorsSnapshot.clear();
	earlyDrainedRequests.clear();
	numberOfSpawnedWorkerThreads = 0;
	isInMainThreadImmediateMode = false;
	currentSyncFrameNumber = 0;
	lastRequestFrameNumber = 0;
	lastCacheUpdateFrameNumber = 0;

	isInitialized = false;
}

namespace {
	/**
	 * RAII helper for an advisory atomic flag, raises it on construction and lowers it on
	 * destruction, so no exit path can leave it latched.
	 * Relaxed ordering is deliberate, as this will be used only for non-data carrying flags.
	 */
	class ScopedAtomicFlag {
	public:
		explicit ScopedAtomicFlag(std::atomic<bool>& inFlag)
			: flag(inFlag)
		{
			flag.store(true, std::memory_order_relaxed);
		}

		~ScopedAtomicFlag()
		{
			flag.store(false, std::memory_order_relaxed);
		}

		ScopedAtomicFlag(const ScopedAtomicFlag&) = delete;
		ScopedAtomicFlag& operator=(const ScopedAtomicFlag&) = delete;
		ScopedAtomicFlag(ScopedAtomicFlag&&) = delete;
		ScopedAtomicFlag& operator=(ScopedAtomicFlag&&) = delete;

	private:
		std::atomic<bool>& flag;
	};
}

void PathFinderScheduler::DrainCompletedPathsEarly()
{
	// main thread, at the head of the game tick, before any actor is stepped

	for (size_t workerIdx = 0; workerIdx < workerFoundPaths.size(); ++workerIdx) {
		std::unique_lock<std::timed_mutex> guardWorkerPaths(foundPathsMutexByWorker[workerIdx], std::try_to_lock);
		if (!guardWorkerPaths.owns_lock()) {
			continue;
		}

		auto& foundPathsSlot = workerFoundPaths[workerIdx];
		for (auto& publishedPath : foundPathsSlot) {
			const auto& publishedRequestId = publishedPath.first;

			// Skip results whose request was cancelled while the worker was computing it. Sync()
			// spots those in the worker-side queues; the main-side mirror shows the same thing
			// and needs no lock.
			const auto foundScheduled = scheduledQueue.find(publishedRequestId);
			if (foundScheduled == scheduledQueue.end()) {
				continue;
			}

			// Erasing from the mirror keeps CancelPath() correct: it must now take its foundPaths
			// branch and erase the delivered result, not the scheduled branch, which would defer
			// to a reconciliation that no longer has anything to reconcile.
			scheduledQueue.erase(foundScheduled);
			earlyDrainedRequests.push_back(publishedRequestId);

			foundPaths.emplace(publishedRequestId, std::move(publishedPath.second));
		}
		foundPathsSlot.clear();
	}
}


void PathFinderScheduler::Sync(const std::vector<Map*>& allMaps)
{
	// main thread, once a frame
	++currentSyncFrameNumber;

	/**
	 * OVERVIEW:
	 * The once-a-frame handoff between the game's main thread and the pathfinding workers: it
	 * collects the paths workers finished, hands them the new requests, refreshes the read-only
	 * snapshots they work from, and drops requests that were cancelled or have waited too long.
	 *
	 * Runs on the main thread only.
	 *
	 * Two configurations short-circuit the normal flow:
	 * - isInMainThreadImmediateMode: paths are computed synchronously inside RequestPath(), so
	 *   there is nothing to hand over and this returns immediately.
	 * - numberOfSpawnedWorkerThreads == 0 with queued mode: no worker exists, so the main thread
	 *   drives one PathfinderThreadUpdate(0) itself before continuing below.
	 *   The count is of *spawned* workers, so this covers both thread creation having failed and
	 *   user's requested 0 worker threads.
	 *
	 * THE BACK-OFF PROTOCOL:
	 * The main thread MUST NOT stall while waiting on a worker, but a worker holds the queue mutex
	 * for a whole selection phase, and there may be several of them cycling. Rather than let the main
	 * thread queue up behind them, the two sides cooperate through one advisory flag `mainThreadWantsSync`.
	 * In simple terms: if this flag is raised, it means the main thread reached the `Sync` and all the worker
	 * threads must back off from claiming the scheduledAndCancelledQueuesMutex.
	 *
	 * - mainThreadWantsSync is a plain atomic carrying no data - all actual data either side reads is
	 *   already behind a mutex - so it is read and written relaxed. A stale read costs one
	 *   iteration of picking the other branch, nothing more.
	 * - The main thread raises it before trying to lock, and holds it raised for the entire locked
	 *   section via ScopedAtomicFlag, so no exit path can leave it latched. It is declared *before*
	 *   guardQueue, so on the way out the mutex is released before the flag drops.
	 * - A worker checks the flag at the top of each drain-loop iteration. Raised, it downgrades its
	 *   acquisition to try_lock_for(500us) and, on failure, returns instead of blocking - it has
	 *   done no work at that point, so yielding is free. Lowered, it takes the mutex normally.
	 * - Because the check is per iteration, each worker performs at most one more selection phase
	 *   after the flag goes up. The main thread's wait is therefore bounded by roughly
	 *   (worker count x one selection phase), which is why its own deadline is only 5 ms.
	 * - If that deadline is missed anyway, the frame is given up on and retried next frame; the
	 *   acquisition step in the flow below spells out what it does on the way out.
	 *
	 * CALCULATED PATHS' COLLECTION HAPPENS TWICE PER TICK:
	 * This is the second of the two points that move finished paths out of the worker slots, and
	 * the order within a tick is: DrainCompletedPathsEarly(), then the actor updates that consume
	 * foundPaths, then this. A path a worker publishes mid-tick would otherwise sit in its slot
	 * until the next frame's Sync(), costing its actor a tick; the early drain exists to claim it
	 * in time for the same tick's DoStep().
	 *
	 * The two are not interchangeable. The early drain only try_locks the output slots, so it never
	 * blocks a worker or pushes one into its back-off path - but that also means it cannot take
	 * scheduledAndCancelledQueuesMutex, and so cannot erase what it took from the worker-side
	 * scheduled queues. It records those ids in earlyDrainedRequests and step 1.5 here finishes the
	 * job. Step 2 then collects whatever the early drain left: slots it could not lock, and results
	 * published after it ran.
	 *
	 * EXECUTION FLOW:
	 * 0. Refresh every map's traversability cache, gated on a request having arrived since the last
	 *    refresh (shouldSyncCache). Each map records whether its cache actually changed, so step 4
	 *    can skip the untouched ones. Deliberately outside every lock: it is heavy and must not
	 *    block workers.
	 *
	 *    >> LOCK newRequestsAvailableMutex, clear newRequestsAvailable, UNLOCK. Tells the workers
	 *       there is temporarily nothing to take; step 5 sets the real answer.
	 *
	 *    >> RAISE mainThreadWantsSync (ScopedAtomicFlag).
	 *    >> LOCK scheduledAndCancelledQueuesMutex with try_lock_until, 5 ms deadline.
	 *       On failure: LOCK newRequestsAvailableMutex, set newRequestsAvailable, UNLOCK,
	 *       notify_all, and return - lowering the flag on the way out. The queues could not be
	 *       inspected, so it must assume work remains rather than leave every worker asleep; a
	 *       worker that then finds nothing to take lowers the flag itself.
	 *
	 *       Steps 1-4 below all run with scheduledAndCancelledQueuesMutex held.
	 *
	 * 1. Cancellations: merge workerCancelledQueue into the main-side cancelledQueue (1.1), then
	 *    drop each cancelled request from the scheduled queues (1.2, 1.3) and from foundPaths if its
	 *    result had already arrived (1.4). 1.5 erases the scheduled entries for results
	 *    DrainCompletedPathsEarly() took mid-tick.
	 *
	 * 2. Collect finished paths. Per worker slot:
	 *    >> TRY-LOCK worker's foundPathsMutexByWorker entry; a slot whose worker holds it right now is
	 *       skipped, and that path arrives next frame instead.
	 *    Drain it and reconcile against the scheduled queues - still scheduled means erase it and
	 *    move the path to foundPaths; gone means it was cancelled while being computed, so drop the
	 *    path.
	 *    >> UNLOCK worker's foundPathsMutexByWorker entry.
	 *    Actors pick results out of foundPaths in their own movement update.
	 * 2.1. Expire anything that has sat in the scheduled queues for requestExpirationFrames,
	 *      publishing an empty path for it. Must run after 2, so a result already sitting in a slot
	 *      cannot be expired out from under it.
	 *
	 * 3. Dispatch: move each incomingRequests entry into the queue for its priority (3.1), then
	 *    rebuild the main-side scheduledQueue view (3.2). That same pass counts the requests no
	 *    worker has taken yet, which is what step 5 wakes workers on.
	 *
	 * 4. Refresh the worker snapshots, when shouldSyncCache:
	 *    - 4.1 traversability cache, only for maps whose cache changed this frame, and only the
	 *      dirty pages; bumping its version invalidates the shared snapshot without copying here,
	 *      and the next worker to claim a request for that map re-takes it,
	 *    - 4.2 tileprops and actor data, for *every* map rather than only the changed ones: neither
	 *      is gated on wasTravUpdated, so both are rebuilt from scratch on any frame a request
	 *      arrived. That is a full tileprops buffer copy and a full actor vector per map, and it is
	 *      the expensive half of this step. Both are published as new shared_ptr handles rather
	 *      than written into the existing ones, so a worker still reading the previous handle is
	 *      unaffected,
	 *    - 4.3 drop the snapshots of maps that no longer exist.
	 *
	 *    >> UNLOCK scheduledAndCancelledQueuesMutex, then LOWER mainThreadWantsSync - in that order,
	 *       both by scope exit, which is why the flag guard is declared before the lock.
	 *
	 * 5. Wake workers: LOCK newRequestsAvailableMutex, set newRequestsAvailable from the untaken
	 *    count of 3.2, UNLOCK, then notify_one() worker thread for a single untaken request or notify_all()
	 *    for more.
	 *
	 * 6. Clear incomingRequests.
	 */

	// if we're operating in immediate mode on the main thread, no further sync is necessary
	if (isInMainThreadImmediateMode) {
		return;
	}

	// if we don't have any active worker thread for pathfinder, run its update from here
	if (numberOfSpawnedWorkerThreads == 0) {
		// no workers, so the main thread does the work itself and uses the always-present slot 0
		PathfinderThreadUpdate(0);
	}

	// 0. Update traversability cache outside the locks' scope: it can be heavy operation in comparison
	// to the queues sync, no need to block worker threads from work during this update.
	// `shouldSyncCache` is only true if we received any new request since last cache update - no need to
	// update the cached state if no new request was filed.
	std::vector<bool> wasTravUpdated;
	const bool shouldSyncCache = lastRequestFrameNumber >= lastCacheUpdateFrameNumber;
	if (shouldSyncCache) {
		for (auto* map : allMaps) {
			const auto wasUpdated = map->UpdateTraversabilityCache();
			wasTravUpdated.push_back(wasUpdated);
		}
	}

	{ // newRequestsAvailableLock scope
		// tell worker threads there is temporarily no work to do
		std::unique_lock<std::mutex> newRequestsAvailableLock { newRequestsAvailableMutex };
		newRequestsAvailable = false;
	}
	// Number of scheduled requests that no worker has picked up yet, counted below in 3.2 while the
	// queues are still locked. This is the workers' wake-up condition: it covers requests left over
	// from earlier frames as well as the ones arriving now, so work can never be left sitting in the
	// queues with every worker asleep.
	size_t untakenScheduledCount = 0;
	{ // guardQueue, guardPaths scope

		// Tell worker threads the main thread wants to sync, so they back off for a while.
		// This is scoped to the whole locked section on purpose: every exit path - including
		// the bail-out below, when the locks can't be acquired in time - must lower the flag.
		const ScopedAtomicFlag wantsSync { mainThreadWantsSync };

		// Only the scheduled/cancelled queues are taken here. The per-worker output slots are
		// drained further down with try_lock, so they cannot hold up the frame.
		std::unique_lock<std::timed_mutex> guardQueue(scheduledAndCancelledQueuesMutex, std::defer_lock);

		// try to acquire the queue lock for max 5ms, to not stall main thread;
		// with the back-off mechanism implemented on worker threads, this shouldn't ever happen,
		// but just to be safe
		const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(5);
		const bool acquired = guardQueue.try_lock_until(deadline);

		if (!acquired) {
			// couldn't acquire the queue lock in time, move on, sync in the next frame.
			// The queues couldn't be inspected, so assume they still hold work and wake the workers
			// to find out: one that sees nothing to take lowers the flag again by itself.
			{ // newRequestsAvailableLock scope
				std::unique_lock<std::mutex> newRequestsAvailableLock { newRequestsAvailableMutex };
				newRequestsAvailable = true;
			}
			newRequestsAvailableSignal.notify_all();
			return;
		}

		// 1.1. Merge any canceled requests from worker thread to one queue
		for (const auto& cancelledRequestId : workerCancelledQueue) {
			cancelledQueue.push_back(cancelledRequestId);
		}
		workerCancelledQueue.clear();
		// 1.2. Remove any request that has been canceled from any pending queues
		for (const auto& cancelledRequestId : cancelledQueue) {
			// 1.3. Remove from scheduled:
			for (auto& priorityQueue : workerScheduledQueuesByPriority) {
				const auto foundItScheduled = priorityQueue.find(cancelledRequestId);
				if (foundItScheduled != priorityQueue.end()) {
					priorityQueue.erase(foundItScheduled);
					break;
				}
			}
			// 1.4. Also drop any result already sitting in foundPaths for it.
			const auto foundItComputed = foundPaths.find(cancelledRequestId);
			if (foundItComputed != foundPaths.end()) {
				foundPaths.erase(foundItComputed);
			}
		}
		cancelledQueue.clear();

		// 1.5. Erase the scheduled entries of results DrainCompletedPathsEarly() already took; it
		// could not do that itself without the queue lock.
		for (const auto& earlyDrainedRequestId : earlyDrainedRequests) {
			for (auto& priorityQueue : workerScheduledQueuesByPriority) {
				const auto foundItScheduled = priorityQueue.find(earlyDrainedRequestId);
				if (foundItScheduled != priorityQueue.end()) {
					priorityQueue.erase(foundItScheduled);
					break;
				}
			}
		}
		earlyDrainedRequests.clear();

		// 2. Collect the completed paths from the per-worker output slots.
		// Each slot is only tried, never waited on: one that its worker happens to hold right now
		// is left for the next frame. That way the workers can never hold up the frame when publishing
		// their results.
		// This has to run before the expiration below, so that a request whose result is already
		// waiting in a slot doesn't get expired out from under it.
		for (size_t workerIdx = 0; workerIdx < workerFoundPaths.size(); ++workerIdx) {
			std::unique_lock<std::timed_mutex> guardWorkerPaths(foundPathsMutexByWorker[workerIdx], std::try_to_lock);
			if (!guardWorkerPaths.owns_lock()) {
				continue;
			}

			auto& foundPathsSlot = workerFoundPaths[workerIdx];
			for (auto& publishedPath : foundPathsSlot) {
				// Workers publish without touching the scheduled queues, so the reconciliation
				// happens here: a request that is no longer scheduled was cancelled while it was
				// being computed, and its path is dropped.
				const auto& publishedRequestId = publishedPath.first;
				const auto queuePriorityIdx = static_cast<size_t>(publishedPath.second.second.payload.priority);
				auto& priorityQueue = workerScheduledQueuesByPriority[queuePriorityIdx];

				const auto foundScheduled = priorityQueue.find(publishedRequestId);
				if (foundScheduled == priorityQueue.end()) {
					continue;
				}
				priorityQueue.erase(foundScheduled);


				foundPaths.emplace(publishedRequestId, std::move(publishedPath.second));
			}
			foundPathsSlot.clear();
		}

		// 2.1. If any paths in the scheduled queues is older than the expiration threshold, consider it as stale:
		// report no path found and remove it from the scheduled queue
		std::vector<FindPathRequestId> staleRequestsPerQueue[ScheduledQueuesPrioritiesCount];
		for (size_t queueIdx = 0; queueIdx < ScheduledQueuesPrioritiesCount; ++queueIdx) {
			for (auto& scheduledRequest : workerScheduledQueuesByPriority[queueIdx]) {
				const auto requestWaitingInQueueFrames = currentSyncFrameNumber - scheduledRequest.second.originFrame;
				if (requestWaitingInQueueFrames >= requestExpirationFrames) {
					LogDebugPathfinder("PathfinderThreadUpdate", "[main] Path request with ID={} was "
										     "waiting in the queue for too long ({} frames), dropping it.",
							   scheduledRequest.first.GetId(), requestWaitingInQueueFrames);
					// insert empty path for this request
					foundPaths.insert({ scheduledRequest.first, { Path {}, scheduledRequest.second } });
					staleRequestsPerQueue[queueIdx].push_back(scheduledRequest.first);
				}
			}
		}
		// remove all stale requests from the scheduled queue
		for (size_t queueIdx = 0; queueIdx < ScheduledQueuesPrioritiesCount; ++queueIdx) {
			for (auto& request : staleRequestsPerQueue[queueIdx]) {
				workerScheduledQueuesByPriority[queueIdx].erase(request);
			}
		}

		// 3. Handle all incoming requests
		// 3.1. Assign each incoming request to its proper (selected by priority) scheduled
		// queue on the worker threads side
		for (auto& incomingRequest : incomingRequests) {
			auto workerData = FindPathRequestWorkerData {
				incomingRequest.second,
				currentSyncFrameNumber,
				incomingRequest.second.map->GetGlobalID(),
				false
			};

			const auto priorityQueueIdx = static_cast<size_t>(incomingRequest.second.priority);
			workerScheduledQueuesByPriority[priorityQueueIdx].insert({ incomingRequest.first, workerData });
		}

		// 3.2. Sync main-thread facing scheduledQueue with worker threads facing queues, and count
		// the requests still awaiting a worker on the way through
		scheduledQueue.clear();
		for (auto& priorityQueue : workerScheduledQueuesByPriority) {
			for (auto& scheduledRequest : priorityQueue) {
				scheduledQueue.insert(scheduledRequest);
				if (!scheduledRequest.second.taken) {
					++untakenScheduledCount;
				}
			}
		}
		// 4. Sync necessary data snapshots
		if (shouldSyncCache) {
			lastCacheUpdateFrameNumber = currentSyncFrameNumber;
			// 4.1. sync the traversabilityCache and tileprops
			for (size_t mapIdx = 0; mapIdx < allMaps.size(); ++mapIdx) {
				auto* map = allMaps[mapIdx];
				const auto mapID = map->GetGlobalID();
				if (wasTravUpdated[mapIdx]) {
					auto& mapTraversabilityData = GetOrCreateTraversabilityData(mapID);
					mapTraversabilityData.SyncFrom(map->GetTraversabilityCacheData());
					// invalidates the snapshot without copying anything here; the next worker
					// to claim a request for this map re-takes it
					++traversabilityCacheDataSnapshotVersion[mapID];
				}

				// replaces the handle rather than overwriting the snapshot, so any worker still
				// reading the previous one is unaffected
				tilePropsSnapshot[mapID] = std::make_shared<const OwningTileProps>(OwningTileProps::CopyFrom(map->tileProps));
				// 4.2. Snapshot actor data needed for searchmap operations on worker threads
				const auto& liveActors = map->GetAllActors();
				std::vector<ActorSearchMapData> snapshot;
				snapshot.reserve(liveActors.size());
				for (const Actor* actor : liveActors) {
					snapshot.push_back({ actor,
							     actor->Pos,
							     actor->SMPos,
							     actor->circleSize,
							     actor->IsPC(),
							     actor->BlocksSearchMap() });
				}
				actorsSnapshot[mapID] = std::make_shared<const std::vector<ActorSearchMapData>>(std::move(snapshot));
			}
		}
		// 4.3. Remove any stale maps
		std::vector<ScriptID> mapsToRemove;
		for (const auto& zz : traversabilityCacheData) {
			const auto found = std::find_if(allMaps.cbegin(), allMaps.cend(), [&](const auto& candidateMap) {
				return candidateMap->GetGlobalID() == zz.first;
			});
			if (found == allMaps.cend()) {
				mapsToRemove.push_back(zz.first);
			}
		}
		for (const auto& mapToRemove : mapsToRemove) {
			traversabilityCacheData.erase(mapToRemove);
			traversabilityCacheDataSnapshotVersion.erase(mapToRemove);
			traversabilityCacheDataSnapshot.erase(mapToRemove);
			tilePropsSnapshot.erase(mapToRemove);
			actorsSnapshot.erase(mapToRemove);
		}
	}

	// 5. Notify sleeping threads whether there is a work to do
	{ // newRequestsAvailableLock scope
		std::unique_lock<std::mutex> newRequestsAvailableLock { newRequestsAvailableMutex };
		newRequestsAvailable = untakenScheduledCount > 0;
	}
	if (untakenScheduledCount == 1) {
		LogDebugPathfinder("PathfinderThreadUpdate", "[main] There is 1 request awaiting a worker, waking up 1 thread.");
		newRequestsAvailableSignal.notify_one();
	} else if (untakenScheduledCount > 1) {
		LogDebugPathfinder("PathfinderThreadUpdate", "[main] There are {} requests awaiting a worker, waking up all threads.", untakenScheduledCount);
		newRequestsAvailableSignal.notify_all();
	}
	// 6. Cleanup
	incomingRequests.clear();
}


void PathFinderScheduler::OnMapDeletion(const Map* deletedMap)
{
	// main thread - called before a Map is deleted.
	// Acquires the pathfinder locks so no worker thread can be
	// dereferencing this map or its actors concurrently.
	// Unlike Sync(), this blocks on every lock rather than trying them: the purge has to be
	// complete before the map goes away, so it cannot be deferred to a later frame.
	std::lock_guard<std::timed_mutex> guardQueue(scheduledAndCancelledQueuesMutex);

	// Purge all scheduled requests that reference this map
	for (auto& priorityQueue : workerScheduledQueuesByPriority) {
		for (auto it = priorityQueue.begin(); it != priorityQueue.end();) {
			if (it->second.mapID == deletedMap->GetGlobalID()) {
				it = priorityQueue.erase(it);
			} else {
				++it;
			}
		}
	}

	// Also purge any already-computed paths for this map, still sitting in the worker slots
	for (size_t workerIdx = 0; workerIdx < workerFoundPaths.size(); ++workerIdx) {
		std::lock_guard<std::timed_mutex> guardWorkerPaths(foundPathsMutexByWorker[workerIdx]);
		auto& foundPathsSlot = workerFoundPaths[workerIdx];
		for (auto it = foundPathsSlot.begin(); it != foundPathsSlot.end();) {
			if (it->second.second.mapID == deletedMap->GetGlobalID()) {
				it = foundPathsSlot.erase(it);
			} else {
				++it;
			}
		}
	}

	// Remove from main-thread-side queues too
	for (auto it = incomingRequests.begin(); it != incomingRequests.end();) {
		if (it->second.map->GetGlobalID() == deletedMap->GetGlobalID()) {
			it = incomingRequests.erase(it);
		} else {
			++it;
		}
	}
	for (auto it = foundPaths.begin(); it != foundPaths.end();) {
		if (it->second.second.mapID == deletedMap->GetGlobalID()) {
			it = foundPaths.erase(it);
		} else {
			++it;
		}
	}

	// Remove cached map data
	traversabilityCacheData.erase(deletedMap->GetGlobalID());
	traversabilityCacheDataSnapshotVersion.erase(deletedMap->GetGlobalID());
	traversabilityCacheDataSnapshot.erase(deletedMap->GetGlobalID());
	tilePropsSnapshot.erase(deletedMap->GetGlobalID());
	actorsSnapshot.erase(deletedMap->GetGlobalID());
}

Path PathFinderScheduler::PerformImmediatePathCalculation(
	FindPathRequestId currentRequestId,
	FindPathRequestWorkerData& InOutCurrentRequest)
{
	// main thread

	// This runs against the live tileprops, not a worker's private copy, so the requester's own
	// footprint has to be painted back once the search is done
	const bool clearedOwnFootprint = InOutCurrentRequest.payload.blocksSearchMaps && InOutCurrentRequest.payload.instigatorIdentity;
	if (clearedOwnFootprint) {
		InOutCurrentRequest.payload.map->ClearSearchMapFor(
			InOutCurrentRequest.payload.instigatorIdentity);
	}
	const Path foundPath = PerformPathCalculation(InOutCurrentRequest.payload.map->GetTraversabilityCacheData(),
						      InOutCurrentRequest.payload.map->tileProps, currentRequestId, InOutCurrentRequest);
	if (clearedOwnFootprint) {
		InOutCurrentRequest.payload.map->BlockSearchMapFor(
			InOutCurrentRequest.payload.instigatorIdentity);
	}
	return foundPath;
}

Path PathFinderScheduler::PerformPathCalculation(const TraversabilityCache::Data_t& currentTraversabilityCacheSnapshot, const TileProps& currentTileProps, FindPathRequestId currentRequestId, FindPathRequestWorkerData& InOutCurrentRequest)
{
	// could be called both from main and workers thread, no access to any shared states
	const ActorPathContext actorContext {
		static_cast<unsigned int>(InOutCurrentRequest.payload.actorCircleSize),
		InOutCurrentRequest.payload.instigatorIdentity,
		InOutCurrentRequest.payload.actorSpeed,
		InOutCurrentRequest.payload.instigatorScriptName
	};

	auto foundPath = PathFinder::FindPath(
		currentTraversabilityCacheSnapshot,
		currentTileProps,
		InOutCurrentRequest.payload.source,
		InOutCurrentRequest.payload.destination,
		actorContext,
		InOutCurrentRequest.payload.minDistance,
		InOutCurrentRequest.payload.pathfindingFlags);

	if (!foundPath && InOutCurrentRequest.payload.canRePathIgnoringActors) {
		LogDebugPathfinder("WalkTo", "RequestID={} of {}, re-pathing ignoring actors",
				   currentRequestId.GetId(), InOutCurrentRequest.payload.instigatorScriptName);
		InOutCurrentRequest.payload.pathfindingFlags &= ~static_cast<int>(PF_ACTORS_ARE_BLOCKING);
		foundPath = PathFinder::FindPath(
			currentTraversabilityCacheSnapshot,
			currentTileProps,
			InOutCurrentRequest.payload.source,
			InOutCurrentRequest.payload.destination,
			actorContext,
			InOutCurrentRequest.payload.minDistance,
			InOutCurrentRequest.payload.pathfindingFlags);
	}
	return foundPath;
}

void PathFinderScheduler::PathfinderThreadUpdate(const size_t workerIdx)
{
	// worker thread

	/**
	 * OVERVIEW:
	 * Worker-side drain of the scheduled queues. Repeatedly takes the highest-priority request no
	 * worker has claimed, computes its path from thread-safe snapshots, and publishes the result
	 * into this worker's own output slot, until nothing untaken is left.
	 *
	 * THREADING:
	 * - Runs on a worker thread, or on the main thread when numberOfSpawnedWorkerThreads == 0.
	 * - Holds scheduledAndCancelledQueuesMutex only to claim a request and take snapshot handles,
	 *   releases it for the expensive search, then locks this worker's own output slot to publish.
	 *   Never both at once, which is why there is no lock cycle with Sync().
	 * - Workers do contend for the queue mutex while claiming - that is inherent to a shared work
	 *   queue - but the contended section is short and holds no computation. They never contend
	 *   while publishing, since each writes to its own output slot.
	 * - Main-thread priority applies to the selection phase only: while mainThreadWantsSync is set,
	 *   acquisition is bounded to try_lock_for(500us) and the worker returns rather than blocking.
	 *   That costs nothing, because no work has been done yet.
	 * - The publish phase needs no back-off protocol at all. Each output slot belongs to a single
	 *   worker, so no queue of workers can build up in front of the main thread there, and the main
	 *   thread only ever try_locks the slots anyway.
	 *
	 * EXECUTION FLOW (one iteration; loops while the queues hold anything untaken):
	 *    >> CHECK shouldStop - set means leave the loop and let the thread exit.
	 *
	 *    >> CHECK mainThreadWantsSync:
	 *         - if raised: LOCK scheduledAndCancelledQueuesMutex with try_lock_for(500us), and leave
	 *         back-off (leave the loop) on failure.
	 *         - if lowered: LOCK scheduledAndCancelledQueuesMutex blocking.
	 *
	 *       Steps 1-2 run with scheduledAndCancelledQueuesMutex held.
	 *
	 * 1. Claim a request: scan the priority queues highest-first and, in the first tier holding an
	 *    untaken request, take the one that has waited longest (smallest originFrame), mark it taken
	 *    and copy it. Priority decides which tier is served, age decides within a tier, so
	 *    a request cannot be passed over indefinitely by newer ones of its own priority.
	 *    If there is none requests left to be processed:
	 *       >> LOCK newRequestsAvailableMutex, clear newRequestsAvailable, UNLOCK,
	 *    then leave the loop. This is the only place the worker side lowers that flag. The caller
	 *    puts this thread to sleep.
	 *
	 * 2. Take snapshot handles. A missing tileprops or traversability entry means the map went away
	 *    between the request being made and it being picked up, so the request is cancelled and the
	 *    loop moves to the next one; likewise a missing actor snapshot when the requester blocks
	 *    searchmaps.
	 *    - tileprops and actor data are taken as shared_ptr handles, no copying under the lock.
	 *    - The traversability data is the exception: Sync() updates it in place, so it cannot be
	 *      shared directly. It is re-snapshotted here, and only when its version says Sync() changed
	 *      it - every other claim against the same version, on this worker or another, is a refcount
	 *      bump rather than a copy.
	 *
	 *    >> UNLOCK scheduledAndCancelledQueuesMutex. Steps 3-5 run lock-free.
	 *
	 * 3. Deep-copy the tileprops out of the handle. The copy is private and paintable, which is what
	 *    step 4 needs; actor data is read straight through its handle.
	 *
	 * 4. Prepare the searchmap: if the requester blocks searchmaps, clear its own footprint using
	 *    the snapshotted actor data.
	 *
	 * 5. Compute the path: PathFinder::FindPath over the snapshots, optionally retried ignoring
	 *    actors when canRePathIgnoringActors is set. See PerformPathCalculation().
	 *
	 * 6. Publish:
	*     >> LOCK this worker's foundPathsMutexByWorker entry, blocking.
	 *    Move the result into the slot. The request stays in its scheduled queue, still marked
	 *    taken, so later scans skip it; Sync() erases it when it reconciles the slot.
	 *    >> UNLOCK this worker's foundPathsMutexByWorker entry, then loop back to the top.
	 *
	 * CANCELLATION SAFETY:
	 * - A request cancelled while it was being computed is detected by Sync() when it drains the
	 *   slot: a published request that is no longer in its scheduled queue was cancelled, and its
	 *   path is dropped.
	 *
	 * CALLED BY:
	 * - WorkerThreadMainLoop(), each time a worker wakes on newRequestsAvailableSignal.
	 * - Sync() directly, when numberOfSpawnedWorkerThreads == 0.
	 */
	FindPathRequestId currentRequestId = FindPathRequestId::NullId();
	FindPathRequestWorkerData currentRequest;

	// this worker's private, paintable tileprops; filled from currentTilePropsHandle outside the lock
	OwningTileProps currentTileProps;
	std::shared_ptr<const OwningTileProps> currentTilePropsHandle;
	std::shared_ptr<const std::vector<ActorSearchMapData>> currentActorsSnapshot;
	std::shared_ptr<const TraversabilityDataSnapshot> currentTraversabilityHandle;

	// we will break from this loop if the scheduled queue is drained or the game requested the pathfinder threads to stop:
	while (!shouldStop.load(std::memory_order_relaxed)) {
		{ // guardQueue scope: from scheduled queues, book a next request to work on
			if (mainThreadWantsSync.load(std::memory_order_relaxed)) {
				if (!scheduledAndCancelledQueuesMutex.try_lock_for(std::chrono::microseconds(500))) {
					break; // back off, let main thread have priority
				}
			} else {
				// Explanation for sonar exemption:
				// sonar wants to move the manual lock to RAII construction, but that's not really a viable option,
				// without other compromises.
				// Blocking here but timing out on the other branch cannot be expressed by a guard's
				// constructor; the lock is adopted by RAII guardQueue on the very next line
				scheduledAndCancelledQueuesMutex.lock(); // NOSONAR
			}
			std::lock_guard<std::timed_mutex> guardQueue(scheduledAndCancelledQueuesMutex, std::adopt_lock);

			// go through all workerScheduledQueuesByPriority by their priority (highest prio is first in
			// array) and select the longest-waiting non-taken request of the first tier that has one:
			// the tier is chosen by priority, the request within it by age. Requests dispatched by the
			// same Sync() share an originFrame, so ties among equally old ones are broken arbitrarily
			currentRequestId = FindPathRequestId::NullId();
			for (auto& priorityQueue : workerScheduledQueuesByPriority) {
				// nullptr means this tier has yielded no candidate yet; it points into the queue, so
				// claiming the winner below needs no second lookup
				FindPathRequestWorkerData* oldestRequest = nullptr;
				for (auto& scheduledRequest : priorityQueue) {
					if (scheduledRequest.second.taken) {
						// NOTE: these DebugMode::PATHFINDER blocks sit inside scheduledAndCancelledQueuesMutex.
						// Log() takes the logger's own mutex and does a notify_all, so enabling the flag puts a
						// blocking lock and a futex wake inside the worker's selection phase - the very section
						// the main-thread back-off protocol is built around.
						// This is fine for debugging, but interferes with the designed threading model
						// and may (will) affect performance.
						LogDebugPathfinder("PathfinderThreadUpdate", "[worker {}] request ID={}, is already taken, going to next one.",
								   std::this_thread::get_id(),
								   scheduledRequest.first.GetId());
						continue;
					}

					if (!oldestRequest || scheduledRequest.second.originFrame < oldestRequest->originFrame) {
						oldestRequest = &scheduledRequest.second;
						currentRequestId = scheduledRequest.first;
					}
				}

				if (oldestRequest) {
					oldestRequest->taken = true;
					currentRequest = *oldestRequest;
					break;
				}
			}

			if (currentRequestId.IsNull()) {
				LogDebugPathfinder("PathfinderThreadUpdate", "[worker {}] No valid requests found, leaving.", std::this_thread::get_id());
				std::unique_lock<std::mutex> newRequestsAvailableLock { newRequestsAvailableMutex };
				newRequestsAvailable = false;
				break; // break from this update; will put thread to sleep, nothing to work on right now; thread update will be called again when ready
			}

			// Deliberately resolved here and not at scheduling time: a request planned at pickup
			// sees the freshest published world state, which is what compensates for it having
			// waited in the queue. The snapshots are immutable, so the one we grab cannot change
			// under us.
			const auto foundProps = tilePropsSnapshot.find(currentRequest.mapID);
			const auto foundTraversabilityData = traversabilityCacheData.find(currentRequest.mapID);
			if (foundProps == tilePropsSnapshot.end() || foundTraversabilityData == traversabilityCacheData.end()) {
				// the map went away between the request being made and it being picked up
				workerCancelledQueue.push_back(currentRequestId);
				continue;
			}

			// The traversability data is updated in place by Sync(), so it cannot be shared
			// directly. Re-snapshot it here, on this worker rather than on the main thread, and
			// only when Sync() actually changed it - every other claim against the same version,
			// including the ones on other workers, then costs a refcount bump instead of a copy.
			auto& publishedTraversability = traversabilityCacheDataSnapshot[currentRequest.mapID];
			const auto currentSnapshotVersion = traversabilityCacheDataSnapshotVersion[currentRequest.mapID];
			if (!publishedTraversability || publishedTraversability->version != currentSnapshotVersion) {
				auto snapshot = std::make_shared<TraversabilityDataSnapshot>();
				snapshot->data.CopyFrom(foundTraversabilityData->second);
				snapshot->version = currentSnapshotVersion;
				publishedTraversability = std::move(snapshot);
			}

			if (currentRequest.payload.blocksSearchMaps) { // if needs to clear search map, capture also a copy of actors' state
				const auto foundActors = actorsSnapshot.find(currentRequest.mapID);
				if (foundActors == actorsSnapshot.end()) {
					workerCancelledQueue.push_back(currentRequestId);
					continue;
				}
				currentActorsSnapshot = foundActors->second;
			}
			// the tileprops are copied from this handle once the lock is released
			currentTilePropsHandle = foundProps->second;
			currentTraversabilityHandle = publishedTraversability;

			if (InDebugMode(DebugMode::PATHFINDER)) {
				size_t totalRequests = 0;
				for (auto& sq : workerScheduledQueuesByPriority) {
					totalRequests += sq.size();
				}
				Log(DEBUG, "PathfinderThreadUpdate", "[worker {}] Processing request ID={} of {}, on the priorityQueue there is {} elements total.",
				    std::this_thread::get_id(),
				    currentRequestId.GetId(),
				    currentRequest.payload.instigatorScriptName,
				    totalRequests);
			}
		} // guardQueue scope

		// the worker paints into its tileprops, so it needs a private copy; the handle keeps the
		// source snapshot alive while it is taken
		currentTileProps = *currentTilePropsHandle;

		if (currentRequest.payload.blocksSearchMaps) {
			PathFinder::ClearSearchMapFor(*currentActorsSnapshot, currentRequest.payload.instigatorIdentity, currentRequest.payload.source, currentRequest.payload.actorSMPos, currentRequest.payload.actorCircleSize, currentTileProps);
		}

		// perform actual pathfinding computations
		Path foundPath = PerformPathCalculation(currentTraversabilityHandle->data, currentTileProps, currentRequestId, currentRequest);

		{ // guardWorkerPaths scope: publish the result into this worker's own output slot
			// No main-thread back-off here, and no scheduledAndCancelledQueuesMutex either. The
			// back-off exists to stop workers piling up in front of the main thread, and this slot
			// belongs to this worker alone: the main thread is the only other party that touches
			// it, and only with try_lock, so there is nothing to pile up and nothing to yield to.
			// The request stays in its scheduled queue, still marked taken; Sync() reconciles the
			// slot against the queues, erases the entry and detects a cancellation that happened
			// while we were computing. Subsequent scans here skip it because it is taken.
			std::lock_guard<std::timed_mutex> guardWorkerPaths(foundPathsMutexByWorker[workerIdx]);
			workerFoundPaths[workerIdx].emplace(currentRequestId, std::make_pair(std::move(foundPath), currentRequest));
		} // guardWorkerPaths scope
	}
}

void PathFinderScheduler::WorkerThreadMainLoop(const size_t workerIdx)
{
	LogDebugPathfinder("PathfinderThreadUpdate", "[worker {}] Starting pathfinder thread", std::this_thread::get_id());
	while (!shouldStop.load(std::memory_order_relaxed)) {
		{ // newRequestsAvailableLock scope, put the worker thread to sleep if no new requests are available
			std::unique_lock<std::mutex> newRequestsAvailableLock { newRequestsAvailableMutex };
			if (!newRequestsAvailable) {
				LogDebugPathfinder("PathfinderThreadUpdate", "[worker {}] No requests are available, going to sleep", std::this_thread::get_id());
				// sleep and await on signal from condition_variable that there is a work to do:
				newRequestsAvailableSignal.wait(newRequestsAvailableLock, [] {
					return newRequestsAvailable || shouldStop.load(std::memory_order_relaxed);
				});
			}
			LogDebugPathfinder("PathfinderThreadUpdate", "[worker {}] Some requests are available, waking up!", std::this_thread::get_id());
		} // newRequestsAvailableLock scope

		PathfinderThreadUpdate(workerIdx);
	}
	LogDebugPathfinder("PathfinderThreadUpdate", "[worker {}] Exiting pathfinder thread", std::this_thread::get_id());
}
}
