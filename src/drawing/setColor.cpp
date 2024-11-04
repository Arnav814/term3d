#include "setColor.hpp"

#include <unordered_map>
#include <limits>
#include <stdexcept>
#include <curses.h>

// TODO: recycle colors/color_pairs

template <> struct std::hash<std::pair<uchar, uchar>> {
	size_t operator()(const pair<uchar, uchar> chPair) const {
		return (size_t) (chPair.first << 8) | chPair.second;
	}
};

namespace ColorCache {
	short nextPair = 64; // in case I want to use normal color pairs for anything
	std::unordered_map<std::pair<uchar, uchar>, short> storedColorPairs;

	short nextColor = 64; // same reason
	std::unordered_map<RGB, short> storedColors;
};

// jank to work around ncurses not allowing direct rgb control
short getColor(const RGB color) {
	using namespace ColorCache;
	auto tryFind = storedColors.find(color);
	if (tryFind == storedColors.end()) {
		if (nextColor == std::numeric_limits<decltype(nextColor)>::max())
			throw std::runtime_error("Reached maximium number of colors");
		nextColor++;
		#define SCALE(val) ((short) val * 1000 / 255)
		int status = init_extended_color(nextColor, SCALE(color.r), SCALE(color.g), SCALE(color.b));
		if (status == ERR)
			throw std::runtime_error(std::format("Cannot create more colors (at {})", nextColor));
		#undef SCALE
		storedColors.insert(std::make_pair(color, nextColor));
		return nextColor;
	} else {
		return tryFind->second;
	}
}

// this function is just jank to work around ncurses color pairs.
// we can only use color pairs, so if we want to set fg and bg colors individually, 
// we need to dynamically create more color pairs
int getColorPair(const uchar fg, const uchar bg) {
	using namespace ColorCache;
	auto tryFind = storedColorPairs.find(std::make_pair(fg, bg));
	if (tryFind == storedColorPairs.end()) {
		if (nextPair == std::numeric_limits<decltype(nextPair)>::max())
			throw std::runtime_error("Reached maximium number of color_pairs");
		nextPair++;
		int status = init_extended_pair(nextPair, fg, bg);
		if (status == ERR)
			throw std::runtime_error("Cannot create more color_pairs");
		storedColorPairs.insert(std::make_pair(std::make_pair(fg, bg), nextPair));
		return nextPair;
	} else {
		return tryFind->second;
	}
}

