#ifndef PHOSPHOR_CMD_ARGS_HPP
#define PHOSPHOR_CMD_ARGS_HPP

#include "common/typedefs.h"

#include <ostream>
#include <string>
#include <unordered_map>
#include <unordered_set>

constexpr u32 DEFAULT_WIDTH = 1024;
constexpr u32 DEFAULT_HEIGHT = 1024;
constexpr f32 DEFAULT_EXPOSURE = 1.0f;

constexpr u32 DEFAULT_MAX_PHOTONS_PER_BATCH = (1 << 20);
constexpr u32 DEFAULT_WORKGROUP_SIZE = 32;
constexpr u32 DEFAULT_SEED = 2137;

constexpr u32 DEFAULT_PHOTONS = (1 << 20);
constexpr f32 DEFAULT_RAY_STEP = 0.0001f;
constexpr u32 DEFAULT_MAX_PHOTON_BOUNCES = 16;
constexpr u32 DEFAULT_GRID_RES = 64;
constexpr f32 DEFAULT_DEFOCUS_ANGLE = 0.0;
constexpr f32 DEFAULT_FOCUS_DISTANCE = 1.0;
constexpr u32 DEFAULT_SPPM_ROUNDS = 64;
constexpr f32 DEFAULT_SPPM_ALPHA = 0.7f;
constexpr u32 DEFAULT_DIRECT_SAMPLES = 16;

constexpr const char *DEFAULT_MODEL_PATH = "./models/sample/sample.glb";
constexpr const char *DEFAULT_OUTPUT_DIR = "./phosphor_output";
constexpr const char *DEFAULT_LOG_FILENAME = "./phosphor.log";
constexpr const char *DEFAULT_ENVMAP = ""; // no envmap by default
constexpr const bool DEFAULT_SNAPSHOTS = false;

#define ARG_TABLE(X)                                                                                                   \
    X("--help", help, bool, parse_bool, false, "show help?")                                                           \
    X("-w", width, u32, parse_u32, DEFAULT_WIDTH, "image width (px)")                                                  \
    X("-h", height, u32, parse_u32, DEFAULT_HEIGHT, "image height (px)")                                               \
    X("--exposure", exposure, f32, f32_non_neg(), DEFAULT_EXPOSURE, "color exposure multiplier")                       \
    X("--batch-size", max_photons_per_batch, u32, parse_u32, DEFAULT_MAX_PHOTONS_PER_BATCH,                            \
      "max. number of photons emitted in a single batch")                                                              \
    X("--workgroup", workgroup_size, u32, u32_range(1, 1024), DEFAULT_WORKGROUP_SIZE,                                  \
      "desired number of threads in a single OpenCL workgroup")                                                        \
    X("--seed", seed, u32, parse_u32, DEFAULT_SEED, "seed for the random number generator")                            \
    X("-p", photons_per_round, u32, parse_u32, DEFAULT_PHOTONS, "number of photons to emit (in each SPPM round)")      \
    X("--ray-step", ray_step, f32, f32_non_neg(), DEFAULT_RAY_STEP, "ray step as a fraction of scene radius")          \
    X("--max-bounces", max_photon_bounces, u32, u32_range(1, 32), DEFAULT_MAX_PHOTON_BOUNCES,                          \
      "max number of times a single photon can bounce during rendering")                                               \
    X("--grid-res", grid_res, u32, u32_range(1, 255), DEFAULT_GRID_RES, "resolution of the spatial hash structure")    \
    X("--defocus-angle", defocus_angle, f32, f32_non_neg(), DEFAULT_DEFOCUS_ANGLE,                                     \
      "variation angle of rays through each pixel (degrees)")                                                          \
    X("--focus-distance", focus_distance, f32, f32_non_neg(), DEFAULT_FOCUS_DISTANCE,                                  \
      "distance from the camera where an object will be perfectly in focus")                                           \
    X("--sppm-rounds", sppm_rounds, u32, parse_u32, DEFAULT_SPPM_ROUNDS, "number of SPPM rounds")                      \
    X("--sppm-alpha", sppm_alpha, f32, f32_non_neg(), DEFAULT_SPPM_ALPHA,                                              \
      "radius reduction ratio after each SPPM round")                                                                  \
    X("--direct-samples", direct_samples, u32, u32_range(1, 128), DEFAULT_DIRECT_SAMPLES,                              \
      "number of direct lighting rays per pixel per camera pass")                                                      \
    X("-m", model, std::string, parse_string, DEFAULT_MODEL_PATH, "path to the glTF file to render")                   \
    X("-o", output_dir, std::string, parse_string, DEFAULT_OUTPUT_DIR, "output directory prefix")                      \
    X("--log", log_filename, std::string, parse_string, DEFAULT_LOG_FILENAME, "log filename")                          \
    X("--envmap", envmap, std::string, parse_string, DEFAULT_ENVMAP, "path to an HDR environment map")                 \
    X("--snapshots", save_snapshots, bool, parse_bool, DEFAULT_SNAPSHOTS, "should rendering snapshots be saved?")

struct ArgsList {
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
