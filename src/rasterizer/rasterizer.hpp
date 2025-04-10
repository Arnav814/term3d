#ifndef RASTERIZER_HPP
#define RASTERIZER_HPP
#include "../drawing/sextantBlocks.hpp"
#include "scene.hpp"
#include <glm/exponential.hpp>

using glm::dvec3, glm::dvec4, glm::ivec2, glm::dmat4;

void renderScene(SextantDrawing& canvas, const Scene& scene);

#endif /* RASTERIZER_HPP */
