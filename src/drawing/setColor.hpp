#ifndef SETCOLOR_HPP
#define SETCOLOR_HPP

#include "../extraAssertions.hpp"

struct Category {
	bool allowMixing : 1;
	ushort categoryId : 15;

	Category(bool allowMixing, ushort categoryId) {
		// I don't get hints for default constructors, so...
		this->allowMixing = allowMixing;
		this->categoryId = categoryId;
	}

	Category() {
		Category(false, 0);
	}
};

struct RGB {
	uchar r;
	uchar g;
	uchar b;

	RGB(uchar red, uchar green, uchar blue) {
		this->r = red;
		this->g = green;
		this->b = blue;
	}

	RGB() {
		RGB(0, 0, 0);
	}
};

struct RGBA : public RGB {
	uchar a;

	RGBA(uchar red, uchar green, uchar blue, uchar alpha) : RGB(red, green, blue) {
		this->a = alpha;
	}

	RGBA() {
		RGBA(0, 0, 0, 0);
	}
};

struct Color {
	Category category;
	RGBA color;

	Color(Category category, RGBA color) {
		this->category = category;
		this->color = color;
	}

	Color() {
		this->category = Category();
		this->color = RGBA();
	}
};

int getColorPair(unsigned char fg, unsigned char bg);

#endif /* SETCOLOR_HPP */
