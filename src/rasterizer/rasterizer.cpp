#include "rasterizer.hpp"
#include <glm/ext/vector_double3.hpp>

using glm::dvec3;

void rasterRenderLoop(WindowedDrawing& rawCanvas, const bool& exit_requested, std::function<int()> refresh) {
	int minDimension = std::min(rawCanvas.getHeight(), rawCanvas.getWidth()/2);
	SextantDrawing canvas{minDimension, minDimension*2};
	dvec3 origin = dvec3();

	while (not exit_requested) {
		// TODO: implement the thing

		rawCanvas.insert(SextantCoord(0, 0), canvas);
		rawCanvas.render();
		refresh();
		sleep(1);
	}
}

