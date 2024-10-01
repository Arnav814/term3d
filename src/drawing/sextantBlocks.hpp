#ifndef SEXTANTBLOCKS_HPP
#define SEXTANTBLOCKS_HPP

#include <array>
#include <boost/multi_array.hpp>
#include <initializer_list>

#include "../extraAssertions.hpp"
#include "coord2d.hpp"

typedef unsigned char colorType;
typedef unsigned char priorityType;
template <typename storeAs> using charArray = std::array<std::array<storeAs, 3>, 2>;
enum class OverrideStyle {Nonzero, Always, Priority};

struct PriorityColor {
	colorType color;
	priorityType priority;

	PriorityColor(colorType color, priorityType priority) {
		this->color = color; this->priority = priority;
	}

	PriorityColor() {
		this->color = 0; this->priority = 0;
	}
};

void testAllSextants();

typedef boost::multi_array<PriorityColor, 2> drawing_type;

class SextantDrawing {
	private:
		drawing_type drawing;
		[[nodiscard]] PriorityColor getWithFallback(const SextantCoord& coord, const PriorityColor fallback) const;
		[[nodiscard]] charArray<PriorityColor> getChar(const SextantCoord& topLeft) const;
	
	public:
		SextantDrawing(std::initializer_list<std::initializer_list<PriorityColor>> init);
		SextantDrawing(const int height, const int width);
		[[nodiscard]] int getWidth() const {return this->drawing[0].size();}
		[[nodiscard]] int getHeight() const {return this->drawing.size();}
		[[nodiscard]] PriorityColor get(const SextantCoord& coord) const;
		[[nodiscard]] CoordIterator<SextantCoord> getIterator() const;
		void clear();
		void set(const SextantCoord& coord, const PriorityColor setTo);
		void trySet(const SextantCoord& coord, const PriorityColor setTo);
		void resize(int newY, int newX);
		void insert(const SextantCoord& topLeft, const SextantDrawing& toCopy, const OverrideStyle overrideStyle);
		void render(const CharCoord& topLeft) const;
		void debugPrint() const;
};

std::pair<colorType, colorType> getTrimmedColors(const charArray<PriorityColor>& arrayChar);

#endif /* SEXTANTBLOCKS_HPP */
