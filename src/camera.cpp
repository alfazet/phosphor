#include "camera.hpp"
#include "random.h"

Camera make_camera(vec3 position, vec3 look_at, vec3 up, f32 hfov_deg, f32 aspect) {
    Camera c{};
    vec3 w_dir = normalize(position - look_at);
    vec3 u_dir = normalize(cross(up, w_dir));
    vec3 v_dir = cross(w_dir, u_dir);

    f32 hfov_rad = glm::radians(hfov_deg);
    f32 half_width = glm::tan(hfov_rad * 0.5f);
    f32 half_height = half_width / aspect;

    vec3 horizontal = 2.0f * half_width * u_dir;
    vec3 vertical = 2.0f * half_height * v_dir;
    vec3 lower_left = position - 0.5f * horizontal - 0.5f * vertical - w_dir;

    c.position = vec3_to_float4(position);
    c.target = vec3_to_float4(look_at);
    c.up = vec3_to_float4(up);
    c.lower_left_corner = vec3_to_float4(lower_left);
    c.horizontal = vec3_to_float4(horizontal);
    c.vertical = vec3_to_float4(vertical);
    c.u = vec3_to_float4(u_dir);
    c.v = vec3_to_float4(v_dir);
    c.w = vec3_to_float4(w_dir);
    c.hfov = hfov_deg;
    c.aspect_ratio = aspect;

    return c;
}

Ray get_camera_ray(const Camera &cam, f32 s, f32 t) {
    vec3 position(cam.position.x, cam.position.y, cam.position.z);
    vec3 lower_left_corner(cam.lower_left_corner.x, cam.lower_left_corner.y, cam.lower_left_corner.z);
    vec3 horizontal(cam.horizontal.x, cam.horizontal.y, cam.horizontal.z);
    vec3 vertical(cam.vertical.x, cam.vertical.y, cam.vertical.z);
    vec3 direction = lower_left_corner + s * horizontal + t * vertical - position;
    vec3 dir_n = normalize(direction);

    Ray r{};
    r.origin = float4{{position.x, position.y, position.z, 0.0f}};
    r.dir = float4{{dir_n.x, dir_n.y, dir_n.z, 0.0f}};

    return r;
}

void generate_primary_rays(const Camera &cam, RngState &rng, u32 image_width, u32 image_height, u32 image_iters,
                           std::vector<float4> &origins, std::vector<float4> &dirs) {
    origins.resize(image_width * image_height * image_iters);
    dirs.resize(image_width * image_height * image_iters);

    for (u32 y = 0; y < image_height; y++) {
        for (u32 x = 0; x < image_width; x++) {
            for (u32 j = 0; j < image_iters; j++) {
                const f32 s = (x + 0.5f + random_float(&rng) - 0.5f) / static_cast<f32>(image_width);
                const f32 t = 1.0f - (y + 0.5f + random_float(&rng) - 0.5f) / static_cast<f32>(image_height);
                Ray r = get_camera_ray(cam, s, t);
                u32 idx = (y * image_width + x) * image_iters + j;
                origins[idx] = r.origin;
                dirs[idx] = r.dir;
            }
        }
    }
}
