#include "triangles.hpp"
#include "common.hpp"
#include <boost/multi_array.hpp>
#include <limits>

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

void drawFilledTriangle(SextantDrawing& canvas, ivec2 p0, ivec2 p1, ivec2 p2, const Color color) {
	// sort top to bottom, so p0.y < p1.y < p2.y
	if (p1.y < p0.y) std::swap(p1, p0);
	if (p2.y < p0.y) std::swap(p2, p0);
	if (p2.y < p1.y) std::swap(p2, p1);

	// indexes represent y-values
	std::vector<double> shortSide1 = interpolate(p0.y, p0.x, p1.y, p1.x);
	std::vector<double> shortSide2 = interpolate(p1.y, p1.x, p2.y, p2.x);
	std::vector<double> longSide = interpolate(p0.y, p0.x, p2.y, p2.x);

	// combine vectors
	shortSide1.pop_back();
	// from here, shortSide1 is actually both short sides combined
	shortSide1.reserve(shortSide1.size() + shortSide2.size());
	// TODO: don't actually copy here, use boost::join() or something
	shortSide1.insert(shortSide1.end(), shortSide2.begin(), shortSide2.end());

	std::unique_ptr<std::vector<double>> xLeft;
	std::unique_ptr<std::vector<double>> xRight;
	int middleIndex = longSide.size() / 2; // some arbitrary index
	if (longSide.at(middleIndex) < shortSide1.at(middleIndex)) {
		xLeft = std::make_unique<std::vector<double>>(longSide);
		xRight = std::make_unique<std::vector<double>>(shortSide1);
	} else {
		xLeft = std::make_unique<std::vector<double>>(shortSide1);
		xRight = std::make_unique<std::vector<double>>(longSide);
	}

	for (int y = p0.y; y <= p2.y; y++) {
		for (int x = xLeft->at(y - p0.y); x <= xRight->at(y - p0.y); x++) {
			putPixel(canvas, SextantCoord(y, x), color);
		}
	}
}

// this function is a mess
void drawFilledTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
                        Triangle<ivec2> points, Triangle<float> depth, const Color color) {
	// sort top to bottom, so p0.y < p1.y < p2.y
	if (points[1].y < points[0].y) {
		std::swap(depth[1], depth[0]);
		std::swap(points[1], points[0]);
	}
	if (points[2].y < points[0].y) {
		std::swap(depth[2], depth[0]);
		std::swap(points[2], points[0]);
	}
	if (points[2].y < points[1].y) {
		std::swap(depth[2], depth[1]);
		std::swap(points[2], points[1]);
	}

	// indexes represent y-values
	std::vector<double> shortSide1 =
	    interpolate(points[0].y, points[0].x, points[1].y, points[1].x);
	std::vector<double> shortSide2 =
	    interpolate(points[1].y, points[1].x, points[2].y, points[2].x);
	std::vector<double> longSide = interpolate(points[0].y, points[0].x, points[2].y, points[2].x);

	// interpolated reciprocal of depth
	std::vector<double> shortSideDepth1 =
	    interpolate(points[0].y, 1.0 / depth[0], points[1].y, 1.0 / depth[1]);
	std::vector<double> shortSideDepth2 =
	    interpolate(points[1].y, 1.0 / depth[1], points[2].y, 1.0 / depth[2]);
	std::vector<double> longSideDepth =
	    interpolate(points[0].y, 1.0 / depth[0], points[2].y, 1.0 / depth[2]);

	// combine vectors
	shortSide1.pop_back();
	// from here, shortSide1 is actually both short sides combined
	shortSide1.reserve(shortSide1.size() + shortSide2.size());
	// TODO: don't actually copy here, use boost::join() or something
	shortSide1.insert(shortSide1.end(), shortSide2.begin(), shortSide2.end());

	// same thing
	shortSideDepth1.pop_back();
	shortSideDepth1.reserve(shortSideDepth1.size() + shortSideDepth2.size());
	shortSideDepth1.insert(shortSideDepth1.end(), shortSideDepth2.begin(), shortSideDepth2.end());

	std::unique_ptr<std::vector<double>> xLeft;
	std::unique_ptr<std::vector<double>> xRight;
	std::unique_ptr<std::vector<double>> dLeft;
	std::unique_ptr<std::vector<double>> dRight;
	int middleIndex = longSide.size() / 2; // some arbitrary index
	if (longSide.at(middleIndex) == shortSide1.at(middleIndex) and middleIndex != 0)
		middleIndex--; // fix problems if length==2

	if (longSide.at(middleIndex) < shortSide1.at(middleIndex)) {
		xLeft = std::make_unique<std::vector<double>>(longSide);
		xRight = std::make_unique<std::vector<double>>(shortSide1);
		dLeft = std::make_unique<std::vector<double>>(longSideDepth);
		dRight = std::make_unique<std::vector<double>>(shortSideDepth1);
	} else {
		xLeft = std::make_unique<std::vector<double>>(shortSide1);
		xRight = std::make_unique<std::vector<double>>(longSide);
		dLeft = std::make_unique<std::vector<double>>(shortSideDepth1);
		dRight = std::make_unique<std::vector<double>>(longSideDepth);
	}

	for (int y = points[0].y; y <= points[2].y; y++) {
		int rowLeftX = round(xLeft->at(y - points[0].y));
		int rowRightX = round(xRight->at(y - points[0].y));
		assertGtEq(rowRightX, rowLeftX, "right is left of left");

		double rowLeftDepth = dLeft->at(y - points[0].y);
		double rowRightDepth = dRight->at(y - points[0].y);
		std::vector<double> rowDepth =
		    interpolate(rowLeftX, rowLeftDepth, rowRightX, rowRightDepth);

		for (int x = rowLeftX; x <= rowRightX; x++) {
			if (getBufPixel(depthBuffer, {x, y}, std::numeric_limits<float>::infinity())
			    < rowDepth.at(x - rowLeftX)) {
				putBufPixel(depthBuffer, {x, y}, (float)rowDepth.at(x - rowLeftX));
				putPixel(canvas, SextantCoord(y, x), color);
			}
		}
	}
}

void drawShadedTriangle(SextantDrawing& canvas, ivec2 p0, ivec2 p1, ivec2 p2,
                        Triangle<float> intensities, const Color color) {
	// p0, p1, and p2 = intensities a, b, and c
	// sort top to bottom, so p0.y < p1.y < p2.y
	if (p1.y < p0.y) {
		std::swap(p1, p0);
		std::swap(intensities[1], intensities[0]);
	}
	if (p2.y < p0.y) {
		std::swap(p2, p0);
		std::swap(intensities[2], intensities[0]);
	}
	if (p2.y < p1.y) {
		std::swap(p2, p1);
		std::swap(intensities[2], intensities[1]);
	}

	// Compute the x coordinates and h values of the triangle edges
	auto x01 = interpolate(p0.y, p0.x, p1.y, p1.x);
	auto h01 = interpolate(p0.y, intensities[0], p1.y, intensities[1]);

	auto x12 = interpolate(p1.y, p1.x, p2.y, p2.x);
	auto h12 = interpolate(p1.y, intensities[1], p2.y, intensities[2]);

	auto x02 = interpolate(p0.y, p0.x, p2.y, p2.x);
	auto h02 = interpolate(p0.y, intensities[0], p2.y, intensities[2]);

	// Concatenate the short sides
	x01.pop_back();
	x01.insert(x01.end(), x12.begin(), x12.end());
#define x012 x01 // from here, x01 is reused for both x01 and x12 joined together

	// h means intensity
	h01.pop_back();
	h01.insert(h01.end(), h12.begin(), h12.end());
#define h012 h01 // same for h

	// Determine which is left and which is right
	uint middleIndex = x012.size() / 2; // some arbitrary index
	std::unique_ptr<std::vector<double>> xLeft;
	std::unique_ptr<std::vector<double>> xRight;
	std::unique_ptr<std::vector<double>> hLeft;
	std::unique_ptr<std::vector<double>> hRight;
	if (x02.at(middleIndex) < x012.at(middleIndex)) {
		xLeft = std::make_unique<std::vector<double>>(x02);
		hLeft = std::make_unique<std::vector<double>>(h02);

		xRight = std::make_unique<std::vector<double>>(x012);
		hRight = std::make_unique<std::vector<double>>(h012);
	} else {
		xLeft = std::make_unique<std::vector<double>>(x012);
		hLeft = std::make_unique<std::vector<double>>(h012);

		xRight = std::make_unique<std::vector<double>>(x02);
		hRight = std::make_unique<std::vector<double>>(h02);
	}

	// Draw the horizontal segments
	for (int y = p0.y; y <= p2.y; y++) {
		double rowLeftX = xLeft->at(y - p0.y);
		double rowRightX = xRight->at(y - p0.y);

		auto rowHVals = interpolate(rowLeftX, hLeft->at(y - p0.y), rowRightX, hRight->at(y - p0.y));
		for (int x = rowLeftX; x <= rowRightX; x++) {
			RGBA shadedColor = color.color * rowHVals.at(x - rowLeftX);
			putPixel(canvas, SextantCoord(y, x), Color(color.category, shadedColor));
		}
	}

#undef x012
#undef h012
}

void renderTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
                    const Triangle<ivec2>& triangle, const Triangle<float>& depth, Color color) {
	// std::println(std::cerr, "p0: {}, p1: {}, p2: {} ", glm::to_string(triangle.a),
	// 	glm::to_string(triangle.b), glm::to_string(triangle.c));
	drawFilledTriangle(canvas, depthBuffer, triangle, depth, color);
}
