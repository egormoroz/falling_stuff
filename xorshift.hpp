#ifndef XORSHIFT_HPP
#define XORSHIFT_HPP

#include <cstdint>

//Xorshift128+
//https://en.wikipedia.org/wiki/Xorshift
class XorShift {
public:
    struct State {
        uint64_t x[2];
    };


    XorShift();
    explicit XorShift(State seed);

    using result_type = uint64_t;
    static constexpr result_type min() { return 0; }
    static constexpr result_type max() { return UINT64_MAX; }

    result_type operator()();
    
private:
    State m_state;
};

#endif

