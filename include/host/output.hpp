#ifndef PHOSPHOR_OUTPUT_HPP
#define PHOSPHOR_OUTPUT_HPP

#include "common/typedefs.h"
#include "host/cmd_args.hpp"

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

struct RenderSize {
    u32 width;
    u32 height;
    f32 aspect;
};

struct WriteJob {
    std::filesystem::path path;
    u32 width;
    u32 height;
    std::string metadata;
    std::vector<u8> ldr;
};

RenderSize resolve_render_size(const ArgsList &args, f32 base_aspect);

class ImageWriter {
  public:
    ImageWriter();

    ~ImageWriter();

    void submit(const std::filesystem::path &path, u32 width, u32 height, f32 exposure, const std::vector<float4> &flux,
                const std::vector<float4> &total_radiance, u64 total_photons, u32 sppm_rounds,
                const std::string &metadata);

    void flush();

  private:
    void worker_loop();

    std::thread worker;
    std::mutex mutex;
    std::condition_variable cv_jobs;
    std::condition_variable cv_flush;
    std::queue<WriteJob> jobs;
    u32 active_jobs = 0;
    bool stop = false;
};

#endif // PHOSPHOR_OUTPUT_HPP
