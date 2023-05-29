#include <memory>
#include <utility>


template<typename F>
class simpleGuard {
  private: 
    F m_f;
  public:
    simpleGuard(F func) : m_f(func) {};
    ~simpleGuard() { m_f(); };
};

template<typename F>
simpleGuard<F>
make_simple_guard(F f) {
    return simpleGuard<F>(f);
}
