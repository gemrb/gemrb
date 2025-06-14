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
#define PATH_BENCHMARK_ITERS 10

#define PATHFINDER_STATIC 1

#include <iostream>
#include <fstream>
#include <chrono>

struct FRemStat
{
    FRemStat()
    {
        std::ofstream os("perf.txt", std::ios_base::out | std::ios_base::trunc);
        os << "..\n";
    }
};

class ScopedTimer {
	char tag[15] = "";
#if WIN32
	std::chrono::time_point<std::chrono::steady_clock> startTime;
#else
    std::chrono::time_point<std::chrono::system_clock> startTime;
#endif

public:
	ScopedTimer(const char* InTag) {
		std::strncpy(tag, InTag, 15);
		startTime = std::chrono::high_resolution_clock::now();
	}

	~ScopedTimer() {
		const auto elapsed = std::chrono::high_resolution_clock::now() - startTime;
        std::ofstream os("perf.txt", std::ios_base::out | std::ios_base::app);
		os << tag << std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count() << "us\n";
	}
};

#endif //PATHFINDINGSETTINGS_H
