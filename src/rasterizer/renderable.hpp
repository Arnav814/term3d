#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include <memory>
#include <vector>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include "../drawing/setColor.hpp"
#include "glm/ext/matrix_transform.hpp"

using glm::dvec3;

template<typename T> struct Triangle {
	T p0; T p1; T p2;
};

struct Transformation {
	dvec3 translation;
	glm::dmat3 rotation;
	double scale;

	Transformation(const dvec3 translation, const glm::dmat3 rotation, const double scale) :
		translation(translation), rotation(rotation), scale(scale) {}
	
	Transformation() : Transformation({0, 0, 0}, glm::dmat3x3(1 /* identity matrix */), 1.0) {}
};

inline dvec3 transform(const dvec3& vertex, const Transformation& transform) {
	return vertex * transform.rotation * transform.scale + transform.translation;
}

inline dvec3 invTransform(const dvec3& vertex, const Transformation& transform) {
	return vertex * glm::inverse(transform.rotation) / transform.scale - transform.translation;
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
	Transformation transformation;
};

#endif /* RENDERABLE_HPP */
