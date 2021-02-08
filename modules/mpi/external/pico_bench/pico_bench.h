#ifndef PICO_BENCH_H
#define PICO_BENCH_H

#include <vector>
#include <cassert>
#include <chrono>
#include <algorithm>
#include <type_traits>
#include <numeric>
#include <ostream>
#include <iterator>
#include <cmath>
#include <utility>

namespace pico_bench {

/* Statistics on some time measurement value T, e.g. T = std::chrono::milliseconds
 * T must be some std::chrono::duration type
 */
template<typename T>
class Statistics {
	using rep = typename T::rep;
	std::vector<T> samples;

public:
	std::string time_suffix;

	Statistics(const std::vector<T> &s) : samples(s) {
		std::sort(samples.begin(), samples.end());
	}
	T percentile(const float p) const {
		return percentile(p, samples);
	}
	// Winsorize the data, sets all entries above 100 - limit percentile and below limit percentile
	// to the value of that percentile
	void winsorize(const float limit){
		winsorize(limit, samples);
	}
	T median() const {
		return percentile(50.0, samples);
	}
	T median_abs_dev() const {
		const auto m = median();
		std::vector<T> deviations;
		deviations.reserve(samples.size());
		std::transform(samples.begin(), samples.end(), std::back_inserter(deviations),
			[&m](const T &t){
				return T{std::abs((t - m).count())};
			});
		std::sort(deviations.begin(), deviations.end());
		return percentile(50.0, deviations);
	}
	T mean() const {
		const auto m = std::accumulate(samples.begin(), samples.end(), T{0});
		return m / samples.size();
	}
	T std_dev() const {
		const auto m = mean();
		auto val = std::accumulate(samples.begin(), samples.end(), T{0},
			[&m](const T &p, const T &t){
				return T{static_cast<rep>(p.count() + std::pow((t - m).count(), 2))};
			});
		return T{static_cast<rep>(std::sqrt(1.0 / static_cast<double>(samples.size())
						* static_cast<double>(val.count())))};
	}
	T min() const {
		return samples.front();
	}
	T max() const {
		return samples.back();
	}
	size_t size() const {
		return samples.size();
	}
	const T& operator[](size_t i) const {
		return samples[i];
	}

private:
	// Winsorize the data, sets all entries above 100 - limit percentile and below limit percentile
	// to the value of that percentile
	static void winsorize(const float limit, std::vector<T> &samples){
		const auto low = percentile(limit, samples);
		const auto high = percentile(100.0 - limit, samples);
		for (auto &t : samples){
			if (t < low){
				t = low;
			}
			else if (t > high){
				t = high;
			}
		}
	}
	static T percentile(const float p, const std::vector<T> &samples){
		assert(!samples.empty());
		assert(p <= 100.0);
		assert(p >= 0.0);
		if (samples.size() == 1){
			return samples.front();
		}
		if (p == 100.0){
			return samples.back();
		}
		const double rank = p / 100.0 * (static_cast<double>(samples.size()) - 1.0);
		const double low_r = std::floor(rank);
		const double dist = rank - low_r;
		const size_t k = static_cast<size_t>(low_r);
		const auto low = samples[k];
		const auto high = samples[k + 1];
		return T{static_cast<rep>(low.count() + (high - low).count() * dist)};
	}
};

/* Benchmarking measurement using some desired unit of time measurement,
 * e.g. T = std::chrono::milliseconds. T must be some std::chrono::duration
 */
template<typename T>
class Benchmarker {
	const size_t MAX_ITER;
	const size_t WARM_UPS;
	const T MAX_RUNTIME;

	template<typename Fn>
	struct BenchWrapper {
		Fn fn;

		BenchWrapper(Fn fn) : fn(fn){}
		T operator()(){
			auto start = std::chrono::high_resolution_clock::now();
			fn();
			auto end = std::chrono::high_resolution_clock::now();
			return std::chrono::duration_cast<T>(end - start);
		}
	};

public:
	using stats_type = Statistics<T>;

	// Benchmark the functions either max_iter times or until max_runtime seconds have elapsed
	// max_runtime should be > 0
	Benchmarker(const size_t max_iter, const std::chrono::seconds max_runtime, const size_t warm_ups = 1)
		: MAX_ITER(max_iter), WARM_UPS(warm_ups), MAX_RUNTIME(std::chrono::duration_cast<T>(max_runtime))
	{}
	// Create a benchmarker that will run the function for the desired number of iterations,
	// regardless of how long it takes
	Benchmarker(const size_t max_iter, const size_t warm_ups = 1)
    : MAX_ITER(max_iter), WARM_UPS(warm_ups), MAX_RUNTIME(0)
	{}

#ifdef PICO_BENCH_NO_DECLVAL
	template<typename Fn>
	stats_type operator()(Fn _fn) const {
		BenchWrapper<Fn> fn{_fn};
		// Do the warm up runs
    for (size_t i = 0; i < WARM_UPS; ++i) {
      fn();
    }
		T elapsed{0};
		std::vector<T> samples;
		for (size_t i = 0; i < MAX_ITER && (MAX_RUNTIME.count() == 0 || elapsed < MAX_RUNTIME);
				++i, elapsed += samples.back())
		{
			samples.push_back(fn());
		}
		return stats_type{samples};
	}
#else
	template<typename Fn>
	typename std::enable_if<std::is_void<decltype(std::declval<Fn>()())>::value, stats_type>::type
	operator()(Fn fn) const {
		return (*this)(BenchWrapper<Fn>{fn});
	}

	template<typename Fn>
	typename std::enable_if<std::is_same<decltype(std::declval<Fn>()()), T>::value,
		stats_type>::type
	operator()(Fn fn) const {
		// Do a the warm up runs
    for (size_t i = 0; i < WARM_UPS; ++i) {
      fn();
    }
		T elapsed{0};
		std::vector<T> samples;
		for (size_t i = 0; i < MAX_ITER && (MAX_RUNTIME.count() == 0 || elapsed < MAX_RUNTIME);
				++i, elapsed += samples.back())
		{
			samples.push_back(fn());
		}
		return stats_type{samples};
	}
#endif
};
}

template<typename T>
std::ostream& operator<<(std::ostream &os, const pico_bench::Statistics<T> &stats){
	os << "Statistics:\n"
		<< "\tmax: " << stats.max().count() << stats.time_suffix << "\n"
		<< "\tmin: " << stats.min().count() << stats.time_suffix << "\n"
		<< "\tmedian: " << stats.median().count() << stats.time_suffix << "\n"
		<< "\tmedian abs dev: " << stats.median_abs_dev().count() << stats.time_suffix << "\n"
		<< "\tmean: " << stats.mean().count() << stats.time_suffix << "\n"
		<< "\tstd dev: " << stats.std_dev().count() << stats.time_suffix << "\n"
		<< "\t# of samples: " << stats.size();
	return os;
}

#endif

