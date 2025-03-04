#include "sextantBlocks.hpp"

#include <array>
#include <bitset>
#include <boost/type_traits/extent.hpp>
#include <cassert>
#include <cmath>
#include <codecvt>
#include <csignal>
#include <initializer_list>
#include <iostream>
#include <locale>
#include <unistd.h>
#include <unordered_map>

#include "../extraAssertions.hpp"
#include "coord2d.hpp"
#include "quantizeChars.hpp"
#include "setColor.hpp"

char packArray(charArray<bool>& myArray) {
	return (((myArray[1][2] != 0 ? 1 : 0) << 0) + ((myArray[1][1] != 0 ? 1 : 0) << 1)
	        + ((myArray[1][0] != 0 ? 1 : 0) << 2) + ((myArray[0][2] != 0 ? 1 : 0) << 3)
	        + ((myArray[0][1] != 0 ? 1 : 0) << 4) + ((myArray[0][0] != 0 ? 1 : 0) << 5));
}

// Numbered:
// 0 3
// 1 4
// 2 5

std::unordered_map<char, wchar_t> sextantMap{
    {0b000000, L'⠀'},
    {0b000001, L'🬞'},
    {0b000010, L'🬇'},
    {0b000011, L'🬦'},
    {0b000100, L'🬁'},
    {0b000101, L'🬠'},
    {0b000110, L'🬉'},
    {0b000111, L'▐'},
    {0b001000, L'🬏'},
    {0b001001, L'🬭'},
    {0b001010, L'🬖'},
    {0b001011, L'🬵'},
    {0b001100, L'🬑'},
    {0b001101, L'🬯'},
    {0b001110, L'🬘'},
    {0b001111, L'🬷'},
    {0b010000, L'🬃'},
    {0b010001, L'🬢'},
    {0b010010, L'🬋'},
    {0b010011, L'🬩'},
    {0b010100, L'🬅'},
    {0b010101, L'🬤'},
    {0b010110, L'🬍'},
    {0b010111, L'🬫'},
    {0b011000, L'🬓'},
    {0b011001, L'🬱'},
    {0b011010, L'🬚'},
    {0b011011, L'🬹'},
    {0b011100, L'🬔'},
    {0b011101, L'🬳'},
    {0b011110, L'🬜'},
    {0b011111, L'🬻'},
    {0b100000, L'🬀'},
    {0b100001, L'🬟'},
    {0b100010, L'🬈'},
    {0b100011, L'🬧'},
    {0b100100, L'🬂'},
    {0b100101, L'🬡'},
    {0b100110, L'🬊'},
    {0b100111, L'🬨'},
    {0b101000, L'🬐'},
    {0b101001, L'🬮'},
    {0b101010, L'🬗'},
    {0b101011, L'🬶'},
    {0b101100, L'🬒'},
    {0b101101, L'🬰'},
    {0b101110, L'🬙'},
    {0b101111, L'🬸'},
    {0b110000, L'🬄'},
    {0b110001, L'🬣'},
    {0b110010, L'🬌'},
    {0b110011, L'🬪'},
    {0b110100, L'🬆'},
    {0b110101, L'🬥'},
    {0b110110, L'🬎'},
    {0b110111, L'🬬'},
    {0b111000, L'▌'},
    {0b111001, L'🬲'},
    {0b111010, L'🬛'},
    {0b111011, L'🬺'},
    {0b111100, L'🬕'},
    {0b111101, L'🬴'},
    {0b111110, L'🬝'},
    {0b111111, L'█'}
};

SextantDrawing::SextantDrawing(const int height, const int width) {
	assertGtEq(height, 0, "height must be positive");
	assertGtEq(width, 0, "width must be positive");
	this->resize(height, width);
}

SextantDrawing::SextantDrawing(std::initializer_list<std::initializer_list<Color>> init) {
	uint height = init.size();
	uint width;
	if (height == 0) width = 0;
	else width = init.begin()->size();

	this->drawing.resize(boost::extents[height][width]);

	int x = 0, y = 0;
	for (const auto& list : init) {
		assertEq(width, list.size(), "All sub-lists must have the same size");
		x = 0;
		for (const auto& elem : list) {
			this->set(SextantCoord(y, x), elem);
			x++;
		}
		y++;
	}
}

[[nodiscard]] Color SextantDrawing::get(const SextantCoord& coord) const {
	assertBetweenHalfOpen(0, coord.y, this->getHeight(), "Height out of range in get");
	assertBetweenHalfOpen(0, coord.x, this->getWidth(), "Width out of range in get");
	return this->drawing[coord.y][coord.x];
}

[[nodiscard]] Color SextantDrawing::getWithFallback(const SextantCoord& coord,
                                                    const Color fallback) const {
	if (coord.y < 0 || coord.y >= getHeight() || coord.x < 0 || coord.x >= getWidth())
		return fallback;
	else return this->get(coord);
}

void SextantDrawing::set(const SextantCoord& coord, const Color setTo) {
	assertBetweenHalfOpen(0, coord.y, (int)this->drawing.size(), "Height out of range in set");
	assertBetweenHalfOpen(0, coord.x, (int)this->drawing[coord.y].size(),
	                      "Width out of range in set");
	this->drawing[coord.y][coord.x] = setTo;
}

void SextantDrawing::trySet(const SextantCoord& coord, const Color setTo) {
	if (coord.y < 0 || coord.y >= getHeight() || coord.x < 0 || coord.x >= getWidth()) return;
	else this->set(coord, setTo);
}

[[nodiscard]] CoordIterator<SextantCoord> SextantDrawing::getIterator() const {
	return CoordIterator(SextantCoord(0, 0),
	                     SextantCoord(this->getHeight() - 1, this->getWidth() - 1));
}

void SextantDrawing::clear() {
	this->clear(Color());
}

void SextantDrawing::clear(const Color& color) {
	for (SextantCoord coord : this->getIterator()) {
		this->set(coord, color);
	}
}

void SextantDrawing::resize(int newY, int newX) {
	assertGtEq(newY, 0, "height must be positive");
	assertGtEq(newX, 0, "width must be positive");
	this->drawing.resize(boost::extents[newY][newX]);
}

charArray<Color> SextantDrawing::getChar(const SextantCoord& topLeft) const {
	return {
	    {{{get(topLeft), get(topLeft + SextantCoord(1, 0)), get(topLeft + SextantCoord(2, 0))}},
	     {{get(topLeft + SextantCoord(0, 1)), get(topLeft + SextantCoord(1, 1)),
	       get(topLeft + SextantCoord(2, 1))}}}
    };
}

// copies toCopy onto this drawing
void SextantDrawing::insert(const SextantCoord& topLeft, const SextantDrawing& toCopy) {
	// cerr << topLeft.x << ' ' << topLeft.y << ';' << toCopy.getWidth() << ' ' <<
	// toCopy.getHeight() << endl;
	assertGtEq(topLeft.x, 0, "Top left must be greater than or equal to 0");
	assertGtEq(topLeft.y, 0, "Top left must be greater than or equal to 0");
	for (SextantCoord coord : CoordIterator(
	         SextantCoord(0, 0),
	         SextantCoord(std::min(this->getHeight() - topLeft.y - 1, toCopy.getHeight() - 1),
	                      std::min(this->getWidth() - topLeft.x - 1, toCopy.getWidth() - 1)))) {
		Color newColor = toCopy.get(coord);
		Color oldColor = this->get(coord);

#define TRANSFORM_COLOR(channel) \
	(((float)newColor.color.channel * (newColor.color.a / 255.0f) \
	  + (float)oldColor.color.channel * (oldColor.color.a / 255.0f) \
	        * (1.0f - newColor.color.a / 255.0f)))
		uchar red = TRANSFORM_COLOR(r);
		uchar green = TRANSFORM_COLOR(g);
		uchar blue = TRANSFORM_COLOR(b);
		uchar alpha = newColor.color.a + oldColor.color.a * (1.0f - newColor.color.a / 255.0f);
#undef TRANSFORM_COLOR
		RGBA color = RGBA(red, green, blue, alpha);

		// decide whether to use the old category or new category based on alpha
		Category category;
		// if it's mostly newColor, use newColor, otherwise use oldColor
		if (newColor.color.a > std::min<uchar>(128, oldColor.color.a)) category = newColor.category;
		else category = oldColor.category;

		this->set(topLeft + coord, Color(category, color));
	}
}

void SextantDrawing::debugPrint() const {
	for (int y = 0; y < this->getHeight(); y++) {
		for (int x = 0; x < this->getWidth(); x++) {
			std::cerr << (this->drawing[y][x].color.a == 255 ? "█" : " ");
		}
		std::cerr << '\n';
	}
	std::cerr << std::flush;
}

WindowedDrawing::WindowedDrawing(ncplane* win) : SextantDrawing(0, 0) {
	this->win = win;
	assertMsg(win != NULL, "win cannot be null");
	this->autoRescale();
}

void WindowedDrawing::autoRescale() {
	uint maxY, maxX;
	ncplane_dim_yx(this->win, &maxY, &maxX);
	this->resize(maxY * 3, maxX * 2);
}

void WindowedDrawing::render() const {
	for (int y = 0; y < this->getHeight(); y += 3) {
		for (int x = 0; x < this->getWidth(); x += 2) {
			charArray<Color> asArray = getChar(SextantCoord(y, x));
			auto trimmed = getTrimmedColors(asArray);

			ncplane_cursor_move_yx(this->win, y / 3, x / 2);
			ncplane_set_fg_rgb8(this->win, trimmed.second.first.r, trimmed.second.first.g,
			                    trimmed.second.first.b);
			ncplane_set_bg_rgb8(this->win, trimmed.second.second.r, trimmed.second.second.g,
			                    trimmed.second.second.b);
			ncplane_putwc(this->win, sextantMap[packArray(trimmed.first)]);
			// std::cerr << "p(" << std::to_string(y/3) << ", " << std::to_string(x/2) << ") " <<
			// std::to_string(colors.first) << " " << std::to_string(colors.second) << '\n';
		}
	}
}
