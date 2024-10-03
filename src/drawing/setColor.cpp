#include "setColor.hpp"

#include <unordered_map>
#include <limits>
#include <stdexcept>
#include <curses.h>

template <> struct std::hash<std::pair<unsigned char, unsigned char>> {
	size_t operator()(const pair<unsigned char, unsigned char> chPair) const {
		return (size_t) (chPair.first << 8) | chPair.second;
	}
};

namespace ColorCache {
	short next = 64; // in case I want to use normal color pairs for anything
	std::unordered_map<std::pair<unsigned char, unsigned char>, short> storedColors;
};

// this function is just jank to work around ncurses color pairs.
// we can only use color pairs, so if we want to set fg and bg colors individually, 
// we need to dynamically create more color pairs
int getColorPair(unsigned char fg, unsigned char bg) {
	using namespace ColorCache;
	auto tryFind = storedColors.find(std::make_pair(fg, bg));
	if (tryFind == storedColors.end()) {
		if (next == std::numeric_limits<decltype(next)>::max())
			throw std::runtime_error("Reached maximium number of color_pairs.");
		next++;
		init_pair(next, fg, bg);
		storedColors.insert(std::make_pair(std::make_pair(fg, bg), next));
		return COLOR_PAIR(next);
	} else [[likely]] {
		return COLOR_PAIR(tryFind->second);
	}
}
