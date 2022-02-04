#include "xorshift.hpp"


XorShift::XorShift()
    : XorShift(State{0xDEADBEEF, 0xB16B00B5}) {}

XorShift::XorShift(State state)
    : m_state(state) {}

XorShift::result_type XorShift::operator()() {
	uint64_t t = m_state.x[0];
	uint64_t const s = m_state.x[1];
	m_state.x[0] = s;
	t ^= t << 23;		// a
	t ^= t >> 18;		// b -- Again, the shifts and the multipliers are tunable
	t ^= s ^ (s >> 5);	// c
	m_state.x[1] = t;
	return t + s;
}

