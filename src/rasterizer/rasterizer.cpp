#include "rasterizer.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"
#include "renderable.hpp"
#include "structures.hpp"
#include "triangles.hpp"
#include <boost/multi_array.hpp>
#include <boost/range/join.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
#include <ranges>
#include <vector>

#define cwhite Color(Category(true, 8), RGBA(255, 255, 255, 255))
#define cred Color(Category(true, 8), RGBA(255, 0, 0, 255))
#define cgreen Color(Category(true, 8), RGBA(0, 255, 0, 255))
#define cblue Color(Category(true, 8), RGBA(0, 0, 255, 255))
#define cyellow Color(Category(true, 8), RGBA(255, 255, 0, 255))
#define cmagenta Color(Category(true, 8), RGBA(255, 0, 255, 255))
#define ccyan Color(Category(true, 8), RGBA(0, 255, 255, 255))
#define cblack Color(Category(true, 8), RGBA(0, 0, 0, 255))

#define origin {0, 0, 0}

// adds a point to the object, replacing a triangle with three new triangles
// creates 2 more triangles (-1 +3), but does NOT delete the original triangle,
// so the caller must then call clearEmptyTris
void splitTriangle(Object3D& object, uint triangleIdx, dvec3 newPoint) {
	uint newIdx = object.addVertex(newPoint);
	ColoredTriangle triangle = object.getTriangles()[triangleIdx];
	object.getTriangles()[triangleIdx] = NO_TRIANGLE;

	dvec3 newVec =
	    glm::normalize((triangle.normals[0] + triangle.normals[1] + triangle.normals[2]) / 3.);

	object.addTriangle({
	    {triangle.triangle[0], triangle.triangle[1], newIdx},
        triangle.color, {newVec}
    });
	object.addTriangle({
	    {triangle.triangle[1], triangle.triangle[2], newIdx},
        triangle.color, {newVec}
    });
	object.addTriangle({
	    {triangle.triangle[2], triangle.triangle[0], newIdx},
        triangle.color, {newVec}
    });

	// object.addTriangle({
	//     {triangle.triangle[0], triangle.triangle[1], newIdx},
	// cred, {newVec}
	// });
	// object.addTriangle({
	//     {triangle.triangle[1], triangle.triangle[2], newIdx},
	// cgreen, {newVec}
	// });
	// object.addTriangle({
	//     {triangle.triangle[2], triangle.triangle[0], newIdx},
	// cblue, {newVec}
	// });
}

// approximate a sphere with an inscribed tetrahedron
// more iterations == more triangles == more accurate
Object3D makeSphere(Color color, double specular, double radius, uint iterations) {
	// points for a unit sphere
	// these are also the normals,
	// and can be scaled to form the points
	std::array<dvec3, 4> points{
	    // FIXME: these are wrong
	    dvec3{0,             1,             0          },
        dvec3{0,             -1 / 2,        sqrt(3) / 2},
        dvec3{-sqrt(3) / 2., -1. / sqrt(3), -0.5       },
	    dvec3{sqrt(3) / 2.,  -1. / sqrt(3), -0.5       }
    };

	for (auto& point : points) {
		assertLt(abs(glm::length(point) - 1), 0.01, "Vector is wrong length.");
	}

	Object3D sphere{
	    // initially a triangular pyramid
	    {
         points[0] * radius,
         points[1] * radius,
         points[2] * radius,
         points[3] * radius,
	     },
	    {// all possible combinations
	     {{2, 1, 0}, color, {points[0], points[1], points[2]}},
	     {{3, 2, 0}, color, {points[0], points[2], points[3]}},
	     {{1, 3, 0}, color, {points[0], points[3], points[1]}},
	     {{1, 2, 3}, color, {points[1], points[2], points[3]}}},
	    specular
    };

	for (uint iterationNum = 0; iterationNum < iterations; iterationNum++) {
		// triangle count changes while iterating, so we have to store it here
		uint triangleCount = sphere.getTriangles().size();
		for (uint triangleIdx = 0; triangleIdx < triangleCount; triangleIdx++) {
			Triangle<dvec3> trianglePoints{
			    sphere.getPoints()[sphere.getTriangles()[triangleIdx].triangle[0]],
			    sphere.getPoints()[sphere.getTriangles()[triangleIdx].triangle[1]],
			    sphere.getPoints()[sphere.getTriangles()[triangleIdx].triangle[2]]};

			dvec3 triangleCenter{
			    // FIXME: I think this is wrong
			    (trianglePoints[0].x + trianglePoints[1].x + trianglePoints[2].x) / 3.0,
			    (trianglePoints[0].y + trianglePoints[1].y + trianglePoints[2].y) / 3.0,
			    (trianglePoints[0].z + trianglePoints[1].z + trianglePoints[2].z) / 3.0};

			assert((triangleCenter != dvec3{0, 0, 0}));

			dvec3 newPoint = triangleCenter * (radius / glm::length(triangleCenter));

			splitTriangle(sphere, triangleIdx, newPoint);
			// new triangles are placed at the end, so we don't have to handle those specially
		}
		sphere.clearEmptyTris();

#ifndef NDEBUG
		for (ColoredTriangle tri : sphere.getTriangles()) {
			Triangle<dvec3> trianglePoints{sphere.getPoints()[tri.triangle[0]],
			                               sphere.getPoints()[tri.triangle[1]],
			                               sphere.getPoints()[tri.triangle[2]]};
			Triangle<double> lengths{glm::distance(trianglePoints[0], trianglePoints[1]),
			                         glm::distance(trianglePoints[1], trianglePoints[2]),
			                         glm::distance(trianglePoints[2], trianglePoints[0])};
			assertBetweenIncl(-0.1, lengths[0] - lengths[1], 0.1,
			                  "All triangles should be equilateral in a sphere");
			assertBetweenIncl(-0.1, lengths[1] - lengths[2], 0.1,
			                  "All triangles should be equilateral in a sphere");
			assertBetweenIncl(-0.1, lengths[2] - lengths[0], 0.1,
			                  "All triangles should be equilateral in a sphere");
		}
#endif
	}

	for (auto i : sphere.getPoints()) {
		assertBetweenIncl(radius - 0.1, glm::length(i), radius + 0.1,
		                  "Sphere point is wrong length.");
	}
	// for (auto i : sphere.getPoints()) {
	// 	std::println(std::cerr, "point {}", glm::to_string(i));
	// }
	// for (auto i : sphere.getTriangles()) {
	// 	std::println(std::cerr, "tri ({}, {}, {})", i.triangle[0], i.triangle[1], i.triangle[2]);
	// }

	return sphere;
};

// join duplicated points, using tolerance as the threshold for points to join
void combinePoints(Object3D& object, const double tolerance = 0.001);

// Add a square based pyramid to the supplied object.
// baseCenter is the center of the pyramid's base
// peakPoint is the location of the pointy end
// baseSide indicates the side length of the base and also how the square is rotated
//
//    * <-- peakPoint
//   / \
//  /   \
// +--*--* <-- baseSide
//    ^-- baseCenter
//
void makePyramid(Object3D& object, const Color& color, const dvec3& baseCenter,
                 const dvec3& peakPoint, const dvec3& baseSide) {
	uint peakPointIdx = object.addVertex(peakPoint);

	dvec3 baseSideOffset = baseSide - baseCenter;
	dvec3 axis = peakPoint - baseCenter;
	// see https://en.wikipedia.org/wiki/Rotation_matrix#Rotation_matrix_from_axis_and_angle
	glm::dmat3 rot90{
	    // clang-format off
		pow(axis.x, 2),           axis.x * axis.y - pow(axis.z, 2), axis.x * axis.z + axis.y,
		axis.x * axis.y + axis.z, pow(axis.y, 2),                   axis.y * axis.z - axis.x,
		axis.x * axis.z - axis.y, axis.y * axis.z + axis.x,         pow(axis.z, 2)
	    // clang-format on
	};
	dvec3 offsetRot90 = baseSideOffset * rot90;

	// all four corners of the base
	std::array cornerIdxs{
	    object.addVertex(baseCenter + baseSideOffset - offsetRot90),
	    object.addVertex(baseCenter + baseSideOffset + offsetRot90),
	    object.addVertex(baseCenter - baseSideOffset + offsetRot90),
	    object.addVertex(baseCenter - baseSideOffset - offsetRot90),
	};

	std::println(std::cerr, "{}",
	             cornerIdxs | std::ranges::views::transform([object](const uint idx) {
		             return object.getPoints()[idx];
	             }));

	// side triangles
	for (uint i = 0; i < cornerIdxs.size(); i++) {
		// TODO: make sure all this is clockwise
		uint i2 = i < cornerIdxs.size() ? i : 0; // the second vertex's index
		dvec3 normal = glm::normalize(glm::cross(object.getPoints()[cornerIdxs[i]] - peakPoint,
		                                         object.getPoints()[cornerIdxs[i2]] - peakPoint));

		object.addTriangle(ColoredTriangle{
		    {peakPointIdx, cornerIdxs[i], cornerIdxs[i2]},
            color, {normal,       normal,        normal        }
        });
	}

	// base triangles
	dvec3 baseNormal = glm::normalize(-axis);
	object.addTriangle(ColoredTriangle{
	    {cornerIdxs[0], cornerIdxs[1], cornerIdxs[2]},
        color, {baseNormal}
    });
	object.addTriangle(ColoredTriangle{
	    {cornerIdxs[2], cornerIdxs[3], cornerIdxs[0]},
        color, {baseNormal}
    });
}

[[nodiscard]] Scene initScene() {
	Camera camera{1, 1, 1};
	camera.setTransform(
	    Transform({-3, 1, 0}, glm::yawPitchRoll<double>(glm::radians(-30.0), 0, 0), 1.0));
	// camera.setTransform(
	//     Transform({9, 0, 0}, glm::identity<glm::dmat3>(), 1.0));

	double ambientLight = 0.1;
	Scene scene{{}, {}, {}, camera, Color(Category(true, 7), RGBA(0, 0, 0, 255)), ambientLight};

	Object3D cube(
	    {
	        {1,  1,  1 },
	        {-1, 1,  1 },
	        {-1, -1, 1 },
	        {1,  -1, 1 },
	        {1,  1,  -1},
	        {-1, 1,  -1},
	        {-1, -1, -1},
	        {1,  -1, -1}
    },
	    {
	        {{0, 1, 2}, cred, {dvec3{0, 0, 1}, dvec3{0, 0, 1}, dvec3{0, 0, 1}}},
	        {{0, 2, 3}, cred, {dvec3{0, 0, 1}, dvec3{0, 0, 1}, dvec3{0, 0, 1}}},
	        {{4, 0, 3}, cgreen, {dvec3{1, 0, 0}, dvec3{1, 0, 0}, dvec3{1, 0, 0}}},
	        {{4, 3, 7}, cgreen, {dvec3{1, 0, 0}, dvec3{1, 0, 0}, dvec3{1, 0, 0}}},
	        {{5, 4, 7}, cblue, {dvec3{0, 0, -1}, dvec3{0, 0, -1}, dvec3{0, 0, -1}}},
	        {{5, 7, 6}, cblue, {dvec3{0, 0, -1}, dvec3{0, 0, -1}, dvec3{0, 0, -1}}},
	        {{1, 5, 6}, cyellow, {dvec3{-1, 0, 0}, dvec3{-1, 0, 0}, dvec3{-1, 0, 0}}},
	        {{1, 6, 2}, cyellow, {dvec3{-1, 0, 0}, dvec3{-1, 0, 0}, dvec3{-1, 0, 0}}},
	        {{4, 5, 1}, cmagenta, {dvec3{0, 1, 0}, dvec3{0, 1, 0}, dvec3{0, 1, 0}}},
	        {{4, 1, 0}, cmagenta, {dvec3{0, 1, 0}, dvec3{0, 1, 0}, dvec3{0, 1, 0}}},
	        {{2, 6, 7}, ccyan, {dvec3{0, -1, 0}, dvec3{0, -1, 0}, dvec3{0, -1, 0}}},
	        {{2, 7, 3}, ccyan, {dvec3{0, -1, 0}, dvec3{0, -1, 0}, dvec3{0, -1, 0}}},
	    },
	    1);
	scene.objects.push_back(std::make_shared<Object3D>(cube));

	// Object3D sphere = makeSphere(ccyan, -1, 1.0, 3);
	// scene.objects.push_back(std::make_shared<Object3D>(sphere));

	scene.instances.push_back(InstanceRef3D(std::make_shared<Object3D>(cube),
	                                        {
	                                            {-1.5, 0, 7},
	                                            // glm::yawPitchRoll<double>(0, 0, 0),
	                                            glm::yawPitchRoll<double>(glm::radians(90.0), 0, 0),
	                                            0.75
    }));

	Object3D axes{{}, {}, -1}; // helpful visualization
	makePyramid(axes, cred, origin, {3, 0, 0}, {0, 0.5, 0}); // x axis
	makePyramid(axes, cgreen, origin, {0, 3, 0}, {0.5, 0, 0}); // y axis
	makePyramid(axes, cblue, origin, {0, 0, 3}, {0, 0, 0.5}); // z axis
	scene.objects.push_back(std::make_shared<Object3D>(axes));

	scene.instances.push_back(InstanceRef3D(std::make_shared<Object3D>(axes), {}));

	scene.instances.push_back(InstanceRef3D(
	    std::make_shared<Object3D>(cube),
	    {
	        {1.25, 2.5, 7.5},
            glm::yawPitchRoll<double>(glm::radians(195.0), 0, 0), 1.0
    }));

	// scene.instances.push_back(
	//     InstanceRef3D(std::make_shared<Object3D>(sphere),
	//                   {
	//                       {2, 0, 7},
	// glm::yawPitchRoll<double>(0, 0, 0), 1.0
	// }));

	scene.instances.push_back(InstanceRef3D(
	    std::make_shared<Object3D>(cube), {
	                                          {2, 1, 0},
                                              glm::yawPitchRoll<double>(0, 0, 0), 0.5
    }));

	// scene.lights.push_back(std::make_shared<DirectionalLight>(0.3, dvec3(1.0, 4.0, 4.0)));
	scene.lights.push_back(std::make_shared<PointLight>(0.6, dvec3(2.0, 1.0, 0.0)));
	// scene.lights.push_back(std::make_shared<PointLight>(2, dvec3(2.0, 1.0, 0.0)));
	// scene.lights.push_back(std::make_shared<DirectionalLight>(0.8, dvec3(1.0, 4.0, 4.0)));

	return scene;
}

std::vector<std::shared_ptr<Light>>
translateLights(const std::vector<std::shared_ptr<Light>>& lights, const dmat4 translation) {
	std::vector<std::shared_ptr<Light>> out;
	out.reserve(lights.size());

	for (const std::shared_ptr<const Light> light : lights) {
		auto asPoint = dynamic_pointer_cast<const PointLight>(light);
		if (asPoint != NULL) {
			dvec3 position = asPoint->getPosition();
			dvec4 homogenous = {position.x, position.y, position.z, 1};
			out.push_back(std::make_shared<PointLight>(
			    PointLight{asPoint->getIntensity(), canonicalize(translation * homogenous)}));
			if (debugFrame)
				std::println(std::cerr, "light at {} translated to {}", glm::to_string(position),
				             glm::to_string(canonicalize(translation * homogenous)));
		} else {
			// do nothing for directional lights
			// FIXME: this isn't right
		}
	}

	return out;
}

// light positions should already be relative to camera
static void renderInstance(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
                           const Camera& camera, const InstanceRef3D& objectInst,
                           const double ambientLight,
                           const std::vector<std::shared_ptr<Light>> lights) {
	std::unique_ptr<InstanceSC3D> copied = std::make_unique<InstanceSC3D>(InstanceSC3D{objectInst});

	for (dvec3& vertex : copied->getPoints()) {
		dvec4 homogenous = {vertex.x, vertex.y, vertex.z, 1};
		homogenous = (camera.getInvTransform() * objectInst.getObjTransform()) * homogenous;
		vertex = canonicalize(homogenous);
	}

	std::vector<Plane> clippingPlanes = camera.getClippingPlanes();
	clipInstance(copied, clippingPlanes);
	if (copied == NULL) return;

	// copied = backFaceCulling(std::move(copied)); // FIXME: re-enable

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

		assertGt(abs(homogenous2d.z), 0.1, "Clipping should have dealt with this?!");
		glm::dvec2 canvasPoint = canonicalize(homogenous2d);
		projected.push_back(canvasPoint);
	}

	if (debugFrame) {
		std::println(std::cerr, "Rendering instance.");
		for (uint i = 0; i < projected.size(); i++) {
			std::println(std::cerr, "{} {} {}", glm::to_string(projected[i]),
			             glm::to_string(copied->getPoints()[i]),
			             glm::length(copied->getPoints()[i]));
		}
	}

	for (const ColoredTriangle& triangle : copied->getTriangles()) {
		if (debugFrame) // print 3d points, renderTriangle prints 2d points
			std::println(std::cerr, "Drawing tri {}.", copied->getDvecTri(triangle.triangle));
		renderTriangle(
		    canvas, depthBuffer,
		    {projected[triangle.triangle[0]], projected[triangle.triangle[1]],
		     projected[triangle.triangle[2]]},
		    {
		        static_cast<float>(glm::length(copied->getPoints()[triangle.triangle[0]])),
		        static_cast<float>(glm::length(copied->getPoints()[triangle.triangle[1]])),
		        static_cast<float>(glm::length(copied->getPoints()[triangle.triangle[2]])),
		    },
		    triangle.normals, triangle.color, ambientLight, copied->getSpecular(), camera, lights);
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

	if (debugFrame)
		std::println(std::cerr, "camera at {}", glm::to_string(scene.camera.getInvTransform()));

	std::vector<std::shared_ptr<Light>> translatedLights =
	    translateLights(scene.lights, scene.camera.getInvTransform());
	for (const InstanceRef3D& objectInst : scene.instances) {
		renderInstance(canvas, depthBuffer, scene.camera, objectInst, scene.ambientLight,
		               translatedLights);
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
