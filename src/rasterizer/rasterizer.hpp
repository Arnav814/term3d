#ifndef RASTERIZER_HPP
#define RASTERIZER_HPP
#include "../drawing/sextantBlocks.hpp"

void rasterRenderLoop(WindowedDrawing& rawCanvas, const bool& exit_requested, std::function<int()> refresh);

#endif /* RASTERIZER_HPP */
