#ifndef PHOSPHOR_CMD_ARGS_HPP
#define PHOSPHOR_CMD_ARGS_HPP

#include "common.hpp"

#include <ostream>
#include <string>
#include <unordered_map>

constexpr u32 DEFAULT_RESOLUTION = 256;
constexpr u32 DEFAULT_SAMPLES = 50;
constexpr u32 DEFAULT_PHOTONS_PER_LIGHT = 10000;
constexpr u32 DEFAULT_DEPTH = 3;
constexpr u32 DEFAULT_SEED = 2137;

#define ARG_TABLE(X)                                                                                                   \
    X("-r", resolution, u32, parse_u32, DEFAULT_RESOLUTION, "resolution")                                              \
    X("-s", samples, u32, parse_u32, DEFAULT_SAMPLES, "number of samples")                                             \
    X("-p", photons_per_light, u32, parse_u32, DEFAULT_PHOTONS_PER_LIGHT, "photons per light source")                  \
    X("-d", depth, u32, parse_u32, DEFAULT_DEPTH, "depth")                                                             \
    X("--seed", seed, u32, parse_u32, DEFAULT_SEED, "rng seed")

struct ArgsList {
    std::string dataset_path;
#define X(flag, field, type, parser, default_val, help) type field = default_val;
    ARG_TABLE(X)
#undef X
};

class ArgParser {
  public:
    usize n_args;
    char **values;
    char *prog_name;
    usize arg_i = 0;
    std::ostream &out;

    ArgParser(usize n_args_, char **values_, std::ostream &out_);

    ArgsList parse_all();

    void print_help() const;
    void print_values(const ArgsList &args) const;

  private:
    static std::unordered_map<std::string, void (ArgParser::*)(ArgsList &) const> flag_parsers;

#define X(flag, field, type, parser, default_val, help) void parse_##field(ArgsList &list) const;
    ARG_TABLE(X)
#undef X
};

class ArgParseError : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

class UnknownFlagError : public ArgParseError {
  public:
    explicit UnknownFlagError(std::string flag) : ArgParseError("unknown flag: " + flag), flag(std::move(flag)) {}

    std::string flag;
};

class MissingValueError : public ArgParseError {
  public:
    explicit MissingValueError(std::string flag) : ArgParseError("missing value for " + flag) {}
};

class InvalidValueError : public ArgParseError {
  public:
    InvalidValueError(std::string flag) : ArgParseError("invalid value for " + flag) {}
};

class HelpRequested {};

#endif // PHOSPHOR_CMD_ARGS_HPP
