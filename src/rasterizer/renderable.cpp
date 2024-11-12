#include "renderable.hpp"
#include <stdexcept>

Cube::Cube(std::initializer_list<dvec3> init) {
	if (init.size() != 8)
		throw std::logic_error("A cube must have 8 verticies.");

	char i = 0;
	for (const dvec3 vertex: init) {
		this->verticies.at(i) = vertex;
		i++;
	}
}

dvec3 Cube::getPoint(const uint index) const {
	return this->verticies.at(index);
}

std::vector<Triangle> Cube::getTriangles() const {
	return {this->triangleIndexes.begin(), this->triangleIndexes.end()};
}

