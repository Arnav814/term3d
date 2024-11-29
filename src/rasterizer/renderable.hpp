#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include <format>
#include <memory>
#include <vector>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include "../drawing/setColor.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>

using glm::dvec3, glm::dmat4, glm::dvec4;

template<typename T> struct Triangle {
	T p0; T p1; T p2;
};

struct Transform {
	dvec3 translation;
	glm::dmat3 rotation;
	dvec3 scale; // x, y, and z scale

	Transform(const dvec3 translation, const glm::dmat3 rotation, const dvec3 scale) :
		translation(translation), rotation(rotation), scale(scale) {}

	Transform(const dvec3 translation, const glm::dmat3 rotation, const double scale) :
		translation(translation), rotation(rotation), scale(scale, scale, scale) {}
	
	Transform() : Transform({0, 0, 0}, glm::dmat3x3(1 /* identity matrix */), 1.0) {}
};

template <> struct std::formatter<Transform> : std::formatter<string> {
	auto format(const Transform& transform, std::format_context& context) const {
		return formatter<string>::format(
			std::format("translation: {}, rotation: {}, scale: {}", 
				glm::to_string(transform.translation),
				glm::to_string(transform.rotation),
				glm::to_string(transform.scale)
				),
			context
		);
	}
};

inline dvec3 transform(const dvec3& vertex, const Transform& transform) {
	return vertex * transform.rotation * transform.scale + transform.translation;
}

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

#endif /* RENDERABLE_HPP */
