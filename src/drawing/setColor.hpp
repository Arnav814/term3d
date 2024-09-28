#ifndef SETCOLOR_HPP
#define SETCOLOR_HPP

#include <utility>
#include <hash_map>

// this file is just jank to work around ncurses color pairs.
// we can only use color pairs, so if we want to set fg and bg colors individually, 
// we need to dynamically create more color pairs

template <> struct std::hash<std::pair<unsigned char, unsigned char>> {
	size_t operator()(const pair<unsigned char, unsigned char> chPair) const {
		return (size_t) (chPair.first >> 8) | chPair.second;
	}
};

int getColorPair(unsigned char fg, unsigned char bg);

#endif /* SETCOLOR_HPP */
