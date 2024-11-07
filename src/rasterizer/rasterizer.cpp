#include "rasterizer.hpp"
#include <boost/concept_check.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <limits>

// TODO: a lot of this stuff should be passed by reference

using glm::dvec3, glm::ivec2;

#define fg Color(Category(false, 2), RGBA(255, 255, 255, 255))

// assumes x0 <= x1
std::vector<double> interpolate(int x0, double y0, int x1, double y1) {
		// TODO: do these really need to be doubles?
	if (x0 == x1) // for only one point
		return {y0};

	std::vector<double> out;
	out.reserve(x1 - x0 + 1);

	double slope = (double) (y1 - y0) / (x1 - x0);
	double y = x0;
	for (int x = x0; x <= x1; x++) {
		out.push_back(y);
		y += slope;
	}

	return out; // FIXME: Passing a vector like this is horrendously inefficient. Use an iterator.
}

void drawLine(SextantDrawing& canvas, ivec2 p0, ivec2 p1, const Color color) {
	if (abs(p0.x - p1.x) > abs(p0.y - p1.y)) { // line is horizontalish
		if (p0.x > p1.x) // make sure p0 is left of p1
			std::swap(p0, p1);

		std::vector<double> yVals = interpolate(p0.x, p0.y, p1.x, p1.y);
		for (int x = p0.x; x <= p1.x; x++) {
			putPixel(canvas, SextantCoord(yVals.at(x - p0.x), x), color);
		}
	} else { // line is verticalish
		if (p0.y > p1.y) // make sure p0 is under p1
			std::swap(p0, p1);

		std::vector<double> xVals = interpolate(p0.y, p0.x, p1.y, p1.x);
		for (int y = p0.y; y <= p1.y; y++) {
			putPixel(canvas, SextantCoord(y, xVals.at(y - p0.y)), color);
		}
	}
}

void rasterRenderLoop(WindowedDrawing& rawCanvas, const bool& exit_requested, std::function<int()> refresh) {
	int minDimension = std::min(rawCanvas.getHeight(), rawCanvas.getWidth()/2);
	SextantDrawing canvas(minDimension, minDimension*2);
	/* dvec3 origin = dvec3(); */

	while (not exit_requested) {
		drawLine(canvas, ivec2(100, 5), ivec2(1, 20), fg);
		drawLine(canvas, ivec2(0, 10), ivec2(0, -29), fg);

		rawCanvas.clear();
		rawCanvas.insert(SextantCoord(0, 0), canvas);
		rawCanvas.render();
		refresh();
		sleep(1);
	}
}

