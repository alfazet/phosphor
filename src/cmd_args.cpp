#include "cmd_args.hpp"

#include <charconv>
#include <cstring>
#include <stdexcept>

u32 parse_u32(const char *s, const char *arg_name) {
    u32 value;
    auto [ptr, ec] = std::from_chars(s, s + std::strlen(s), value);

    if (ec != std::errc() || *ptr != '\0') {
        throw std::runtime_error("invalid uint value for " + std::string(arg_name));
    }

    return value;
}

f32 parse_f32(const char *s, const char *arg_name) {
    char *end = nullptr;
    f32 x = std::strtof(s, &end);
    if (*end != '\0') {
        throw std::runtime_error("expected a float value for " + std::string(arg_name));
    }

    return x;
}

std::string parse_string(const char *s, const char *arg_name) {
    if (!s || *s == '\0') {
        throw std::runtime_error("expected a string value for " + std::string(arg_name));
    }

    return s;
}

using ParserFn = void (ArgParser::*)(ArgsList &) const;
std::unordered_map<std::string, ParserFn> ArgParser::flag_parsers = {
#define X(flag, field, type, parser, default_val, help) {flag, &ArgParser::parse_##field},
    ARG_TABLE(X)
#undef X
};

#define X(flag, field, type, parser, default_val, help)                                                                \
    void ArgParser::parse_##field(ArgsList &list) const {                                                              \
        if (this->arg_i >= this->n_args) {                                                                             \
            print_help(this->out);                                                                                     \
            throw std::runtime_error("missing value for " #field);                                                     \
        }                                                                                                              \
        list.field = parser(this->values[this->arg_i], #field);                                                        \
    }
ARG_TABLE(X)
#undef X

void ArgParser::print_help(std::ostream &out) {
    out << "Arguments:\n[flags]\nwhere:\n";
#define X(flag, field, type, parser, default_val, help)                                                                \
    out << flag << ": " << help << " (default: " << default_val << ")\n";
    ARG_TABLE(X)
#undef X
}

ArgParser::ArgParser(usize n_args_, char **values_, std::ostream &out_) : n_args(n_args_), values(values_), out(out_) {}

/// assumes that CLI args are <flag_1> <value_1> <flag_2> <value_2> ...
ArgsList ArgParser::parse_all() {
    ArgsList list{};

    while (this->arg_i < this->n_args) {
        const char *flag = this->values[this->arg_i];
        auto iter = flag_parsers.find(flag);
        if (iter == flag_parsers.end()) {
            print_help(this->out);
            throw std::runtime_error("invalid flag " + std::string(flag));
        }
        const auto &parser = iter->second;

        // move to the flag's value and parse it
        this->arg_i++;
        (this->*parser)(list);
        // move to the next flag
        this->arg_i++;
    }

    return list;
}
