#ifndef RAYTRACER_HPP
#define RAYTRACER_HPP
#include "../drawing/sextantBlocks.hpp"

void renderLoop(SextantDrawing canvas, const bool& exit_requested, std::function<int()> refresh);

#endif /* RAYTRACER_HPP */
