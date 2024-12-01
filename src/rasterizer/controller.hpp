#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP
#include "rasterizer.hpp"

// this code bridges the renderer and notcurses
// it also lets you move, which is nice

void renderLoop(notcurses* nc, ncplane* plane, const bool& exitRequested);

#endif /* CONTROLLER_HPP */
