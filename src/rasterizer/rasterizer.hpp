#ifndef RASTERIZER_HPP
#define RASTERIZER_HPP
#include "../drawing/sextantBlocks.hpp"
#include "renderable.hpp"
#include <glm/exponential.hpp>

using glm::dvec3, glm::dvec4, glm::ivec2, glm::dmat4;

struct Camera {
	double viewportWidth;
	double viewportHeight;
	double viewportDistance;
	dmat4 invTransform;

	glm::dmat3x4 viewportTransform(const ivec2 canvasSize) const {
		glm::dmat3x4 matrix{1};
		matrix[0][0] = viewportDistance * canvasSize.x / viewportWidth;
		matrix[1][1] = viewportDistance * canvasSize.y / viewportHeight;
		return matrix;
	}

	std::vector<Plane> getClippingPlanes() const { // TODO: allow changing FOV
		return {
		    {{glm::inversesqrt(2.0), 0, glm::inversesqrt(2.0)},  0},
		    {{-glm::inversesqrt(2.0), 0, glm::inversesqrt(2.0)}, 0},
		    {{0, glm::inversesqrt(2.0), glm::inversesqrt(2.0)},  0},
		    {{0, -glm::inversesqrt(2.0), glm::inversesqrt(2.0)}, 0},
		};
	}
};

struct Scene {
	std::vector<std::shared_ptr<Object3D>>
	    objects; // TODO: do I need this if it's all pointed to by instances?
	std::vector<InstanceRef3D> instances;
	Camera camera;
	Color bgColor;
};

[[nodiscard]] Scene initScene();
void renderScene(SextantDrawing& canvas, const Scene& scene);

#endif /* RASTERIZER_HPP */
