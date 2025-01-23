#include "rasterizer.hpp"
#include <boost/multi_array.hpp>
#include <boost/range/join.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/gtx/string_cast.hpp>
#include <memory>
#include <vector>
#include <glm/gtx/euler_angles.hpp>
#include <glm/trigonometric.hpp>
#include "common.hpp"
#include "renderable.hpp"
#include "triangles.hpp"

#define cwhite Color(Category(true, 8), RGBA(255, 255, 255, 255))
#define cred Color(Category(true, 8), RGBA(255, 0, 0, 255))
#define cgreen Color(Category(true, 8), RGBA(0, 255, 0, 255))
#define cblue Color(Category(true, 8), RGBA(0, 0, 255, 255))
#define cyellow Color(Category(true, 8), RGBA(255, 255, 0, 255))
#define cmagenta Color(Category(true, 8), RGBA(255, 0, 255, 255))
#define ccyan Color(Category(true, 8), RGBA(0, 255, 255, 255))
#define cblack Color(Category(true, 8), RGBA(0, 0, 0, 255))

[[nodiscard]] Scene initScene() {
	Camera camera(1, 1, 1, parseTransform(invertTransform(Transform(
		{-3, 1, 0},
		glm::yawPitchRoll<double>(glm::radians(-30.0), 0, 0),
		1.0
	))));

	Scene scene{{}, {},
		camera,
		Color(Category(true, 7), RGBA(0, 0, 0, 255)),
	};

	Object3D cube({
		{ 1,  1,  1},
		{-1,  1,  1},
		{-1, -1,  1},
		{ 1, -1,  1},
		{ 1,  1, -1},
		{-1,  1, -1},
		{-1, -1, -1},
		{ 1, -1, -1}
		},
		{
			{{0, 1, 2}, cred},
			{{0, 2, 3}, cred},
			{{4, 0, 3}, cgreen},
			{{4, 3, 7}, cgreen},
			{{5, 4, 7}, cblue},
			{{5, 7, 6}, cblue},
			{{1, 5, 6}, cyellow},
			{{1, 6, 2}, cyellow},
			{{4, 5, 1}, cmagenta},
			{{4, 1, 0}, cmagenta},
			{{2, 6, 7}, ccyan},
			{{2, 7, 3}, ccyan},
		}
	);
	scene.objects.push_back(std::make_shared<Object3D>(cube));

	scene.instances.push_back(InstanceRef3D(
		std::make_shared<Object3D>(cube),
		{
			{-1.5, 0, 7},
			// glm::yawPitchRoll<double>(0, 0, 0),
			glm::yawPitchRoll<double>(glm::radians(90.0), 0, 0),
			0.75
		}
	));

	scene.instances.push_back(InstanceRef3D(
		std::make_shared<Object3D>(cube),
		{{1.25, 2.5, 7.5},
		glm::yawPitchRoll<double>(glm::radians(195.0), 0, 0),
		1.0}
	));

	return scene;
}

static void renderInstance(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
		const Camera& camera, const InstanceRef3D& objectInst) {
	std::unique_ptr<InstanceSC3D> copied = std::make_unique<InstanceSC3D>(InstanceSC3D{objectInst});

	for (dvec3& vertex: copied->getPoints()) {
		dvec4 homogenous = {vertex.x, vertex.y, vertex.z, 1};
		homogenous = (camera.invTransform * objectInst.getObjTransform()) * homogenous;
		vertex = canonicalize(homogenous);
	}

	clipInstance(copied, camera.getClippingPlanes());
	if (copied == NULL) return;

	copied = backFaceCulling(std::move(copied));

	std::vector<ivec2> projected;
	projected.reserve(copied->getPoints().size());

	for (const dvec3& vertex: copied->getPoints()) {
		dvec4 homogenous = {vertex.x, vertex.y, vertex.z, 1};
		dvec3 homogenous2d =
			camera.viewportTransform({canvas.getWidth(),
			canvas.getHeight()}) * homogenous;

		assertGt(abs(homogenous.w), 0.1, "Clipping should have dealt with this?!");
		glm::dvec2 canvasPoint = canonicalize(homogenous2d);
		projected.push_back(canvasPoint);
	}

	if (debugFrame) {
		for (uint i = 0; i < projected.size(); i++) {
			std::println(std::cerr, "{} {} {}",
				glm::to_string(projected[i]),
				glm::to_string(copied->getPoints()[i]), glm::length(copied->getPoints()[i]));
		}
	}

	for (const ColoredTriangle& triangle: copied->getTriangles()) {
		renderTriangle(canvas, depthBuffer, {
				projected[triangle.triangle[0]],
				projected[triangle.triangle[1]],
				projected[triangle.triangle[2]]
			}, {
				static_cast<float>(glm::length(copied->getPoints()[triangle.triangle[0]])),
				static_cast<float>(glm::length(copied->getPoints()[triangle.triangle[1]])),
				static_cast<float>(glm::length(copied->getPoints()[triangle.triangle[2]])),
			}, triangle.color);
	}
}

void renderScene(SextantDrawing& canvas, const Scene& scene) {
	boost::multi_array<float, 2> depthBuffer; // TODO: don't reallocate every frame
	depthBuffer.resize(boost::extents[canvas.getHeight()][canvas.getWidth()]); // coords are (y, x)
	
	for (uint y = 0; y < depthBuffer.shape()[0]; y++) {
		for (uint x = 0; x < depthBuffer.shape()[1]; x++) {
			depthBuffer[y][x] = 0;
		}
	}

	for (const InstanceRef3D& objectInst: scene.instances) {
		renderInstance(canvas, depthBuffer, scene.camera, objectInst);
	}

	if (debugFrame) {
		for (uint y = 0; y < depthBuffer.shape()[0]; y++) {
			for (uint x = 0; x < depthBuffer.shape()[1]; x++) {
				std::print(std::cerr, "{:.2f} ", depthBuffer[y][x]);
			}
			std::println(std::cerr, "");
		}
		std::println(std::cerr, "---------------------------------------------------------");
	}
}

