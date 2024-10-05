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

	#define F Color(Category(false, 1), RGBA(255, 255, 255, 255))
	#define O Color(Category(false, 1), RGBA(0, 0, 0, 255))
	#define R Color(Category(false, 1), RGBA(0, 255, 0, 255))
	SextantDrawing drawing({
		{F,F,O,O,F,F,F,R},
		{R,F,F,F,F,O,F,F},
		{O,F,F,F,F,F,F,F}
	});
	#undef F
	#undef O
	#undef R
	
	WindowedDrawing finalDrawing(stdscr);
	finalDrawing.clear();
	finalDrawing.insert(SextantCoord(0, 0), drawing);
	finalDrawing.render();
	refresh();
	sleep(10);

	endwin();
	return 0;
}

