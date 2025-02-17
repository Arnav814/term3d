#ifndef INTERPOLATE_HPP
#define INTERPOLATE_HPP
#include "../extraAssertions.hpp"
#include <vector>

inline std::vector<double> interpolate(const int x0, const double y0, const int x1,
                                       const double y1) {
	// TODO: do these really need to be doubles?
	if (x0 == x1) // for only one point
		return {y0};

	assertGt(x1, x0, "Can't interpolate backwards");

	std::vector<double> out;
	out.reserve(x1 - x0 + 1);

	double slope = (double)(y1 - y0) / (x1 - x0);
	double y = y0;
	for (int x = x0; x <= x1; x++) {
		out.push_back(y);
		y += slope;
	}

	return out; // FIXME: Passing a vector like this is horrendously inefficient. Use an iterator.
}

// find the interpolated value at x; use for one-offs where you don't need the whole std::vector
inline double interpolateValue(const int x0, const double y0, const int x1, const double y1,
                               const int x) {
	assertGt(x1, x0, "Can't interpolate backwards");
	double slope = (double)(y1 - y0) / (x1 - x0);
	return y0 + (x - x1) * slope;
}

// interpolate a member of Elem
template <typename Elem>
inline void interpolateField(std::vector<Elem>& baseVector, double Elem::*member,
                             const int x0, const double y0, const int x1, const double y1) {
	// TODO: do these really need to be doubles?

	assertEq(baseVector.size(), static_cast<uint>(x1 - x0 + 1),
	         "baseVector must be set to the correct size beforehand.");

	if (x0 == x1) { // for only one point
		baseVector[0].*member = y0;
		return;
	}

	assertGt(x1, x0, "Can't interpolate backwards");

	double slope = (double)(y1 - y0) / (x1 - x0);
	double y = y0;
	for (int x = 0; x <= x1 - x0; x++) {
		baseVector[x].*member = y;
		y += slope;
	}
}

#endif /* INTERPOLATE_HPP */
