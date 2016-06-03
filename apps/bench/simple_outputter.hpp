#ifndef __HAYAI_SIMPLEOUTPUTTER
#define __HAYAI_SIMPLEOUTPUTTER
#include "hayai/hayai_outputter.hpp"
#include "hayai/hayai_console.hpp"


namespace hayai
{
    /// Console outputter.

    /// Prints the result to standard output.
    class SimpleOutputter
        :   public Outputter
    {
    public:
        /// Initialize console outputter.

        /// @param stream Output stream. Must exist for the entire duration of
        /// the outputter's use.
        SimpleOutputter(std::ostream& stream = std::cout)
            :   _stream(stream)
        {

        }


        void Begin(const std::size_t&, const std::size_t&) override
        {
        }


        void End(const std::size_t&, const std::size_t&) override
        {
        }


        void BeginTest(const std::string&,
                       const std::string&,
                       const TestParametersDescriptor&,
                       const std::size_t&,
                       const std::size_t&) override
        {
        }


        void SkipDisabledTest(const std::string&,
                              const std::string&,
                              const TestParametersDescriptor&,
                              const std::size_t&,
                              const std::size_t&) override
        {
        }


        void EndTest(const std::string&,
                     const std::string&,
                     const TestParametersDescriptor&,
                     const TestResult& result) override
        {
            _stream << Console::TextBlue
                    << Console::TextDefault
                    << std::setprecision(5)
                    << std::setw(9)
                    << result.IterationsPerSecondAverage()
                    << " fps";
        }


        std::ostream& _stream;
    };
}
#endif
