#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include <format>
#include <memory>
#include <unordered_set>
#include <vector>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include "../drawing/setColor.hpp"
#include "glm/geometric.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>

using glm::dvec3, glm::dmat4, glm::dvec4;

template<typename T> struct Triangle {
	T p0; T p1; T p2;
};

struct Sphere {
	dvec3 center; double radius;
};

inline Sphere createBoundingSphere(const std::vector<dvec3>& points) {
	Sphere output;

	dvec3 pointsSum;
	for (const dvec3& point: points) {
		pointsSum += point;
	}
	output.center = pointsSum / static_cast<double>(points.size());

	double maxDist = 0;
	for (const dvec3& point: points) {
		if (maxDist < glm::distance(output.center, point)) {
			maxDist = glm::distance(output.center, point);
		}
	}
	output.radius = maxDist;

	return output;
}

struct Plane {
	dvec3 normal;
	double distance; // distance from origin
};

inline double signedDistance(const Plane& plane, const dvec3& vertex) {
	return vertex.x * plane.normal.x
		+ vertex.y * plane.normal.y
		+ vertex.z * plane.normal.z
		+ plane.distance;
}

inline dvec3 intersectPlaneSeg(const std::pair<dvec3, dvec3>& segment, const Plane& plane) {
	double t = (-plane.distance - glm::dot(plane.normal, segment.first))
		/ glm::dot(plane.normal, segment.second - segment.first);
	return segment.first + t * (segment.second - segment.first);
}

struct Transform {
	dvec3 translation;
	glm::dmat3 rotation;
	dvec3 scale; // x, y, and z scale

	Transform(const dvec3 translation, const glm::dmat3 rotation, const dvec3 scale) :
		translation(translation), rotation(rotation), scale(scale) {}

	Transform(const dvec3 translation, const glm::dmat3 rotation, const double scale) :
		translation(translation), rotation(rotation), scale(scale, scale, scale) {}
	
	Transform() : Transform({0, 0, 0}, glm::dmat3(1 /* identity matrix */), 1.0) {}
};

template <> struct std::formatter<Transform> : std::formatter<string> {
	auto format(const Transform& transform, std::format_context& context) const {
		return formatter<string>::format(
			std::format("transform: (translation: {}, rotation: {}, scale: {})", 
				glm::to_string(transform.translation),
				glm::to_string(transform.rotation),
				glm::to_string(transform.scale)
				),
			context
		);
	}
};

inline dmat4 parseTransform(const Transform& transform) { // TODO: cache calls to this
	dmat4 scaleMatrix{1}; // identity matrix
	scaleMatrix[0][0] = transform.scale.x;
	scaleMatrix[1][1] = transform.scale.y;
	scaleMatrix[2][2] = transform.scale.z;
	
	dmat4 rotMatrix{1};
	for (int x = 0; x < 3; x++) {
		for (int y = 0; y < 3; y++) {
			rotMatrix[x][y] = transform.rotation[y][x];
			// flipping x and y makes it work
			// I have no idea why
		}
	}

	dmat4 translateMatrix{1};
	translateMatrix[3][0] = transform.translation.x;
	translateMatrix[3][1] = transform.translation.y;
	translateMatrix[3][2] = transform.translation.z;

	return translateMatrix * scaleMatrix * rotMatrix;
}

inline Transform invertTransform(const Transform& transform) {
	return {
		-transform.translation,
		glm::inverse(transform.rotation),
		{1.0/transform.scale.x, 1.0/transform.scale.y, 1.0/transform.scale.z}
	};
}

inline dvec3 canonicalize(const dvec4& homogenous) {
	return {
		homogenous.x / homogenous.w,
		homogenous.y / homogenous.w,
		homogenous.z / homogenous.w,
	};
}

inline glm::dvec2 canonicalize(const dvec3& homogenous) {
	return {
		homogenous.x / homogenous.z,
		homogenous.y / homogenous.z,
	};
}

struct ColoredTriangle {
	Triangle<uint> triangle; // refers to array indexes
	Color color;
};

struct Object3D {
	std::vector<dvec3> points;
	std::vector<ColoredTriangle> triangles;
};

struct Instance3D {
	std::shared_ptr<Object3D> object;
	Transform transformation;
	dmat4 objTransform() const {
		return parseTransform(this->transformation);
	};
};

// two functions for multiple planes and a single plane

inline std::vector<Triangle<dvec3>> clipTriangle(Triangle<dvec3> triangle, const Plane& plane) {
	std::vector<Triangle<dvec3>> output;
	output.reserve(2); // clipping a triangle can produce 1-2 new triangles

	Triangle<double> distances{};
	distances.p0 = signedDistance(plane, triangle.p0);
	distances.p1 = signedDistance(plane, triangle.p1);
	distances.p2 = signedDistance(plane, triangle.p2);
	uchar numPositive = (distances.p0 >= 0) + (distances.p1 >= 0) + (distances.p2 >= 0);

	if (numPositive == 3) { // fully inside plane
		output.push_back(triangle);

	} else if (numPositive == 1) { // 1 vertex inside
		if (distances.p0 >= 0); // make p0 be the only positive vertex
		else if (distances.p1 >= 0) std::swap(triangle.p0, triangle.p1);
		else if (distances.p2 >= 0) std::swap(triangle.p0, triangle.p2);
		
		output.push_back({
			triangle.p0,
			intersectPlaneSeg(std::make_pair(triangle.p0, triangle.p1), plane),
			intersectPlaneSeg(std::make_pair(triangle.p0, triangle.p2), plane)
		});

	} else if (numPositive == 2) { // 2 verticies inside
		if (distances.p0 < 0); // make p0 be the only negative vertex
		else if (distances.p1 < 0) std::swap(triangle.p0, triangle.p1);
		else if (distances.p2 < 0) std::swap(triangle.p0, triangle.p2);

		dvec3 p1Prime = intersectPlaneSeg(std::make_pair(triangle.p0, triangle.p1), plane);
		dvec3 p2Prime = intersectPlaneSeg(std::make_pair(triangle.p0, triangle.p2), plane);

		output.push_back({triangle.p1, p1Prime, triangle.p2});
		output.push_back({triangle.p2, p2Prime, p1Prime});

	} else if (numPositive == 0) { // all outside
		// nothing inside plane
	}

	return output;
}

// this function is a mess; TODO: fix
inline std::vector<Triangle<dvec3>> clipTriangles(const std::vector<Triangle<dvec3>>& triangles, const Plane& plane) {
	std::unordered_set<dvec3> points; // TODO: pass in
	for (const Triangle<dvec3>& triangle: triangles) {
		points.insert(triangle.p0);
		points.insert(triangle.p1);
		points.insert(triangle.p2);
	}
	std::vector<dvec3> pointsVec{points.begin(), points.end()};

	Sphere bounding = createBoundingSphere(pointsVec); // TODO: pass in
	double distance = signedDistance(plane, bounding.center);

	if (distance >= bounding.radius) { // fully inside
		return triangles;

	} else if (distance <= -bounding.radius) { // fully outside
		return {};

	} else { // split
		std::vector<Triangle<dvec3>> output{};
		for (const Triangle<dvec3>& triangle: triangles) {
			for (const Triangle<dvec3>& clipped: clipTriangle(triangle, plane)) {
				output.push_back(clipped);
			}
		}
		return output;
	}
}

inline std::vector<Triangle<dvec3>> clipTriangles(std::vector<Triangle<dvec3>>& triangles, const std::vector<Plane>& planes) {
	for (const Plane& plane: planes) {
		triangles = clipTriangles(triangles, plane);
	}
	return triangles;
}

inline std::vector<Triangle<dvec3>> clipTriangle(Triangle<dvec3> triangle, const std::vector<Plane>& planes) {
	std::vector<Triangle<dvec3>> triangles{triangle};
	for (const Plane& plane: planes) {
		triangles = clipTriangles(triangles, plane);
	}
	return triangles;
}

#endif /* RENDERABLE_HPP */
