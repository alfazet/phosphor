#include "scene.hpp"
#include "image.hpp"
#include "logger.hpp"
#include "random.hpp"
#include "texture.hpp"

#include <thread>

void Scene::add_point_light(const PointLight &light) { point_lights_.push_back(light); }
void Scene::add_textured_light(const TexturedLight &light) { textured_lights_.push_back(light); }
void Scene::add_triangle(const Triangle &object) { triangles_.push_back(object); }
void Scene::add_camera(const Camera &camera) { cameras_.push_back(camera); }
void Scene::add_texture(const Texture &texture) { textures_.push_back(texture); }

static LightSample sample_textured_light(RngState &rng, const TexturedLight &light, const Scene &scene,
                                         f32 photon_fraction);
static LightSample sample_point_light(RngState &rng, const PointLight &l);
static LightSample sample_area_light(RngState &rng, const AreaLight &l);

bool Scene::hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out, vec2 &uv) const {
    HitRecord temp;
    bool hit_anything = false;
    f32 closest = t_max;
    const Triangle *closest_t = nullptr;

    // space for improvement - do not check all objects in scene
    for (const auto &object : triangles_) {
        if (object.hit(r, t_min, closest, temp, textures())) {
            hit_anything = true;
            closest = temp.t;
            rec = temp;
            closest_t = &object;
        }
    }

    if (hit_anything) {
        mat_out = closest_t->mat_;
        uv = compute_bary(rec.bary, closest_t->uv0_, closest_t->uv1_, closest_t->uv2_);
    }

    return hit_anything;
}

void Scene::generate_image_row(RngState &rng, Image &img, u32 row_number, u32 image_height, u32 image_width, u32 n,
                               u32 image_iters) {
    u32 y = row_number;
    HitRecord rec;
    Material mat;
    vec2 uv;
    for (u32 x = 0; x < image_width; x++) {
        vec3 color = vec3(0.0f);
        for (i32 j = 0; j < image_iters; j++) {
            const f32 s = ((x + 0.5f + random_float(rng) - 0.5f) / static_cast<f32>(image_width));
            const f32 t = (1.0f - (y + 0.5f + random_float(rng) - 0.5f) / static_cast<f32>(image_height));
            Ray r = get_camera().get_ray(rng, s, t);

            if (hit(r, 0.001f, std::numeric_limits<f32>::max(), rec, mat, uv))
                color += get_color(r, rec, n, mat, uv, 5);
        }
        img.set_pixel(x, y, color / static_cast<f32>(image_iters));
    }
}

void Scene::run_thread_image_generation(RngState &rng, u32 offset, u32 n_threads, ProgressScope &img_progress,
                                        Image &img, u32 image_height, u32 image_width, u32 n, u32 image_iters) {
    for (u32 i = offset; i < image_height; i += n_threads) {
        generate_image_row(rng, img, i, image_height, image_width, n, image_iters);
        img_progress.increase(1);
    }
}

void Scene::generate_image(RngState rng, u32 image_height, u32 n, u32 photons_per_light, u32 max_bounces,
                           const char *output_path, u32 n_threads, u32 image_iters) {
    TimerScope timer_scope("generating image");

    this->rng = rng;
    if (point_lights_.empty() && textured_lights_.empty())
        LOG_ERROR("scene contains no lights");
    if (triangles_.empty())
        LOG_ERROR("scene contains no triangles");

    const u32 image_width = image_height * get_camera().aspect_ratio;
    Image img(image_width, image_height);
    emit(photons_per_light, max_bounces, n_threads);

    ProgressScope img_progress("generating image", image_height);
    std::thread threads[n_threads];
    for (u32 i = 0; i < n_threads; i++) {
        threads[i] = std::thread(&Scene::run_thread_image_generation, this, std::ref(rng), i, n_threads,
                                 std::ref(img_progress), std::ref(img), image_height, image_width, n, image_iters);
    }
    for (u32 i = 0; i < n_threads; i++) {
        threads[i].join();
    }

    img.write_png(output_path);

    LOG_INFO("saved image to {}", output_path);
}

void Scene::trace_photon(u32 id, const Ray &r, vec3 power, u32 depth, u32 max_bounces) {
    if (depth >= max_bounces)
        return;

    HitRecord rec;
    Material mat;
    vec2 uv;
    if (!hit(r, 0.001f, std::numeric_limits<f32>::max(), rec, mat, uv))
        return;

    f32 phi = glm::atan(r.direction.y, r.direction.x);
    f32 theta = glm::acos(glm::clamp(r.direction.z, -1.0f, 1.0f));
    photon_map_.store(id, {rec.point, power, phi, theta});
    vec3 base_color = vec3(mat.base_color);
    if (mat.diff_index.has_value()) {
        base_color *= sample(&textures_[*mat.diff_index], uv);
    }

    f32 metallic = mat.metallic;
    f32 roughness = mat.roughness;
    if (mat.metal_rough_index.has_value()) {
        const Texture *tex = &textures_[*mat.metal_rough_index];
        metallic *= sample(tex, uv, CHANNEL_B);
        roughness *= sample(tex, uv, CHANNEL_G);
    }
    const f32 xi = random_float(this->rng);

    // https://github.com/KhronosGroup/glTF/blob/77b44be7bef26e01fb0b140e3d5bb1716421c5e9/extensions/2.0/Archived/KHR_materials_pbrSpecularGlossiness/examples/convert-between-workflows-bjs/js/babylon.pbrUtilities.js#L12
    vec3 dielectric_specular = vec3(0.04f);
    vec3 s = glm::mix(dielectric_specular, base_color, metallic);
    f32 max_s = glm::max(s.r, glm::max(s.g, s.b));
    vec3 d = base_color * ((1.0f - dielectric_specular.r) * (1.0f - metallic) / (1.0f - max_s));

    f32 alpha = roughness * roughness;
    f32 g1 = smith_g1_ggx(glm::acos(glm::dot(r.direction, rec.normal)), alpha);
    vec3 s_eff = s * g1;

    f32 sum_d = d.r + d.g + d.b;
    f32 sum_s = s_eff.r + s_eff.g + s_eff.b;
    f32 sum_total = sum_d + sum_s;
    f32 rho_r = glm::max(d.r + s_eff.r, glm::max(d.g + s_eff.g, d.b + s_eff.b));
    f32 rho_d = sum_total > EPS ? (rho_r * sum_d / sum_total) : 0.0f;
    f32 rho_s = rho_r - rho_d;

    if (xi < rho_d) {
        const vec3 new_dir = random_in_hemisphere_cosine(this->rng, rec.normal);
        trace_photon(id, Ray(rec.point, new_dir), power * d / rho_d, depth + 1, max_bounces);
    } else if (xi < rho_s + rho_d) {
        const vec3 new_dir = ggx_sample_direction(this->rng, r.direction, rec.normal, roughness);
        if (new_dir != ZERO_VEC)
            trace_photon(id, Ray(rec.point, new_dir), power * s_eff / rho_s, depth + 1, max_bounces);
    }
    // else the photon is absorbed
}

void Scene::emit(u32 photons_per_light, u32 max_bounces, u32 n_threads) {
    photon_map_.init_thread_buffers(n_threads);
    // TODO: change to random sampling
    u32 total_photons = photons_per_light * (point_lights_.size() + textured_lights_.size());
    ProgressScope progress("emitting photons", total_photons);

    for (const auto &light : point_lights_) {
        const vec3 photon_power = vec3(light.power / static_cast<f32>(photons_per_light));
        u32 photons_left = photons_per_light;
        u32 photons_per_thread = (photons_per_light + n_threads - 1) / n_threads;

        std::thread threads[n_threads];
        for (u32 i = 0; i < n_threads; i++) {
            u32 photons_to_cast = glm::min(photons_per_thread, photons_left);
            threads[i] = std::thread(&Scene::run_thread_emit, this, i, photons_to_cast, std::ref(progress), max_bounces,
                                     photon_power, light.pos);
            photons_left -= photons_per_thread;
        }
        for (u32 i = 0; i < n_threads; i++) {
            threads[i].join();
        }
    }

    for (const auto &light : textured_lights_) {
        const f32 fraction = 1.0f / static_cast<f32>(photons_per_light);
        u32 photons_left = photons_per_light;
        u32 photons_per_thread = (photons_per_light + n_threads - 1) / n_threads;

        std::thread threads[n_threads];
        for (u32 i = 0; i < n_threads; i++) {
            u32 photons_to_cast = glm::min(photons_per_thread, photons_left);
            threads[i] = std::thread(&Scene::run_thread_textured_emit, this, i, photons_to_cast, std::ref(progress),
                                     max_bounces, fraction, std::ref(light));
            photons_left -= photons_per_thread;
        }
        for (u32 i = 0; i < n_threads; i++) {
            threads[i].join();
        }
    }

    photon_map_.merge_thread_buffers();
    TimerScope timer_("building photon map kd-tree", true);
    photon_map_.build();
    timer_.stop();
}

void Scene::run_thread_emit(u32 id, u32 photons, ProgressScope &img_progress, u32 max_bounces, const vec3 photon_power,
                            const vec3 light_pos) {
    for (u32 i = 0; i < photons; i++) {
        const vec3 dir = random_unit_vector(this->rng);
        trace_photon(id, Ray(light_pos, dir), photon_power, 0, max_bounces);
        img_progress.increase(1);
    }
}

void Scene::run_thread_textured_emit(u32 id, u32 photons, ProgressScope &img_progress, u32 max_bounces, f32 fraction,
                                     const TexturedLight &light) {
    for (u32 i = 0; i < photons; i++) {
        auto sample = sample_textured_light(this->rng, light, *this, fraction);
        trace_photon(id, sample.ray, sample.power, 0, max_bounces);
        img_progress.increase(1);
    }
}

vec3 Scene::get_color(const Ray &ray, const HitRecord &rec, const u32 n, Material &mat, vec2 &uv, u32 depth_left) {
    if (depth_left == 0)
        return BLACK;
    auto pos = rec.point;
    auto normal = rec.normal;

    // makes emissive surfaces visible even when the photons have nothing to bounce off of
    vec3 emissive = BLACK;
    if (mat.emis_index.has_value())
        emissive = sample(&textures_[*mat.emis_index], uv);

    std::vector<const Photon *> nearest;
    photon_map_.locate(pos, n, 1000.0f, nearest);
    if (nearest.empty())
        return emissive;

    vec3 flux(0.0f);
    float max_dist_sq = 0.0f;
    for (auto p : nearest) {
        vec3 from(glm::cos(p->phi) * glm::sin(p->theta), glm::sin(p->phi) * glm::sin(p->theta), glm::cos(p->theta));
        // don't count photons coming from "inside" the surface
        if (glm::dot(from, normal) > 0.0f)
            continue;
        float dist = glm::dot(p->pos - pos, p->pos - pos);
        max_dist_sq = glm::max(max_dist_sq, dist);
        flux += p->power;
    }

    f32 area = glm::pi<f32>() * max_dist_sq;
    if (area < EPS)
        return emissive;

    vec3 base_color = mat.base_color;
    if (mat.diff_index.has_value())
        base_color = sample(&textures_[*mat.diff_index], uv);

    f32 metallic = mat.metallic;
    f32 roughness = mat.roughness;
    if (mat.metal_rough_index.has_value()) {
        const Texture *tex = &textures_[*mat.metal_rough_index];
        metallic *= sample(tex, uv, CHANNEL_B);
        roughness *= sample(tex, uv, CHANNEL_G);
    }

    vec3 reflected_color = BLACK;
    const vec3 new_dir = ggx_sample_direction(this->rng, ray.direction, normal, roughness);
    if (new_dir != ZERO_VEC) {
        HitRecord reflected_rec;
        Material reflected_mat;
        vec2 reflected_uv;
        Ray reflected = Ray(pos, glm::normalize(new_dir));
        if (hit(reflected, 0.001f, std::numeric_limits<f32>::max(), reflected_rec, reflected_mat, reflected_uv))
            reflected_color = get_color(reflected, reflected_rec, n, reflected_mat, reflected_uv, depth_left - 1);
    }

    f32 occlusion = 1.0f;
    if (mat.occlusion_index.has_value())
        occlusion = sample(&textures_[*mat.occlusion_index], uv, CHANNEL_R);

    vec3 diffuse_color = flux * base_color * occlusion / area;
    return glm::mix(diffuse_color, reflected_color, metallic) + emissive;
}

Camera &Scene::get_camera() {
    ASSERT(chosen_camera >= 0, "no camera set");
    return cameras_[chosen_camera];
}

void Scene::set_camera(i32 i) {
    ASSERT(i >= 0 && i < cameras_.size(), "cannot set camera index out of range");
    chosen_camera = i;
    LOG_INFO("using camera {}", i);
}

void Scene::add_default_camera() {
    BoundingBox b = get_bounding_box();
    cameras_.emplace_back(b.min, b.max, DEFAULT_CAMERA_HFOV, DEFAULT_CAMERA_RATIO);
}

BoundingBox Scene::get_bounding_box() const {
    vec3 minp(INF);
    vec3 maxp(-INF);

    for (const auto &tri : triangles_) {
        minp = glm::min(minp, tri.v0_);
        minp = glm::min(minp, tri.v1_);
        minp = glm::min(minp, tri.v2_);

        maxp = glm::max(maxp, tri.v0_);
        maxp = glm::max(maxp, tri.v1_);
        maxp = glm::max(maxp, tri.v2_);
    }

    return {minp, maxp};
}

static LightSample sample_point_light(RngState &rng, const PointLight &l) {
    return {Ray(l.pos, random_unit_vector(rng)), l.power};
}

static LightSample sample_area_light(RngState &rng, const AreaLight &l) {
    float u = random_float(rng);
    float v = random_float(rng);
    vec3 pos = l.position + u * l.edge_u + v * l.edge_v;
    vec3 normal = normalize(cross(l.edge_u, l.edge_v));
    vec3 dir = random_in_hemisphere(rng, normal);

    return {Ray(pos, dir), l.emission};
}

LightSample sample_textured_light(RngState &rng, const TexturedLight &light, const Scene &scene, f32 photon_fraction) {
    const Texture &tex = scene.textures()[light.tex_index];

    // pick a random triangle from this mesh's range
    // TODO: importance sampling proportional to triangle's area
    u32 i = static_cast<u32>(random_float(rng) * light.triangle_count);
    if (i >= light.triangle_count)
        i = light.triangle_count - 1;
    const Triangle &tri = scene.triangles()[light.triangle_start + i];

    f32 u = random_float(rng);
    f32 v = random_float(rng);
    if (u + v > 1.0f) {
        u = 1.0f - u;
        v = 1.0f - v;
    }

    vec3 point = tri.point_at(u, v);
    vec2 uv = tri.uv_at(u, v);
    vec3 emission = sample(&tex, uv);
    vec3 normal = tri.get_normal(vec2(u, v), scene.textures());
    point += normal * 0.001f;
    vec3 dir = random_in_hemisphere(rng, normal);
    vec3 power = emission * light.total_area * glm::pi<f32>() * photon_fraction;

    return {Ray(point, dir), power};
}
