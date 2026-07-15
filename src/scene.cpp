#include "scene.hpp"
#include "image.hpp"
#include "random.hpp"
#include "texture.hpp"

#include <ostream>

void Scene::add_point_light(const PointLight &light) { point_lights_.push_back(light); }
void Scene::add_textured_light(const TexturedLight &light) { textured_lights_.push_back(light); }
void Scene::add_triangle(const Triangle &object) { triangles_.push_back(object); }
void Scene::add_camera(const Camera &camera) { cameras_.push_back(camera); }
void Scene::add_texture(const Texture &texture) { textures_.push_back(texture); }

static LightSample sample_textured_light(RngState &rng, const TexturedLight &light, const Scene &scene,
                                         f32 photon_fraction);
static LightSample sample_point_light(RngState &rng, const PointLight &l);
static LightSample sample_area_light(RngState &rng, const AreaLight &l);

bool Scene::hit(const Ray &r, f32 t_min, f32 t_max, HitRecord &rec, Material &mat_out, i32 &texture_index,
                vec2 &uv) const {
    HitRecord temp;
    bool hit_anything = false;
    f32 closest = t_max;
    const Triangle *closest_t = nullptr;

    // space for improvement - do not check all objects in scene
    for (const auto &object : triangles_) {
        if (object.hit(r, t_min, closest, temp)) {
            hit_anything = true;
            closest = temp.t;
            rec = temp;
            closest_t = &object;
        }
    }

    if (hit_anything) {
        mat_out = closest_t->mat();
        texture_index = closest_t->index_;
        uv = (1.0f - rec.bary.x - rec.bary.y) * closest_t->uv0_ + rec.bary.x * closest_t->uv1_ +
             rec.bary.y * closest_t->uv2_;
    }

    return hit_anything;
}

void show_progress_bar(f32 percentage, const int width = 32) {
    printf("\r");
    for (u32 i = 0; i < percentage * width; i++) {
        printf("=");
    }
    for (u32 i = percentage * width; i < width - 1; i++) {
        printf("-");
    }
    fflush(stdout);
}

void Scene::generate_image(RngState rng, u32 image_height, u32 n, u32 photons_per_light, u32 max_bounces,
                           const char *output_path) {
    this->rng = rng;
    if (point_lights_.empty() && textured_lights_.empty())
        throw std::logic_error("No lights");
    if (triangles_.empty())
        throw std::logic_error("No triangles");
    const u32 image_width = image_height * get_camera().aspect_ratio();
    Image img(image_width, image_height);

    printf("emitting photons\n");
    emit(photons_per_light, max_bounces);
    printf("\n");

    HitRecord rec;
    Material mat;
    i32 texture_index;
    vec2 uv;
    Camera cam = get_camera();

    printf("generating image\n");
    for (u32 y = 0; y < image_height; y++) {
        for (u32 x = 0; x < image_width; x++) {
            const f32 s = (x + 0.5f) / static_cast<f32>(image_width);
            const f32 t = 1.0f - (y + 0.5f) / static_cast<f32>(image_height);
            Ray r = cam.get_ray(s, t);

            if (hit(r, 0.001f, std::numeric_limits<f32>::max(), rec, mat, texture_index, uv)) {
                img.set_pixel(x, y, get_color(rec.point, rec.normal, n, mat, texture_index, uv));
            }
        }
        show_progress_bar((y + 1) / (f32)image_height);
    }
    img.write_png(output_path);
    printf("\nsaved to %s\n", output_path);
}

void Scene::trace_photon(const Ray &r, vec3 power, u32 depth, u32 max_bounces) {
    if (depth >= max_bounces)
        return;

    HitRecord rec;
    Material mat;
    // TODO: replace all "-1 == error" moments with std::optional
    i32 texture_index = -1;
    vec2 uv;
    if (!hit(r, 0.001f, std::numeric_limits<f32>::max(), rec, mat, texture_index, uv))
        return;

    // std::printf("texture index: %i\n", texture_index);
    if (texture_index != -1) {
        vec3 sampled_color = sample(&textures_[texture_index], uv);
        power *= sampled_color;
    }

    f32 phi = glm::atan(r.direction.y, r.direction.x);
    f32 theta = glm::acos(glm::clamp(r.direction.z, -1.0f, 1.0f));
    photon_map_.store({rec.point, power, phi, theta});

    // source: page 63
    const f32 xi = random_float(this->rng);
    vec3 d = mat.diff;
    vec3 s = mat.spec;
    f32 rho_r = glm::max(d.r + s.r, glm::max(d.g + s.g, d.b + s.b));
    f32 rho_d = rho_r * (d.r + d.g + d.b) / (d.r + d.g + d.b + s.r + s.g + s.b);
    f32 rho_s = rho_r - rho_d;

    if (xi < rho_d) {
        const vec3 new_dir = random_in_hemisphere(this->rng, rec.normal);
        trace_photon(Ray(rec.point, new_dir), power * mat.diff / rho_d, depth + 1, max_bounces);
    } else if (xi < rho_s + rho_d) {
        const vec3 new_dir = glm::reflect(r.direction, rec.normal);
        trace_photon(Ray(rec.point, new_dir), power * mat.spec / rho_s, depth + 1, max_bounces);
    }
    // else the photon is absorbed
}

void Scene::emit(u32 photons_per_light, u32 max_bounces) {
    // TODO: change to random sampling
    i32 total_photons = photons_per_light * (point_lights_.size() + textured_lights_.size());
    i32 photons_done = 0;
    for (const auto &light : point_lights_) {
        const vec3 photon_power = vec3(light.power / static_cast<f32>(photons_per_light));
        for (u32 i = 0; i < photons_per_light; i++) {
            const vec3 dir = random_unit_vector(this->rng);
            trace_photon(Ray(light.pos, dir), photon_power, 0, max_bounces);
            show_progress_bar((photons_done + i + 1) / (f32)(total_photons));
        }
        photons_done += photons_per_light;
    }
    for (const auto &light : textured_lights_) {
        const f32 fraction = 1.0f / static_cast<f32>(photons_per_light);
        for (u32 i = 0; i < photons_per_light; i++) {
            auto sample = sample_textured_light(this->rng, light, *this, fraction);
            trace_photon(sample.ray, sample.power, 0, max_bounces);
            show_progress_bar((photons_done + i + 1) / (f32)(total_photons));
        }
        photons_done += photons_per_light;
    }

    photon_map_.build();
}

vec3 Scene::get_color(const vec3 &pos, const vec3 &normal, const u32 n, Material &mat, i32 &texture_index,
                      vec2 &uv) const {
    std::vector<const Photon *> nearest;
    photon_map_.locate(pos, n, 1000.0f, nearest);
    if (nearest.empty())
        return vec3(0.0f);

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
        return vec3(0.0f);

    return flux * mat.diff * (texture_index != -1 ? sample(&textures_[texture_index], uv) : vec3(1.0f, 1.0f, 1.0f)) /
           area;
}
void Scene::set_camera(i32 i) {
    if (i < 0 || i >= cameras_.size()) {
        throw std::out_of_range("cannot set camera index out of range");
    }
    chosen_camera = i;
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
    const Triangle &tri = scene.triangles()[light.triangle_index];
    const Texture &tex = scene.textures()[light.texture_index];

    f32 u = random_float(rng);
    f32 v = random_float(rng);
    if (u + v > 1.0f) {
        u = 1.0f - u;
        v = 1.0f - v;
    }

    vec3 point = tri.point_at(u, v);
    vec2 uv = tri.uv_at(u, v);
    vec3 emission = sample(&tex, vec2(u, v));
    vec3 normal = tri.normal();
    point += normal * 0.001f;
    vec3 dir = random_in_hemisphere(rng, normal);
    vec3 power = emission * tri.area() * glm::pi<f32>() * photon_fraction;

    return {Ray(point, dir), power};
}
