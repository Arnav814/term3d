#include "raytracer.hpp"

// converts from origin at center to origin at top left
void putPixel(SextantDrawing canvas, SextantCoord coord, Color color) {
	SextantCoord translated{canvas.getHeight() / 2 - coord.y, canvas.getWidth() / 2 + coord.x};
	canvas.trySet(translated, color);
}

void renderLoop(SextantDrawing canvas, std::function<int()> refresh) {
	while (not EXIT_REQUESTED) {
		
		refresh();
	}
}

