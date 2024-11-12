#include "rasterizer.hpp"
#include <boost/range/join.hpp>
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_double3.hpp>
#include <limits>
#include <memory>
#include <vector>
#include "renderable.hpp"

// TODO: a lot of this stuff should be passed by reference

using glm::dvec3, glm::ivec2;

const double viewportWidth = 1.0;
const double viewportHeight = 1.0;
const double viewportDistance = 1.0;
const Color BACKGROUND_COLOR = Color(Category(true, 8), RGBA(0, 0, 0, 255));

#define cwhite Color(Category(true, 8), RGBA(255, 255, 255, 255))
#define cred Color(Category(true, 9), RGBA(255, 0, 0, 255))
#define cgreen Color(Category(true, 10), RGBA(0, 255, 0, 255))
#define cblue Color(Category(true, 11), RGBA(0, 0, 255, 255))
#define cblack Color(Category(true, 12), RGBA(0, 0, 0, 255))

// assumes x0 <= x1
std::vector<double> interpolate(const int x0, const double y0, const int x1, const double y1) {
		// TODO: do these really need to be doubles?
	if (x0 == x1) // for only one point
		return {y0};

	std::vector<double> out;
	out.reserve(x1 - x0 + 1);

	double slope = (double) (y1 - y0) / (x1 - x0);
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

void drawWireframeTriangle(SextantDrawing& canvas, const ivec2 p0, const ivec2 p1, const ivec2 p2, const Color color) {
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

void drawShadedTriangle(SextantDrawing& canvas, ivec2 p0, ivec2 p1, ivec2 p2,
		TemplatedTriangle<float> intensities, const Color color) {
	// p0, p1, and p2 = intensities a, b, and c
	// sort top to bottom, so p0.y < p1.y < p2.y
	if (p1.y < p0.y) {std::swap(p1, p0); std::swap(intensities.b, intensities.a);}
	if (p2.y < p0.y) {std::swap(p2, p0); std::swap(intensities.c, intensities.a);}
	if (p2.y < p1.y) {std::swap(p2, p1); std::swap(intensities.c, intensities.b);}

	// Compute the x coordinates and h values of the triangle edges
	auto x01 = interpolate(p0.y, p0.x, p1.y, p1.x);
	auto h01 = interpolate(p0.y, intensities.a, p1.y, intensities.b);

	auto x12 = interpolate(p1.y, p1.x, p2.y, p2.x);
	auto h12 = interpolate(p1.y, intensities.b, p2.y, intensities.c);

	auto x02 = interpolate(p0.y, p0.x, p2.y, p2.x);
	auto h02 = interpolate(p0.y, intensities.a, p2.y, intensities.c);

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

// converts a 3d point on the viewplane to a 2d point
ivec2 viewportToCanvas(const SextantDrawing& canvas, const dvec3 p) {
	return ivec2(p.x * static_cast<double>(canvas.getHeight() / viewportHeight),
		p.y * static_cast<double>(canvas.getWidth() / viewportWidth));
}

// converts a 3d point to a 2d canvas point
ivec2 projectVertex(const SextantDrawing& canvas, const dvec3 v) {
	return viewportToCanvas(canvas, {v.x * viewportDistance / v.z, v.y * viewportDistance / v.z, viewportDistance});
}

void rasterRenderLoop(WindowedDrawing& rawCanvas, const bool& exit_requested, std::function<int()> refresh) {
	int minDimension = std::min(rawCanvas.getHeight(), rawCanvas.getWidth()/2);
	SextantDrawing canvas(minDimension, minDimension*2);
	dvec3 origin = dvec3();

	while (not exit_requested) {
		drawShadedTriangle(canvas, {-10, -10}, {100, 25}, {50, -30},
			{0.1, 1.0, 0.5}, cgreen);

		rawCanvas.clear();
		rawCanvas.insert(SextantCoord(0, 0), canvas);
		rawCanvas.render();
		refresh();
		sleep(1);
	}
}

