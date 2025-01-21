#include "triangles.hpp"
#include <boost/core/use_default.hpp>
#include <boost/iterator/iterator_categories.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/multi_array.hpp>
#include <limits>
#include "interpolate.hpp"
// #include "common.hpp"

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
	// note: uses interpolated _reciprocal_ of depth
	enum class PointField {X = 0, Depth = 1};

	Collate<2> shortSide1 = {{
		Interpolate{points[0].y, (double) points[0].x, points[1].y, (double) points[1].x},
		Interpolate{points[0].y, 1.0/depth[0], points[1].y, 1.0/depth[1]}
	}};
	Collate<2> shortSide2 = {{
		Interpolate{points[1].y, (double) points[1].x, points[2].y, (double) points[2].x},
		Interpolate{points[1].y, 1.0/depth[1], points[2].y, 1.0/depth[2]}
	}};
	Collate<2> longSide = {{
		Interpolate{points[0].y, (double) points[0].x, points[2].y, (double) points[2].x},
		Interpolate{points[0].y, 1.0/depth[0], points[2].y, 1.0/depth[2]}
	}};

	CollateJoin shortSides = {shortSide1, shortSide2};

	std::unique_ptr<BaseCollate<2>> left;
	std::unique_ptr<BaseCollate<2>> right;
	int middleIndex = longSide.size() / 2; // some arbitrary index
	if (longSide[middleIndex] == shortSide1[middleIndex] and middleIndex != 0) middleIndex--; // fix problems if length==2

	if (longSide[middleIndex] > shortSide1[middleIndex]) {
		left = std::make_unique<Collate<2>>(longSide);
		right = std::make_unique<CollateJoin<2>>(shortSides);
	} else {
		left = std::make_unique<CollateJoin<2>>(shortSides);
		right = std::make_unique<Collate<2>>(longSide);
	}

	auto rowLeftIt = left->beginCommon();
	auto rowRightIt = right->beginCommon();

	for (int y = points[0].y; y <= points[2].y; y++) {
		auto rowLeft = *(*rowLeftIt);
		auto rowRight = *(*rowRightIt);
		assertNotEq(rowLeftIt, left->endCommon(), "Hit end of left.");
		assertNotEq(rowRightIt, right->endCommon(), "Hit end of right.");

		int rowLeftX = round(rowLeft[(uint) PointField::X]);
		int rowRightX = round(rowRight[(uint) PointField::X]);
		std::println(std::cerr, "{}, {}", rowLeftX, rowRightX);
		assertGtEq(rowRightX, rowLeftX, "right is left of left");

		double rowLeftDepth = rowLeft[(uint) PointField::Depth];
		double rowRightDepth = rowRight[(uint) PointField::Depth];
		Interpolate rowDepth = {rowLeftX, rowLeftDepth, rowRightX, rowRightDepth};
		auto rowDepthIt = rowDepth.begin();

		for (int x = rowLeftX; x <= rowRightX; x++) {
			if (getBufPixel(depthBuffer, {x, y}, std::numeric_limits<float>::infinity()) < *rowDepthIt) {
				putBufPixel(depthBuffer, {x, y}, (float) *rowDepthIt);
				putPixel(canvas, SextantCoord(y, x), color);
			}
			++rowDepthIt;
		}

		++(*rowLeftIt);
		++(*rowRightIt);
	}
}

void renderTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer, const Triangle<ivec2>& triangle, const Triangle<float>& depth, Color color) {
	// std::println(std::cerr, "p0: {}, p1: {}, p2: {} ", glm::to_string(triangle.a),
	// 	glm::to_string(triangle.b), glm::to_string(triangle.c));
	drawFilledTriangle(canvas, depthBuffer, triangle, depth, color);
}

