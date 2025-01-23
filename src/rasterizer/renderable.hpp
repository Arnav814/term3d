#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include "../drawing/setColor.hpp"
#include "../extraAssertions.hpp"
#include <array>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>
#include <memory>

using glm::dvec3, glm::dmat4, glm::dvec4;

template <typename T> using Triangle = std::array<T, 3>;

#define NO_TRIANGLE \
	ColoredTriangle { \
		{std::numeric_limits<uint>::max(), std::numeric_limits<uint>::max(), \
		 std::numeric_limits<uint>::max()}, \
		        Color() \
	}

struct ColoredTriangle {
	Triangle<uint> triangle; // refers to array indexes
	Color color;

	bool operator==(const ColoredTriangle& other) const = default;
	bool operator!=(const ColoredTriangle& other) const = default;
};

struct Sphere {
	dvec3 center;
	double radius;
};

Sphere createBoundingSphere(const std::vector<dvec3>& points);

struct Plane {
	dvec3 normal;
	double distance; // distance from origin
};

double signedDistance(const Plane& plane, const dvec3& vertex);

inline dvec3 intersectPlaneSeg(const std::pair<dvec3, dvec3>& segment, const Plane& plane) {
	double t = (-plane.distance - glm::dot(plane.normal, segment.first))
	           / glm::dot(plane.normal, segment.second - segment.first);
	return segment.first + t * (segment.second - segment.first);
}

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
		        std::format("transform: (translation: {}, rotation: {}, scale: {})",
		                    glm::to_string(transform.translation),
		                    glm::to_string(transform.rotation), glm::to_string(transform.scale)),
		        context);
	}
};

dmat4 parseTransform(const Transform& transform);

Transform invertTransform(const Transform& transform);

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

class Object3D {
  private:
	mutable std::optional<Sphere> cachedSphere{};
	std::vector<dvec3> points;
	std::vector<ColoredTriangle> triangles;

  public:
	Object3D(const std::vector<dvec3>& points, const std::vector<ColoredTriangle> triangles)
	    : points(points), triangles(triangles) {}

	const std::vector<dvec3>& getPoints() const { return this->points; }

	const std::vector<ColoredTriangle>& getTriangles() const { return this->triangles; }

	const Sphere& getBoundingSphere() const {
		if (not this->cachedSphere.has_value())
			this->cachedSphere = createBoundingSphere(getPoints());
		return this->cachedSphere.value();
	}
};

// Uses a pointer to the object to save space.
// Unfortunately, this doesn't really work for clipping.
class InstanceRef3D {
  private:
	mutable std::optional<dmat4> cachedTransform{};
	mutable std::optional<Sphere> cachedSphere{};

  public:
	std::shared_ptr<Object3D> object3d;
	Transform transform;

	InstanceRef3D(const std::shared_ptr<Object3D> object3d, const Transform& tr)
	    : object3d(object3d), transform(tr) {}

	// parses to a matrix
	const dmat4& getObjTransform() const {
		if (not this->cachedTransform.has_value())
			this->cachedTransform = parseTransform(this->transform);
		return this->cachedTransform.value();
	};

	Sphere getBoundingSphere() const {
		if (not this->cachedSphere.has_value())
			this->cachedSphere = createBoundingSphere(this->object3d->getPoints());
		return this->cachedSphere.value();
	};
};

// Fully Self Contained (SC) instance.
// Useful for clipping.
class InstanceSC3D {
  private:
	std::vector<dvec3> points;
	std::vector<ColoredTriangle> triangles;
	Transform transform;
	mutable std::optional<dmat4> cachedTransform{};
	mutable std::optional<Sphere> cachedSphere{};

  public:
	InstanceSC3D(const InstanceRef3D& ref)
	    : points(ref.object3d->getPoints()), triangles(ref.object3d->getTriangles()),
	      transform(ref.transform), cachedTransform(ref.getObjTransform()),
	      cachedSphere(ref.getBoundingSphere()) {}

	InstanceSC3D(const InstanceSC3D& inst)
	    : points(inst.points), triangles(inst.triangles), transform(inst.transform) {}

	// @return the added vertex' index
	[[nodiscard]] uint addVertex(const dvec3& vertex) {
		this->points.push_back(vertex);
		return this->points.size() - 1;
	}

	void addTriangle(const ColoredTriangle& triangle) {
		for (uint i = 0; i < 3; i++) {
			assertLt(triangle.triangle[i], this->points.size(), "Triangle index out of range.");
		}
		this->triangles.push_back(triangle);
	}

	const std::vector<dvec3>& getPoints() const { return this->points; }

	const std::vector<ColoredTriangle>& getTriangles() const { return this->triangles; }

	std::vector<dvec3>& getPoints() { return this->points; }

	std::vector<ColoredTriangle>& getTriangles() { return this->triangles; }

	void clearEmptyTris() { std::erase(this->triangles, NO_TRIANGLE); }

	Sphere getBoundingSphere() const {
		if (not this->cachedSphere.has_value())
			this->cachedSphere = createBoundingSphere(this->points);
		return this->cachedSphere.value();
	};

	void setTransform(const Transform& tr) {
		this->transform = tr;
		this->cachedTransform = {};
	}

	const Transform& getTransform() const { return this->transform; }

	// parses to a matrix
	const dmat4& getObjTransform() const {
		if (not this->cachedTransform.has_value())
			this->cachedTransform = parseTransform(this->transform);
		return this->cachedTransform.value();
	};

	virtual ~InstanceSC3D() = default;
};

// clips the triangle in inst at index targetIdx
void clipTriangle(InstanceSC3D& inst, const Plane& plane, const uint targetIdx);

void clipInstance(std::unique_ptr<InstanceSC3D>& inst, const std::vector<Plane>& planes);

// should be called with camera-relative coordinates
std::unique_ptr<InstanceSC3D> backFaceCulling(std::unique_ptr<InstanceSC3D> inst);

#endif /* RENDERABLE_HPP */
