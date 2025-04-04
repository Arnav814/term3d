#ifndef MOREASSERTIONS_CPP
#define MOREASSERTIONS_CPP
#include <cassert>
#include <cmath>
#include <exception>
#include <execinfo.h>
#include <glm/detail/qualifier.hpp>
#include <iostream>

// this file serves as a place for everything that needs to be included everywhere

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

// slightly less verbose
#define ALL_OF(vector) (vector).begin(), (vector).end()

inline void pass() {} // I <3 python

inline void printTrace() {
	std::cerr << "Traceback:" << '\n';
	void* callstack[128];
	int frames = backtrace(callstack, 128);
	char** strs = backtrace_symbols(callstack, frames);
	for (int i = 0; i < frames; ++i) {
		std::cerr << strs[i] << '\n';
	}
	std::cerr << std::flush;
	free(strs);
}

template <glm::length_t Length, typename VecType, glm::qualifier Qual>
inline bool isFinite(glm::vec<Length, VecType, Qual> vec) {
	if constexpr (Length >= 1)
		if (not std::isfinite(vec.x)) return false;
	if constexpr (Length >= 2)
		if (not std::isfinite(vec.y)) return false;
	if constexpr (Length >= 3)
		if (not std::isfinite(vec.z)) return false;
	if constexpr (Length >= 4)
		if (not std::isfinite(vec.w)) return false;
	return true;
}

#ifndef NDEBUG

	// this one was mostly copied from
	// https://stackoverflow.com/questions/3767869/adding-message-to-assert
	#define assertMsg(condition, message) \
		do { \
			if (!(condition)) { \
				std::cerr << "Assertion `" #condition "` failed in " << __FILE__ << " line " \
				          << __LINE__ << ": " << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	#define assertEq(value, target, message) \
		do { \
			if (!((value) == (target))) { \
				std::cerr << "Assertion `" #value "` (" << value << ") equal to `" #target "` (" \
				          << target << ") failed in " << __FILE__ << " line " << __LINE__ << ": " \
				          << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	#define assertNotEq(value, target, message) \
		do { \
			if (!((value) != (target))) { \
				std::cerr << "Assertion `" #value "` (" << value \
				          << ") not equal to `" #target "` (" << target << ") failed in " \
				          << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	#define assertGt(value, target, message) \
		do { \
			if (!((value) > (target))) { \
				std::cerr << "Assertion `" #value "` (" << value \
				          << ") greater than `" #target "` (" << target << ") failed in " \
				          << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	#define assertGtEq(value, target, message) \
		do { \
			if (!((value) >= (target))) { \
				std::cerr << "Assertion `" #value "` (" << value \
				          << ") greater than or equal to `" #target "` (" << target \
				          << ") failed in " << __FILE__ << " line " << __LINE__ << ": " << message \
				          << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	#define assertLt(value, target, message) \
		do { \
			if (!((value) < (target))) { \
				std::cerr << "Assertion `" #value "` (" << value << ") less than `" #target "` (" \
				          << target << ") failed in " << __FILE__ << " line " << __LINE__ << ": " \
				          << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	#define assertLtEq(value, target, message) \
		do { \
			if (!((value) <= (target))) { \
				std::cerr << "Assertion `" #value "` (" << value \
				          << ") less than or equal to `" #target "` (" << target << ") failed in " \
				          << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	/* checks that value is between [min, max) */
	#define assertBetweenHalfOpen(min, value, max, message) \
		do { \
			if (!((min) <= (value) && (value) < (max))) { \
				std::cerr << "Assertion `" #min "` (" << min << ") <= `" #value "` (" << value \
				          << ") < `" #max "` (" << max << ") failed in " << __FILE__ << " line " \
				          << __LINE__ << ": " << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	#define assertBetweenIncl(min, value, max, message) \
		do { \
			if (!((min) <= (value) && (value) <= (max))) { \
				std::cerr << "Assertion `" #min "` (" << min << ") <= `" #value "` (" << value \
				          << ") <= `" #max "` (" << max << ") failed in " << __FILE__ << " line " \
				          << __LINE__ << ": " << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	#define assertFinite(value, message) \
		do { \
			if (not std::isfinite(value)) { \
				std::cerr << "Assertion `" #value "` (" << value << ") is finite failed in " \
				          << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

	#define assertFiniteVec(value, message) \
		do { \
			if (not isFinite(value)) { \
				std::cerr << "Assertion `" #value "` (" << std::format("{}", value) \
				          << ") is finite failed in " << __FILE__ << " line " << __LINE__ << ": " \
				          << message << '\n'; \
				std::terminate(); \
			} \
		} while (false)

#else

	#define assertMsg(condition, message) \
		do { \
		} while (false)
	#define assertEq(value, target, message) \
		do { \
		} while (false)
	#define assertNotEq(value, target, message) \
		do { \
		} while (false)
	#define assertGt(value, target, message) \
		do { \
		} while (false)
	#define assertGtEq(value, target, message) \
		do { \
		} while (false)
	#define assertLt(value, target, message) \
		do { \
		} while (false)
	#define assertLtEq(target, value, message) \
		do { \
		} while (false)
	#define assertBetweenHalfOpen(min, value, max, message) \
		do { \
		} while (false)
	#define assertBetweenIncl(min, value, max, message) \
		do { \
		} while (false)
	#define assertFinite(value, message) \
		do { \
		} while (false)
	#define assertFiniteVec(value, message) \
		do { \
		} while (false)

#endif

#endif /* MOREASSERTIONS_CPP */
