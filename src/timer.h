#include <chrono>


class timer {
  public:
    timer() {};

    void reset()  { start = std::chrono::steady_clock::now(); }
    void timeit() { end   = std::chrono::steady_clock::now(); }
    double getElapsedSC() {
        std::chrono::duration<double> et = end - start;
        return et.count();
    }

  private:
    std::chrono::time_point<std::chrono::steady_clock> start;
    std::chrono::time_point<std::chrono::steady_clock> end;
};
