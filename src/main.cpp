#include "drawing/sextantBlocks.hpp"
#include <clocale>
#include <vector>
#include <ncursesw/curses.h>

int main() {
	setlocale(LC_ALL, "");
	initscr(); // initialize curses
	cbreak(); // don't wait for a newline
	noecho(); // don't echo input
	nodelay(stdscr, true); // don't block for input

	#define F PriorityColor(COLOR_BLACK, 1)
	#define O PriorityColor(COLOR_WHITE, 1)
	SextantDrawing drawing({
		{F,F,O,O,O,F,F,F,O},
		{O,F,F,O,F,F,O,F,F},
		{O,F,F,O,F,F,O,F,F},
		{F,F,O,O,O,F,F,F,O}
	});
	#undef F
	#undef O
	
	drawing.render(CharCoord(0, 0));
	refresh();
	sleep(10);

	return 0;
}

