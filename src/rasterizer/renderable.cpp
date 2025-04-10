#include "renderable.hpp"

#include "glm/geometric.hpp"
#include "interpolate.hpp"

#include <ranges>

Sphere createBoundingSphere(const std::vector<dvec3>& points) {
	Sphere output;

	dvec3 pointsSum;
	for (const dvec3& point : points) {
		pointsSum += point;
	}
	output.center = pointsSum / static_cast<double>(points.size());

	double maxDist = 0;
	for (const dvec3& point : points) {
		if (maxDist < glm::distance(output.center, point)) {
			maxDist = glm::distance(output.center, point);
		}
	}
	output.radius = maxDist;

	return output;
}

double signedDistance(const Plane& plane, const dvec3& vertex) {
	return vertex.x * plane.normal.x + //
	       vertex.y * plane.normal.y + //
	       vertex.z * plane.normal.z //
	       + plane.distance;
}

// reorganize the triangle so index is the first v alue
// preserves order, so back face culling still works
template <typename T> Triangle<T> reorderTri(Triangle<T>& tri, const int index) {
	Triangle<T> out;

	int from = index; // the index to copy from
	int to = 0; // the index to copy to

	while (to != 3) {
		out[to] = tri[from];

		to++;
		from++;
		if (from > 2) from = 0; // wrap around
	}

	return out;
}

// finds the first vertex that makes selector true
template <typename T, typename lambdaType>
int whichOf(const Triangle<T>& tri, const lambdaType& selector) {
	for (uint i = 0; i < tri.size(); i++) {
		if (selector(tri[i])) return i;
	}
	return -1;
}

void clipTriangle(InstanceSC3D& inst, const Plane& plane, const uint targetIdx) {
	ColoredTriangle target = inst.getTriangle(targetIdx);
	Triangle<uint>& targetTri = target.triangle;
	Triangle<dvec3>& normals = target.normals;
	if (targetTri == NO_TRIANGLE.triangle) return;
	Color color = inst.getTriangle(targetIdx).color;

	Triangle<double> distances{};
	for (uint i = 0; i < 3; i++) {
		distances[i] = signedDistance(plane, inst.getPoint(targetTri[i]));
	}
	uchar numPositive = (distances[0] >= 0) + (distances[1] >= 0) + (distances[2] >= 0);

	if (numPositive == 3) { // fully inside plane
		return;

	} else if (numPositive == 1) { // 1 vertex inside
		// make targetTri[0] be the only positive vertex
		int positiveIdx = whichOf(distances, [](const double& num) { return num >= 0; });
		targetTri = reorderTri(targetTri, positiveIdx);
		normals = reorderTri(normals, positiveIdx);

		auto vertexA = intersectPlaneSegT(
		    std::make_pair(inst.getPoint(targetTri[0]), inst.getPoint(targetTri[1])), plane);
		auto vertexB = intersectPlaneSegT(
		    std::make_pair(inst.getPoint(targetTri[0]), inst.getPoint(targetTri[2])), plane);

		inst.setTriangle(targetIdx, NO_TRIANGLE);

		// this will create duplicate points, but catching those would be too much work
		inst.addTriangle({
		    {targetTri[0], inst.addVertex(vertexA.intersection),
		     inst.addVertex(vertexB.intersection)                     },
		    color,
		    {normals[0],
		     {interpolateValue(normals[0].x, normals[1].x, vertexA.t),
		      interpolateValue(normals[0].y, normals[1].y, vertexA.t),
		      interpolateValue(normals[0].z, normals[1].z, vertexA.t)},
		     {interpolateValue(normals[0].x, normals[2].x, vertexA.t),
		      interpolateValue(normals[0].y, normals[2].y, vertexA.t),
		      interpolateValue(normals[0].z, normals[2].z, vertexA.t)}}
        });

	} else if (numPositive == 2) { // 2 verticies inside
		// make targetTri[0] be the only negative vertex
		int negativeIdx = whichOf(distances, [](const double& num) { return num < 0; });
		targetTri = reorderTri(targetTri, negativeIdx);
		normals = reorderTri(normals, negativeIdx);

		auto p1Prime = intersectPlaneSegT(
		    std::make_pair(inst.getPoint(targetTri[0]), inst.getPoint(targetTri[1])), plane);
		auto p2Prime = intersectPlaneSegT(
		    std::make_pair(inst.getPoint(targetTri[0]), inst.getPoint(targetTri[2])), plane);

		uint p1Idx = inst.addVertex(p1Prime.intersection);
		uint p2Idx = inst.addVertex(p2Prime.intersection);

		inst.setTriangle(targetIdx, NO_TRIANGLE);

		dvec3 p1Normals =
		    glm::normalize(dvec3{interpolateValue(normals[0].x, normals[1].x, p1Prime.t),
		                         interpolateValue(normals[0].y, normals[1].y, p1Prime.t),
		                         interpolateValue(normals[0].z, normals[1].z, p1Prime.t)});

		dvec3 p2Normals =
		    glm::normalize(dvec3{interpolateValue(normals[0].x, normals[2].x, p1Prime.t),
		                         interpolateValue(normals[0].y, normals[2].y, p1Prime.t),
		                         interpolateValue(normals[0].z, normals[2].z, p1Prime.t)});

		inst.addTriangle({
		    {p1Idx,     targetTri[1], targetTri[2]},
            color, {p1Normals, normals[1],   normals[2]  }
        });
		inst.addTriangle({
		    {targetTri[2], p2Idx,     p1Idx    },
            color, {normals[2],   p2Normals, p1Normals}
        });

	} else if (numPositive == 0) { // all outside
		inst.setTriangle(targetIdx, NO_TRIANGLE);
	}
}

void clipInstance(std::unique_ptr<InstanceSC3D>& inst, const std::vector<Plane>& planes) {
	assert(inst != NULL); // a reference to a pointer... makes sense...

	for (const Plane& plane : planes) {
		Sphere bounding = inst->getBoundingSphere();
		double distance = signedDistance(plane, bounding.center);

		if (distance >= bounding.radius) { // fully inside
			pass();

		} else if (distance <= -bounding.radius) { // fully outside
			inst = NULL;
			return;

		} else { // split
			// clipping creates more triangles; we don't want to clip them again for no reason
			uint numTriangles = inst->getTriangles().size();

			for (uint i = 0; i < numTriangles; i++) {
				clipTriangle(*inst, plane, i);
				if (debugFrame)
					std::println(
					    std::cerr, "tris after clipping: {}",
					    inst->getTriangles()
					        | std::ranges::views::filter(
					            [](const ColoredTriangle& tri) { return not(tri == NO_TRIANGLE); })
					        | std::ranges::views::transform([&inst](const ColoredTriangle& tri) {
						          return inst->getDvecTri(tri.triangle);
					          }));
			}
		}
	}

	inst->clearEmptyTris();
	inst->clearUnusedPoints();
}

std::unique_ptr<InstanceSC3D> backFaceCulling(std::unique_ptr<InstanceSC3D> inst) {
	// this algorithm requires vertices to be clockwise
	for (uint triIdx = 0; triIdx < inst->getTriangles().size(); triIdx++) {
		ColoredTriangle tri = inst->getTriangle(triIdx);

		// camera is at {0, 0, 0}, so this is the vector from the camera to triangle
		dvec3 cameraVector = -inst->getPoint(tri.triangle[0]);

		dvec3 triVecA = inst->getPoint(tri.triangle[1]) - inst->getPoint(tri.triangle[0]);
		dvec3 triVecB = inst->getPoint(tri.triangle[2]) - inst->getPoint(tri.triangle[0]);
		dvec3 triNormal = glm::cross(triVecA, triVecB); // normal vector of triangle

		// the == case is for directly side on triangles, so don't render them
		if (glm::dot(triNormal, cameraVector) <= 0) inst->setTriangle(triIdx, NO_TRIANGLE);
	}

	inst->clearEmptyTris();
	return inst;
}

void InstanceSC3D::clearUnusedPoints() {
	std::vector<bool> usedMap(this->getPoints().size(), false);
	for (const ColoredTriangle& tri : this->getTriangles()) {
		for (uint i : tri.triangle) {
			if (tri != NO_TRIANGLE) usedMap[i] = true;
		}
	}

	for (uint i = 0; i < this->getPoints().size(); i++) {
		// actually deleting them would break all subsequent indexes
		if (usedMap[i] == false) this->getPoint(i) = NO_POINT;
	}
}
