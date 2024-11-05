#include "raytracer.hpp"
#include "coord3d.hpp"
#include "../drawing/setColor.hpp"
#include "../extraAssertions.hpp"
#include <csignal>
#include <limits>
#include <utility>
#include <vector>

const double viewportWidth = 1.0;
const double viewportHeight = 1.0;
const double viewportDistance = 1.0;
const Color BACKGROUND_COLOR = Color(Category(true, 8), RGBA(255, 255, 255, 255));

// converts from origin at center to origin at top left
void putPixel(SextantDrawing& canvas, const SextantCoord coord, const Color color) {
	SextantCoord translated{canvas.getHeight() / 2 - coord.y, canvas.getWidth() / 2 + coord.x};
	canvas.trySet(translated, color);
}

Coord3d<double> canvasToViewport(const SextantCoord coord, const SextantCoord canvasSize) {
	return Coord3d{
		coord.x * (viewportWidth / canvasSize.x),
		coord.y * (viewportHeight / canvasSize.y),
		viewportDistance
	};
}

struct Sphere {Coord3d<double> center; double radius; Color color;};

// TODO: polymorphism magic?
struct DirectionalLight {double intensity; Coord3d<double> direction;};
struct PointLight {double intensity; Coord3d<double> position;};

struct Scene {
	std::vector<Sphere> spheres;
	double ambientLight;
	std::vector<DirectionalLight> directionalLights;
	std::vector<PointLight> pointLights;
};

// solves the quadratic
std::pair<double, double> intersectRaySphere(
		const Coord3d<double> origin, const Coord3d<double> ray, const Sphere sphere) {
	Coord3d CO = origin - sphere.center;

	double a = dotProduct(ray, ray);
	double b = 2 * dotProduct(CO, ray);
	double c = dotProduct(CO, CO) - pow(sphere.radius, 2);

	double discriminant = pow(b, 2) - 4*a*c;
	if (discriminant < 0)
		return std::make_pair(std::numeric_limits<double>::infinity(),
			std::numeric_limits<double>::infinity());
	
	return std::make_pair(
		(-b + sqrt(discriminant)) / (2*a),
		(-b - sqrt(discriminant)) / (2*a)
	);
}

double computeLighting(const Scene& scene, const Coord3d<double> point,
		const Coord3d<double> normal) {
	double intensity = scene.ambientLight;

	// contains direction from point and intensity
	std::vector<std::pair<Coord3d<double>, double>> directionVecs{};
	
	for (PointLight light: scene.pointLights) {
		directionVecs.push_back(std::make_pair(light.position - point, light.intensity));
	}
	for (DirectionalLight light: scene.directionalLights) {
		directionVecs.push_back(std::make_pair(light.direction, light.intensity));
	}

	for (auto light: directionVecs) {
		double pointDotLight = dotProduct(point, light.first);
		if (pointDotLight > 0) // ignore lights behind the surface
			intensity += (light.second * pointDotLight) / (normal.length() * light.first.length());
	}

	return intensity;
}

Color traceRay(const Coord3d<double> origin, const Coord3d<double> direction,
		const Scene& scene, const double tMin, const double tMax) {
	double closestT = std::numeric_limits<double>::infinity();
	std::optional<Sphere> closestSphere{};
	
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

	if (closestSphere.has_value()) {
		Coord3d<double> intersection = origin + direction * closestT;
		Coord3d<double> normal = intersection - closestSphere->center;
		normal = normal / normal.length(); // make length == 1
		Color color = closestSphere->color;
		color.color *= std::min(computeLighting(scene, intersection, normal), 1.0);
		return color;
	} else {
		return BACKGROUND_COLOR;
	}
}

void renderLoop(WindowedDrawing& rawCanvas, const bool& exit_requested, std::function<int()> refresh) {
	const static Scene scene{
		{
			Sphere(Coord3d{0.0, -1.0, 3.0}, 1.0, Color{Category{true, 9}, RGBA{255, 0, 0, 255}}),
			Sphere(Coord3d{2.0, 0.0, 4.0}, 1.0, Color{Category{true, 10}, RGBA{0, 0, 255, 255}}),
			Sphere(Coord3d{-2.0, 0.0, 4.0}, 1.0, Color{Category{true, 11}, RGBA{0, 255, 0, 255}}),
			Sphere(Coord3d{0.0, -5000.0, 0.0}, 5000.0, Color{Category{true, 12}, RGBA{255, 255, 0, 255}}),
		},
		0.2,
		{DirectionalLight(0.2, Coord3d(1.0, 4.0, 4.0))},
		{PointLight(0.6, Coord3d(2.0, 1.0, 0.0))}
	};

	int minDimension = std::min(rawCanvas.getHeight(), rawCanvas.getWidth()/2);
	SextantDrawing canvas{minDimension, minDimension*2};
	Coord3d<double> origin = Coord3d<double>();
	while (not exit_requested) {
		for (int x = -canvas.getWidth() / 2; x < canvas.getWidth() / 2; x++) {
			for (int y = -canvas.getHeight() / 2; y < canvas.getHeight() / 2; y++) {
				Coord3d direction = canvasToViewport(SextantCoord(y, x), canvas.getSize());
				Color color = traceRay(origin, direction, scene, viewportDistance,
					std::numeric_limits<double>::infinity());
				putPixel(canvas, SextantCoord(y, x), color);
			}
		}
		rawCanvas.insert(SextantCoord(0, 0), canvas);
		rawCanvas.render();
		refresh();
		sleep(1);
	}
}

