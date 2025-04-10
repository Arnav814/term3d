#ifndef STRUCTURES_HPP
#define STRUCTURES_HPP
#include "glm/ext/matrix_transform.hpp"
#include "glm/gtx/dual_quaternion.hpp"
#include "glm/gtx/string_cast.hpp"
#include "../drawing/setColor.hpp"

#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_double4.hpp>
#include <glm/ext/vector_int2.hpp>

#include <vector>

#define cwhite Color(Category(true, 8), RGBA(255, 255, 255, 255))
#define cred Color(Category(true, 8), RGBA(255, 0, 0, 255))
#define cgreen Color(Category(true, 8), RGBA(0, 255, 0, 255))
#define cblue Color(Category(true, 8), RGBA(0, 0, 255, 255))
#define cyellow Color(Category(true, 8), RGBA(255, 255, 0, 255))
#define cmagenta Color(Category(true, 8), RGBA(255, 0, 255, 255))
#define ccyan Color(Category(true, 8), RGBA(0, 255, 255, 255))
#define cblack Color(Category(true, 8), RGBA(0, 0, 0, 255))

using glm::dvec3, glm::dvec4, glm::ivec2, glm::dmat4;

#define origin \
	dvec3 { \
		0, 0, 0 \
	}

extern bool debugFrame;

template <typename T> using Triangle = std::array<T, 3>;

// applies lambda to each element of tri and discards the return value
template <typename T, typename LambdaType>
inline void forAll(const Triangle<T> tri, const LambdaType lambda) {
	for (const T& elem : tri) {
		lambda(elem);
	}
}

// applies lambda to each element of tri and sets the element to the return value
template <typename T, typename LambdaType>
inline void setAll(Triangle<T> tri, const LambdaType lambda) {
	for (T& elem : tri) {
		elem = lambda(elem);
	}
}

// applies lambda to all pairs of 2 elements from tri and discards the return value
template <typename T, typename LambdaType>
inline void forAllPairs(const Triangle<T> tri, const LambdaType lambda) {
	lambda(std::make_pair(tri[0], tri[1]));
	lambda(std::make_pair(tri[0], tri[2]));
	lambda(std::make_pair(tri[1], tri[2]));
}

// applies lambda to all pairs of 2 elements from tri and returns the return value
template <typename T, typename LambdaType>
inline Triangle<T> forAllPairsRet(const Triangle<T> tri, const LambdaType lambda) {
	return {lambda(std::make_pair(tri[0], tri[1])), //
	        lambda(std::make_pair(tri[0], tri[2])), //
	        lambda(std::make_pair(tri[1], tri[2]))};
}

#define NO_TRIANGLE \
	ColoredTriangle { \
		{std::numeric_limits<uint>::max(), std::numeric_limits<uint>::max(), \
		 std::numeric_limits<uint>::max()}, \
		    Color(), {} \
	}

// I would use NaN instead of infinity, but the equality nonsense is annoying
#define NO_POINT \
	dvec3 { \
		std::numeric_limits<double>::infinity(), std::numeric_limits<double>::infinity(), \
		    std::numeric_limits<double>::infinity() \
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

struct Transform {
	dvec3 translation;
	glm::dmat3 rotation;
	dvec3 scale; // x, y, and z scale

	Transform(const dvec3 translation, const glm::dmat3 rotation, const dvec3 scale)
	    : translation(translation), rotation(rotation), scale(scale) {}

	Transform(const dvec3 translation, const glm::dmat3 rotation, const double scale)
	    : translation(translation), rotation(rotation), scale(scale, scale, scale) {}

	Transform() : Transform({0, 0, 0}, glm::dmat3(1 /* identity matrix */), 1.0) {}
};

template <> struct std::formatter<Transform> : std::formatter<string> {
	auto format(const Transform& transform, std::format_context& context) const {
		return formatter<string>::format(
		    std::format("transform(translation:{}, rotation:{}, scale:{})",
		                glm::to_string(transform.translation), glm::to_string(transform.rotation),
		                glm::to_string(transform.scale)),
		    context);
	}
};

dmat4 parseTransform(const Transform& transform);

bool isAffine(const dmat4& mat);
Transform decompose(dmat4 mat);

struct MatAndTranslation {
	glm::dmat3 rotation; // also includes scaled
	dvec3 translation;
};

// Much cheaper that full decompose.
// Keeps the rotation and scale together.
inline MatAndTranslation partialDecompose(const dmat4& mat) {
	assertMsg(isAffine(mat), "Can't decompose a non-affine matrix.");

	dvec3 translation = {mat[3][0], mat[3][1], mat[3][2]};
	glm::dmat3 rotation = mat; // just crop outside the top left 3x3

	return {rotation, translation};
}

Transform invertTransform(const Transform& transform);

class Camera {
  private:
	dmat4 invTransform;
	dmat4 matTransform;

  public:
	double viewportWidth;
	double viewportHeight;
	double viewportDistance;

	Camera(double viewportWidth, double viewportHeight, double viewportDistance)
	    : invTransform(glm::identity<dmat4>()), matTransform(glm::identity<dmat4>()),
	      viewportWidth(viewportWidth), viewportHeight(viewportHeight),
	      viewportDistance(viewportDistance) {}

	glm::dmat3x4 viewportTransform(const ivec2 canvasSize) const {
		glm::dmat3x4 matrix{1};
		matrix[0][0] = viewportDistance * canvasSize.x / viewportWidth;
		matrix[1][1] = viewportDistance * canvasSize.y / viewportHeight;
		return matrix;
	}

	// Sets the camera's position.
	// Do not invert -- this will do that by itself.
	void setTransform(const Transform& transform) {
		this->invTransform = parseTransform(invertTransform(transform));
		this->matTransform = parseTransform(transform);
	}

	// Do not invert -- this will do that by itself.
	void translateBy(const Transform& transform) {
		this->invTransform = parseTransform(invertTransform(transform)) * this->invTransform;
		this->matTransform = glm::inverse(this->invTransform);
	}

	// quite slow, so don't call often
	const Transform getTransform() { return decompose(this->matTransform); }

	const dmat4& toCameraSpace() const { return this->invTransform; }

	const dmat4& fromCameraSpace() const { return this->matTransform; }

	std::vector<Plane> getClippingPlanes() const;
};

#endif /* STRUCTURES_HPP */
