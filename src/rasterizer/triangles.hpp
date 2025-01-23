#ifndef TRIANGLES_HPP
#define TRIANGLES_HPP
#include "../drawing/sextantBlocks.hpp"
#include <glm/ext/vector_int2.hpp>
#include <glm/ext/vector_double3.hpp>
#include "renderable.hpp"

using glm::ivec2, glm::dvec3;

// assumes x0 <= x1
std::vector<double> interpolate(const int x0, const double y0, const int x1, const double y1);
void drawLine(SextantDrawing& canvas, ivec2 p0, ivec2 p1, const Color color);
void drawWireframeTriangle(SextantDrawing& canvas, const ivec2 p0, const ivec2 p1, const ivec2 p2, const Color color);
void drawFilledTriangle(SextantDrawing& canvas, ivec2 p0, ivec2 p1, ivec2 p2, const Color color);
void drawFilledTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
		Triangle<ivec2> points, Triangle<float> distances, const Color color);
void drawShadedTriangle(SextantDrawing& canvas, ivec2 p0, ivec2 p1, ivec2 p2,
		Triangle<float> intensities, const Color color);
void renderTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>&
		depthBuffer, const Triangle<ivec2>& triangle, const Triangle<float>&
		depth, Color color);

#endif /* TRIANGLES_HPP */
