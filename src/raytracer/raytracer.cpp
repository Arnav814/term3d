#include "raytracer.hpp"
#include "../drawing/setColor.hpp"
#include "../extraAssertions.hpp"
#include "glm/geometric.hpp"
#include <csignal>
#include <limits>
#include <memory>
#include <utility>
#include <vector>
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/matrix_double3x3.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/gtx/euler_angles.hpp>
#include <glm/glm.hpp>

using glm::dvec3;

const double viewportWidth = 1.0;
const double viewportHeight = 1.0;
const double viewportDistance = 1.0;
const Color BACKGROUND_COLOR = Color(Category(true, 8), RGBA(0, 0, 0, 255));
const uint REFLECTION_MAX = 3;

dvec3 canvasToViewport(const SextantCoord coord, const SextantCoord canvasSize) {
	return dvec3{
		coord.x * (viewportWidth / canvasSize.x),
		coord.y * (viewportHeight / canvasSize.y),
		viewportDistance
	};
}

struct Sphere {
	dvec3 center;
	double radius;
	Color color;
	double specular; // [0, inf)
	double reflective; // [0, 1]
};

enum class LightType {Point, Directional};

struct Light {
	virtual dvec3 getDirection([[maybe_unused]] dvec3 point) const = 0;
	virtual double getIntensity() const = 0;
	virtual LightType getType() const = 0; // screw "good polymorphic design"
};

class DirectionalLight : public Light {
		double intensity;
		dvec3 direction;

	public:
		DirectionalLight(double intensity, dvec3 direction) : intensity(intensity), direction(direction) {}

		virtual dvec3 getDirection([[maybe_unused]] dvec3 point) const {
			return this->direction;
		}

		virtual double getIntensity() const {
			return this->intensity;
		}

		virtual LightType getType() const {return LightType::Directional;}
};

class PointLight : public Light {
		double intensity;
		dvec3 position;

	public:
		PointLight(double intensity, dvec3 position) : intensity(intensity), position(position) {}
		
		virtual dvec3 getDirection(dvec3 point) const {
			return this->position - point;
		}

		virtual double getIntensity() const {
			return this->intensity;
		}

		virtual LightType getType() const {return LightType::Point;}
};

struct Scene {
	std::vector<Sphere> spheres;
	double ambientLight;
	std::vector<std::shared_ptr<Light>> lights;
};

dvec3 reflectRay(const dvec3 ray, const dvec3 around) {
	return around * glm::dot(around, ray) * 2.0 - ray;
}

// solves the quadratic
std::pair<double, double> intersectRaySphere(
		const dvec3 origin, const dvec3 ray, const Sphere sphere) {
	dvec3 CO = origin - sphere.center;

	double a = glm::dot(ray, ray);
	double b = 2 * glm::dot(CO, ray);
	double c = glm::dot(CO, CO) - pow(sphere.radius, 2);

	double discriminant = pow(b, 2) - 4.0*a*c;
	if (discriminant < 0)
		return std::make_pair(std::numeric_limits<double>::infinity(),
			std::numeric_limits<double>::infinity());
	
	return std::make_pair(
		(-b + sqrt(discriminant)) / (2.0*a),
		(-b - sqrt(discriminant)) / (2.0*a)
	);
}

struct SphereDist {Sphere sphere; double closestT;};
SphereDist closestIntersection(const Scene& scene, const dvec3 origin,
		const dvec3 direction, const double tMin, const double tMax) {
	double closestT = std::numeric_limits<double>::infinity();
	Sphere closestSphere{};
	
	for (const Sphere sphere: scene.spheres) {
		std::pair<double, double> t = intersectRaySphere(origin, direction, sphere);
		if (t.first >= tMin && t.first <= tMax && t.first < closestT) {
			closestT = t.first;
			closestSphere = sphere;
		}
		if (t.second >= tMin && t.second <= tMax && t.second < closestT) {
			closestT = t.second;
			closestSphere = sphere;
		}
	}
	
	return {closestSphere, closestT};
}

double computeLighting(const Scene& scene, const dvec3 point,
		const dvec3 normal, const dvec3 exitVec, const double specular) {
	double intensity = scene.ambientLight;

	for (const std::shared_ptr<const Light> light: scene.lights) {
		dvec3 lightDir = light->getDirection(point);
	
		// shadow check
		double tMax;
		switch(light->getType()) {
			using enum LightType;
			case Point: tMax = 1.0; break;
			case Directional: tMax = std::numeric_limits<double>::infinity(); break;
		}

		auto sphereDist = closestIntersection(scene, point,
			lightDir, 0.001, tMax); // can't use numeric_limits::min() because of floating point errors
		if (sphereDist.closestT != std::numeric_limits<double>::infinity()) {
			continue;
		}

		// diffuse
		double normalDotLight = glm::dot(normal, lightDir);
		if (normalDotLight > 0) { // ignore lights behind the surface
			intensity += (light->getIntensity() * normalDotLight) /
			             (glm::length(normal) * glm::length(lightDir));
		}

		// specular
		if (specular != -1) {
			dvec3 reflected = reflectRay(lightDir, normal);
			double reflectedDotExit = glm::dot(reflected, exitVec);
			if (reflectedDotExit > 0) {
				intensity += light->getIntensity() * pow(reflectedDotExit /
				             (glm::length(reflected) * glm::length(exitVec)), specular);
			}
		}
	}

	return std::min(intensity, 1.0);
}

Color traceRay(const Scene& scene, const dvec3 origin, const dvec3 direction,
		const double tMin, const double tMax, const uint recursionLimit) {
	auto closest = closestIntersection(scene, origin, direction, tMin, tMax);

	if (closest.closestT == std::numeric_limits<double>::infinity())
		return BACKGROUND_COLOR;

	// compute color of object
	dvec3 intersection = origin + direction * closest.closestT;
	dvec3 normal = intersection - closest.sphere.center;
	normal = glm::normalize(normal);
	Color localColor = closest.sphere.color;
	localColor.color *= computeLighting(scene, intersection, normal, -direction, closest.sphere.specular);

	// if we've hit the recursion limit or the intersected sphere isn't reflective, we're done
	if (recursionLimit == 0 ||  closest.sphere.reflective <= 0.0) {
		return localColor;
	}

	// compute reflected color
	dvec3 reflected = reflectRay(-direction, normal);
	Color reflectedColor = traceRay(scene, intersection, reflected, 0.001, tMax, recursionLimit - 1);

	// add them together
	RGBA combinedColor = localColor.color * (1.0-closest.sphere.reflective) + reflectedColor.color * closest.sphere.reflective;

	return Color(localColor.category, combinedColor); // always use this object's category
}

void rayRenderLoop(WindowedDrawing& rawCanvas, const bool& exit_requested, std::function<int()> refresh) {
	const static Scene scene{
		{
			Sphere(dvec3{0.0, -1.0, 3.0}, 1.0, Color{Category{true, 9}, RGBA{255, 0, 0, 255}}, 500, 0.2),
			Sphere(dvec3{2.0, 0.0, 4.0}, 1.0, Color{Category{true, 10}, RGBA{0, 0, 255, 255}}, 500, 0.3),
			Sphere(dvec3{-2.0, 0.0, 4.0}, 1.0, Color{Category{true, 11}, RGBA{0, 255, 0, 255}}, 10, 0.4),
			Sphere(dvec3{0.0, -5001.0, 0.0}, 5000.0, Color{Category{true, 12}, RGBA{255, 255, 0, 255}}, 1000, 0.5),
		},
		0.2,
		{
			std::make_shared<DirectionalLight>(0.2, dvec3(1.0, 4.0, 4.0)),
			std::make_shared<PointLight>(0.6, dvec3(2.0, 1.0, 0.0)),
		}
	};

	int minDimension = std::min(rawCanvas.getHeight(), rawCanvas.getWidth()/2);
	SextantDrawing canvas{minDimension, minDimension*2};
	dvec3 origin = dvec3();
	glm::dmat3x3 cameraRotation = glm::yawPitchRoll(0.0, 0.0, 0.0);
	uchar rot = 0;

	while (not exit_requested) {
		rot++;
		rot %= 25;
		cameraRotation = glm::yawPitchRoll(rot / 12.5 * M_PI + M_PI, 0.0, 0.0);
		origin = dvec3(sin(rot/12.5 * M_PI) * 5, 0.0, cos(rot/12.5 * M_PI) * 5 + 3.5);

		for (int x = -canvas.getWidth() / 2; x < canvas.getWidth() / 2; x++) {
			for (int y = -canvas.getHeight() / 2; y < canvas.getHeight() / 2; y++) {
				dvec3 direction = cameraRotation * canvasToViewport(SextantCoord(y, x), canvas.getSize());
				Color color = traceRay(scene, origin, direction, viewportDistance,
					std::numeric_limits<double>::infinity(), REFLECTION_MAX);
				putPixel(canvas, SextantCoord(y, x), color);
			}
		}
		rawCanvas.insert(SextantCoord(0, 0), canvas);
		rawCanvas.render();
		refresh();
		//sleep(1);
	}
}

