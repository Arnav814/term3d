#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include <array>
#include <format>
#include <memory>
#include <vector>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_double4x4.hpp>
#include "../drawing/setColor.hpp"
#include "glm/geometric.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/hash.hpp>
#include "../extraAssertions.hpp"

using glm::dvec3, glm::dmat4, glm::dvec4;

template<typename T> using Triangle = std::array<T, 3>;

struct ColoredTriangle {
	Triangle<uint> triangle; // refers to array indexes
	Color color;
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

class Object3D {
	private:
		mutable std::optional<Sphere> cachedSphere{};
		std::vector<dvec3> points;
		std::vector<ColoredTriangle> triangles;

	public:
		Object3D(const std::vector<dvec3>& points, const std::vector<ColoredTriangle> triangles) : 
			points(points), triangles(triangles) {}
		const std::vector<dvec3>& getPoints() const {return this->points;}
		const std::vector<ColoredTriangle>& getTriangles() const {return this->triangles;}
		const Sphere& getBoundingSphere() const {
			if (not this->cachedSphere.has_value())
				this->cachedSphere = createBoundingSphere(getPoints());
			return this->cachedSphere.value();
		}
};

// arbitrary 3d instance
class Instance3D {
	private:
		mutable std::optional<dmat4> cachedTransform{};

	protected:
		Transform transform;
		Instance3D(const Transform& tr) : transform(tr) {}

	public:
		virtual const std::vector<dvec3>& getPoints() const = 0;
		virtual const std::vector<ColoredTriangle>& getTriangles() const = 0;
		virtual Sphere getBoundingSphere() const = 0;

		void setTransform(const Transform& tr) {
			this->transform = tr;
			this->cachedTransform = {};
		}
		const Transform& getTransform() const {
			return this->transform;
		}
		// parses to a matrix
		const dmat4& getObjTransform() const {
			if (not this->cachedTransform.has_value())
				this->cachedTransform = parseTransform(this->transform);
			return this->cachedTransform.value();
		};

		virtual ~Instance3D() = default;
};

// Uses a pointer to the object to save space.
// Unfortunately, this doesn't really work for clipping.
class InstanceRef3D : public Instance3D {
	private:
		std::shared_ptr<Object3D> object3d;
	public:
		InstanceRef3D(const std::shared_ptr<Object3D> object3d, const Transform& tr) :
			Instance3D(tr), object3d(object3d) {}
		virtual Sphere getBoundingSphere() const {return this->object3d->getBoundingSphere();}

		virtual const std::vector<dvec3>& getPoints() const {
			return this->object3d->getPoints();
		}
		virtual const std::vector<ColoredTriangle>& getTriangles() const{
			return this->object3d->getTriangles();
		}

		virtual ~InstanceRef3D() = default;
};

// Fully Self Contained (SC) instance.
// Useful for clipping.
class InstanceSC3D : public Instance3D {
	private:
		std::vector<dvec3> points;
		std::vector<ColoredTriangle> triangles;
		mutable std::optional<Sphere> cachedSphere{};

	public:
		InstanceSC3D(const Instance3D& inst) :
			Instance3D(inst.getTransform()),
			points(inst.getPoints()),
			triangles(inst.getTriangles()) {}

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
		void rmTriangle(const uint index) {
			this->triangles.erase(this->triangles.begin() + index);
		}

		virtual const std::vector<dvec3>& getPoints() const {
			return this->points;
		}
		virtual const std::vector<ColoredTriangle>& getTriangles() const {
			return this->triangles;
		}
		virtual Sphere getBoundingSphere() const {
			if (not this->cachedSphere.has_value())
				this->cachedSphere = createBoundingSphere(this->points);
			return this->cachedSphere.value();
		};
		virtual ~InstanceSC3D() = default;
};

// clips the triangle in inst at index targetIdx
inline void clipTriangle(InstanceSC3D& inst, const Plane& plane, const uint targetIdx) {
	Triangle<uint> target = inst.getTriangles()[targetIdx].triangle;
	Color color = inst.getTriangles()[targetIdx].color;

	Triangle<double> distances{};
	for (uint i = 0; i < 3; i++) {
		distances[i] = signedDistance(plane, inst.getPoints()[target[i]]);
	}
	uchar numPositive = (distances[0] >= 0) + (distances[1] >= 0) + (distances[2] >= 0);

	if (numPositive == 3) { // fully inside plane
		return;

	} else if (numPositive == 1) { // 1 vertex inside
		if (distances[0] >= 0); // make p0 be the only positive vertex
		else if (distances[1] >= 0) std::swap(target[0], target[1]);
		else if (distances[2] >= 0) std::swap(target[0], target[2]);
		
		dvec3 vertexA = intersectPlaneSeg(std::make_pair(inst.getPoints()[target[0]], inst.getPoints()[target[1]]), plane);
		dvec3 vertexB = intersectPlaneSeg(std::make_pair(inst.getPoints()[target[0]], inst.getPoints()[target[2]]), plane);

		inst.rmTriangle(targetIdx);

		// this will create duplicate points, but catching those would be too much work
		inst.addTriangle({{
				target[0], 
				inst.addVertex(vertexA),
				inst.addVertex(vertexB)
			},
			color}
		);

	} else if (numPositive == 2) { // 2 verticies inside
		if (distances[0] < 0); // make p0 be the only negative vertex
		else if (distances[1] < 0) std::swap(target[0], target[1]);
		else if (distances[2] < 0) std::swap(target[0], target[2]);

		dvec3 p1Prime = intersectPlaneSeg(std::make_pair(inst.getPoints()[target[0]], inst.getPoints()[target[1]]), plane);
		dvec3 p2Prime = intersectPlaneSeg(std::make_pair(inst.getPoints()[target[0]], inst.getPoints()[target[2]]), plane);

		uint p1Idx = inst.addVertex(p1Prime);
		uint p2Idx = inst.addVertex(p2Prime);

		inst.rmTriangle(targetIdx);

		inst.addTriangle({{
				target[1],
				p1Idx,
				target[2]
			},
			color
		});
		inst.addTriangle({{
				target[2],
				p2Idx,
				p1Idx
			},
			color
		});

	} else if (numPositive == 0) { // all outside
		// nothing inside plane
	}
}

inline std::shared_ptr<const Instance3D> clipInstance(std::shared_ptr<const Instance3D> inst, const std::vector<Plane>& planes) {
	std::shared_ptr<InstanceSC3D> copied = NULL; // we only need to make a copy sometimes

	for (const Plane& plane: planes) {
		Sphere bounding = inst->getBoundingSphere();
		double distance = signedDistance(plane, bounding.center);

		if (distance >= bounding.radius) { // fully inside
			pass();

		} else if (distance <= -bounding.radius) { // fully outside
			return NULL;

		} else { // split
			if (copied == NULL)
				copied = std::make_shared<InstanceSC3D>(InstanceSC3D{*inst});
			
			// clipping creates more triangles; we don't want to clip them again for no reason
			uint numTriangles = inst->getTriangles().size();

			for (uint i = 0; i < numTriangles; i++) {
				clipTriangle(*copied, plane, i);
			}
		}
	}

	// if we made a copy, return it
	if (copied != NULL)
		return copied;
	else
		return inst;
}

#endif /* RENDERABLE_HPP */
