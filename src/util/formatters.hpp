#ifndef GLMFORMATTERS_HPP
#define GLMFORMATTERS_HPP
#include "glm/detail/qualifier.hpp"
#include "glm/ext/vector_double3.hpp"
#include "glm/gtx/string_cast.hpp"
#include <format>
#include <iostream>

template <glm::length_t Length, typename VecType, glm::qualifier Qual>
struct std::formatter<glm::vec<Length, VecType, Qual>> : std::formatter<std::string> {
	using std::formatter<std::string>::parse;

	auto format(glm::dvec3 const& val, auto& ctx) const {
		auto out = ctx.out();
		out = std::format_to(out, glm::to_string(val));
		ctx.advance_to(out);
		return out;
	};
};

#endif /* GLMFORMATTERS_HPP */
