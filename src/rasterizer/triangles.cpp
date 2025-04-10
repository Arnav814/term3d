#include "triangles.hpp"

#include "glm/geometric.hpp"
#include "glm/gtx/string_cast.hpp"
#include "interpolate.hpp"
#include "renderable.hpp"
#include "structures.hpp"

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

static dvec3 reflectRay(const dvec3 ray, const dvec3 around) {
	return around * glm::dot(around, ray) * 2.0 - ray;
}

static double computeLighting(const dvec3 point, const dvec3 camera, const dvec3 normal,
                              const double specular, const double ambientLight,
                              const std::vector<std::shared_ptr<Light>> lights) {
	assertFiniteVec(point, "");
	assertFiniteVec(camera, "");
	assertFiniteVec(normal, "");
	assertFinite(specular, "");
	assertFinite(ambientLight, "");

	double intensity = ambientLight;
	dvec3 camToPoint = point - camera;

	for (const std::shared_ptr<const Light> light : lights) {
		dvec3 lightDir = light->getDirection(point);

		if (debugFrame) {
			std::print(std::cerr, "[light from vec {:.2f}:", lightDir);
			std::print(std::cerr, "cam to point:{:.2f}, ", camToPoint);
			std::print(std::cerr, "point:{:.2f}, ", point);
			std::print(std::cerr, "camera:{:.2f}, ", camera);
			std::print(std::cerr, "normal:{:.2f}, ", normal);
		}

		// diffuse
		double normalDotLight = glm::dot(normal, glm::normalize(lightDir));
		if (debugFrame) std::print(std::cerr, "ndl:{:.2f}, ", normalDotLight);
		if (normalDotLight > 0) { // ignore lights behind the surface
			intensity += (light->getIntensity() * normalDotLight)
			             / (glm::length(normal) * glm::length(lightDir));
			if (debugFrame) std::print(std::cerr, "intensity diffuse:{:.2f}, ", intensity);
		} else if (debugFrame) {
			std::print(std::cerr, "skipped diffuse, ");
		}

		// specular
		if (specular != -1) {
			dvec3 reflected = reflectRay(lightDir, normal);
			double reflectedDotExit = glm::dot(reflected, -camToPoint);
			if (reflectedDotExit > 0) {
				intensity +=
				    light->getIntensity()
				    * pow(reflectedDotExit / (glm::length(reflected) * glm::length(camToPoint)),
				          specular);
				if (debugFrame) {
					std::print(std::cerr, "rde:{:.2f}, ", reflectedDotExit);
					std::print(std::cerr, "reflected:{:.2f}, ", reflected);
					std::print(std::cerr, "specular:{:.2f}, ", specular);
					std::print(std::cerr, "intensity specular:{:.2f}, ", intensity);
				}
			} else if (debugFrame) {
				std::print(std::cerr, "skipped specular, ");
			}
		} else if (debugFrame) {
			std::print(std::cerr, "no specular");
		}

		if (debugFrame) std::print(std::cerr, "], ");
	}

	return std::min(intensity, 1.0);
}

void drawLine(SextantDrawing& canvas, ivec2 p0, ivec2 p1, const Color color) {
	if (std::abs(p0.x - p1.x) > std::abs(p0.y - p1.y)) { // line is horizontalish
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

// interpolated on the y axis
struct InterpElems {
	double x; // x value (duh)
	double invDepth; // 1 / depth
	double normalX; // interpolate x, y, and z of the normal vector
	double normalY;
	double normalZ;
};

// interpolated for each row (the x-axis)
struct RowElems {
	double invDepth; // 1 / depth
	double normalX; // interpolate x, y, and z of the normal vector
	double normalY;
	double normalZ;
};

// this function is a mess
void drawFilledTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
                        Triangle<ivec2> points, Triangle<float> depth, Triangle<dvec3> normals,
                        const Color color, const double ambientLight, const double specular,
                        const Camera& camera, const dmat4& camToObj,
                        const std::vector<std::shared_ptr<Light>>& lights) {
	// sort top to bottom, so p0.y < p1.y < p2.y
	// we don't care about ordering clockwise anymore, so this is fine
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

	ivec2 canvasSize{canvas.getWidth(), canvas.getHeight()};

	dvec3 camPosInObjCoords = canonicalize(camToObj * toHomogenous(origin));
	if (debugFrame)
		std::println(std::cerr, "drawing tri: {}, cam @ {:.2f}", points, camPosInObjCoords);

		// easier syntax
#define interpField(vec, field, x0, y0, x1, y1) \
	do { \
		interpolateField<decltype(vec)::value_type>((vec), &decltype(vec)::value_type::field, \
		                                            (x0), (y0), (x1), (y1)); \
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
				if (debugFrame) std::print(std::cerr, "pixel ({}, {}): ", x, y);

				dvec3 viewportPoint{(x * camera.viewportWidth) / canvasSize.x,
				                    (y * camera.viewportHeight) / canvasSize.y,
				                    camera.viewportDistance};

				double depth = 1. / invDepth;
				double scaleFactor = depth / camera.viewportDistance;
				// because the camera is at {0, 0, 0},
				// the camera to point vector is the same as the point itself
				dvec3 camToDrawnPoint{viewportPoint.x * scaleFactor, viewportPoint.y * scaleFactor,
				                      0 /* set to a placeholder */};
				camToDrawnPoint.z =
				    sqrt(pow(depth, 2) - pow(camToDrawnPoint.x, 2) - pow(camToDrawnPoint.y, 2));

				// point in object-relative coordinates
				dvec3 pointObj = canonicalize(camToObj * toHomogenous(camToDrawnPoint));
				if (debugFrame) std::print(std::cerr, "pointObj:{:.2f}, ", pointObj);

				dvec3 normal = glm::normalize(dvec3{pixel.normalX, pixel.normalY, pixel.normalZ});
				double lighting = computeLighting(pointObj, camPosInObjCoords, normal, specular,
				                                  ambientLight, lights);
				RGBA newColor = color.color * lighting;

				// #ifndef NDEBUG
				//				ivec2 reversed = canonicalize(toHomogenous(camToDrawnPoint)
				//				                              *
				// camera.viewportTransform(canvasSize));
				// assertEq(glm::to_string(reversed), glm::to_string(ivec2{x, y}),
				//"Reversed does not match."); #endif

				if (debugFrame)
					std::println(
					    std::cerr,
					    "normal: {:.2f}, cam to point: {:.2f}, depth: {:.2f}, lighting: {:.2f}",
					    normal, camToDrawnPoint, depth, lighting);

				putBufPixel(depthBuffer, {x, y}, (float)invDepth);
				putPixel(canvas, SextantCoord(y, x), Color(color.category, newColor));
			}
		}
	}
#undef interpField
}

void renderTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
                    const Triangle<ivec2>& triangle, const Triangle<float>& depth,
                    const Triangle<dvec3> normals, const Color color, const double ambientLight,
                    const double specular, const Camera& camera, const dmat4& camToObj,
                    const std::vector<std::shared_ptr<Light>> lights) {
	// randomize colors so tris can be distinguished
	Color color2 = color;
	if (debugFrame) {
		color2 = Color{
		    color.category,
		    {static_cast<uchar>(rand() % 255), static_cast<uchar>(rand() % 255),
		               static_cast<uchar>(rand() % 255), 255}
        };
	}

	drawFilledTriangle(canvas, depthBuffer, triangle, depth, normals, color2, ambientLight,
	                   specular, camera, camToObj, lights);
}
