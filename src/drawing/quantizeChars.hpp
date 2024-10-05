#ifndef QUANTIZECHARS_HPP
#define QUANTIZECHARS_HPP

// only one function?!
// how does this deserve its own file?

// note: do not look at the implementation of this if you value your sanity

#include <utility>
#include "setColor.hpp"

std::pair<charArray<bool>, int> getTrimmedColors(const charArray<Color>& arrayChar);

#endif /* QUANTIZECHARS_HPP */
