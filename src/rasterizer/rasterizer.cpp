#include "rasterizer.hpp"
#include <boost/concept_check.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <limits>

using glm::dvec3, glm::ivec2;

#define fg Color(Category(false, 2), RGBA(255, 255, 255, 255))
#define sNaN std::numeric_limits<double>::signaling_NaN()

class Interpolate {
	ivec2 p0; ivec2 p1;

	Interpolate(ivec2 p0, ivec2 p1) {
		// ensure p0 is left of p1
		if (p0.x > p1.x)
			std::swap(p0, p1);

		this->p0 = p0;
		this->p1 = p1;
	}

	class InterpolateIterator : public boost::iterator_facade<InterpolateIterator, double, boost::forward_traversal_tag> {
		public:
			InterpolateIterator() : slope(sNaN), x(0), y(sNaN) {}
			InterpolateIterator(double slope, int x, double y) : slope(slope), x(x), y(y) {}
		private:
			friend class boost::iterator_core_access;
			double slope; int x; double y;

			void increment() {y += slope;}

			// y and slope are uncessary for comparisons this will be used for, and
			// floating point precision errors would be a pain
			bool equal(const InterpolateIterator& other) {return this->x == other.x;}

			double dereference() const {return y;}
	};

	InterpolateIterator begin() const {
		double slope = (double) (p1.y - p0.y) / (p1.x - p0.x);
		return {slope, p0.x, static_cast<double>(p0.y)};
	}
	
	InterpolateIterator end() const {
		return {sNaN, p1.x+1 /* one past the last */, sNaN}; // only need x for equality
	}
};

void drawLine(SextantDrawing& canvas, ivec2 p0, ivec2 p1, const Color color) {
	// ensure p0 is left of p1
	if (p0.x > p1.x)
		std::swap(p0, p1);

	double slope = (double) (p1.y - p0.y) / (p1.x - p0.x);
	double y = p0.x;
	for (int x = p0.x; x <= p1.x; x++) {
		putPixel(canvas, SextantCoord(y, x), color);
		y += slope;
	}
}

void rasterRenderLoop(WindowedDrawing& rawCanvas, const bool& exit_requested, std::function<int()> refresh) {
	int minDimension = std::min(rawCanvas.getHeight(), rawCanvas.getWidth()/2);
	SextantDrawing canvas(minDimension, minDimension*2);
	/* dvec3 origin = dvec3(); */

	while (not exit_requested) {
		drawLine(canvas, ivec2(100, 5), ivec2(1, 20), fg);

		rawCanvas.clear();
		rawCanvas.insert(SextantCoord(0, 0), canvas);
		rawCanvas.render();
		refresh();
		sleep(1);
	}
}

