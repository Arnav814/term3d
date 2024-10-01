#include "setColor.hpp"

#include <unordered_map>
#include <limits>
#include <stdexcept>
#include <curses.h>

template <> struct std::hash<std::pair<unsigned char, unsigned char>> {
	size_t operator()(const pair<unsigned char, unsigned char> chPair) const {
		return (size_t) (chPair.first >> 8) | chPair.second;
	}
};

namespace ColorCache {
	short next = 0;
	std::unordered_map<std::pair<unsigned char, unsigned char>, short> storedColors;
};

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
