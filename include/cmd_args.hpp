#ifndef PHOSPHOR_CMD_ARGS_HPP
#define PHOSPHOR_CMD_ARGS_HPP

#include "typedefs.h"

#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

constexpr u32 DEFAULT_WIDTH = 1024;
constexpr u32 DEFAULT_HEIGHT = 1024;
constexpr u32 DEFAULT_PHOTONS = 1048576;
constexpr f32 DEFAULT_RAY_STEP = 0.0001f;
constexpr u32 DEFAULT_SEED = 2137;
constexpr u32 DEFAULT_MAX_PHOTON_BOUNCES = 16;
constexpr u32 DEFAULT_GRID_RES = 64;
constexpr f32 DEFAULT_DEFOCUS_ANGLE = 0.0;
constexpr f32 DEFAULT_FOCUS_DISTANCE = 1.0;
constexpr u32 DEFAULT_SPPM_ROUNDS = 64;
constexpr f32 DEFAULT_SPPM_ALPHA = 0.7f;
constexpr f32 DEFAULT_EXPOSURE = 1.0f;
constexpr u32 DEFAULT_DIRECT_SAMPLES = 16;
constexpr u32 DEFAULT_WORKGROUP_SIZE = 32;
constexpr const char *DEFAULT_MODEL_PATH = "./models/sample/sample.glb";
constexpr const char *DEFAULT_OUTPUT_DIR = "./phosphor_output";

#define ARG_TABLE(X)                                                                                                   \
    X("--help", help, bool, parse_bool, false, "show help?")                                                           \
    X("-w", width, u32, parse_u32, DEFAULT_WIDTH, "image width (px)")                                                  \
    X("-h", height, u32, parse_u32, DEFAULT_HEIGHT, "image height (px)")                                               \
    X("-p", photons, u32, parse_u32, DEFAULT_PHOTONS, "number of photons to emit per SPPM round")                      \
    X("-m", model, std::string, parse_string, DEFAULT_MODEL_PATH, "gltf model path")                                   \
    X("-o", output_dir, std::string, parse_string, DEFAULT_OUTPUT_DIR, "output directory prefix")                      \
    X("--max-bounces", max_photon_bounces, u32, u32_range(1, 32), DEFAULT_MAX_PHOTON_BOUNCES,                          \
      "max number of bounces for a single photon")                                                                     \
    X("--grid-res", grid_res, u32, u32_range(1, 128), DEFAULT_GRID_RES, "spatial hash resolution")                     \
    X("--direct-samples", direct_samples, u32, u32_range(1, 128), DEFAULT_DIRECT_SAMPLES,                              \
      "number of direct lighting rays per pixel per camera pass")                                                      \
    X("--workgroup", workgroup_size, u32, u32_range(1, 1024), DEFAULT_WORKGROUP_SIZE,                                  \
      "size of OpenCL workgroups (number of threads)")                                                                 \
    X("--sppm-rounds", sppm_rounds, u32, parse_u32, DEFAULT_SPPM_ROUNDS, "number of SPPM rounds")                      \
    X("--sppm-alpha", sppm_alpha, f32, parse_f32, DEFAULT_SPPM_ALPHA, "SPPM radius reduction factor")                  \
    X("--ray-step", ray_step, f32, parse_f32, DEFAULT_RAY_STEP, "ray step as a fraction of scene diagonal")            \
    X("--defocus-angle", defocus_angle, f32, parse_f32, DEFAULT_DEFOCUS_ANGLE,                                         \
      "variation angle of rays through each pixel")                                                                    \
    X("--focus-distance", focus_distance, f32, parse_f32, DEFAULT_FOCUS_DISTANCE,                                      \
      "distance from the camera where an object is perfectly in focus")                                                \
    X("--exposure", exposure, f32, parse_f32, DEFAULT_EXPOSURE, "color exposure multiplier")                           \
    X("--seed", seed, u32, parse_u32, DEFAULT_SEED, "rng seed")                                                        \
    X("--snapshots", save_snapshots, bool, parse_bool, false, "should rendering snapshots be saved?")                  \
    X("--envmap", envmap, std::string, parse_string, "", "path to an HDR environment map")

struct ArgsList {
    std::string dataset_path;

    std::unordered_set<std::string> provided_flags;
    bool was_provided(const char *flag) const { return provided_flags.contains(flag); }

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
    std::string build_image_metadata(const ArgsList &args) const;

  private:
    static std::unordered_map<std::string, void (ArgParser::*)(ArgsList &)> flag_parsers;

#define X(flag, field, type, parser, default_val, help) void parse_##field(ArgsList &list);
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

#endif // PHOSPHOR_CMD_ARGS_HPP
