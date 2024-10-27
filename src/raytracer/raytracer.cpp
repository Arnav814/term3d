#include "raytracer.hpp"
#include "coord3d.hpp"
#include "../extraAssertions.hpp"

const double viewportWidth = 1.0;
const double viewportHeight = 1.0;
const double viewportDistance = 1.0;

// converts from origin at center to origin at top left
void putPixel(SextantDrawing canvas, SextantCoord coord, Color color) {
	SextantCoord translated{canvas.getHeight() / 2 - coord.y, canvas.getWidth() / 2 + coord.x};
	canvas.trySet(translated, color);
}

Coord3d<double> canvasToViewport(SextantCoord coord, SextantCoord canvasSize) {
	return Coord3d{
		coord.x * (viewportWidth / canvasSize.x),
		coord.y * (viewportHeight / canvasSize.y),
		viewportDistance
	};
}

void renderLoop(SextantDrawing canvas, const bool& exit_requested, std::function<int()> refresh) {
	while (not exit_requested) {
		for (int x = -canvas.getWidth() / 2; x < canvas.getWidth() / 2; x++) {
			for (int y = -canvas.getHeight() / 2; y < canvas.getHeight() / 2; y++) {
				Coord3d direction = canvasToViewport(SextantCoord(y, x), canvas.getSize());
				
			}
		}
		refresh();
	}
}

