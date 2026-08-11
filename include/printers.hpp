#ifndef PHOSPHOR_PRINTERS_HPP
#define PHOSPHOR_PRINTERS_HPP

#include "common.hpp"

#include <format>

template <glm::length_t L, typename T, glm::qualifier Q> struct std::formatter<glm::vec<L, T, Q>> {
    constexpr auto parse(std::format_parse_context &ctx) { return ctx.begin(); }

    auto format(const glm::vec<L, T, Q> &v, std::format_context &ctx) const {
        auto out = ctx.out();
        *out++ = '(';
        for (glm::length_t i = 0; i < L; ++i) {
            if (i > 0) {
                *out++ = ',';
                *out++ = ' ';
            }
            out = std::format_to(out, "{}", v[i]);
        }
        *out++ = ')';
        return out;
    }
};

#endif // PHOSPHOR_PRINTERS_HPP
