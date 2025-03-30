#ifndef TRIANGLES_HPP
#define TRIANGLES_HPP
#include "../drawing/sextantBlocks.hpp"
#include "renderable.hpp"
#include <glm/ext/vector_double3.hpp>
#include <glm/ext/vector_int2.hpp>
#include <memory>

using glm::ivec2, glm::dvec3;

// assumes x0 <= x1
std::vector<double> interpolate(const int x0, const double y0, const int x1, const double y1);
void drawLine(SextantDrawing& canvas, ivec2 p0, ivec2 p1, const Color color);
void renderTriangle(SextantDrawing& canvas, boost::multi_array<float, 2>& depthBuffer,
                    const Triangle<ivec2>& triangle, const Triangle<float>& depth,
                    const Triangle<dvec3> normals, const Color color, const double ambientLight,
                    const double specular, const Camera& camera, const dmat4& camToObj,
                    const std::vector<std::shared_ptr<Light>> lights);

#endif /* TRIANGLES_HPP */
