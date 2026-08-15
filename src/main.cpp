#include "cmd_args.hpp"
#include "logger.hpp"
#include "opencl_ctx.hpp"
#include "printers.hpp"
#include "random.h"
#include "ray.h"
#include "scene.hpp"
#include "stb_image_write.h"
#include "texture_meta.h"

#include <glm/glm.hpp>
#include <iostream>
#include <vector>

Ray get_camera_ray(const Camera &cam, f32 s, f32 t) {
    glm::vec3 position(cam.position.x, cam.position.y, cam.position.z);
    glm::vec3 lower_left_corner(cam.lower_left_corner.x, cam.lower_left_corner.y, cam.lower_left_corner.z);
    glm::vec3 horizontal(cam.horizontal.x, cam.horizontal.y, cam.horizontal.z);
    glm::vec3 vertical(cam.vertical.x, cam.vertical.y, cam.vertical.z);

    glm::vec3 direction = lower_left_corner + s * horizontal + t * vertical - position;
    glm::vec3 dir_n = glm::normalize(direction);

    Ray r{};
    r.origin = float4{{position.x, position.y, position.z, 0.0f}};
    r.dir = float4{{dir_n.x, dir_n.y, dir_n.z, 0.0f}};
    return r;
}

void generate_primary_rays(RngState &rng, const Camera &cam, u32 image_width, u32 image_height, u32 image_iters,
                           std::vector<float4> &origins, std::vector<float4> &dirs) {
    origins.resize(static_cast<usize>(image_width) * image_height * image_iters);
    dirs.resize(static_cast<usize>(image_width) * image_height * image_iters);

    for (u32 y = 0; y < image_height; y++) {
        for (u32 x = 0; x < image_width; x++) {
            for (u32 j = 0; j < image_iters; j++) {
                const f32 s = (x + 0.5f + random_float(&rng) - 0.5f) / static_cast<f32>(image_width);
                const f32 t = 1.0f - (y + 0.5f + random_float(&rng) - 0.5f) / static_cast<f32>(image_height);

                Ray r = get_camera_ray(cam, s, t);

                usize idx = (static_cast<usize>(y) * image_width + x) * image_iters + j;
                origins[idx] = r.origin;
                dirs[idx] = r.dir;
            }
        }
    }
}

void init_logger() {
    auto &l = logger::Logger::instance();
    l.set_level(logger::Level::Debug);

    auto multi = std::make_unique<logger::MultiSink>();
    multi->add(std::make_unique<logger::ConsoleSink>());
    multi->add(std::make_unique<logger::FileSink>("phosphor.log", false));
    l.set_sink(std::move(multi));
}

void phosphor_main(const ArgsList &args) {
    ClContext ctx;
    print_opencl_data(ctx);

    SceneData scene = read_file(args.model.c_str());
    if (scene.triangles.empty()) {
        LOG_ERROR("scene has no triangles, nothing to render");
        return;
    }
    const Camera &cam = scene.cameras[*scene.chosen_camera];

    usize n_tris = scene.triangles.size();
    std::vector<float4> tv0(n_tris), tv1(n_tris), tv2(n_tris);
    std::vector<float2> tuv0(n_tris), tuv1(n_tris), tuv2(n_tris);
    std::vector<u32> tmat(n_tris);
    for (usize i = 0; i < n_tris; i++) {
        const auto &t = scene.triangles[i];
        tv0[i] = t.v0;
        tv1[i] = t.v1;
        tv2[i] = t.v2;
        tuv0[i] = t.uv0;
        tuv1[i] = t.uv1;
        tuv2[i] = t.uv2;
        tmat[i] = t.mat_index;
    }

    std::vector<TextureMeta> tex_meta(scene.textures.size());
    std::vector<u8> tex_atlas;
    for (usize i = 0; i < scene.textures.size(); i++) {
        const auto &tex = scene.textures[i];
        TextureMeta m{};
        m.atlas_offset = static_cast<u32>(tex_atlas.size());
        m.width = tex.width;
        m.height = tex.height;
        m.channels = 3;
        m.mip_levels_count = static_cast<u32>(tex.tex_offsets.size());
        m.mip_table_offset = 0;
        tex_meta[i] = m;
        tex_atlas.insert(tex_atlas.end(), tex.tex_atlas.begin(), tex.tex_atlas.end());
    }
    if (tex_atlas.empty())
        tex_atlas.push_back(0);

    RngState rng = pcg_seed(args.seed);
    std::vector<float4> h_origin, h_dir;
    generate_primary_rays(rng, cam, args.resolution, args.resolution, args.image_iters, h_origin, h_dir);
    usize n_rays = h_origin.size();

    cl::Buffer d_origin(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n_rays * sizeof(float4), h_origin.data());
    cl::Buffer d_dir(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n_rays * sizeof(float4), h_dir.data());
    cl::Buffer d_tv0(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n_tris * sizeof(float4), tv0.data());
    cl::Buffer d_tv1(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n_tris * sizeof(float4), tv1.data());
    cl::Buffer d_tv2(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n_tris * sizeof(float4), tv2.data());
    cl::Buffer d_tuv0(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n_tris * sizeof(float2), tuv0.data());
    cl::Buffer d_tuv1(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n_tris * sizeof(float2), tuv1.data());
    cl::Buffer d_tuv2(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n_tris * sizeof(float2), tuv2.data());
    cl::Buffer d_tmat(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, n_tris * sizeof(u32), tmat.data());
    cl::Buffer d_materials(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
                           scene.materials.size() * sizeof(Material), scene.materials.data());
    cl::Buffer d_tex_meta(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tex_meta.size() * sizeof(TextureMeta),
                          tex_meta.data());
    cl::Buffer d_tex_atlas(ctx.context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, tex_atlas.size(), tex_atlas.data());
    cl::Buffer d_out(ctx.context, CL_MEM_WRITE_ONLY, n_rays * sizeof(float4));

    cl::Program program = ctx.build_program(std::string(PROJECT_DIR) + "/kernels/naive_hit_test.cl", "-I./include");
    cl::Kernel kernel(program, "naive_hit_test");

    kernel.setArg(0, d_origin);
    kernel.setArg(1, d_dir);
    kernel.setArg(2, n_rays);
    kernel.setArg(3, d_tv0);
    kernel.setArg(4, d_tv1);
    kernel.setArg(5, d_tv2);
    kernel.setArg(6, d_tuv0);
    kernel.setArg(7, d_tuv1);
    kernel.setArg(8, d_tuv2);
    kernel.setArg(9, d_tmat);
    kernel.setArg(10, n_tris);
    kernel.setArg(11, d_materials);
    kernel.setArg(12, d_tex_meta);
    kernel.setArg(13, d_tex_atlas);
    kernel.setArg(14, d_out);

    ctx.queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(n_rays), cl::NullRange);

    std::vector<float4> h_out(n_rays);
    ctx.queue.enqueueReadBuffer(d_out, CL_TRUE, 0, n_rays * sizeof(float4), h_out.data());

    usize n_pixels = static_cast<usize>(args.resolution) * args.resolution;
    std::vector<u8> ldr(n_pixels * 3);
    for (usize p = 0; p < n_pixels; p++) {
        glm::vec3 accum(0.0f);
        for (u32 j = 0; j < args.image_iters; j++) {
            const float4 &c = h_out[p * args.image_iters + j];
            accum += glm::vec3(c.x, c.y, c.z);
        }
        accum /= static_cast<f32>(args.image_iters);

        ldr[p * 3 + 0] = static_cast<u8>(std::clamp(accum.x, 0.0f, 1.0f) * 255.0f);
        ldr[p * 3 + 1] = static_cast<u8>(std::clamp(accum.y, 0.0f, 1.0f) * 255.0f);
        ldr[p * 3 + 2] = static_cast<u8>(std::clamp(accum.z, 0.0f, 1.0f) * 255.0f);
    }
    stbi_write_png(args.output_path.c_str(), static_cast<i32>(args.resolution), static_cast<i32>(args.resolution), 3,
                   ldr.data(), static_cast<i32>(args.resolution) * 3);

    LOG_INFO("wrote {}", args.output_path);
}

i32 main(i32 argc, char **argv) {
    init_logger();

    ArgParser arg_parser(argc, argv, std::cout);
    try {
        auto args = arg_parser.parse_all();
        LOG_INFO("chosen parameters:");
        arg_parser.print_values(args);

        phosphor_main(args);
    } catch (const HelpRequested &) {
        arg_parser.print_help();
        return 0;
    } catch (const ArgParseError &e) {
        LOG_ERROR("parsing arguments: {}", e.what());
        arg_parser.print_help();
        return 1;
    }

    return 0;
}
