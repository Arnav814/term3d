#include "triangles.hpp"
#include "common.hpp"
#include "glm/gtx/string_cast.hpp"
#include <boost/multi_array.hpp>
#include <limits>
#include <memory>

// converts from origin at center to origin at top left
template <typename T>
inline void putBufPixel(boost::multi_array<T, 2>& buffer, const ivec2 coord, const T val) {
	ivec2 transformed = {buffer.shape()[0] / 2 - coord.y, buffer.shape()[1] / 2 + coord.x};
	if (0 <= transformed.y and transformed.y < (int)buffer.shape()[0] and 0 <= transformed.x
	    and transformed.x < (int)buffer.shape()[1])
		buffer[transformed.y][transformed.x] = val;
}

// converts from origin at center to origin at top left
template <typename T>
inline T getBufPixel(const boost::multi_array<T, 2>& buffer, const ivec2 coord, const T fallback) {
	ivec2 transformed = {buffer.shape()[0] / 2 - coord.y, buffer.shape()[1] / 2 + coord.x};
	if (0 <= transformed.y and transformed.y < (int)buffer.shape()[0] and 0 <= transformed.x
	    and transformed.x < (int)buffer.shape()[1])
		return buffer[transformed.y][transformed.x];
	return fallback;
}

std::vector<double> interpolate(const int x0, const double y0, const int x1, const double y1) {
	// TODO: do these really need to be doubles?
	if (x0 == x1) // for only one point
		return {y0};

	assertGt(x1, x0, "Can't interpolate backwards");

	std::vector<double> out;
	out.reserve(x1 - x0 + 1);

	double slope = (double)(y1 - y0) / (x1 - x0);
	double y = y0;
	for (int x = x0; x <= x1; x++) {
		out.push_back(y);
		y += slope;
	}

	return out; // FIXME: Passing a vector like this is horrendously inefficient. Use an iterator.
}

// interpolate a member of Elem
template <typename Elem, typename Field, typename LambdaType>
void interpolateField(std::vector<Elem>& baseVector, LambdaType getElemRef, const int x0,
                      const double y0, const int x1, const double y1) {
	// TODO: do these really need to be doubles?

	assertEq(baseVector.size(), static_cast<uint>(x1 - x0 + 1),
	         "baseVector must be set to the correct size beforehand.");

	if (x0 == x1) { // for only one point
		getElemRef(baseVector[0]) = y0;
		return;
	}

	assertGt(x1, x0, "Can't interpolate backwards");

	double slope = (double)(y1 - y0) / (x1 - x0);
	double y = y0;
	for (int x = 0; x <= x1 - x0; x++) {
		getElemRef(baseVector[x]) = y;
		y += slope;
	}
}

static dvec3 reflectRay(const dvec3 ray, const dvec3 around) {
	return around * glm::dot(around, ray) * 2.0 - ray;
}

static double computeLighting(const dvec3 point, const dvec3 normal, const double specular,
                              const double ambientLight,
                              const std::vector<std::shared_ptr<Light>> lights) {
	double intensity = ambientLight;

	for (const std::shared_ptr<const Light> light : lights) {
		dvec3 lightDir = light->getDirection(point);

		// diffuse
		double normalDotLight = glm::dot(normal, lightDir);
		if (normalDotLight > 0) { // ignore lights behind the surface
			intensity += (light->getIntensity() * normalDotLight)
			             / (glm::length(normal) * glm::length(lightDir));
		}

		// specular
		if (specular != -1) {
			dvec3 reflected = reflectRay(lightDir, normal);
			double reflectedDotExit = glm::dot(reflected, -point);
			if (reflectedDotExit > 0) {
				intensity += light->getIntensity()
				             * pow(reflectedDotExit / (glm::length(reflected) * glm::length(point)),
				                   specular);
			}
		}
	}

	return std::min(intensity, 1.0);
}

void drawLine(SextantDrawing& canvas, ivec2 p0, ivec2 p1, const Color color) {
	if (abs(p0.x - p1.x) > abs(p0.y - p1.y)) { // line is horizontalish
		if (p0.x > p1.x) // make sure p0 is left of p1
			std::swap(p0, p1);

		std::vector<double> yVals = interpolate(p0.x, p0.y, p1.x, p1.y);
		for (int x = p0.x; x <= p1.x; x++) {
			putPixel(canvas, SextantCoord(yVals.at(x - p0.x), x), color);
		}
	} else { // line is verticalish
		if (p0.y > p1.y) // make sure p0 is under p1
			std::swap(p0, p1);

		std::vector<double> xVals = interpolate(p0.y, p0.x, p1.y, p1.x);
		for (int y = p0.y; y <= p1.y; y++) {
			putPixel(canvas, SextantCoord(y, xVals.at(y - p0.y)), color);
		}
	}
}

void drawWireframeTriangle(SextantDrawing& canvas, const ivec2 p0, const ivec2 p1, const ivec2 p2,
                           const Color color) {
	drawLine(canvas, p0, p1, color);
	drawLine(canvas, p1, p2, color);
	drawLine(canvas, p2, p0, color);
}

struct InterpElems {
	double x; // x value (duh)
	double invDepth; // 1 / depth
	double normalX; // interpolate x, y, and z of the normal vector
	double normalY;
	double normalZ;
};

struct RowElems {
	double invDepth; // 1 / depth
	double normalX; // interpolate x, y, and z of the normal vector
	double normalY;
	double normalZ;
};

// this function is a mess
void drawFilledTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
                        Triangle<ivec2> points, Triangle<float> depth, Triangle<dvec3> normals,
                        const Color color, const double viewportDistance, const double ambientLight,
                        const double specular, const std::vector<std::shared_ptr<Light>>& lights) {
	// sort top to bottom, so p0.y < p1.y < p2.y
	if (points[1].y < points[0].y) {
		std::swap(depth[1], depth[0]);
		std::swap(points[1], points[0]);
		std::swap(normals[1], normals[0]);
	}
	if (points[2].y < points[0].y) {
		std::swap(depth[2], depth[0]);
		std::swap(points[2], points[0]);
		std::swap(normals[2], normals[0]);
	}
	if (points[2].y < points[1].y) {
		std::swap(depth[2], depth[1]);
		std::swap(points[2], points[1]);
		std::swap(normals[2], normals[1]);
	}

	// interpolate a member of a struct with macro hax
#define interpField(vec, field, x0, y0, x1, y1) \
	do { \
		auto getField = \
		    [](decltype(vec)::value_type& elem) -> decltype(decltype(vec)::value_type::field)& { \
			return elem.field; \
		}; \
		interpolateField<decltype(vec)::value_type, decltype(decltype(vec)::value_type::field), \
		                 decltype(getField)>((vec), getField, (x0), (y0), (x1), (y1)); \
	} while (false)

	std::vector<InterpElems> shortSide1{
	    static_cast<uint>(points[1].y - points[0].y + 1), InterpElems{0, 0, 0, 0, 0}
    };
	interpField(shortSide1, x, points[0].y, points[0].x, points[1].y, points[1].x);
	interpField(shortSide1, invDepth, points[0].y, 1.0 / depth[0], points[1].y, 1.0 / depth[1]);
	interpField(shortSide1, normalX, points[0].y, normals[0].x, points[1].y, normals[1].x);
	interpField(shortSide1, normalY, points[0].y, normals[0].y, points[1].y, normals[1].y);
	interpField(shortSide1, normalZ, points[0].y, normals[0].z, points[1].y, normals[1].z);

	std::vector<InterpElems> shortSide2{
	    static_cast<uint>(points[2].y - points[1].y + 1), InterpElems{0, 0, 0, 0, 0}
    };
	interpField(shortSide2, x, points[1].y, points[1].x, points[2].y, points[2].x);
	interpField(shortSide2, invDepth, points[1].y, 1.0 / depth[1], points[2].y, 1.0 / depth[2]);
	interpField(shortSide2, normalX, points[1].y, normals[1].x, points[2].y, normals[2].x);
	interpField(shortSide2, normalY, points[1].y, normals[1].y, points[2].y, normals[2].y);
	interpField(shortSide2, normalZ, points[1].y, normals[1].z, points[2].y, normals[2].z);

	std::vector<InterpElems> longSide{
	    static_cast<uint>(points[2].y - points[0].y + 1), InterpElems{0, 0, 0, 0, 0}
    };
	interpField(longSide, x, points[0].y, points[0].x, points[2].y, points[2].x);
	interpField(longSide, invDepth, points[0].y, 1.0 / depth[0], points[2].y, 1.0 / depth[2]);
	interpField(longSide, normalX, points[0].y, normals[0].x, points[2].y, normals[2].x);
	interpField(longSide, normalY, points[0].y, normals[0].y, points[2].y, normals[2].y);
	interpField(longSide, normalZ, points[0].y, normals[0].z, points[2].y, normals[2].z);

	// combine vectors
	shortSide1.pop_back();
	// from here, shortSide1 is actually both short sides combined
	shortSide1.reserve(shortSide1.size() + shortSide2.size());
	// TODO: don't actually copy here, use boost::join() or something
	shortSide1.insert(shortSide1.end(), shortSide2.begin(), shortSide2.end());

	std::unique_ptr<std::vector<InterpElems>> left;
	std::unique_ptr<std::vector<InterpElems>> right;
	int middleIndex = longSide.size() / 2; // some arbitrary index
	if (longSide.at(middleIndex).x == shortSide1.at(middleIndex).x and middleIndex != 0)
		middleIndex--; // fix problems if length==2

	if (longSide.at(middleIndex).x < shortSide1.at(middleIndex).x) {
		left = std::make_unique<std::vector<InterpElems>>(longSide);
		right = std::make_unique<std::vector<InterpElems>>(shortSide1);
	} else {
		left = std::make_unique<std::vector<InterpElems>>(shortSide1);
		right = std::make_unique<std::vector<InterpElems>>(longSide);
	}

	for (int y = points[0].y; y <= points[2].y; y++) {
		InterpElems rowLeft = left->at(y - points[0].y);
		InterpElems rowRight = right->at(y - points[0].y);
		int rowLeftX = round(rowLeft.x);
		int rowRightX = round(rowRight.x);
		assertGtEq(rowRightX, rowLeftX, "right is left of left");

		std::vector<RowElems> row{
		    static_cast<uint>(rowRightX - rowLeftX + 1), RowElems{0, 0, 0, 0}
        };

		interpField(row, invDepth, rowLeftX, rowLeft.invDepth, rowRightX, rowRight.invDepth);
		interpField(row, normalX, rowLeftX, rowLeft.normalX, rowRightX, rowRight.normalX);
		interpField(row, normalY, rowLeftX, rowLeft.normalY, rowRightX, rowRight.normalY);
		interpField(row, normalZ, rowLeftX, rowLeft.normalZ, rowRightX, rowRight.normalZ);

		for (int x = rowLeftX; x <= rowRightX; x++) {
			RowElems pixel = row.at(x - rowLeftX);
			double invDepth = pixel.invDepth;
			if (getBufPixel(depthBuffer, {x, y}, std::numeric_limits<float>::infinity())
			    < invDepth) {
				// because the camera is at {0, 0, 0},
				// the camera to point vector is the same as the point itself
				dvec3 camToDrawnPoint = {(double)x / (viewportDistance * invDepth),
				                         (double)y / (viewportDistance * invDepth), 1. / invDepth};
				dvec3 normal = {pixel.normalX, pixel.normalY, pixel.normalZ};
				double lighting =
				    computeLighting(camToDrawnPoint, normal, specular, ambientLight, lights);
				RGBA newColor = color.color * lighting;

				if (debugFrame)
					std::println(
					    std::cerr,
					    "pixel ({}, {}): normal: {}, cam to point: {}, depth: {}, lighting: {}", x,
					    y, glm::to_string(normal), glm::to_string(camToDrawnPoint), 1 / invDepth,
					    lighting);

				putBufPixel(depthBuffer, {x, y}, (float)invDepth);
				putPixel(canvas, SextantCoord(y, x), Color(color.category, newColor));
			}
		}
	}
#undef interpField
}

void renderTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
                    const Triangle<ivec2>& triangle, const Triangle<float>& depth,
                    const Triangle<dvec3> normals, const Color color, const double viewportDistance,
                    const double ambientLight, const double specular,
                    const std::vector<std::shared_ptr<Light>> lights) {
	if (debugFrame) {
		std::println(std::cerr, "drawing tri: p0: {}, p1: {}, p2: {}", glm::to_string(triangle[0]),
		             glm::to_string(triangle[1]), glm::to_string(triangle[2]));
	}
	drawFilledTriangle(canvas, depthBuffer, triangle, depth, normals, color, viewportDistance,
	                   ambientLight, specular, lights);
}
