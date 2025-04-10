#include "shapeBuilders.hpp"

#include "renderable.hpp"
#include "structures.hpp"

// adds a point to the object, replacing a triangle with three new triangles
// creates 2 more triangles (-1 +3), but does NOT delete the original triangle,
// so the caller must then call clearEmptyTris
void splitTriangle(Object3D& object, uint triangleIdx, dvec3 newPoint) {
	uint newIdx = object.addVertex(newPoint);
	ColoredTriangle triangle = object.getTriangle(triangleIdx);
	object.setTriangle(triangleIdx, NO_TRIANGLE);

	dvec3 newVec =
	    glm::normalize((triangle.normals[0] + triangle.normals[1] + triangle.normals[2]) / 3.);

	object.addTriangle({
	    {triangle.triangle[0], triangle.triangle[1], newIdx},
	    triangle.color,
	    {newVec,               newVec,               newVec}
    });
	object.addTriangle({
	    {triangle.triangle[1], triangle.triangle[2], newIdx},
	    triangle.color,
	    {newVec,               newVec,               newVec}
    });
	object.addTriangle({
	    {triangle.triangle[2], triangle.triangle[0], newIdx},
	    triangle.color,
	    {newVec,               newVec,               newVec}
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
// does not work
Object3D makeSphere(Color color, double specular, double radius, uint iterations) {
	// points for a unit sphere
	// these are also the normals,
	// and can be scaled to form the points
	std::array<dvec3, 4> points{
	    dvec3{1,  1,  1 },
        dvec3{-1, -1, 1 },
        dvec3{-1, 1,  -1},
        dvec3{1,  -1, -1}
    };
	for (auto& point : points) {
		point = glm::normalize(point);
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
			    sphere.getPoint(sphere.getTriangle(triangleIdx).triangle[0]),
			    sphere.getPoint(sphere.getTriangle(triangleIdx).triangle[1]),
			    sphere.getPoint(sphere.getTriangle(triangleIdx).triangle[2])};

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
	}

	for (const dvec3& i : sphere.getPoints()) {
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

	// verify axis and baseSideOffset are perpendicular
	if (not(std::abs(glm::dot(axis, baseSideOffset)) <= 0.001)) {
		throw std::runtime_error(
		    "Base side, base center, and peak point must form a right triangle");
	}

	dvec3 offsetRot90 =
	    glm::normalize(glm::cross(baseSideOffset, axis)) * glm::length(baseSideOffset);

	// all four corners of the base
	std::array cornerIdxs{
	    object.addVertex(baseCenter + baseSideOffset - offsetRot90),
	    object.addVertex(baseCenter + baseSideOffset + offsetRot90),
	    object.addVertex(baseCenter - baseSideOffset + offsetRot90),
	    object.addVertex(baseCenter - baseSideOffset - offsetRot90),
	};

	// side triangles
	for (uint i = 0; i < cornerIdxs.size(); i++) {
		uint i2 = (i + 1) % cornerIdxs.size(); // the second vertex's index
		// TODO: check this works
		dvec3 normal = glm::normalize(glm::cross(object.getPoint(cornerIdxs[i2]) - peakPoint,
		                                         object.getPoint(cornerIdxs[i]) - peakPoint));

		object.addTriangle(ColoredTriangle{
		    {peakPointIdx, cornerIdxs[i2], cornerIdxs[i]},
            color, {normal,       normal,         normal       }
        });
	}

	// base triangles
	dvec3 baseNormal = glm::normalize(-axis);
	object.addTriangle(ColoredTriangle{
	    {cornerIdxs[0], cornerIdxs[1], cornerIdxs[2]},
	    color,
	    {baseNormal,    baseNormal,    baseNormal   }
    });
	object.addTriangle(ColoredTriangle{
	    {cornerIdxs[2], cornerIdxs[3], cornerIdxs[0]},
	    color,
	    {baseNormal,    baseNormal,    baseNormal   }
    });
}
