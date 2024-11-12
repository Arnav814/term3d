#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include <vector>
#include <array>
#include <glm/ext/vector_double3.hpp>
#include "../extraAssertions.hpp"

using glm::dvec3;

template<typename T> struct TemplatedTriangle {
	T a;
	T b;
	T c;
};
using Triangle = TemplatedTriangle<uint>; // uint is an array index

class Renderable {
	public:
		virtual dvec3 getPoint(const uint index) const = 0;
		virtual std::vector<Triangle> getTriangles() const = 0;
};

class Cube : public Renderable {
	private:
		std::array<dvec3, 8> verticies;
		constexpr static std::array<Triangle, 12> triangleIndexes = { // same for every cube
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
			Triangle{0, 0, 0},
		};
	public:
		Cube(std::initializer_list<dvec3> init);
		Cube() : Cube({
			{sNaN_d, sNaN_d, sNaN_d},
			{sNaN_d, sNaN_d, sNaN_d},
			{sNaN_d, sNaN_d, sNaN_d},
			{sNaN_d, sNaN_d, sNaN_d},
			{sNaN_d, sNaN_d, sNaN_d},
			{sNaN_d, sNaN_d, sNaN_d},
			{sNaN_d, sNaN_d, sNaN_d},
			{sNaN_d, sNaN_d, sNaN_d}
		}) {}
		virtual dvec3 getPoint(const uint index) const;
		virtual std::vector<Triangle> getTriangles() const;
};

#endif /* RENDERABLE_HPP */
