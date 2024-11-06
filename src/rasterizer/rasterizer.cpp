#include "rasterizer.hpp"
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_double3.hpp>

using glm::dvec3, glm::ivec2;

#define fg Color(Category(false, 2), RGBA(255, 255, 255, 255))

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

