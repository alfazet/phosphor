#ifndef PHOSPHOR_CMD_ARGS_HPP
#define PHOSPHOR_CMD_ARGS_HPP

#include "typedefs.h"

#include <ostream>
#include <string>
#include <unordered_map>

constexpr const char *HELP_FLAG = "--help";

constexpr u32 DEFAULT_RES = 1024;
constexpr u32 DEFAULT_IMAGE_ITERS = 8;
constexpr u32 DEFAULT_SAMPLES = 64;
constexpr u32 DEFAULT_PHOTONS_PER_LIGHT = (1 << 18);
constexpr f32 DEFAULT_RAY_STEP = 0.0001f;
constexpr u32 DEFAULT_SEED = 2137;
constexpr u32 DEFAULT_GRID_RES = 128;
constexpr const char *DEFAULT_MODEL_PATH = "./models/sample/sample.glb";
constexpr const char *DEFAULT_OUTPUT_PATH = "output.png";

#define ARG_TABLE(X)                                                                                                   \
    X("-r", res, u32, parse_u32, DEFAULT_RES, "image resolution (px)")                                               \
    X("-i", image_iters, u32, parse_u32, DEFAULT_IMAGE_ITERS, "number of image iterations")                            \
    X("-s", samples, u32, parse_u32, DEFAULT_SAMPLES, "number of samples for photon gathering")                        \
    X("-p", photons, u32, parse_u32, DEFAULT_PHOTONS_PER_LIGHT,                                                        \
      "number of photons to emit (will be rounded up to power of 2)")                                                  \
    X("-m", model, std::string, parse_string, DEFAULT_MODEL_PATH, "gltf model path")                                   \
    X("-o", output_path, std::string, parse_string, DEFAULT_OUTPUT_PATH, "output image path")                          \
    X("-k", grid_res, u32, parse_u32, DEFAULT_GRID_RES, "spatial hash resolution")                                     \
    X("--ray-step", ray_step, f32, parse_f32, DEFAULT_RAY_STEP, "ray step as a fraction of scene diagonal")            \
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
    void write_image_metadata(const ArgsList &args) const;

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
