#include "quantizeChars.hpp"

#include <algorithm>
#include <boost/functional/hash.hpp>
#include <climits>
#include <cmath>
#include <csignal>
#include <limits>
#include <utility>
#include <vector>

#include "../extraAssertions.hpp"

/* @return whether a is higher priority than b (a > b) */
bool categoryCompare(const Category a, const Category b) {
	// sort by categoryId (less is higher priority) first, then
	// allowMixing (false is higher priority)
	if (a.id != b.id) return a.id < b.id;
	else return a.allowMixing < b.allowMixing;
}

std::vector<Category> rankCategories(const std::vector<Color>& colors) {
	std::vector<Category> categories;
	categories.reserve(std::min<ushort>(10, colors.size())); // cap it at 10
	for (const Color color : colors) {
		if (std::find(categories.begin(), categories.end(), color.category) == categories.end())
			categories.push_back(color.category);
	}

	std::sort(categories.begin(), categories.end(), categoryCompare); // highest priority first

	return categories;
}

std::vector<RGBA> filterColorsByCategory(const Category category, const std::vector<Color>& input) {
	std::vector<RGBA> colors;
	colors.reserve(std::min<size_t>(10, input.size())); // cap it at 10
	for (const Color color : input) {
		if (color.category == category) {
			colors.push_back(color.color);
		}
	}

	return colors;
}

template <typename T> std::vector<T> flattenCharArray(const charArray<T>& arrayChar) {
	std::vector<T> output;
	output.reserve(6);

	for (const auto& subarray : arrayChar) {
		for (const T cell : subarray) {
			output.push_back(cell);
		}
	}

	return output;
}

std::vector<RGBA> extractRGBA(std::vector<Color> vec) {
	std::vector<RGBA> output;
	output.reserve(vec.size());
	for (const auto& i : vec) {
		output.push_back(i.color);
	}
	return output;
}

std::vector<Category> extractCategory(std::vector<Color> vec) {
	std::vector<Category> output;
	output.reserve(vec.size());
	for (const auto& i : vec) {
		output.push_back(i.category);
	}
	return output;
}

// TODO: make everything in this file use RGB instead of RGBA in the first place
std::vector<RGB> applyAlphas(const std::vector<RGBA>& vec) {
	std::vector<RGB> output;
	output.reserve(vec.size());
	for (const auto& i : vec) {
		output.push_back(i.applyAlpha());
	}
	return output;
}

/*
 * Compare two RGBA values in a deterministic way
 *
 * This function is neccesary to prevent flickering, even when categories are the same.
 * @return whether a is higher priority than b (a > b)
 */
bool RGBACompare(const RGBA a, const RGBA b) {
	// just use some random formula (brightness)
	float aAvg = (a.r + a.g + a.b) * a.a;
	float bAvg = (b.r + b.g + b.b) * b.a;

	if (aAvg != bAvg) return aAvg > bAvg;
	else if (a == b) return false;
	else // screw it, just return something
		return (int32_t)((a.r << 24) | (a.g << 16) | (a.b << 8) | a.a)
		       > (int32_t)((b.r << 24) | (b.g << 16) | (b.b << 8) | b.a);
}

/*
 * Generates a histogram of colors.
 *
 * @return a sorted vector of pairs containing the color and the number of occurences.
 */
std::vector<std::pair<RGBA, ushort>> generateColorHistogram(const std::vector<RGBA> rgbaVector) {
	assertLtEq(rgbaVector.size(), std::numeric_limits<ushort>::max(), "That's one big vector.");

	std::vector<std::pair<RGBA, ushort>> counts;
	if (rgbaVector.size() == 0) return counts;
	counts.reserve(std::min<ushort>(10, rgbaVector.size())); // cap it at 10

	// create a histogram of the vector
	// I'm not using a map because I don't want to write a hash function for RGBA
	// also it'd take more memory
	for (const RGBA color : rgbaVector) {
		bool set = false;
		for (auto& count : counts) {
			if (color == count.first) {
				set = true;
				count.second++;
			}
		}
		if (not set) {
			counts.push_back(std::make_pair(color, 1));
		}
	}

	// this function must be order invariant and deterministic (to prevent flickering)
	// therefore, even if two colors have the same count we need to use the same ordering every time
	std::sort(counts.begin(), counts.end(),
	          [](const std::pair<RGBA, ushort> a, const std::pair<RGBA, ushort> b) {
		          return a.second != b.second ? a.second > b.second : RGBACompare(a.first, b.first);
	          });

	return counts;
}

template <> struct std::hash<std::pair<RGB, RGB>> {
	[[deprecated("I don't think this is used anywhere, but I'll leave it for now")]]
	size_t operator()(const std::pair<RGB, RGB>& rgbPair) const {
#if SIZEOF_SIZE_T >= 6
		return hash<RGB>()(rgbPair.first) << sizeof(RGB) * CHAR_BIT | hash<RGB>()(rgbPair.second);
#else
		size_t val = hash<RGB>()(rgbPair.first);
		boost::hash_combine(val, hash<RGB>()(rgbPair.second));
		return val;
#endif
	}
};

float colourDistance(const RGB e1, const RGB e2) {
	// from https://www.compuphase.com/cmetric.htm
	int rmean = ((int)e1.r + (int)e2.r) / 2;
	int r = (int)e1.r - (int)e2.r;
	int g = (int)e1.g - (int)e2.g;
	int b = (int)e1.b - (int)e2.b;
	return sqrt((((512 + rmean) * r * r) >> 8) + 4 * g * g + (((767 - rmean) * b * b) >> 8));
}

std::pair<RGB, RGB> getMostDifferentColors(const std::vector<RGB>& colors) {
	float maxDiff = -1;
	std::pair<RGB, RGB> output = std::make_pair(RGB(), RGB());
	for (const RGB colorA : colors) {
		for (const RGB colorB : colors) {
			if (colourDistance(colorA, colorB) > maxDiff) {
				output = std::make_pair(colorA, colorB);
				maxDiff = colourDistance(colorA, colorB);
			}
		}
	}
	return output;
}

RGB averageColor(const std::vector<RGB>& colors) {
	assertNotEq(colors.size(), 0, "Cannot take the average of no colors");
	uint r = 0, g = 0, b = 0;
	for (const RGB color : colors) {
		r += color.r;
		g += color.g;
		b += color.b;
	}

	r /= colors.size();
	g /= colors.size();
	b /= colors.size();

	return RGB(r, g, b);
}

/* Checks for equality -- anything that isn't one of colors is assumed to be colors.second */
charArray<bool> applyEquals(const std::pair<RGB, RGB> colors, const charArray<Color>& arrayChar) {
	charArray<bool> output;
	for (uchar x = 0; x < arrayChar.size(); x++) { // FIXME: these funcs should use y, x instead
		for (uchar y = 0; y < arrayChar[0].size(); y++) {
			if (arrayChar[x][y].color.applyAlpha() == colors.first) output[x][y] = true;
			else output[x][y] = false;
		}
	}
	return output;
}

/* Checks for closeness -- the closes color from colors is used */
charArray<bool> applyClosest(const std::pair<RGB, RGB> colors, const charArray<Color>& arrayChar) {
	charArray<bool> output;
	for (uchar x = 0; x < arrayChar.size(); x++) {
		for (uchar y = 0; y < arrayChar[0].size(); y++) {
			output[x][y] = colourDistance(arrayChar[x][y].color.applyAlpha(), colors.first)
			               <= colourDistance(arrayChar[x][y].color.applyAlpha(), colors.second);
		}
	}
	return output;
}

/* Checks for category -- anything that isn't one of categories is matched by closeness */
charArray<bool> applyCategory(const std::pair<Category, Category> categories,
                              const std::pair<RGB, RGB> colors, const charArray<Color>& arrayChar) {
	charArray<bool> output;
	for (uchar x = 0; x < arrayChar.size(); x++) {
		for (uchar y = 0; y < arrayChar[0].size(); y++) {
			if (arrayChar[x][y].category == categories.first) {
				output[x][y] = true;
			} else if (arrayChar[x][y].category == categories.second) {
				output[x][y] = false;
			} else {
				output[x][y] = colourDistance(arrayChar[x][y].color.applyAlpha(), colors.first)
				               <= colourDistance(arrayChar[x][y].color.applyAlpha(), colors.second);
			}
		}
	}
	return output;
}

/* Checks for category -- anything that isn't one of categories is treated as category 2  */
charArray<bool> applyCategoryStrict(const std::pair<Category, Category> categories,
                                    const charArray<Color>& arrayChar) {
	charArray<bool> output;
	for (uchar x = 0; x < arrayChar.size(); x++) {
		for (uchar y = 0; y < arrayChar[0].size(); y++) {
			if (arrayChar[x][y].category == categories.first) {
				output[x][y] = true;
			} else {
				output[x][y] = false;
			}
		}
	}
	return output;
}

/*
 * Trim the colors used to fg and bg for display
 *
 * @return array with fg and bg and a color pair
 */
std::pair<charArray<bool>, std::pair<RGB, RGB>>
getTrimmedColors(const charArray<Color>& arrayChar) {
	std::vector<Color> flattened = flattenCharArray(arrayChar);
	std::vector<Category> rankedCategories = rankCategories(flattened);
	std::pair<RGB, RGB> finalColors = std::make_pair(RGB(), RGB());

	/*
	    This function works by handling several seperate cases based on number
	    of categories and whether mixing is allowed
	    size == 1, non-mixing: get 1-2 most common colors
	    size >= 2, first two non-mixing: get most common color from each category
	    size == 1, mixing: get two most different colors, then average
	    size >= 2, first two mixing: average each category
	    size >= 2, first mixing, second non-mixing: average first, most common second
	    size >= 2, first non-mixing, second mixing: most common first, average second
	*/

	if (rankedCategories.size() == 1 && not rankedCategories.at(0).allowMixing) {
		auto histogram = generateColorHistogram(extractRGBA(flattened));
		finalColors.first = histogram.at(0).first.applyAlpha();
		if (histogram.size() >= 2) finalColors.second = histogram.at(1).first.applyAlpha();

		return std::make_pair(applyEquals(finalColors, arrayChar), finalColors);

	} else if (rankedCategories.size() >= 2 && not rankedCategories.at(0).allowMixing
	           && not rankedCategories.at(1).allowMixing) {

		auto histogram1 =
		        generateColorHistogram(filterColorsByCategory(rankedCategories.at(0), flattened));
		auto histogram2 =
		        generateColorHistogram(filterColorsByCategory(rankedCategories.at(1), flattened));

		finalColors.first = histogram1.at(0).first.applyAlpha();
		finalColors.second = histogram2.at(0).first.applyAlpha();

		std::pair<Category, Category> categories =
		        std::make_pair(rankedCategories.at(0), rankedCategories.at(1));
		return std::make_pair(applyCategoryStrict(categories, arrayChar), finalColors);

	} else if (rankedCategories.size() == 1 && rankedCategories.at(0).allowMixing) {
		std::pair<RGB, RGB> mostDifferentColors =
		        getMostDifferentColors(applyAlphas(extractRGBA(flattened)));
		std::vector<RGB> vecFirst;
		vecFirst.reserve(6);
		std::vector<RGB> vecSecond;
		vecSecond.reserve(6);

		for (const RGB color : applyAlphas(extractRGBA(flattened))) {
			if (colourDistance(color, mostDifferentColors.first)
			    <= colourDistance(color, mostDifferentColors.second))
				vecFirst.push_back(color);
			else vecSecond.push_back(color);
		}

		finalColors.first = averageColor(vecFirst);
		if (vecSecond.size() != 0) finalColors.second = averageColor(vecSecond);

		return std::make_pair(applyClosest(finalColors, arrayChar), finalColors);

	} else if (rankedCategories.size() >= 2 && rankedCategories.at(0).allowMixing
	           && rankedCategories.at(1).allowMixing) {

		finalColors.first = averageColor(
		        applyAlphas(filterColorsByCategory(rankedCategories.at(0), flattened)));
		finalColors.second = averageColor(
		        applyAlphas(filterColorsByCategory(rankedCategories.at(1), flattened)));

		std::pair<Category, Category> categories =
		        std::make_pair(rankedCategories.at(0), rankedCategories.at(1));
		return std::make_pair(applyCategory(categories, finalColors, arrayChar), finalColors);

	} else if (rankedCategories.size() >= 2 && rankedCategories.at(0).allowMixing
	           && not rankedCategories.at(1).allowMixing) {

		finalColors.first = averageColor(
		        applyAlphas(filterColorsByCategory(rankedCategories.at(0), flattened)));

		auto histogram2 =
		        generateColorHistogram(filterColorsByCategory(rankedCategories.at(1), flattened));
		finalColors.second = histogram2.at(0).first.applyAlpha();

		std::pair<Category, Category> categories =
		        std::make_pair(rankedCategories.at(0), rankedCategories.at(1));
		return std::make_pair(applyCategory(categories, finalColors, arrayChar), finalColors);

	} else if (rankedCategories.size() >= 2 && not rankedCategories.at(0).allowMixing
	           && rankedCategories.at(1).allowMixing) {

		auto histogram1 =
		        generateColorHistogram(filterColorsByCategory(rankedCategories.at(0), flattened));
		finalColors.first = histogram1.at(0).first.applyAlpha();

		finalColors.second = averageColor(
		        applyAlphas(filterColorsByCategory(rankedCategories.at(1), flattened)));

		std::pair<Category, Category> categories =
		        std::make_pair(rankedCategories.at(0), rankedCategories.at(1));
		return std::make_pair(applyCategoryStrict(categories, arrayChar), finalColors);

	} else {
		throw std::logic_error("This is impossible. What. (error in getTrimmedColors)");
	}
}
