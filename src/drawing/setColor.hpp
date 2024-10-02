#ifndef SETCOLOR_HPP
#define SETCOLOR_HPP

// this file is just jank to work around ncurses color pairs.
// we can only use color pairs, so if we want to set fg and bg colors individually, 
// we need to dynamically create more color pairs

int getColorPair(unsigned char fg, unsigned char bg);

#endif /* SETCOLOR_HPP */
