#include "drawing/sextantBlocks.hpp"
#include <clocale>
#include <csignal>
#include <vector>
#include <ncursesw/curses.h>

int main() {
	setlocale(LC_ALL, "");
	initscr(); // initialize curses
	cbreak(); // don't wait for a newline
	noecho(); // don't echo input
	nodelay(stdscr, true); // don't block for input
	start_color();

	#define A Color(Category(false, 1), RGBA(255, 255, 255, 255))
	#define B Color(Category(false, 1), RGBA(0, 0, 0, 255))
	#define C Color(Category(false, 2), RGBA(0, 255, 0, 255))
	#define D Color(Category(false, 2), RGBA(0, 1, 255, 255))
	SextantDrawing drawing({
		{B,A,A,A,C,D,C,A},
		{B,A,A,B,C,D,A,A},
		{A,A,C,D,C,D,A,A}
	});
	#undef A
	#undef B
	#undef C
	#undef D
	
	WindowedDrawing finalDrawing(stdscr);
	finalDrawing.clear();
	finalDrawing.insert(SextantCoord(0, 0), drawing);
	finalDrawing.render();
	refresh();
	sleep(10);

	endwin();
	return 0;
}

