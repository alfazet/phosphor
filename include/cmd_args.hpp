#ifndef PHOSPHOR_CMD_ARGS_HPP
#define PHOSPHOR_CMD_ARGS_HPP

#include "common.hpp"

#include <ostream>
#include <string>
#include <unordered_map>

constexpr u32 DEFAULT_RESOLUTION = 256;
constexpr u32 DEFAULT_SAMPLES = 50;
constexpr u32 DEFAULT_PHOTONS_PER_LIGHT = 10000;
constexpr u32 DEFAULT_PHOTON_DEPTH = 4;
constexpr u32 DEFAULT_RAY_DEPTH = 4;
constexpr f32 DEFAULT_RAY_STEP = 0.0001f;
constexpr const char *DEFAULT_MODEL = "./models/cornell/smooth_sphere.glb";
constexpr const char *DEFAULT_OUTPUT_PATH = "output.png";
constexpr u32 DEFAULT_SEED = 2137;
constexpr u32 DEFAULT_N_THREADS = 6;
constexpr u32 DEFAULT_IMAGE_ITERS = 1;
constexpr f32 DEFAULT_SEARCH_RADIUS = 0.2f;

#define ARG_TABLE(X)                                                                                                   \
    X("-r", resolution, u32, parse_u32, DEFAULT_RESOLUTION, "resolution")                                              \
    X("-s", samples, u32, parse_u32, DEFAULT_SAMPLES, "number of samples")                                             \
    X("-p", photons_per_light, u32, parse_u32, DEFAULT_PHOTONS_PER_LIGHT, "average photons per light source")          \
    X("-m", model, std::string, parse_string, DEFAULT_MODEL, "model name")                                             \
    X("-o", output_path, std::string, parse_string, DEFAULT_OUTPUT_PATH, "output image path")                          \
    X("-t", n_threads, u32, parse_u32, DEFAULT_N_THREADS, "number of CPU threads")                                     \
    X("-i", image_iters, u32, parse_u32, DEFAULT_IMAGE_ITERS, "image iterations")                                      \
    X("--photon-depth", photon_depth, u32, parse_u32, DEFAULT_PHOTON_DEPTH, "photon emission depth")                   \
    X("--ray-step", ray_step, f32, parse_f32, DEFAULT_RAY_STEP, "ray step as a fraction of scene diagonal")            \
    X("--ray-depth", ray_depth, u32, parse_u32, DEFAULT_RAY_DEPTH, "raytracing depth")                                 \
    X("--seed", seed, u32, parse_u32, DEFAULT_SEED, "rng seed")                                                        \
    X("--search-radius", search_radius, f32, parse_f32, DEFAULT_SEARCH_RADIUS,                                         \
      "photon search radius as a fraction of scene diagonal")

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
