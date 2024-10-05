#include "quantizeChars.hpp"

#include <limits>
#include <vector>
#include <algorithm>
#include <utility>
#include "../extraAssertions.hpp"

/* @return whether a is higher priority than b (a > b) */
bool categoryCompare(const Category a, const Category b) {
	// sort by categoryId (less is higher priority) first, then 
	// allowMixing (false is higher priority)
	if (a.categoryId != b.categoryId)
		return a.categoryId < b.categoryId;
	else
		return a.allowMixing < b.allowMixing;
}

std::vector<Category> rankCategories(const charArray<Color>& arrayChar) {
	std::vector<Category> categories;
	categories.reserve(6);
	for (const auto& subarray: arrayChar) {
		for (const Color color: subarray) {
			if (std::find(categories.begin(), categories.end(), color.category) == categories.end())
				categories.push_back(color.category);
		}
	}

	std::sort(categories.begin(), categories.end(), categoryCompare); // highest priority first

	return categories;
}

std::vector<RGBA> getColorsByCategory(const Category category, const charArray<Color>& arrayChar) {
	std::vector<RGBA> colors;
	colors.reserve(6);
	for (const auto& subarray: arrayChar) {
		for (const Color color: subarray) {
			if (color.category == category) {
				colors.push_back(color.color);
			}
		}
	}

	return colors;
}

template <typename T> std::vector<T> flattenCharArray(const charArray<T>& arrayChar) {
	std::vector<T> output;
	output.reserve(6);
	
	for (const auto& subarray: arrayChar) {
		for (const T cell: subarray) {
			output.push_back(cell);
		}
	}

	return output;
}

std::vector<RGBA> extractRGBA(std::vector<Color> vec) {
	std::vector<RGBA> output;
	output.reserve(vec.size());
	for (const auto& i: vec) {
		output.push_back(i.color);
	}
	return output;
}

std::vector<Category> extractCategory(std::vector<Color> vec) {
	std::vector<Category> output;
	output.reserve(vec.size());
	for (const auto& i: vec) {
		output.push_back(i.category);
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
	// just use some random formula
	// if we get here, this is the point where we just choose something
	float aAvg = (a.r + a.g + a.b) * a.a;
	float bAvg = (b.r + b.g + b.b) * b.a;

	if (aAvg != bAvg)
		return aAvg > bAvg;
	else if (a == b)
		return false;
	else // screw it, just return something
		return (int32_t) ((a.r << 24) | (a.g << 16) | (a.b << 8) | a.a) >
		       (int32_t) ((b.r << 24) | (b.g << 16) | (b.b << 8) | b.a);
}

/*
 * Generates a histogram of colors.
 *
 * @return a sorted vector of pairs containing the color and the number of occurences.
 */
std::vector<std::pair<RGBA, ushort>> generateColorHistogram(const std::vector<RGBA> rgbaVector) {
	assertLtEq(rgbaVector.size(), std::numeric_limits<ushort>::max(), "That's one big vector.");

	std::vector<std::pair<RGBA, ushort>> counts;
	if (rgbaVector.size() == 0)
		return counts;
	counts.reserve(std::min<ushort>(10, rgbaVector.size())); // cap it at 10

	// create a histogram of the vector
	// I'm not using a map because I don't want to write a hash function for RGBA
	// also it'd take more memory
	for (const RGBA color: rgbaVector) {
		bool set = false;
		for (auto& count: counts) {
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
		[] (const std::pair<RGBA, ushort> a, const std::pair<RGBA, ushort> b)
		{return a.second != b.second ? a.second > b.second : RGBACompare(a.first, b.first);});

	return counts;
}

/*
 * Trim the colors used to fg and bg for display
 *
 * @return array with fg and bg and a color pair
 */
std::pair<charArray<bool>, int> getTrimmedColors(const charArray<Color>& arrayChar) {
	std::vector rankedCategories = rankCategories(arrayChar);
	std::pair<RGB, RGB> finalColors = std::make_pair(RGB(), RGB());
	#define RETCOLORS getColorPair(getColor(finalColors.first), getColor(finalColors.second))

	if (rankedCategories.size() == 1 && not rankedCategories.at(0).allowMixing) {
		auto histogram = generateColorHistogram(extractRGBA(flattenCharArray(arrayChar)));
		finalColors.first = histogram.at(0).first.applyAlpha();
		if (histogram.size() >= 2)
			finalColors.second = histogram.at(1).first.applyAlpha();
		
		charArray<bool> output;
		for (uchar x = 0; x < arrayChar.size(); x++) {
			for (uchar y = 0; y < arrayChar[0].size(); y++) {
				if (arrayChar[x][y].color.applyAlpha() == finalColors.first)
					output[x][y] = true;
				else
					output[x][y] = false;
			}
		}

		return std::make_pair(output, RETCOLORS);
	} else {
		throw std::logic_error("I haven't implemented that yet, okay?!?!");
	}

	#undef RETCOLORS
}

