#include "host/output.hpp"
#include "common/constants.h"
#include "host/logger.hpp"

#include "stb_image_write.h"

#include <algorithm>
#include <cmath>
#include <execution>
#include <numeric>
#include <sstream>

RenderSize resolve_render_size(const ArgsList &args, f32 base_aspect) {
    ASSERT(base_aspect > 0.0f, "base aspect must be positive");

    bool has_w = args.was_provided("-w");
    bool has_h = args.was_provided("-h");
    u32 width = args.width;
    u32 height = args.height;

    if ((has_w && has_h) || (!has_w && !has_h)) {
        f32 aspect = static_cast<f32>(width) / static_cast<f32>(height);
        f32 rel_diff = std::abs(aspect - base_aspect) / base_aspect;
        if (rel_diff > 0.1f)
            LOG_WARN("output aspect {:.3f} ({}x{}) differs from the camera's aspect {:.3f}", aspect, width, height,
                     base_aspect);
        return {args.width, args.height, aspect};
    }

    if (has_w) {
        height = std::max(1u, static_cast<u32>(std::lround(width / base_aspect)));
        return {width, height, base_aspect};
    }
    width = std::max(1u, static_cast<u32>(std::lround(height * base_aspect)));

    return {width, height, base_aspect};
}

// https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve
static inline f32 tone_map(f32 x, f32 exposure) {
    x *= exposure;
    f32 a = 2.51f, b = 0.03f, c = 2.43f, d = 0.59f, e = 0.14f;
    f32 out = (x * (a * x + b)) / (x * (c * x + d) + e);

    return std::clamp(out, 0.0f, 1.0f);
}

static std::vector<u8> apply_tonemap(u32 width, u32 height, f32 exposure, const std::vector<float4> &flux,
                                     const std::vector<float4> &total_radiance, u64 total_photons, u32 sppm_rounds) {
    u32 n_pixels = width * height;
    std::vector<u8> ldr(n_pixels * 3);
    f32 inv_photons = (total_photons > 0) ? (1.0f / static_cast<f32>(total_photons)) : 0.0f;
    f32 inv_rounds = (sppm_rounds > 0) ? (1.0f / static_cast<f32>(sppm_rounds)) : 0.0f;

    std::vector<u32> indices(n_pixels);
    std::iota(indices.begin(), indices.end(), 0);
    std::for_each(std::execution::par_unseq, indices.begin(), indices.end(), [&](u32 p) {
        float4 flux_and_r_sq = flux[p];
        f32 area = PI * flux_and_r_sq.w;

        f32 indirect_r = 0.0f, indirect_g = 0.0f, indirect_b = 0.0f;
        if (area > 0.0f) {
            f32 per_area = inv_photons / area;
            indirect_r = flux_and_r_sq.x * per_area;
            indirect_g = flux_and_r_sq.y * per_area;
            indirect_b = flux_and_r_sq.z * per_area;
        }

        const float4 &rad = total_radiance[p];
        f32 r = rad.x * inv_rounds + indirect_r;
        f32 g = rad.y * inv_rounds + indirect_g;
        f32 b = rad.z * inv_rounds + indirect_b;
        ldr[p * 3 + 0] = static_cast<u8>(tone_map(r, exposure) * 255.0f);
        ldr[p * 3 + 1] = static_cast<u8>(tone_map(g, exposure) * 255.0f);
        ldr[p * 3 + 2] = static_cast<u8>(tone_map(b, exposure) * 255.0f);
    });

    return ldr;
}

void write_job(const WriteJob &job) {
    stbi_write_png(job.path.c_str(), static_cast<i32>(job.width), static_cast<i32>(job.height), 3, job.ldr.data(),
                   static_cast<i32>(job.width) * 3);

    // TODO: replace with something less insane
    std::ostringstream cmd;
    cmd << "exiftool -q -overwrite_original "
        << "-Comment=\"" << job.metadata << "\" "
        << "'" << job.path.string() << "'";

    u32 ret = static_cast<u32>(std::system(cmd.str().c_str()));
    if (ret != 0)
        LOG_ERROR("exiftool failed to write metadata for {} (exit code {})", job.path.c_str(), ret);
}

ImageWriter::ImageWriter() : worker([this] { this->worker_loop(); }) {}

ImageWriter::~ImageWriter() {
    flush();
    {
        std::lock_guard<std::mutex> lock(mutex);
        stop = true;
    }
    cv_jobs.notify_one();
    if (worker.joinable())
        worker.join();
}

void ImageWriter::submit(const std::filesystem::path &path, u32 width, u32 height, f32 exposure,
                         const std::vector<float4> &flux, const std::vector<float4> &total_radiance, u64 total_photons,
                         u32 sppm_rounds, const std::string &metadata) {
    std::vector<u8> ldr = apply_tonemap(width, height, exposure, flux, total_radiance, total_photons, sppm_rounds);

    WriteJob job{path, width, height, metadata, std::move(ldr)};
    {
        std::lock_guard<std::mutex> lock(mutex);
        this->jobs.push(std::move(job));
        this->active_jobs++;
    }
    this->cv_jobs.notify_one();
}

void ImageWriter::flush() {
    std::unique_lock<std::mutex> lock(mutex);
    cv_flush.wait(lock, [this] { return this->jobs.empty() && this->active_jobs == 0; });
}

void ImageWriter::worker_loop() {
    while (true) {
        WriteJob job;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv_jobs.wait(lock, [this] { return !this->jobs.empty() || this->stop; });
            if (this->jobs.empty() && this->stop)
                return;
            job = std::move(this->jobs.front());
            this->jobs.pop();
        }
        write_job(job);
        {
            std::lock_guard<std::mutex> lock(mutex);
            this->active_jobs--;
            if (this->active_jobs == 0 && this->jobs.empty()) {
                this->cv_flush.notify_all();
            }
        }
    }
}
