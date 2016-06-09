#ifndef __HAYAI_TESTRESULT
#define __HAYAI_TESTRESULT
#include <vector>
#include <stdexcept>
#include <limits>

#include "hayai_clock.hpp"


namespace hayai
{
    /// Test result descriptor.

    /// All durations are expressed in nanoseconds.
    struct TestResult
    {
    public:
        /// Initialize test result descriptor.

        /// @param runTimes Timing for the individual runs.
        /// @param iterations Number of iterations per run.
        TestResult(const std::vector<uint64_t>& runTimes,
                   std::size_t iterations)
            :   _runTimes(runTimes),
                _iterations(iterations),
                _timeTotal(0),
                _timeRunMin(std::numeric_limits<uint64_t>::max()),
                _timeRunMax(std::numeric_limits<uint64_t>::min())
        {
            // Summarize under the assumption of values being accessed more
            // than once.
            std::vector<uint64_t>::iterator runIt = _runTimes.begin();

            while (runIt != _runTimes.end())
            {
                const uint64_t run = *runIt;

                _timeTotal += run;
                if ((runIt == _runTimes.begin()) || (run > _timeRunMax))
                    _timeRunMax = run;
                if ((runIt == _runTimes.begin()) || (run < _timeRunMin))
                    _timeRunMin = run;

                ++runIt;
            }
        }


        /// Total time.
        inline double TimeTotal() const
        {
            return _timeTotal;
        }


        /// Run times.
        inline const std::vector<uint64_t>& RunTimes() const
        {
            return _runTimes;
        }


        /// Average time per run.
        inline double RunTimeAverage() const
        {
            return double(_timeTotal) / double(_runTimes.size());
        }


        /// Maximum time per run.
        inline double RunTimeMaximum() const
        {
            return _timeRunMax;
        }


        /// Minimum time per run.
        inline double RunTimeMinimum() const
        {
            return _timeRunMin;
        }


        /// Average runs per second.
        inline double RunsPerSecondAverage() const
        {
            return 1000000000.0 / RunTimeAverage();
        }


        /// Maximum runs per second.
        inline double RunsPerSecondMaximum() const
        {
            return 1000000000.0 / _timeRunMin;
        }


        /// Minimum runs per second.
        inline double RunsPerSecondMinimum() const
        {
            return 1000000000.0 / _timeRunMax;
        }


        /// Average time per iteration.
        inline double IterationTimeAverage() const
        {
            return RunTimeAverage() / double(_iterations);
        }


        /// Minimum time per iteration.
        inline double IterationTimeMinimum() const
        {
            return _timeRunMin / double(_iterations);
        }


        /// Maximum time per iteration.
        inline double IterationTimeMaximum() const
        {
            return _timeRunMax / double(_iterations);
        }


        /// Average iterations per second.
        inline double IterationsPerSecondAverage() const
        {
            return 1000000000.0 / IterationTimeAverage();
        }


        /// Minimum iterations per second.
        inline double IterationsPerSecondMinimum() const
        {
            return 1000000000.0 / IterationTimeMaximum();
        }


        /// Maximum iterations per second.
        inline double IterationsPerSecondMaximum() const
        {
            return 1000000000.0 / IterationTimeMinimum();
        }
    private:
        std::vector<uint64_t> _runTimes;
        std::size_t _iterations;
        uint64_t _timeTotal;
        uint64_t _timeRunMin;
        uint64_t _timeRunMax;
    };
}
#endif
