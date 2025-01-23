#ifndef QUANTIZECHARS_HPP
#define QUANTIZECHARS_HPP

// only one function?!
// how does this deserve its own file?

// note: do not look at the implementation of this if you value your sanity

#include "setColor.hpp"
#include <utility>

std::pair<charArray<bool>, std::pair<RGB, RGB>> getTrimmedColors(const charArray<Color>& arrayChar);

#endif /* QUANTIZECHARS_HPP */
