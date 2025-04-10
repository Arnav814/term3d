#ifndef SHAPEBUILDERS_HPP
#define SHAPEBUILDERS_HPP

#include "renderable.hpp"
#include "structures.hpp"
#include "../drawing/setColor.hpp"

void splitTriangle(Object3D& object, uint triangleIdx, dvec3 newPoint);

Object3D makeSphere(Color color, double specular, double radius, uint iterations);

// join duplicated points, using tolerance as the threshold for points to join
void combinePoints(Object3D& object, const double tolerance = 0.001);

void makePyramid(Object3D& object, const Color& color, const dvec3& baseCenter,
                 const dvec3& peakPoint, const dvec3& baseSide);

#endif /* SHAPEBUILDERS_HPP */
