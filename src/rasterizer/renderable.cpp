#include "renderable.hpp"
#include "interpolate.hpp"

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
	return vertex.x * plane.normal.x + vertex.y * plane.normal.y + vertex.z * plane.normal.z
	       + plane.distance;
}

void clipTriangle(InstanceSC3D& inst, const Plane& plane, const uint targetIdx) {
	ColoredTriangle target = inst.getTriangles()[targetIdx];
	Triangle<uint>& targetTri = target.triangle;
	// Triangle<dvec3>& normals = target.normals;
	if (targetTri == NO_TRIANGLE.triangle) return;
	Color color = inst.getTriangles()[targetIdx].color;

	Triangle<double> distances{};
	for (uint i = 0; i < 3; i++) {
		distances[i] = signedDistance(plane, inst.getPoints()[targetTri[i]]);
	}
	uchar numPositive = (distances[0] >= 0) + (distances[1] >= 0) + (distances[2] >= 0);

	if (numPositive == 3) { // fully inside plane
		return;

	} else if (numPositive == 1) { // 1 vertex inside
		if (distances[0] >= 0)
			; // make p0 be the only positive vertex
		else if (distances[1] >= 0) std::swap(targetTri[0], targetTri[1]);
		else if (distances[2] >= 0) std::swap(targetTri[0], targetTri[2]);

		dvec3 vertexA = intersectPlaneSeg(
		    std::make_pair(inst.getPoints()[targetTri[0]], inst.getPoints()[targetTri[1]]), plane);
		dvec3 vertexB = intersectPlaneSeg(
		    std::make_pair(inst.getPoints()[targetTri[0]], inst.getPoints()[targetTri[2]]), plane);

		inst.getTriangles()[targetIdx] = NO_TRIANGLE;

		// this will create duplicate points, but catching those would be too much work
		inst.addTriangle({
		    {targetTri[0], inst.addVertex(vertexA), inst.addVertex(vertexB)},
            color // TODO: normals
        });

	} else if (numPositive == 2) { // 2 verticies inside
		if (distances[0] < 0)
			; // make p0 be the only negative vertex
		else if (distances[1] < 0) std::swap(targetTri[0], targetTri[1]);
		else if (distances[2] < 0) std::swap(targetTri[0], targetTri[2]);

		dvec3 p1Prime = intersectPlaneSeg(
		    std::make_pair(inst.getPoints()[targetTri[0]], inst.getPoints()[targetTri[1]]), plane);
		dvec3 p2Prime = intersectPlaneSeg(
		    std::make_pair(inst.getPoints()[targetTri[0]], inst.getPoints()[targetTri[2]]), plane);

		uint p1Idx = inst.addVertex(p1Prime);
		uint p2Idx = inst.addVertex(p2Prime);

		inst.getTriangles()[targetIdx] = NO_TRIANGLE;

		inst.addTriangle({
		    {targetTri[1], p1Idx, targetTri[2]},
            color // TODO: normals
        });
		inst.addTriangle({
		    {targetTri[2], p2Idx, p1Idx},
            color // TODO: normals
        });

	} else if (numPositive == 0) { // all outside
		inst.getTriangles()[targetIdx] = NO_TRIANGLE;
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
			}
		}
	}

	inst->clearEmptyTris();
}

std::unique_ptr<InstanceSC3D> backFaceCulling(std::unique_ptr<InstanceSC3D> inst) {
	// this algorithm requires vertices to be clockwise
	for (auto& tri : inst->getTriangles()) {
		// camera is at {0, 0, 0}, so this is the vector from the camera to triangle
		dvec3 cameraVector = -inst->getPoints()[tri.triangle[0]];

		dvec3 triVecA = inst->getPoints()[tri.triangle[1]] - inst->getPoints()[tri.triangle[0]];
		dvec3 triVecB = inst->getPoints()[tri.triangle[2]] - inst->getPoints()[tri.triangle[0]];
		dvec3 triNormal = glm::cross(triVecA, triVecB); // normal vector of triangle

		// the == case is for directly side on triangles, so don't render them
		if (glm::dot(triNormal, cameraVector) <= 0) tri = NO_TRIANGLE;
	}

	inst->clearEmptyTris();
	return inst;
}
