#ifndef SETCOLOR_HPP
#define SETCOLOR_HPP

#include "../extraAssertions.hpp"
#include <array>

template <typename storeAs> using charArray = std::array<std::array<storeAs, 3>, 2>;

struct Category {
	bool allowMixing : 1;
	ushort categoryId : 15;

	Category(bool allowMixing, ushort categoryId) {
		// I don't get hints for default constructors, so...
		this->allowMixing = allowMixing;
		this->categoryId = categoryId;
	}

	Category() : Category(false, 0) { }

	bool operator==(const Category& other) const = default;
	bool operator!=(const Category& other) const = default;
};

// TODO: more efficient/cleaner method?
#define CAPOVERFLOW(calculation) static_cast<uchar>(std::max(std::min(static_cast<int>(calculation), 255), 0))

struct RGB {
	uchar r;
	uchar g;
	uchar b;

	RGB(uchar red, uchar green, uchar blue) {
		this->r = red;
		this->g = green;
		this->b = blue;
	}

	RGB() : RGB(0, 0, 0) { }

	RGB operator*=(const double& magnitude) {
		assertGtEq(magnitude, 0, "Magnitude cannot be negative");
		this->r = CAPOVERFLOW(this->r * magnitude);
		this->g = CAPOVERFLOW(this->g * magnitude);
		this->b = CAPOVERFLOW(this->b * magnitude);
		return *this;
	}

	RGB operator/=(const double& magnitude) {
		assertGtEq(magnitude, 0, "Magnitude cannot be negative");
		this->r = CAPOVERFLOW(this->r / magnitude);
		this->g = CAPOVERFLOW(this->g / magnitude);
		this->b = CAPOVERFLOW(this->b / magnitude);
		return *this;
	}

	RGB operator*(const double& magnitude) const {
		RGB temp{*this};
		return temp *= magnitude;
	}

	RGB operator/(const double& magnitude) const {
		RGB temp{*this};
		return temp /= magnitude;
	}

	RGB operator+=(const RGB& color) {
		this->r = CAPOVERFLOW(this->r + color.r);
		this->g = CAPOVERFLOW(this->g + color.g);
		this->b = CAPOVERFLOW(this->b + color.b);
		return *this;
	}

	RGB operator-=(const RGB& color) {
		this->r = CAPOVERFLOW(this->r - color.r);
		this->g = CAPOVERFLOW(this->g - color.g);
		this->b = CAPOVERFLOW(this->b - color.b);
		return *this;
	}

	RGB operator+(const RGB& color) const {
		RGB temp{*this};
		return temp += color;
	}

	RGB operator-(const RGB color) const {
		RGB temp{*this};
		return temp -= color;
	}

	bool operator==(const RGB& other) const = default;
	bool operator!=(const RGB& other) const = default;
};

template <> struct std::hash<RGB> {
	size_t operator()(const RGB rgb) const {
		static_assert(sizeof(size_t) >= 3);
		return (size_t) (rgb.r << 16) | (rgb.g << 8) | rgb.b;
	}
};

struct RGBA {
	uchar r;
	uchar g;
	uchar b;
	uchar a;

	RGBA(uchar red, uchar green, uchar blue, uchar alpha) {
		this->r = red;
		this->g = green;
		this->b = blue;
		this->a = alpha;
	}

	RGBA() : RGBA(0, 0, 0, 0) { }

	RGB applyAlpha() const {
		return RGB(
			this->r * (static_cast<float>(this->a) / 255.0),
			this->g * (static_cast<float>(this->a) / 255.0),
			this->b * (static_cast<float>(this->a) / 255.0)
		);
	}

	RGBA operator*=(const double& magnitude) {
		assertGtEq(magnitude, 0, "Magnitude cannot be negative");
		this->r = CAPOVERFLOW(this->r * magnitude);
		this->g = CAPOVERFLOW(this->g * magnitude);
		this->b = CAPOVERFLOW(this->b * magnitude);
		return *this;
	}

	RGBA operator/=(const double& magnitude) {
		assertGtEq(magnitude, 0, "Magnitude cannot be negative");
		this->r = CAPOVERFLOW(this->r / magnitude);
		this->g = CAPOVERFLOW(this->g / magnitude);
		this->b = CAPOVERFLOW(this->b / magnitude);
		return *this;
	}

	RGBA operator*(const double& magnitude) const {
		RGBA temp{*this};
		return temp *= magnitude;
	}

	RGBA operator/(const double& magnitude) const {
		RGBA temp{*this};
		return temp /= magnitude;
	}

	RGBA operator+=(const RGBA& color) {
		this->a = CAPOVERFLOW(this->a + color.a);
		this->r = CAPOVERFLOW(this->r + color.r);
		this->g = CAPOVERFLOW(this->g + color.g);
		this->b = CAPOVERFLOW(this->b + color.b);
		return *this;
	}

	RGBA operator-=(const RGBA& color) {
		this->a = CAPOVERFLOW(this->a - color.a);
		this->r = CAPOVERFLOW(this->r - color.r);
		this->g = CAPOVERFLOW(this->g - color.g);
		this->b = CAPOVERFLOW(this->b - color.b);
		return *this;
	}

	RGBA operator+(const RGBA& color) const {
		RGBA temp{*this};
		return temp += color;
	}

	RGBA operator-(const RGBA color) const {
		RGBA temp{*this};
		return temp -= color;
	}

	bool operator==(const RGBA& other) const = default;
	bool operator!=(const RGBA& other) const = default;
};

#undef CAPOVERFLOW

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

	bool operator==(const Color& other) const = default;
	bool operator!=(const Color& other) const = default;
};

short getColor(const RGB color);
int getColorPair(const uchar fg, const uchar bg);

#endif /* SETCOLOR_HPP */
