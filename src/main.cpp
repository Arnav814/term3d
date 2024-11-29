#include "drawing/sextantBlocks.hpp"
#include <clocale>
#include <csignal>
#include <exception>
#include <notcurses/notcurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "glm/gtx/euler_angles.hpp"
#include "glm/gtx/string_cast.hpp"
#include "rasterizer/rasterizer.hpp"
#include "rasterizer/renderable.hpp"
#include "rasterizer/triangles.hpp"

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

void pass() {}

int main() {
	std::set_terminate(termHandler);

	// make interrups exit nicely
	signal(SIGINT, sigHandle);
	signal(SIGTERM, sigHandle);

	// Transform t{{1, 2, 3}, glm::yawPitchRoll<double>(0, 1, 0.5), {2, 3.4, 0.2}};
	// t = invertTransform(t);
	// dvec3 v{1, 2, 3};
	// dvec3 norm = transform(v, t);
	// dvec3 homo = canonicalize(parseTransform(t) * dvec4{v.x, v.y, v.z, 1});
	// std::println(std::cout, "norm: {}\nhomo: {}", glm::to_string(norm), glm::to_string(homo));
	// if (glm::to_string(norm) != glm::to_string(homo)) std::println(std::cout, "AAAAAAAAAAA");

	setlocale(LC_ALL, "");
	notcurses* nc = notcurses_core_init(NULL, stdout);
	ncplane* stdplane = notcurses_stdplane(nc);

	WindowedDrawing finalDrawing(stdplane);
	auto render = std::bind(notcurses_render, nc);
	rasterRenderLoop(finalDrawing, EXIT_REQUESTED, render);

	notcurses_stop(nc);
	return 0;
}

