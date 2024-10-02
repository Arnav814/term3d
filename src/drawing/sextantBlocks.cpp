#include "sextantBlocks.hpp"

#include <boost/type_traits/extent.hpp>
#include <cassert>
#include <cmath>
#include <initializer_list>
#include <unistd.h>
#include <codecvt>
#include <locale>
#include <array>
#include <unordered_map>
#include <bitset>
#include <iostream>
#include <csignal>
#include <curses.h>

#include "coord2d.hpp"
#include "../extraAssertions.hpp"
#include "setColor.hpp"

char packArray(charArray<bool>& myArray) {
	return (
		((myArray[1][2] != 0 ? 1 : 0) << 0) +
		((myArray[1][1] != 0 ? 1 : 0) << 1) +
		((myArray[1][0] != 0 ? 1 : 0) << 2) +
		((myArray[0][2] != 0 ? 1 : 0) << 3) +
		((myArray[0][1] != 0 ? 1 : 0) << 4) +
		((myArray[0][0] != 0 ? 1 : 0) << 5)
	);
}

// Numbered:
// 0 3
// 1 4
// 2 5

std::unordered_map<char, wchar_t> sextantMap {
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

void testAllSextants() {
	std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
	for (const auto& i: sextantMap) {
		std::bitset<6> asBitset(i.first);
		std::cout << (asBitset[5] ? '#' : '_');
		std::cout << (asBitset[2] ? '#' : '_') << '\n';
		std::cout << (asBitset[4] ? '#' : '_');
		std::cout << (asBitset[1] ? '#' : '_') << '\n';
		std::cout << (asBitset[3] ? '#' : '_');
		std::cout << (asBitset[0] ? '#' : '_') << '\n';
		std::cout << "\e[43m" << utf8_conv.to_bytes(i.second) << "\e[0m";
		std::cout << "\n\n";
	}
	std::cout << std::flush;
}

SextantDrawing::SextantDrawing(const int height, const int width) {
	assertGtEq(height, 0, "height must be positive");
	assertGtEq(width, 0, "width must be positive");
	this->drawing = drawing_type(boost::extents[height][width]);
}

SextantDrawing::SextantDrawing(std::initializer_list<std::initializer_list<PriorityColor>> init) {
	uint height = init.size();
	uint width;
	if (height == 0)
		width = 0;
	else
		width = init.begin()->size();

	this->drawing.resize(boost::extents[height][width]);

	int x, y = 0;
	for (const auto& list: init) {
		assertEq(width, list.size(), "All sub-lists must have the same size");
		x = 0;
		for (const auto& elem: list) {
			this->set(SextantCoord(y, x), elem);
			x++;
		}
		y++;
	}
}

[[nodiscard]] PriorityColor SextantDrawing::get(const SextantCoord& coord) const {
	assertBetweenHalfOpen(0, coord.y, this->getHeight(), "Height out of range in get");
	assertBetweenHalfOpen(0, coord.x, this->getWidth(), "Width out of range in get");
	return this->drawing[coord.y][coord.x];
}

[[nodiscard]] PriorityColor SextantDrawing::getWithFallback(const SextantCoord& coord, const PriorityColor fallback) const {
	if (coord.y < 0 || coord.y >= getHeight() || coord.x < 0 || coord.x >= getWidth())
		return fallback;
	else
		return this->get(coord);
}

void SextantDrawing::set(const SextantCoord& coord, const PriorityColor setTo) {
	assertBetweenHalfOpen(0, coord.y, (int) this->drawing.size(), "Height out of range in set");
	assertBetweenHalfOpen(0, coord.x, (int) this->drawing[coord.y].size(), "Width out of range in set");
	this->drawing[coord.y][coord.x] = setTo;
}

void SextantDrawing::trySet(const SextantCoord& coord, const PriorityColor setTo) {
	if (coord.y < 0 || coord.y >= getHeight() || coord.x < 0 || coord.x >= getWidth())
		return;
	else
		this->set(coord, setTo);
}

[[nodiscard]] CoordIterator<SextantCoord> SextantDrawing::getIterator() const {
	return CoordIterator(SextantCoord(0, 0), SextantCoord(this->getHeight()-1, this->getWidth()-1));
}

void SextantDrawing::clear() {
	for (SextantCoord coord: this->getIterator()) {
		this->set(coord, PriorityColor(0, 0));
	}
}

void SextantDrawing::resize(int newY, int newX) {
	assertGtEq(newY, 0, "height must be positive");
	assertGtEq(newX, 0, "width must be positive");
	this->drawing.resize(boost::extents[newY][newX]);
}

charArray<PriorityColor> SextantDrawing::getChar(const SextantCoord& topLeft) const {
	return {{
		{{
			getWithFallback(topLeft, PriorityColor(0, 0)),
			getWithFallback(topLeft + SextantCoord(1, 0), PriorityColor(0, 0)),
			getWithFallback(topLeft + SextantCoord(2, 0), PriorityColor(0, 0))
		}},
		{{
			getWithFallback(topLeft + SextantCoord(0, 1), PriorityColor(0, 0)),
			getWithFallback(topLeft + SextantCoord(1, 1), PriorityColor(0, 0)),
			getWithFallback(topLeft + SextantCoord(2, 1), PriorityColor(0, 0))
		}}
	}};
}

// copies toCopy onto this drawing
void SextantDrawing::insert(const SextantCoord& topLeft, const SextantDrawing& toCopy,
                            const OverrideStyle overrideStyle = OverrideStyle::Priority) {
	//cerr << topLeft.x << ' ' << topLeft.y << ';' << toCopy.getWidth() << ' ' << toCopy.getHeight() << endl;
	assertGtEq(topLeft.x, 0, "Top left must be greater than or equal to 0");
	assertGtEq(topLeft.y, 0, "Top left must be greater than or equal to 0");
	for (SextantCoord coord: CoordIterator(SextantCoord(0, 0),
			SextantCoord(std::min(this->getHeight() - topLeft.y - 1, toCopy.getHeight() - 1),
			             std::min(this->getWidth() - topLeft.x - 1, toCopy.getWidth() - 1)))) {
		bool doSet;

		switch (overrideStyle) {
			case OverrideStyle::Always:
				doSet = true;
				break;
			case OverrideStyle::Nonzero:
				doSet = this->get(topLeft + coord).priority == 0;
				break;
			case OverrideStyle::Priority:
				doSet = this->get(topLeft + coord).priority < toCopy.get(coord).priority;
				break;
			default:
				assert(false); // makes GCC shut up
		}

		if (doSet)
			this->set(topLeft + coord, toCopy.get(coord));
	}
}

std::pair<colorType, colorType> getTrimmedColors(const charArray<PriorityColor>& arrayChar) {
	std::pair<priorityType, priorityType> priorities = std::make_pair(0, 0);
	std::pair<colorType, colorType> colors = std::make_pair(0, 0);

	for (auto a: arrayChar) {
		for (PriorityColor b: a) {
			// TODO: this doesn't handle PriorityColors with the
			// same color and different priorities very well.
			if (b.color != colors.first && b.color != colors.second) {
				if (b.priority > priorities.first) {
					colors.second = colors.first;
					priorities.second = priorities.first;

					colors.first = b.color;
					priorities.first = b.priority;
				} else if (b.priority > priorities.second) {
					colors.second = b.color;
					priorities.second = b.priority;
				}
			}
		}
	}

	// prefer setting bg over fg -- a space is 1/4 the bytes of a filled block
	//if (colors.second == 0)
		//std::swap(colors.first, colors.second);

	return colors;
}

void SextantDrawing::debugPrint() const {
	for (int y = 0; y < this->getHeight(); y++) {
		for (int x = 0; x < this->getWidth(); x++) {
			std::cerr << (this->drawing[y][x].color ? "█" : " ");
		}
		std::cerr << '\n';
	}
	std::cerr << std::flush;
}

WindowedDrawing::WindowedDrawing(WINDOW* win) : SextantDrawing(0, 0) {
	this->win = win;
	assertMsg(win != NULL, "win cannot be null");
	this->autoRescale();
}

void WindowedDrawing::autoRescale() {
	int maxX, maxY;
	getmaxyx(win, maxX, maxY);
	this->resize(maxY*3, maxX*2);
}

void WindowedDrawing::render() const {
	static std::wstring_convert<std::codecvt_utf8<wchar_t>> utf8_conv;
	for (int y = 0; y < this->getHeight(); y += 3) {
		for (int x = 0; x < this->getWidth(); x += 2) {
			charArray<PriorityColor> asArray = getChar(SextantCoord(y, x));
			std::pair<colorType, colorType> colors = getTrimmedColors(asArray);
			charArray<bool> trimmed;

			for (unsigned int x = 0; x < asArray.size(); x++) {
				for (unsigned int y = 0; y < asArray[x].size(); y++) {
					if (asArray[x][y].color == colors.first) {
						trimmed[x][y] = true;
					} else {
						trimmed[x][y] = false;
					}
				}
			}

			attrset(getColorPair(colors.first, colors.second));
			//std::cerr << "p(" << std::to_string(y/3) << ", " << std::to_string(x/2) << ") " <<
				//std::to_string(colors.first) << " " << std::to_string(colors.second) << '\n';
			mvaddstr(y / 3, x / 2, utf8_conv.to_bytes(sextantMap[packArray(trimmed)]).c_str());
		}
	}
}

