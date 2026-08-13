# Multithreaded pathfinding

The path calculation runs off the main thread, on a small pool of worker threads. Previously the whole search ran inline in `Path Map::FindPath()`, called directly from `Movable::WalkTo`, so the main thread was blocked for as long as the search took.
The pathfinding is the single most expensive operation happening during a frame, and every millisecond spent in the pathfinding algorithm previously landed directly in the frame time.

This system avoids it: the caller only asks for a path and picks the result up a tick or two later, so the search is not part of the frame at all.

The sections below introduce the important pieces in the order they depend on each other: first what a path request is, then how the algorithm was made callable off the main thread, then the data it is computed against, then the scheduler that ties those together, and finally the life cycle of a single request.

## A path request

The central concept is the *path request*. Previously, asking for a path and getting one were the same operation - `WalkTo` called `FindPath` and had the answer when the call returned. Those two are separate now: the caller files a request, and the answer arrives later.

A request is a self-contained description of one search, represented by `FindPathRequest`. It holds the source and destination, the requesting actor's searchmap position, its circle size, its walking speed, the pathfinding flags, the type of the operation that asked for it (and the requesting actor's script name for logging purposes).

All of that is *copied* into the request at the moment it is filed, on the main thread. This is the important property: a request carries values, not references into the game. The two pointers it does hold - the requesting `Movable` and the `Map` - are used for identity comparison only, to recognize "this is the actor that asked", and are never dereferenced on a pathfinding worker thread.

Every request carries a priority. Worker throughput is finite, so when several requests are pending at once, the order they are served in decides whose delay the player actually notices. A party member ordered to move is being watched directly and any lag is obvious; a background NPC wandering on the far side of the map is not. Priorities exist to spend the workers on the former first.

There are three queued levels, picked in `Actor::GetFindPathRequestPriority`:

1. `Highest` - party members.
2. `High` - NPCs in combat.
3. `Normal` - everything else: background wandering, random walks, repositioning.

Each level has its own queue, and a worker looking for work scans them in that order. In the first queue that holds anything unclaimed, it takes the request that has waited longest. The priority decides which queue is served and the age decides who goes first inside it. Requests handed over by the same `Sync` count as equally old, and among those the pick is arbitrary. A `Normal` request therefore waits for as long as anything more urgent is pending, which is the intended trade - it is also why a request cannot sit unanswered forever, see the expiry at the end of the life cycle below.

There is also a fourth priority value, `Immediate`, which doesn't have its own queue: when a request of this priority is issued, it bypasses the queues' system entirely and computes the path in-place, before the call returns. That is what the main thread uses when the game is configured to run without workers, and this could also be potentially used when certain part of a game would require to get an immediate path (for example some scripted events which would break when having delayed paths).

Filing a request returns a `FindPathRequestId` - a small value the caller keeps in order to poll for the result later, or to cancel the request.

## Making the algorithm callable off the main thread

A request is only useful if something can compute it away from the main thread, and with the algorithm as it was, nothing really could.
`FindPath` was a `Map` method that reached for live game state whenever it needed anything - the terrain, the traversability cache, the actors. None of that can be touched from another thread while the main thread keeps mutating it.

The whole algorithm lives in the `PathFinder` class now. It is fully static and holds no state of its own, it has no `Map` dependency, and everything it reads is passed in as an argument. This is what makes the rest possible: the algorithm reads only what it is handed, so it can be pointed at a copy of interesting parts the world instead of the live state. `Map` keeps its pathfinding methods, but they are now thin wrappers that pass `Map`'s own data down to `PathFinder`.

The per-run scratch storage inside `FindPath` (the open set, the closed set, the parents and the distances) used to be `static`, to avoid reallocating it on every call. It is `thread_local` now, so each worker keeps its own set and the reuse is preserved.

## The copy of the world: snapshots

Since a worker may not read live game state without guarding half of the engine with locks, it has to be given a copy. That copy is called a *snapshot*, and one is published per map, once a frame (when needed), by the main thread.

Three kinds of data are needed to run a search, and each one required something new to make copying it viable.

**The terrain.** `TileProps` holds the searchmap, the material map, the height map and the light map. It used to live inside `Map.h`; it has its own files now.
`OwningTileProps` is a deep copy owning its own buffer. It is needed because a worker has to *modify* the terrain it searches: before running the search it erases the requesting actor's own footprint from the searchmap, so that the actor does not block itself. That cannot be done to the live terrain while the main thread is drawing from it, so it is done to a private copy.

**The traversability cache.** This is the existing per-navmap-cell record of which actor occupies a cell and whether it can be bumped.
It is indexed per navmap cell, and the navmap is 16x12 finer than the searchmap, which for a typical area gives millions cells. Stored densely that can be over a hundred MB per map, and copying it once a frame is out of the question. If you look at it carefully, you can see that almost every cell is empty - only the cells under an actor's collision circle ever hold anything, and those sit in tight clusters.

For that reason it is kept in a `PagedSparseArray`, which holds the data in fixed-size pages, allocates a page on first write and releases it once all its cells are back to default. Memory follows the number of actors rather than the size of the map, and indexing is still just a shift and a mask with no hashing involved. It also records which pages were touched, which is what allows the snapshot to copy only the pages that actually changed instead of the whole array. The pages themselves come from `FixedSizePool`, a small freelist allocator - since every page is the same size, allocation and release are a pop and a push.

**The actors.** `ActorSearchMapData` is a small record holding a pointer for identity, the position, the searchmap position, the circle size and two flags. After a worker clears the requesting actor's footprint it has to repaint the footprints of neighboring actors that the erase may have clipped, and this is the data it needs to do that without ever touching an `Actor`.

Snapshots are handed to workers as shared handles rather than by value. When a new one is published the handle is swapped, so a worker still reading the previous snapshot is unaffected and keeps a stable, immutable view for as long as it needs to. The traversability data is the one exception, since it is updated in place and cannot be shared directly. It is re-snapshotted by the first worker that claims a request for that map after the data changed, which keeps the copy off the main thread, and every other worker claiming against the same version gets it for the cost of a mere refcount bump.

## The scheduler

`PathFinderScheduler` is the component that orchestrates the whole pathfinding system.
It owns the worker threads, manages queues, stores requests until a worker takes them, and publishes the snapshots.

All of its public methods must be called from the main thread; the workers only ever touch its internals and never reach into the live state.

It holds four things worth naming:

1. `incomingRequests` - a queue of requests filed during the current frame, not yet handed to anyone.
2. `workerScheduledQueuesByPriority` - one queue per priority, this is what the workers take from.
3. `workerFoundPaths` - one output slot per worker, each with its own mutex, this is where a worker publishes a finished result.
4. the per-map snapshots described above.

`PathFinderScheduler::Sync` is the once-a-frame handoff between the two sides, called at the end of `Game::UpdateScripts`. It refreshes the snapshots, moves the frame's incoming requests into the priority queues, collects whatever the workers finished, drops requests that were cancelled or have waited too long, and wakes the workers if there is anything left to take.

## Keeping the main thread unblocked

The whole point of introducing this system is to keep the search cost out of the frame time, and that can be easily wasted: a main thread which no longer waits for a path, but waits instead for a mutex some worker happens to be holding, has gained little to nothing. In order to prevent this, the rule, the scheduler is built around, is that the main thread must never wait on a worker.

This is possible because a *worker thread* touches shared state only twice per request, and never while it is performing any heavy computations. First at the start, to claim the request and take the snapshot handles it needs - both under the same lock, so it is one acquisition and not two. Then at the end, to publish the result.
The actual search happens between those two points and works only on immutable data, so it needs no synchronization at all.

That first synchronization point is the one that needs extra care. Every worker has to go through that lock, and a worker holds it for the whole selection phase, so with several of them cycling the main thread can arrive to find a queue of workers in front of it. The costs are not symmetric here: a worker that waits a moment before picking up its next request loses nothing, while a main thread that waits for workers, produces a hitch in the frame. For this reason the workers are the ones made to wait: the main thread raises a flag before it wants the lock, and a worker seeing that flag switches to a short `try_lock_for` and back off instead of queueing up. The main thread does not queue either - it takes the lock with a 5 ms deadline and retries next frame if it does not get in.

The second synchronization point needs no such protocol. Each output slot belongs to a single worker, so workers never meet there at all, and the main thread only ever try_locks the slots and takes whatever is ready. There is no queue that could form and nothing the main thread has to be protected from.

## How an actor waits for its path

`Movable` carries a `MovementState`, with four values: `NoMovement`, `FindPathScheduled`, `PathSearchFailed` and `Moving`. The first three all mean the actor is not walking; `HasMovementInProgress` (and `Scriptable::InMove`, which defers to it) is the test for "is this actor going anywhere", and answers true only for `Moving` and `FindPathScheduled`.

It is necessary because "the actor holds a path" and "the actor is going somewhere" stopped being the same thing. An actor waiting for its first path holds nothing yet, and an actor re-pathing holds an old path it is still walking. The path alone can no longer distinguish those cases.

The re-pathing case is worth spelling out: filing a new request does not clear the current path, and `DoStep` deliberately keeps walking it until the new one arrives. Stopping for the round trip would be plainly visible as an actor stutter, and it would happen constantly while chasing, since a chase re-paths every few frames.

`PathSearchFailed` exists for the opposite reason: a result has to outlive the frame it arrived in. The move actions in `Actions.cpp` and `GSUtils.cpp` give up on an unreachable destination with

```cpp
if (!actor->InMove() || actor->Destination != point) actor->WalkTo(point, ...);
if (!actor->InMove()) { actor->ClearPath(); Sender->ReleaseCurrentAction(); }
```

which the synchronous pathfinder satisfied by resolving the search between the two checks. Asynchronously the failure lands 1+ tick later, when the second check is long past, and the first check - seeing an idle actor - would refile the same request before the second one could ever fire. So a failed `WalkTo` search parks the actor in `PathSearchFailed` along with the destination it failed for. The next `WalkTo` for that same destination files nothing, drops to `NoMovement` and returns, and the action's `!InMove()` gives up exactly as it did in the legacy implementation. Any other destination clears the state and gets a real search.

## Life cycle of a path request

This section describes current logical flow of what happens in order, when a Movable wants to use the new pathfinder.

1. Something wants to move - the player, a script, or the actor's own AI - and calls `WalkTo`, `RunAwayFrom` or `AddWayPoint` on a `Movable`.

2. `Movable::ScheduleFindPath` builds the request, copying in everything the search will need. It has to happen now, on the main thread, because the worker must not read the actor later.

3. `PathFinderScheduler::RequestPath` cancels any request the same actor filed earlier, stores the new one in `incomingRequests` and returns its ID. The actor keeps the ID and switches movement state to `FindPathScheduled`.

4. At the end of the frame, `Sync` refreshes the snapshots, moves the request into the queue matching its priority, and wakes the workers.

5. Awaken worker takes the queue mutex, scans the queues highest priority first, claims the longest-waiting request nobody has taken in the first queue that has one, marks it as taken, grabs the snapshot handles and releases the mutex.

6. With no locks held, the worker copies the terrain out of its handle, clears the requesting actor's footprint from that copy and runs `PathFinder::FindPath`. If the search fails and the request allows it, it is retried once while ignoring other actors.

7. The worker locks its own output slot and moves the result in. The request stays in the queue, still marked as taken, so no other worker picks it up.

8. The main thread collects the result, either in `DrainCompletedPathsEarly` early in the next tick, before any actor is stepped, or in the next `Sync`. Whichever site collects it, reconciles it against the queues: if the request is no longer scheduled it was cancelled while being computed, and the result is dropped.

9. On its next `DoStep` the actor polls the state of his request ID, sees it is waiting, takes the result and hands it to `OnPathCalculated`, which applies it according to the request type - replacing the current path for a walk, appending for a waypoint - and switches the movement state to `Moving`. If nothing was found, a walk request goes to `PathSearchFailed` (see above) and the other request types go back to `NoMovement`.

A request that is never answered is dropped after 60 frames and reported as "no path found", so an actor cannot wait forever. If a request leaves the scheduler for any other reason - the most realistic one being the map it belonged to getting unloaded - the actor notices on its next `DoStep` that the scheduler no longer knows the id, and resolves its own state instead of waiting.

## Configuration

There are 2 config knobs.

`PathfinderThreadsCount` - how many worker threads to spawn, 3 by default. Setting it to 0 spawns none and leaves all the work on the main thread.

`PathfinderMainThreadMode` - what the main thread should do when there are no workers. It is ignored completely as long as at least one worker was spawned. Two values:

**`immediate`** computes the path inside the request call, before it returns, and hands the result straight to the actor. The queues, the snapshots and the delivery delay are not used at all. This is the closest to the legacy behavior: the actor starts moving in the same tick it was ordered to, and the search cost sits in the frame time.

**`queued`** runs the whole scheduler as usual - the request goes into a queue, a snapshot is taken, the result is published into an output slot and collected a tick or two later - except that the worker side is driven by the main thread itself, from inside `Sync`. The timing matches the multithreaded case exactly; the only thing missing is the concurrency.

The distinction matters when hunting a bug, because the three configurations differ in exactly two properties and can be used as a bisection:

| configuration          | delayed delivery | concurrent |
|------------------------|------------------|------------|
| N workers              | yes              | yes        |
| 0 workers, `queued`    | yes              | no         |
| 0 workers, `immediate` | no               | no         |

If something misbehaves with workers enabled, switching to `queued` tells which half it came from. Still reproduces - the cause is the delayed delivery or the scheduler logic, and threading is not involved at all. Disappears - it is a data race or an ordering problem between threads. Dropping further to `immediate` takes the new timing away as well, so anything still reproducing there was not introduced by this system.

`queued` is a diagnostic mode, not something to ship: the search still runs on the main thread, so it costs roughly what `immediate` costs, and on top of that the result arrives late. It is strictly worse than `immediate` for a player and strictly more informative for a developer.

If thread creation fails, the mode follows the number of workers actually spawned, so a failed start still lands in a working configuration rather than breaking.

## Other notes

The delivery delay is a real behavioral change against the legacy implementation, and not merely an implementation detail. An actor ordered to move does not start moving in the same tick anymore; it starts one or two ticks later. Everything downstream of that had to be adjusted, most notably the failed-search handling: the legacy `WalkTo` knew immediately whether a path was found, while now that answer only arrives with the result, so things like the path-retry budget are counted when the result lands.
If anything would ever collapse due to introduction of this system, the game can always switch to the `immediate` requests' priority for a certain section.

`Scriptable::InMove` now tests the movement state rather than path emptiness, so an actor waiting for a path counts as being in move. Without that, a caller could raise a second, conflicting order into the window before the result arrives.

Cancelling a path request is now handled by `Movable`, including on destruction, so an actor cannot leave a request behind pointing at itself. Deleting a `Map` purges every request, result and snapshot belonging to it first, under the pathfinder lock, so no worker can be holding anything that refers to it.

### Note on configuration

#### Thread scaling 

Generally no performance difference between 1-3 threads on any reasonably modern system. 
Advice: if a system has support for 4+ HW threads, set number of workers to 3. For systems with HW support for less than 4 threads, set it to 2.
One worker thread may not keep up with the piling up requests in very NPC heavy scenarios.

#### Single-threaded modes

The immediate mode turns out to be better in terms of perf, than the legacy implementation. This comes as a little surprise to me, but it must be the changes to the traversability cache that paid this difference.
The queued with 0 threads was expected to be slower: we must pay the cost of maintaining the whole queues machinery on top of existing path searches, but the results show also it's comparable or sometimes better.
