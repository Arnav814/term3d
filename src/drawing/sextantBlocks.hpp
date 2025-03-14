#ifndef SEXTANTBLOCKS_HPP
#define SEXTANTBLOCKS_HPP

#include <array>
#include <boost/multi_array.hpp>
#include <initializer_list>
#include <notcurses/notcurses.h>

#include "../extraAssertions.hpp"
#include "coord2d.hpp"
#include "setColor.hpp"

void testAllSextants();

typedef boost::multi_array<Color, 2> drawing_type;

class SextantDrawing {
  private:
	drawing_type drawing;

  protected:
	[[nodiscard]] Color getWithFallback(const SextantCoord& coord, const Color fallback) const;
	[[nodiscard]] charArray<Color> getChar(const SextantCoord& topLeft) const;

  public:
	SextantDrawing(std::initializer_list<std::initializer_list<Color>> init);
	SextantDrawing(const int height, const int width);

	[[nodiscard]] int getWidth() const { return this->drawing[0].size(); }

	[[nodiscard]] int getHeight() const { return this->drawing.size(); }

	[[nodiscard]] SextantCoord getSize() const {
		return SextantCoord(this->getHeight(), this->getWidth());
	}

	[[nodiscard]] Color get(const SextantCoord& coord) const;
	[[nodiscard]] CoordIterator<SextantCoord> getIterator() const;
	void clear();
	void clear(const Color& color);
	void set(const SextantCoord& coord, const Color setTo);
	void trySet(const SextantCoord& coord, const Color setTo);
	void resize(int newY, int newX);
	void insert(const SextantCoord& topLeft, const SextantDrawing& toCopy);
	void debugPrint() const;
};

// converts from origin at center to origin at top left
inline void putPixel(SextantDrawing& canvas, const SextantCoord coord, const Color color) {
	SextantCoord translated{canvas.getHeight() / 2 - coord.y, canvas.getWidth() / 2 + coord.x};
	canvas.trySet(translated, color);
}

class WindowedDrawing : public SextantDrawing {
  private:
	ncplane* win;

  public:
	WindowedDrawing(ncplane* win);
	void autoRescale();
	void render() const;
};

#endif /* SEXTANTBLOCKS_HPP */
