#include "TimerWrapper.h"

    TimerWrapper::TimerWrapper(const std::string& event) : event_name(event)
    {

    }
    void TimerWrapper::startTimers()
    {
      cpu_clock_start = std::clock();
      walltime_clock_start = std::chrono::high_resolution_clock::now();
    }
    void TimerWrapper::stopTimers()
    {
      cpu_clock_end = std::clock();
      walltime_clock_end = std::chrono::high_resolution_clock::now();
    }
    void TimerWrapper::printInfo(std::ostream& os)
    {
      os << "Event name: " << event_name << "\n";
      os << std::fixed << std::setprecision(2) << "CPU time used: "
              << 1000.0 * (cpu_clock_end - cpu_clock_start) / CLOCKS_PER_SEC << " ms\n"
              << "Wall clock time passed: "
              << std::chrono::duration<double, std::milli>(walltime_clock_end-walltime_clock_start).count()
              << " ms\n";      
    }