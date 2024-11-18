#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include <vector>
#include <array>
#include <glm/ext/vector_double3.hpp>
#include "../extraAssertions.hpp"
#include "../drawing/setColor.hpp"

using glm::dvec3;

template<typename T> struct Triangle {
	T a;
	T b;
	T c;
};

struct ColoredTriangle {
	Triangle<uint> triangle; // uint is an array index
	Color color;
};

class Renderable {
	public:
		virtual void setAllColors(const Color color) = 0;
		virtual void setColor(const uint index, const Color color) = 0;
		virtual Color getColor(const uint index) const = 0;
		virtual void setVertex(const uint index, const dvec3 newVertex) = 0;
		virtual dvec3 getVertex(const uint index) const = 0;
		virtual std::vector<dvec3> getPoints() const = 0;
		virtual uint getTriangleCount() const = 0;
		virtual std::vector<ColoredTriangle> getTriangles() const = 0;
};

class Cube : public Renderable {
	private:
		std::array<dvec3, 8> verticies;
		std::array<Color, 12> colors;
		constexpr static std::array<Triangle<uint>, 12> triangleIndexes = { // same for every cube
			Triangle<uint>{0, 1, 2},
			Triangle<uint>{0, 2, 3},
			Triangle<uint>{4, 0, 3},
			Triangle<uint>{4, 3, 7},
			Triangle<uint>{5, 4, 7},
			Triangle<uint>{5, 7, 6},
			Triangle<uint>{1, 5, 6},
			Triangle<uint>{1, 6, 2},
			Triangle<uint>{4, 5, 1},
			Triangle<uint>{4, 1, 0},
			Triangle<uint>{2, 6, 7},
			Triangle<uint>{2, 7, 3}
		};
	public:
		Cube(const std::initializer_list<dvec3> init, const Color color);
		Cube(const std::array<dvec3, 8> init, const Color color);
		Cube() : Cube({
				{sNaN_d, sNaN_d, sNaN_d},
				{sNaN_d, sNaN_d, sNaN_d},
				{sNaN_d, sNaN_d, sNaN_d},
				{sNaN_d, sNaN_d, sNaN_d},
				{sNaN_d, sNaN_d, sNaN_d},
				{sNaN_d, sNaN_d, sNaN_d},
				{sNaN_d, sNaN_d, sNaN_d},
				{sNaN_d, sNaN_d, sNaN_d}
			}, Color()
		) {}
		virtual void setAllColors(const Color color);
		virtual void setColor(const uint index, const Color color);
		virtual Color getColor(const uint index) const;
		virtual void setVertex(const uint index, const dvec3 newVertex);
		virtual dvec3 getVertex(const uint index) const;
		virtual std::vector<dvec3> getPoints() const;
		virtual uint getTriangleCount() const;
		virtual std::vector<ColoredTriangle> getTriangles() const;
};

#endif /* RENDERABLE_HPP */
