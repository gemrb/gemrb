//
// Created by draghan on 01.03.25.
//

#ifndef PATHFINDINGSETTINGS_H
#define PATHFINDINGSETTINGS_H

#define PATH_RUN_ORIGINAL 1
#define PATH_RUN_IMPROVED 1
#define PATH_RETURN_ORIGINAL (PATH_RUN_ORIGINAL && !PATH_RUN_IMPROVED)

#define PATH_RUN_BENCH 1
#define PATH_BENCHMARK_WARMUP 0
#define PATH_BENCHMARK_ITERS 1

#include <iostream>
#include <fstream>
#include <chrono>

#endif //PATHFINDINGSETTINGS_H
