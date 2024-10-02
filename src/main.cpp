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

	#define F PriorityColor(COLOR_WHITE, 1)
	#define O PriorityColor(COLOR_BLACK, 1)
	SextantDrawing drawing({
		{F,F,O,O,F,F,F,O},
		{O,F,F,F,F,O,F,F},
		{O,F,F,F,F,F,F,F},
		{F,F,O,O,F,F,F,O}
	});
	#undef F
	#undef O
	
	WindowedDrawing finalDrawing(stdscr);
	finalDrawing.clear();
	finalDrawing.insert(SextantCoord(0, 0), drawing, OverrideStyle::Priority);
	finalDrawing.render();
	refresh();
	sleep(10);

	endwin();
	return 0;
}

