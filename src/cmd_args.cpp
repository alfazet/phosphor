#include "cmd_args.hpp"
#include "logger.hpp"
#include "typedefs.h"

#include <cstring>
#include <iomanip>
#include <stdexcept>

u32 parse_u32(const char *s, const char *arg_name) {
    u32 value;
    auto [ptr, ec] = std::from_chars(s, s + std::strlen(s), value);

    if (ec != std::errc() || *ptr != '\0') {
        throw InvalidValueError(std::string(arg_name));
    }

    return value;
}

f32 parse_f32(const char *s, const char *arg_name) {
    char *end = nullptr;
    f32 x = std::strtof(s, &end);
    if (*end != '\0') {
        throw MissingValueError(std::string(arg_name));
    }

    return x;
}

std::string parse_string(const char *s, const char *arg_name) {
    if (!s || *s == '\0') {
        throw MissingValueError(std::string(arg_name));
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
            this->print_help();                                                                                        \
            throw std::runtime_error("expected a string value for " #field);                                           \
        }                                                                                                              \
        list.field = parser(this->values[this->arg_i], #field);                                                        \
    }
ARG_TABLE(X)
#undef X

void ArgParser::print_help() const {
    this->out << "usage: " << this->prog_name << " [flags]\nwhere:\n";
#define X(flag, field, type, parser, default_val, help)                                                                \
    this->out << "  " << std::left << std::setw(32) << flag << std::setw(64) << help << "(default: " << default_val    \
              << ")\n";
    ARG_TABLE(X)
#undef X
}

void ArgParser::print_values(const ArgsList &args) const {
#define X(flag, field, type, parser, default_val, help) LOG_INFO("{:<30} : {}", help, args.field);
    ARG_TABLE(X)
#undef X
}

ArgParser::ArgParser(usize n_args_, char **values_, std::ostream &out_)
    : n_args(n_args_), values(values_), prog_name(values_[0]), out(out_) {}

// assumes that CLI args are <flag_1> <value_1> <flag_2> <value_2> ...
ArgsList ArgParser::parse_all() {
    // skip prog_name
    this->arg_i++;
    ArgsList list{};

    while (this->arg_i < this->n_args) {
        const char *flag = this->values[this->arg_i];
        if (strcmp(flag, "-h") == 0 || strcmp(flag, "--help") == 0)
            throw HelpRequested{};

        auto iter = flag_parsers.find(flag);
        if (iter == flag_parsers.end()) {
            throw UnknownFlagError(std::string(flag));
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
