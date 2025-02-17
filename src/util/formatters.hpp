#ifndef GLMFORMATTERS_HPP
#define GLMFORMATTERS_HPP
#include "glm/detail/qualifier.hpp"
#include "glm/ext/vector_double3.hpp"
#include <print>
#include <format>
#include <iostream>
#include <vector>

template <glm::length_t Length, typename VecType, glm::qualifier Qual>
struct std::formatter<glm::vec<Length, VecType, Qual>> : std::formatter<VecType> {
	using std::formatter<VecType>::parse;

	auto format(glm::vec<Length, VecType, Qual> const& val, auto& ctx) const {
		// can't use glm::to_string() here because it's not consteval :(
		auto out = ctx.out();
		out = std::format_to(out, "{}vec{}(", typeid(VecType).name()[0], Length);
		if constexpr (Length >= 1) out = std::format_to(out, "{}", val.x);
		if constexpr (Length >= 2) out = std::format_to(out, ", {}", val.y);
		if constexpr (Length >= 3) out = std::format_to(out, ", {}", val.z);
		if constexpr (Length >= 4) out = std::format_to(out, ", {}", val.w);
		out = std::format_to(out, ")");
		ctx.advance_to(out);
		return out;
	};
};

#endif /* GLMFORMATTERS_HPP */
