#include "rasterizer.hpp"

#include "renderable.hpp"
#include "scene.hpp"
#include "structures.hpp"
#include "triangles.hpp"
#include "../util/floatComparisons.hpp"

#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/quaternion_geometric.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>

#include <boost/multi_array.hpp>
#include <boost/range/join.hpp>

#include <__ostream/print.h>
#include <algorithm>
#include <cmath>
#include <memory>
#include <ranges>
#include <type_traits>
#include <vector>

std::vector<std::shared_ptr<Light>>
translateLights(const std::vector<std::shared_ptr<Light>>& lights, const dmat4 translation) {
	std::vector<std::shared_ptr<Light>> out;
	out.reserve(lights.size());

	for (const std::shared_ptr<const Light> light : lights) {
		auto asPoint = dynamic_pointer_cast<const PointLight>(light);
		if (asPoint != NULL) {
			dvec3 position = asPoint->getPosition();
			dvec4 homogenous = toHomogenous(position);
			dvec3 newPoint = canonicalize(translation * homogenous);

			out.push_back(
			    std::make_shared<PointLight>(PointLight{asPoint->getIntensity(), newPoint}));
			if (debugFrame)
				std::println(std::cerr, "Point light at {} translated to {}.", position, newPoint);
		} else {
			auto asDirectional = dynamic_pointer_cast<const DirectionalLight>(light);
			assertMsg(asDirectional != NULL, "WTF is this light?!");
			dvec3 newDir = partialDecompose(translation).rotation * asDirectional->getDirection();

			out.push_back(std::make_shared<DirectionalLight>(
			    DirectionalLight{asDirectional->getIntensity(), newDir}));
			if (debugFrame)
				std::println(std::cerr, "Directional light from {} rotated to {}",
				             asDirectional->getDirection(), newDir);
		}
	}

	return out;
}

static void renderInstance(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
                           const Camera& camera, const InstanceRef3D& objectInst,
                           const double ambientLight,
                           const std::vector<std::shared_ptr<Light>> lights) {
	std::unique_ptr<InstanceSC3D> copied = std::make_unique<InstanceSC3D>(InstanceSC3D{objectInst});

	dmat4 toCam = camera.toCameraSpace() * objectInst.fromObjectSpace();

	for (uint vertexIdx = 0; vertexIdx < copied->getPoints().size(); vertexIdx++) {
		dvec3 vertex = copied->getPoint(vertexIdx);
		dvec4 homogenous = {vertex.x, vertex.y, vertex.z, 1};
		homogenous = toCam * homogenous;
		copied->setPoint(vertexIdx, canonicalize(homogenous));
	}

	std::vector<Plane> clippingPlanes = camera.getClippingPlanes();
	clipInstance(copied, clippingPlanes);
	if (copied == NULL) return;

	// light translated to be in object coordinates, for lighting calculations
	// lights are inputted in world coordinates
	std::vector instLights = translateLights(lights, copied->toObjectSpace());

	copied = backFaceCulling(std::move(copied));

	std::vector<ivec2> projected;
	projected.reserve(copied->getPoints().size());

	for (const dvec3& vertex : copied->getPoints()) {
		// skip missing points
		if (vertex == NO_POINT) {
			projected.push_back({0, 0});
			continue;
		}

		dvec4 homogenous = {vertex.x, vertex.y, vertex.z, 1};
		dvec3 homogenous2d =
		    camera.viewportTransform({canvas.getWidth(), canvas.getHeight()}) * homogenous;

		glm::dvec2 canvasPoint;
		// if the vertex if bad, just ignore it because clipping should have removed all triangles
		// that use it
		if (std::abs(homogenous2d.z) > 0.001) {
			canvasPoint = canonicalize(homogenous2d);
		} else {
			canvasPoint = {0, 0};
		}
		projected.push_back(canvasPoint);
	}

	if (debugFrame) {
		std::println(std::cerr, "Rendering instance @ {}.", copied->getTransform());
		for (uint i = 0; i < projected.size(); i++) {
			std::println(std::cerr, "{} {} {}", glm::to_string(projected[i]),
			             glm::to_string(copied->getPoints()[i]),
			             glm::length(copied->getPoints()[i]));
		}
	}

	for (const ColoredTriangle& triangle : copied->getTriangles()) {
		if (debugFrame) { // print 3d points, renderTriangle prints 2d points
			std::println(std::cerr, "Drawing tri {};\nnormals: {}.",
			             copied->getDvecTri(triangle.triangle), triangle.normals);
			std::println(std::cerr,
			             "Object transform: {:.2f}\nInv obj: {:.2f}\n"
			             "Camera transform: {:.2f}\nJoined: {:.2f}",
			             copied->toObjectSpace(), copied->fromObjectSpace(),
			             camera.fromCameraSpace(),
			             copied->toObjectSpace() * camera.fromCameraSpace());
		}
		renderTriangle(
		    canvas, depthBuffer,
		    {projected[triangle.triangle[0]], projected[triangle.triangle[1]],
		     projected[triangle.triangle[2]]},
		    {
		        static_cast<float>(glm::length(copied->getPoints()[triangle.triangle[0]])),
		        static_cast<float>(glm::length(copied->getPoints()[triangle.triangle[1]])),
		        static_cast<float>(glm::length(copied->getPoints()[triangle.triangle[2]])),
		    },
		    triangle.normals, triangle.color, ambientLight, copied->getSpecular(), camera,
		    (copied->toObjectSpace() * camera.fromCameraSpace()), instLights);
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

	if (debugFrame) std::println(std::cerr, "camera at {}", scene.camera.toCameraSpace());

	std::vector<std::shared_ptr<Light>> translatedLights =
	    translateLights(scene.lights, scene.camera.toCameraSpace());

	if (debugFrame)
		for (std::shared_ptr<Light> lightPtr : translatedLights) {
			dvec3 lightDir = lightPtr->getDirection(origin);
			dvec4 homogenous = {lightDir.x, lightDir.y, lightDir.z, 1};
			dvec3 homogenous2d =
			    scene.camera.viewportTransform({canvas.getWidth(), canvas.getHeight()})
			    * homogenous;
			if (floatCmp(homogenous2d.z, 0.0)) continue;
			ivec2 point = canonicalize(homogenous2d);

			float dist = glm::length(lightDir);

			renderTriangle(canvas, depthBuffer,
			               {
			                   point + ivec2{2,  0 },
                                 point + ivec2{-1, -1},
                                 point + ivec2{-1, 1 }
            },
			               {dist, dist, dist}, {dvec3{-1, 0, 0}, {-1, 0, 0}, {-1, 0, 0}}, cblack,
			               0.5, -1, scene.camera, glm::identity<dmat4>(), {});
		}

	for (const InstanceRef3D& objectInst : scene.instances) {
		renderInstance(canvas, depthBuffer, scene.camera, objectInst, scene.ambientLight,
		               scene.lights);
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
