#include <clocale>
#include <csignal>
#include <exception>
#include <notcurses/notcurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "rasterizer/interpolate.hpp"
#include "rasterizer/controller.hpp"

static_assert(CHAR_BIT == 8, "WTF are you compiling this on?!");

void termHandler() {
	auto error = std::current_exception();
	if (error != NULL) {
		try {
			std::rethrow_exception(error);
		} catch (std::exception& e) {
			std::cerr << "Exception encountered: " << e.what() << '.';
		}
	} else {
		std::cerr << "Terminate called.";
	}
 	std::cerr << " Stacktrace follows.\n";

	void* trace_elems[25];
	int trace_elem_count = backtrace(trace_elems, 25);
	char** as_strings = backtrace_symbols(trace_elems, trace_elem_count);

	for (int i = 0; i < trace_elem_count; i++) {
		std::cerr << as_strings[i] << '\n';
	}
	std::cerr << std::flush;

	free(as_strings);
	abort();
}

bool EXIT_REQUESTED = false;

void sigHandle([[maybe_unused]] int sig) {
	// try exiting nicely once, then hard abort
	if (not EXIT_REQUESTED) {
		EXIT_REQUESTED = true;
	} else {
		abort();
	}
}

int main() {
	std::set_terminate(termHandler);

	// make interrups exit nicely
	signal(SIGINT, sigHandle);
	signal(SIGTERM, sigHandle);

	Interpolate interpa{-69, -82, -53, -86};
	Interpolate interpb{-53, -86, -7, -86};

	Collate<1> colla{{interpa}};
	Collate<1> collb{{interpb}};

	CollateJoin both{colla, collb};
	for (auto i = both.begin(); i != both.end(); ++i) {
		std::println(std::cerr, "{}", (*i)[0]);
	}

	setlocale(LC_ALL, "");
	notcurses* nc = notcurses_core_init(NULL, stdout);
	ncplane* stdplane = notcurses_stdplane(nc);

	renderLoop(nc, stdplane, EXIT_REQUESTED);

	notcurses_stop(nc);
	return 0;
}

