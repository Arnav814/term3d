#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP
#include "../drawing/setColor.hpp"
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <vector>

using glm::dvec3, glm::dvec4, glm::ivec2, glm::dmat4;

extern bool debugFrame;

template <typename T> using Triangle = std::array<T, 3>;

template <typename T>
struct std::formatter<Triangle<T>> : std::formatter<std::string> {
	using std::formatter<std::string>::parse;

	auto format(glm::dvec3 const& val, auto& ctx) const {
		auto out = ctx.out();
		out = std::format_to(out, "[");
		out = std::format_to(out, val[0]);
		out = std::format_to(out, val[1]);
		out = std::format_to(out, val[2]);
		out = std::format_to(out, "]");
		ctx.advance_to(out);
		return out;
	};
};

#define NO_TRIANGLE \
	ColoredTriangle { \
		{std::numeric_limits<uint>::max(), std::numeric_limits<uint>::max(), \
		 std::numeric_limits<uint>::max()}, \
		    Color(), {} \
	}

struct ColoredTriangle {
	Triangle<uint> triangle; // refers to array indexes
	Color color;
	Triangle<dvec3> normals;

	bool operator==(const ColoredTriangle& other) const = default;
	bool operator!=(const ColoredTriangle& other) const = default;
};

struct Sphere {
	dvec3 center;
	double radius;
};

struct Plane {
	dvec3 normal;
	double distance; // distance from origin
};

struct Camera {
	double viewportWidth;
	double viewportHeight;
	double viewportDistance;
	dmat4 invTransform;

	glm::dmat3x4 viewportTransform(const ivec2 canvasSize) const {
		glm::dmat3x4 matrix{1};
		matrix[0][0] = viewportDistance * canvasSize.x / viewportWidth;
		matrix[1][1] = viewportDistance * canvasSize.y / viewportHeight;
		return matrix;
	}

	std::vector<Plane> getClippingPlanes() const { // TODO: allow changing FOV
		return {
		    {{glm::inversesqrt(2.0), 0, glm::inversesqrt(2.0)},  0},
		    {{-glm::inversesqrt(2.0), 0, glm::inversesqrt(2.0)}, 0},
		    {{0, glm::inversesqrt(2.0), glm::inversesqrt(2.0)},  0},
		    {{0, -glm::inversesqrt(2.0), glm::inversesqrt(2.0)}, 0},
		};
	}
};

#endif /* STRUCTURES_HPP */
