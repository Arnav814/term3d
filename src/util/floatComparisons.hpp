#ifndef FLOATCOMPARISONS_HPP
#define FLOATCOMPARISONS_HPP

#include "glm/detail/qualifier.hpp"
#include "glm/gtc/quaternion.hpp"
#include <glm/matrix.hpp>

#define sNaN_d std::numeric_limits<double>::signaling_NaN()
#define sNaN_f std::numeric_limits<float>::signaling_NaN()
#define SMALL 0.001

template <typename FloatType> bool floatCmp(const FloatType a, const FloatType b) {
	static_assert(std::is_floating_point_v<FloatType>);
	return std::abs(a - b) <= SMALL; // TODO: ULP based calculation?
}

template <glm::length_t Rows, glm::length_t Cols, typename Type, glm::qualifier Qual>
bool matCmp(const glm::mat<Cols, Rows, Type, Qual>& a, const glm::mat<Cols, Rows, Type, Qual>& b) {
	for (uint row = 0; row < Rows; row++) {
		for (uint col = 0; col < Cols; col++) {
			if (not floatCmp(a[row][col], b[row][col])) return false;
		}
	}
	return true;
}

template <glm::length_t Length, typename VecType, glm::qualifier Qual>
bool vecCmp(const glm::vec<Length, VecType, Qual>& a, const glm::vec<Length, VecType, Qual>& b) {
	if constexpr (Length >= 1)
		if (not floatCmp(a.x, b.x)) return false;
	if constexpr (Length >= 2)
		if (not floatCmp(a.y, b.y)) return false;
	if constexpr (Length >= 3)
		if (not floatCmp(a.z, b.z)) return false;
	if constexpr (Length >= 4)
		if (not floatCmp(a.w, b.w)) return false;
	return true;
};

#endif /* FLOATCOMPARISONS_HPP */
