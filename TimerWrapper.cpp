#include "TimerWrapper.h"

    TimerWrapper::TimerWrapper(const std::string& event, size_t default_event_amount) : event_name(event)
    {
      cpu_times.reserve(default_event_amount);
      wall_times.reserve(default_event_amount);
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
      
      cpu_times.push_back((cpu_clock_end - cpu_clock_start));
      //wall_times.push_back(std::chrono::duration<double, std::milli>(walltime_clock_end-walltime_clock_start).count());
    }
    void TimerWrapper::printInfo(std::ostream& os)
    {
      os << "Event name: " << event_name << "\n";
      size_t n = cpu_times.size();
      if(n > 1)
      {
        std::clock_t totalTime = 0;
        for (size_t i = 0; i < n; i++)
        {
          totalTime += cpu_times[i];
        }
        os << "For a total of " << n << " event samplings:" << "\n";
        os << std::fixed << std::setprecision(2) << "CPU time used: "
                << 1000.0 * totalTime / CLOCKS_PER_SEC << " ms\n"
                //<< "Wall clock time passed: "
                //<< std::chrono::duration<double, std::milli>(walltime_clock_end-walltime_clock_start).count()
                //<< " ms\n";
                ;
        
      }
      else 
      {
        os << std::fixed << std::setprecision(2) << "CPU time used: "
                << 1000.0 * (cpu_clock_end - cpu_clock_start) / CLOCKS_PER_SEC << " ms\n"
                << "Wall clock time passed: "
                << std::chrono::duration<double, std::milli>(walltime_clock_end-walltime_clock_start).count()
                << " ms\n";
      }
    }