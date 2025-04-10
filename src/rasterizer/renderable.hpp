#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include "../drawing/setColor.hpp"
#include "../extraAssertions.hpp"
#include "../util/formatters.hpp"
#include "structures.hpp"
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/ext/vector_double3.hpp>
#include <glm/geometric.hpp>
#include <glm/gtx/hash.hpp>
#include <glm/gtx/string_cast.hpp>
#include <memory>

using glm::dvec3, glm::dmat4, glm::dvec4;

Sphere createBoundingSphere(const std::vector<dvec3>& points);

double signedDistance(const Plane& plane, const dvec3& vertex);

inline dvec3 intersectPlaneSeg(const std::pair<dvec3, dvec3>& segment, const Plane& plane) {
	double t = (-plane.distance - glm::dot(plane.normal, segment.first))
	           / glm::dot(plane.normal, segment.second - segment.first);
	return segment.first + t * (segment.second - segment.first);
}

#ifndef NDEBUG
// does not check the indexes are valid
inline void validateTri(const ColoredTriangle& tri) {
	if (tri == NO_TRIANGLE) return;
	forAll(tri.normals, [](const dvec3& normal) {
		assertFiniteVec(normal, "Normals must be finite.");
		assertBetweenIncl(0.999, glm::length(normal), 1.001, "Normals need a length of 1.");
	});
	forAllPairs(tri.normals, [](const std::pair<dvec3, dvec3> normalPair) {
		dvec3 sum = normalPair.first + normalPair.second;
		assertGt(std::abs(sum.x) + std::abs(sum.y) + std::abs(sum.z), 0.001,
		         "Normals can't point directly away from each other.");
	});
}
#else
inline void validateTri(ColoredTriangle& tri) {}
#endif

struct IntersectPlaneSegTRetVal {
	double t;
	dvec3 intersection;
};

// also return the t value
// t describes how far across this intersection is, from 0 to 1
inline IntersectPlaneSegTRetVal intersectPlaneSegT(const std::pair<dvec3, dvec3>& segment,
                                                   const Plane& plane) {
	double t = (-plane.distance - glm::dot(plane.normal, segment.first))
	           / glm::dot(plane.normal, segment.second - segment.first);
	return {t, segment.first + t * (segment.second - segment.first)};
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

inline dvec4 toHomogenous(const dvec3& point, const uint w = 1) {
	return {point.x, point.y, point.z, w};
}

inline dvec3 toHomogenous(const glm::dvec2& point, const uint w = 1) {
	return {point.x, point.y, w};
}

class Object3D {
  private:
	mutable std::optional<Sphere> cachedSphere{};
	std::vector<dvec3> points;
	std::vector<ColoredTriangle> triangles;
	double specular;

  public:
	Object3D(const std::vector<dvec3>& points, const std::vector<ColoredTriangle> triangles,
	         const double specular)
	    : points(points), triangles(triangles), specular(specular) {
#ifndef NDEBUG
		for (const dvec3& point : points) {
			assertFiniteVec(point, "Points must be finite in objects.");
		}
#endif
	}

	const std::vector<dvec3>& getPoints() const { return this->points; }

	const std::vector<ColoredTriangle>& getTriangles() const { return this->triangles; }

	dvec3 getPoint(const uint idx) const { return this->points.at(idx); }

	ColoredTriangle getTriangle(const uint idx) const { return this->triangles.at(idx); }

	void setPoint(const uint idx, const dvec3& val) {
		assertFiniteVec(val, "Setting point to non finite value in object.");
		this->points.at(idx) = val;
	}

	void setTriangle(const uint idx, const ColoredTriangle& val) {
		if (val != NO_TRIANGLE)
			for (uint i = 0; i < 3; i++) {
				assertLt(val.triangle[i], this->points.size(), "val index out of range.");
			}
		validateTri(val);
		this->triangles.at(idx) = val;
	}

	Triangle<dvec3> getDvecTri(Triangle<uint> tri) {
		return {this->points[tri[0]], this->points[tri[1]], this->points[tri[2]]};
	};

	double getSpecular() const { return this->specular; }

	// @return the added vertex's index
	[[nodiscard]] uint addVertex(const dvec3& vertex) {
		assertFiniteVec(vertex, "Vertexes must be finite in objects.");
		this->points.push_back(vertex);
		return this->points.size() - 1;
	}

	void addTriangle(const ColoredTriangle& triangle) {
		if (triangle != NO_TRIANGLE)
			for (uint i = 0; i < 3; i++) {
				assertLt(triangle.triangle[i], this->points.size(), "Triangle index out of range.");
			}
		validateTri(triangle);
		this->triangles.push_back(triangle);
	}

	void clearEmptyTris() { std::erase(this->triangles, NO_TRIANGLE); }

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
	friend class InstanceSC3D;
	std::shared_ptr<Object3D> object3d;
	Transform transform;
	mutable std::optional<dmat4> cachedTransform{};
	mutable std::optional<dmat4> cachedInvTransform{};
	mutable std::optional<Sphere> cachedSphere{};

  public:
	InstanceRef3D(const std::shared_ptr<Object3D> object3d, const Transform& tr)
	    : object3d(object3d), transform(tr) {}

	// parses to a matrix
	const dmat4& fromObjectSpace() const {
		if (not this->cachedTransform.has_value())
			this->cachedTransform = parseTransform(this->transform);
		return this->cachedTransform.value();
	};

	const dmat4& toObjectSpace() const {
		if (not this->cachedInvTransform.has_value())
			this->cachedInvTransform = parseTransform(invertTransform(this->transform));
		return this->cachedInvTransform.value();
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
	double specular;
	mutable std::optional<dmat4> cachedTransform{};
	mutable std::optional<dmat4> cachedInvTransform{};
	mutable std::optional<Sphere> cachedSphere{};

  public:
	InstanceSC3D(const InstanceRef3D& ref)
	    : points(ref.object3d->getPoints()), triangles(ref.object3d->getTriangles()),
	      transform(ref.transform), specular(ref.object3d->getSpecular()),
	      cachedTransform(ref.fromObjectSpace()), cachedSphere(ref.getBoundingSphere()) {}

	InstanceSC3D(const InstanceSC3D& inst)
	    : points(inst.points), triangles(inst.triangles), transform(inst.transform),
	      specular(inst.specular) {}

	// @return the added vertex's index
	[[nodiscard]] uint addVertex(const dvec3& vertex) {
		assertFiniteVec(vertex, "Vertexes must be finite in instances.");
		this->points.push_back(vertex);
		return this->points.size() - 1;
	}

	void addTriangle(const ColoredTriangle& triangle) {
		if (triangle != NO_TRIANGLE)
			for (uint i = 0; i < 3; i++) {
				assertLt(triangle.triangle[i], this->points.size(), "Triangle index out of range.");
			}
		validateTri(triangle);
		this->triangles.push_back(triangle);
	}

	const std::vector<dvec3>& getPoints() const { return this->points; }

	const std::vector<ColoredTriangle>& getTriangles() const { return this->triangles; }

	dvec3 getPoint(const uint idx) const { return this->points.at(idx); }

	ColoredTriangle getTriangle(const uint idx) const { return this->triangles.at(idx); }

	void setPoint(const uint idx, const dvec3& val) {
		assertFiniteVec(val, "Setting point to non finite value in instance.");
		this->points.at(idx) = val;
	}

	void setTriangle(const uint idx, const ColoredTriangle& val) {
		if (val != NO_TRIANGLE)
			for (uint i = 0; i < 3; i++) {
				assertLt(val.triangle[i], this->points.size(), "val index out of range.");
			}
		validateTri(val);
		this->triangles.at(idx) = val;
	}

	// don't use this very much; it's inefficient
	Triangle<dvec3> getDvecTri(Triangle<uint> tri) {
		return {this->points[tri[0]], this->points[tri[1]], this->points[tri[2]]};
	};

	void clearEmptyTris() { std::erase(this->triangles, NO_TRIANGLE); }

	// remove any points not referenced by a triangle
	// sets them to NO_POINT
	void clearUnusedPoints();

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

	double getSpecular() const { return this->specular; }

	// parses to a matrix
	const dmat4& fromObjectSpace() const {
		if (not this->cachedTransform.has_value())
			this->cachedTransform = parseTransform(this->transform);
		return this->cachedTransform.value();
	};

	const dmat4& toObjectSpace() const {
		if (not this->cachedInvTransform.has_value())
			this->cachedInvTransform = parseTransform(invertTransform(this->transform));
		return this->cachedInvTransform.value();
	};

	virtual ~InstanceSC3D() = default;
};

// clips the triangle in inst at index targetIdx
void clipTriangle(InstanceSC3D& inst, const Plane& plane, const uint targetIdx);

void clipInstance(std::unique_ptr<InstanceSC3D>& inst, const std::vector<Plane>& planes);

// should be called with camera-relative coordinates
std::unique_ptr<InstanceSC3D> backFaceCulling(std::unique_ptr<InstanceSC3D> inst);

enum class LightType { Point, Directional };

class Light {
  protected:
	friend class std::formatter<Light>;
	virtual std::string stringify() const = 0;

  public:
	virtual dvec3 getDirection([[maybe_unused]] dvec3 point) const = 0;
	virtual double getIntensity() const = 0;
	virtual LightType getType() const = 0; // screw "good polymorphic design"
	virtual ~Light() = default;
};

class DirectionalLight : public Light {
  private:
	double intensity;
	dvec3 direction;

	std::string stringify() const {
		return std::format("DirectionalLight(direction:{}, intensity:{})", direction, intensity);
	}

  public:
	DirectionalLight(double intensity, dvec3 direction)
	    : intensity(intensity), direction(glm::normalize(direction)) {}

	virtual dvec3 getDirection([[maybe_unused]] dvec3 point) const { return this->direction; }

	virtual double getIntensity() const { return this->intensity; }

	dvec3 getDirection() const { return this->direction; }

	virtual LightType getType() const { return LightType::Directional; }

	virtual ~DirectionalLight() = default;
};

class PointLight : public Light {
  private:
	double intensity;
	dvec3 position;

	std::string stringify() const {
		return std::format("PointLight(position:{}, intensity:{})", position, intensity);
	}

  public:
	PointLight(double intensity, dvec3 position) : intensity(intensity), position(position) {}

	virtual dvec3 getDirection(dvec3 point) const { return this->position - point; }

	virtual double getIntensity() const { return this->intensity; }

	dvec3 getPosition() const { return this->position; }

	virtual LightType getType() const { return LightType::Point; }

	virtual ~PointLight() = default;
};

template <> struct std::formatter<Light> : std::formatter<std::string> {
	using std::formatter<std::string>::parse;

	auto format(Light const& val, auto& ctx) const {
		auto out = ctx.out();
		out = std::format_to(out, val.stringify());
		ctx.advance_to(out);
		return out;
	};
};

struct Scene {
	std::vector<std::shared_ptr<Object3D>>
	    objects; // TODO: do I need this if it's all pointed to by instances?
	std::vector<InstanceRef3D> instances;
	std::vector<std::shared_ptr<Light>> lights;
	Camera camera;
	Color bgColor;
	double ambientLight;
};

#endif /* RENDERABLE_HPP */
