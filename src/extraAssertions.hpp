#ifndef MOREASSERTIONS_CPP
#define MOREASSERTIONS_CPP
#include <cassert>
#include <iostream>
#include <execinfo.h>

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

#ifndef NDEBUG

	// this one was mostly copied from https://stackoverflow.com/questions/3767869/adding-message-to-assert
	#define assertMessage(condition, message) \
		do { \
			if (!(condition)) { \
				std::cerr << "Assertion `" #condition "` failed in " << __FILE__ \
					 << " line " << __LINE__ << ": " << message << '\n'; \
				printTrace(); \
				std::terminate(); \
			} \
		} while (false)

	#define assertEq(value, target, message) \
		do { \
			if (!((value) == (target))) { \
				std::cerr << "Assertion `" #value "` (" << value << ") equal to `" #target "` (" << target << ") failed in " \
					 << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				printTrace(); \
				std::terminate(); \
			} \
		} while (false)

	#define assertGt(value, target, message) \
		do { \
			if (!((value) > (target))) { \
				std::cerr << "Assertion `" #value "` (" << value << ") greater than `" #target "` (" << target << ") failed in " \
					 << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				printTrace(); \
				std::terminate(); \
			} \
		} while (false)

	#define assertGtEq(value, target, message) \
		do { \
			if (!((value) >= (target))) { \
				std::cerr << "Assertion `" #value "` (" << value << ") greater than or equal to `" #target "` (" << target \
					 << ") failed in " << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				printTrace(); \
				std::terminate(); \
			} \
		} while (false)

	#define assertLt(value, target, message) \
		do { \
			if (!((value) < (target))) { \
				std::cerr << "Assertion `" #value "` (" << value << ") less than `" #target "` (" << target << ") failed in " \
					 << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				printTrace(); \
				std::terminate(); \
			} \
		} while (false)

	#define assertLtEq(value, target, message) \
		do { \
			if (!((value) <= (target))) { \
				std::cerr << "Assertion `" #value "` (" << value << ") less than or equal to `" #target "` (" << target \
					 << ") failed in " << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				printTrace(); \
				std::terminate(); \
			} \
		} while (false)

	/* checks that value is between [min, max) */
	#define assertBetweenHalfOpen(min, value, max, message) \
		do { \
			if (!((min) <= (value) && (value) < (max))) { \
				std::cerr << "Assertion `" #min "` (" << min << ") <= `" #value "` (" << value << ") < `" #max "` (" << max \
					 << ") failed in " << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				printTrace(); \
				std::terminate(); \
			} \
		} while (false)

	#define assertBetweenIncl(min, value, max, message) \
		do { \
			if (!((min) <= (value) && (value) <= (max))) { \
				std::cerr << "Assertion `" #min "` (" << min << ") <= `" #value "` (" << value << ") <= `" #max "` (" << max \
					 << ") failed in " << __FILE__ << " line " << __LINE__ << ": " << message << '\n'; \
				printTrace(); \
				std::terminate(); \
			} \
		} while (false)

#else

	#define assertMessage(condition, message) do { } while (false)
	#define assertEq(value, target, message) do { } while (false)
	#define assertGt(value, target, message) do { } while (false)
	#define assertGtEq(value, target, message) do { } while (false)
	#define assertLt(value, target, message) do { } while (false)
	#define assertLtEq(target, value, message) do { } while (false)
	#define assertBetween(min, value, max, message) do { } while (false)
	#define assertBetweenIncl(min, value, max, message) do { } while (false)

#endif

#endif /* MOREASSERTIONS_CPP */
