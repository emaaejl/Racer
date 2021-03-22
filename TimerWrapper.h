#ifndef __TIMER_WRAPPER_H
#define __TIMER_WRAPPER_H

#include <ctime>
#include <chrono>
#include <iomanip>
class TimerWrapper
{
  private:
    std::clock_t cpu_clock_start, cpu_clock_end;
    std::chrono::high_resolution_clock::time_point walltime_clock_start, walltime_clock_end;
    std::string event_name;
  public:
    TimerWrapper(const std::string& event);
    void startTimers();
    void stopTimers();
    void printInfo(std::ostream& os);
};

#endif