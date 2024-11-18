#include "renderable.hpp"
#include <stdexcept>
#include <vector>
#include "../extraAssertions.hpp"

Cube::Cube(const std::initializer_list<dvec3> init, const Color color) {
	assertEq(init.size(), 8, "A cube must have 8 verticies.");

	char i = 0;
	for (const dvec3 vertex: init) {
		this->verticies.at(i) = vertex;
		i++;
	}

	this->colors = std::array<Color, 12>{}; // initialize it
	this->setAllColors(color);
}

Cube::Cube(const std::array<dvec3, 8> init, const Color color) {
	this->verticies = init;
	this->colors = std::array<Color, 12>{}; // initialize it
	this->setAllColors(color);
}

void Cube::setAllColors(const Color color) {
	for (uint i = 0; i < colors.size(); i++) {
		this->colors.at(i) = color;
	}

}

void Cube::setColor(const uint index, const Color color) {
	this->colors.at(index) = color;
}

Color Cube::getColor(const uint index) const {
	return this->colors.at(index);
}

void Cube::setVertex(const uint index, const dvec3 newVertex) {
	this->verticies.at(index) = newVertex;
}

dvec3 Cube::getVertex(const uint index) const {
	return this->verticies.at(index);
}

std::vector<dvec3> Cube::getPoints() const {
	return {this->verticies.begin(), this->verticies.end()};
}

uint Cube::getTriangleCount() const {
	return this->triangleIndexes.size();
}

std::vector<ColoredTriangle> Cube::getTriangles() const {
	std::vector<ColoredTriangle> out{};
	out.reserve(this->triangleIndexes.size());

	for (uint i = 0; i < this->triangleIndexes.size(); i++) {
		out.push_back(ColoredTriangle{this->triangleIndexes.at(i), this->colors.at(i)});
	}

	return out;
}

