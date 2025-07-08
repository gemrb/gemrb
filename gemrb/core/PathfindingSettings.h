//
// Created by draghan on 01.03.25.
//

#ifndef PATHFINDINGSETTINGS_H
#define PATHFINDINGSETTINGS_H

#define PATH_RUN_ORIGINAL    1
#define PATH_RUN_IMPROVED    1
#define PATH_RETURN_ORIGINAL (PATH_RUN_ORIGINAL && !PATH_RUN_IMPROVED)

#define PATH_RUN_BENCH        0
#define PATH_BENCHMARK_WARMUP 0
#define PATH_BENCHMARK_ITERS  1

#include <chrono>
#include <fstream>
#include <iostream>

#include <iostream>
#include <string>
#include <chrono>

class ScopedTimer {
private:
	std::string tag;
	long* resultStorage;
	std::string* tagStorage;
	std::chrono::high_resolution_clock::time_point start_time;

public:
	static std::vector<long> extraTimeTracked;
	static std::vector<std::string> extraTagsTracked;
	const static std::vector<long> noExtraTime;
	const static std::vector<std::string> noTags;
	static bool bIsExtraTimeTrackedInitialized;
	explicit ScopedTimer(const std::string& tag, long* outResultStorage = nullptr, std::string* outTagStorage = nullptr)
		: tag(tag), resultStorage(outResultStorage), tagStorage(outTagStorage), start_time(std::chrono::high_resolution_clock::now()) {}

	// Disable copy constructor and assignment operator
	ScopedTimer(const ScopedTimer&) = delete;
	ScopedTimer& operator=(const ScopedTimer&) = delete;

	// Allow move semantics if needed
	ScopedTimer(ScopedTimer&& other) noexcept
		: tag(std::move(other.tag)), start_time(other.start_time) {}

	ScopedTimer& operator=(ScopedTimer&& other) noexcept {
		if (this != &other) {
			tag = std::move(other.tag);
			start_time = other.start_time;
		}
		return *this;
	}

	~ScopedTimer() {
		auto end_time = std::chrono::high_resolution_clock::now();
		auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end_time - start_time);
		auto microseconds = std::chrono::duration_cast<std::chrono::microseconds>(duration);

		if (resultStorage) {
			*resultStorage = microseconds.count();
		}

		// std::cout << '\n' << tag << microseconds.count() << " us" << '\n';

		if (tagStorage) {
			*tagStorage = std::move(tag);
		}
	}
};

#endif //PATHFINDINGSETTINGS_H
