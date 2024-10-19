#include "drawing/sextantBlocks.hpp"
#include <clocale>
#include <csignal>
#include <exception>
#include <vector>
#include <ncursesw/curses.h>

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

int main() {
	std::set_terminate(termHandler);

	setlocale(LC_ALL, "");
	initscr(); // initialize curses
	cbreak(); // don't wait for a newline
	noecho(); // don't echo input
	nodelay(stdscr, true); // don't block for input
	start_color();
	assertMsg(can_change_color(), "You term == bad."); // TODO: better handling of this

	#define A Color(Category(true, 1), RGBA(255, 255, 255, 255))
	#define B Color(Category(true, 1), RGBA(0, 0, 0, 255))
	#define C Color(Category(true, 2), RGBA(0, 255, 0, 255))
	#define D Color(Category(true, 2), RGBA(0, 1, 255, 255))
	#define E Color(Category(true, 3), RGBA(16, 128, 32, 255))
	SextantDrawing drawing({
		{B,A,A,A,C,D,C,A,A,A,E,E},
		{B,A,A,B,C,D,A,A,C,C,E,E},
		{A,A,C,D,C,D,A,A,A,E,E,E}
	});
	#undef A
	#undef B
	#undef C
	#undef D
	#undef E
	
	WindowedDrawing finalDrawing(stdscr);
	finalDrawing.clear();
	finalDrawing.insert(SextantCoord(0, 0), drawing);
	finalDrawing.render();
	refresh();
	sleep(10);

	endwin();
	return 0;
}

