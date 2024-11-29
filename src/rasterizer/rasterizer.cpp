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

// TODO: a lot of this stuff should be passed by reference

using glm::dvec3, glm::dvec4, glm::ivec2, glm::dmat4;

#define cwhite Color(Category(true, 8), RGBA(255, 255, 255, 255))
#define cred Color(Category(true, 8), RGBA(255, 0, 0, 255))
#define cgreen Color(Category(true, 8), RGBA(0, 255, 0, 255))
#define cblue Color(Category(true, 8), RGBA(0, 0, 255, 255))
#define cyellow Color(Category(true, 8), RGBA(255, 255, 0, 255))
#define cmagenta Color(Category(true, 8), RGBA(255, 0, 255, 255))
#define ccyan Color(Category(true, 8), RGBA(0, 255, 255, 255))
#define cblack Color(Category(true, 8), RGBA(0, 0, 0, 255))

struct Camera {
	double viewportWidth;
	double viewportHeight;
	double viewportDistance;
	Transform transformation; // TODO: cache inverse somehow
	dmat4 camTransform() const {
		return parseTransform(invertTransform(this->transformation));
	}
	glm::dmat3x4 viewportTransform(const ivec2 canvasSize) const {
		glm::dmat3x4 matrix{1};
		matrix[0][0] = viewportDistance*canvasSize.x / viewportWidth;
		matrix[1][1] = viewportDistance*canvasSize.y / viewportHeight;
		return matrix;
	}
};

struct Scene {
	std::vector<Object3D> objects; // TODO: do I need this if it's all pointed to by instances?
	std::vector<Instance3D> instances;
	Camera camera;
	Color bgColor;
};

// converts a 3d point on the viewplane to a 2d point
ivec2 viewportToCanvas(const SextantDrawing& canvas, const Camera camera, const dvec3 p) {
	return ivec2(p.x * static_cast<double>(canvas.getWidth() / camera.viewportWidth),
		p.y * static_cast<double>(canvas.getHeight() / camera.viewportHeight));
}

// converts a 3d point to a 2d canvas point
ivec2 projectVertex(const SextantDrawing& canvas, const Camera camera, const dvec3 v) {
	if (v.z == 0) throw std::logic_error("v.z == 0; cannot divide by 0");
	return viewportToCanvas(canvas, camera, 
		{v.x * camera.viewportDistance / v.z, v.y * camera.viewportDistance / v.z, camera.viewportDistance});
}

void renderInstance(SextantDrawing& canvas, const Camera camera, const Instance3D& objectInst) {
	std::vector<ivec2> projected{};
	projected.reserve(objectInst.object->triangles.size());
	for (const dvec3 vertex: objectInst.object->points) {
		dvec3 s1 = transform(vertex, objectInst.transformation);
		dvec3 s2 = transform(s1, invertTransform(camera.transformation));
		ivec2 s3 = projectVertex(canvas, camera, s2);

		// std::println(std::cerr, "v:{}", glm::to_string(vertex));
		// std::println(std::cerr, "s1:{}\ns2:{}\ns3:{}", glm::to_string(s1), glm::to_string(s2), glm::to_string(s3));

		dvec4 homogenous = {vertex.x, vertex.y, vertex.z, 1};
		homogenous = objectInst.objTransform() * homogenous;
		// std::println(std::cerr, "h1:{}", glm::to_string(homogenous));
		// if (canonicalize(homogenous) != s1) raise(SIGINT);

		// std::println(std::cerr, "ctrmat:{}", glm::to_string(camera.camTransform()));
		// std::println(std::cerr, "invtr:{}", invertTransform(camera.transformation));

		homogenous = camera.camTransform() * homogenous;
		// std::println(std::cerr, "h2:{}", glm::to_string(homogenous));
		// if (canonicalize(homogenous) != s2) raise(SIGINT);

		dvec3 homogenous2d =
			camera.viewportTransform({canvas.getWidth(),
			canvas.getHeight()}) * homogenous;

		assertGt(abs(homogenous.w), 0.1, "Cannot draw things ON the camera"); // TODO: handle properly
		glm::dvec2 canvasPoint = canonicalize(homogenous2d);
		// if (ivec2(canvasPoint) != s3) raise(SIGINT);
		projected.push_back(canvasPoint);
	}
	
	for (const ColoredTriangle triangle: objectInst.object->triangles) {
		renderTriangle(canvas, {
				projected.at(triangle.triangle.p0),
				projected.at(triangle.triangle.p1),
				projected.at(triangle.triangle.p2)
			}, triangle.color
		);
	}
}

void renderScene(SextantDrawing& canvas, const Scene& scene) {
	for (const Instance3D& objectInst: scene.instances) {
		renderInstance(canvas, scene.camera, objectInst);
	}
}

void rasterRenderLoop(WindowedDrawing& rawCanvas, const bool& exit_requested, std::function<int()> refresh) {
	int minDimension = std::min(rawCanvas.getHeight(), rawCanvas.getWidth()/2);
	SextantDrawing canvas(minDimension, minDimension*2);
	
	Camera camera(1, 1, 1, Transform(
		{-3, 1, 0},
		glm::yawPitchRoll<double>(glm::radians(-30.0), 0, 0),
		1.0
	));

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
	scene.objects.push_back(cube);

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

	while (not exit_requested) {
		renderScene(canvas, scene);
		// drawFilledTriangle(canvas, {-30, -30}, {-30, 30}, {30, 30}, Color(Category(false, 4), RGBA(255, 0, 0, 255)));

		rawCanvas.clear();
		rawCanvas.insert(SextantCoord(0, 0), canvas);
		rawCanvas.render();
		refresh();
		sleep(1);
	}
}

