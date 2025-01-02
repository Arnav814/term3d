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

static void renderInstance(SextantDrawing& canvas, const Camera& camera, const InstanceRef3D& objectInst) {
	std::shared_ptr<const Instance3D> clippedInst = clipInstance(std::make_shared<InstanceRef3D>(objectInst), camera.getClippingPlanes());
	if (clippedInst == NULL) return;

	std::vector<ivec2> projected;
	projected.reserve(clippedInst->getPoints().size());

	for (const dvec3& vertex: clippedInst->getPoints()) {

		dvec4 homogenous = {vertex.x, vertex.y, vertex.z, 1};
		homogenous = (camera.invTransform * objectInst.getObjTransform()) * homogenous;
		dvec3 homogenous2d =
			camera.viewportTransform({canvas.getWidth(),
			canvas.getHeight()}) * homogenous;

		assertGt(abs(homogenous.w), 0.1, "Clipping should have dealt with this?!");
		glm::dvec2 canvasPoint = canonicalize(homogenous2d);
		projected.push_back(canvasPoint);
	}

	for (const ColoredTriangle& triangle: clippedInst->getTriangles()) {
		renderTriangle(canvas, {
				projected[triangle.triangle[0]],
				projected[triangle.triangle[1]],
				projected[triangle.triangle[2]]
			}, triangle.color);
	}
}

void renderScene(SextantDrawing& canvas, const Scene& scene) {
	for (const InstanceRef3D& objectInst: scene.instances) {
		renderInstance(canvas, scene.camera, objectInst);
	}
}

