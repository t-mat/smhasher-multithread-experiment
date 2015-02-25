#pragma once

#if defined(__GLIBC__)
#include <sys/sysinfo.h> // get_nprocs()
#endif

#if defined(__APPLE__) || defined(__FreeBSD__)
#include <sys/types.h>
#include <sys/sysctl.h>
#endif

#include <atomic>
#include <mutex>
#include <future>
#include <chrono>

using Mutex = std::mutex;
using Lock = std::unique_lock<Mutex>;

typedef std::chrono::high_resolution_clock Clock;
typedef Clock::time_point TimePoint;

inline TimePoint getTime() {
	return Clock::now();
}

inline double getTimeSpanInSeconds(const TimePoint& tStart, const TimePoint& tEnd) {
	using namespace std::chrono;
	const auto dt = duration_cast<milliseconds>(tEnd - tStart).count();
	return static_cast<double>(dt);
}

inline unsigned getHardwareConcurrency() {
	{
		const auto c = std::thread::hardware_concurrency();
		if(c) {
			return static_cast<unsigned>(c);
		}
	}

	// following code is borrowed from boost-1.53.0/libs/thread/src/pthread/thread.cpp
#if defined(__APPLE__) || defined(__FreeBSD__)
	{
		int c = 0;
		size_t size = sizeof(c);
		if(0 == sysctlbyname("hw.ncpu", &c, &size, NULL, 0)) {
			return static_cast<unsigned>(c);
		}
	}
#endif

#if defined(__GLIBC__)
	{
		auto c = get_nprocs();
		if(c) {
			return static_cast<unsigned>(c);
		}
	}
#endif
	assert(0);
	return 8;
}

inline void lockFunc(Mutex& mut, const std::function<void()>& func) {
	Lock lock(mut);
	func();
}


inline unsigned getThreadCount() {
	return getHardwareConcurrency() * 2;
}

inline void threadForEach(const std::function<void(size_t,size_t)>& func) {
	const size_t nThread = getThreadCount();
	std::vector<std::thread> threads;
	for(size_t i = 0; i < nThread; ++i) {
		threads.emplace_back(func, i, nThread);
	}
	for(auto& t : threads) {
		t.join();
	}
}
