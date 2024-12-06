#include "rasterizer.hpp"
#include <boost/range/join.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/gtx/string_cast.hpp>
#include <memory>
#include <vector>
#include <glm/gtx/euler_angles.hpp>
#include "glm/trigonometric.hpp"
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

	scene.instances.push_back(Instance3D(
		std::make_shared<Object3D>(cube),
		{{-1.5, 0, 7},
		// glm::yawPitchRoll<double>(0, 0, 0),
		glm::yawPitchRoll<double>(glm::radians(90.0), 0, 0),
		0.75}
	));

	scene.instances.push_back(Instance3D(
		std::make_shared<Object3D>(cube),
		{{1.25, 2.5, 7.5},
		glm::yawPitchRoll<double>(glm::radians(195.0), 0, 0),
		1.0}
	));

	return scene;
}

// TODO: I can't even with this mess of a function
static void renderInstance(SextantDrawing& canvas, const Camera& camera, const Instance3D& objectInst) {
	std::vector<std::pair<Color, Triangle<dvec3>>> triangles;
	for (const ColoredTriangle triangle: objectInst.object->triangles) {
		for (const Triangle<dvec3>& clipped: clipTriangle({
				objectInst.object->points.at(triangle.triangle.p0),
				objectInst.object->points.at(triangle.triangle.p1),
				objectInst.object->points.at(triangle.triangle.p2),
				}, camera.getClippingPlanes()
			)) {
			triangles.push_back(std::make_pair(triangle.color, clipped));
		}
	}

	std::vector<std::pair<Color, Triangle<ivec2>>> triangles2d{};
	triangles2d.reserve(triangles.size());
	for (const auto& triangle: triangles) {
		Triangle<ivec2> projected{};
		uchar vertexIdx = 0;

		for (const dvec3 vertex: {triangle.second.p0, triangle.second.p1, triangle.second.p2}) {
			dvec4 homogenous = {vertex.x, vertex.y, vertex.z, 1};
			homogenous = camera.invTransform * objectInst.objTransform() * homogenous;
			dvec3 homogenous2d =
				camera.viewportTransform({canvas.getWidth(),
				canvas.getHeight()}) * homogenous;

			assertGt(abs(homogenous.w), 0.1, "Cannot draw things ON the camera"); // TODO: handle properly
			glm::dvec2 canvasPoint = canonicalize(homogenous2d);
			switch (vertexIdx) { // FIXME: I hate this
				case 0: projected.p0 = canvasPoint; break;
				case 1: projected.p1 = canvasPoint; break;
				case 2: projected.p2 = canvasPoint; break;
			}
			vertexIdx++;
		}

		triangles2d.push_back(std::make_pair(triangle.first, projected));
	}

	for (const auto& triangle: triangles2d) {
		renderTriangle(canvas, triangle.second, triangle.first);
	}
}

void renderScene(SextantDrawing& canvas, const Scene& scene) {
	for (const Instance3D& objectInst: scene.instances) {
		renderInstance(canvas, scene.camera, objectInst);
	}
}

