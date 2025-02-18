#include "controller.hpp"
#include "rasterizer.hpp"
#include "renderable.hpp"
#include "structures.hpp"
#include <glm/gtx/euler_angles.hpp>
#include <limits>
#include <stdexcept>

bool debugFrame;

void renderLoop(notcurses* nc, ncplane* plane, const bool& exitRequested) {
	WindowedDrawing finalDrawing{plane};
	int minDimension = std::min(finalDrawing.getHeight(), finalDrawing.getWidth());
	SextantDrawing squareDrawing{minDimension, minDimension};
	Scene scene = initScene();

	while (not exitRequested) {
		ncinput key;
		uint32_t inputCode;
		debugFrame = false;
		do {
			inputCode = notcurses_get_nblock(nc, &key);
			if (inputCode == std::numeric_limits<uint32_t>::max() - 1)
				throw std::runtime_error(std::format("notcurses get returned {}", inputCode));

			Transform transform{};
			switch (key.id) {
			case NCKEY_RESIZE:
				throw std::runtime_error("resize isn't implemented yet"); // TODO: implement resize
			case NCKEY_SIGNAL: break; // TODO: pause here
			case 'w':
				transform = {
				    {0, 0, 1},
                    glm::yawPitchRoll<double>(0, 0, 0), 1
                };
				break;
			case 's':
				transform = {
				    {0, 0, -1},
                    glm::yawPitchRoll<double>(0, 0, 0), 1
                };
				break;
			case 'q':
				transform = {
				    {-1, 0, 0},
                    glm::yawPitchRoll<double>(0, 0, 0), 1
                };
				break;
			case 'e':
				transform = {
				    {1, 0, 0},
                    glm::yawPitchRoll<double>(0, 0, 0), 1
                };
				break;
			case 'r':
				transform = {
				    {0, 1, 0},
                    glm::yawPitchRoll<double>(0, 0, 0), 1
                };
				break;
			case 'f':
				transform = {
				    {0, -1, 0},
                    glm::yawPitchRoll<double>(0, 0, 0), 1
                };
				break;
			case 'a':
				transform = {
				    {0, 0, 0},
                    glm::yawPitchRoll<double>(0.1, 0, 0), 1
                };
				break;
			case 'd':
				transform = {
				    {0, 0, 0},
                    glm::yawPitchRoll<double>(-0.1, 0, 0), 1
                };
				break;
			case 'x': debugFrame = true; break;
			}
			// FIXME: moving feels off
			scene.camera.setTransform(scene.camera.getTransform() + transform);
			if (debugFrame)
				std::println(std::cerr, "moved:{}, camera:{}", transform,
				             scene.camera.getTransform());
		} while (inputCode != 0 && key.id != NCKEY_EOF /* TODO: what is this EOF thing */);

		static bool frameIndicator = true;
		frameIndicator = not frameIndicator;

		squareDrawing.clear();
		finalDrawing.clear();
		renderScene(squareDrawing, scene);
		finalDrawing.insert({0, 0}, squareDrawing);

		if (frameIndicator)
			finalDrawing.set(
			    SextantCoord{
			        0, 0
            },
			    Color{Category{false, 1}, RGBA{255, 255, 255, 255}});
		else
			finalDrawing.set(
			    SextantCoord{
			        0, 0
            },
			    Color{Category{false, 1}, RGBA{0, 0, 0, 255}});

		finalDrawing.render();
		notcurses_render(nc);

		// std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}
