#include "camera.hpp"
#include "common.hpp"
#include "image.hpp"
#include "limits"
#include "material.hpp"
#include "pointlight.hpp"
#include "ray.hpp"
#include "scene.hpp"
#include "sphere.hpp"

#include <ctime>

int main(int argc, char **argv) {
    srand(time(nullptr));

    const u32 image_width = 512;
    const u32 image_height = 512;
    const u32 n = 64;

    Scene scene;
    scene.add_light(PointLight(vec3(0, 0, 2), 1000.0f));
    Image img(image_width, image_height);
    Camera cam(vec3(0, 2, 0), vec3(0, 0, 0), vec3(0, 0, 1), 60.0f, static_cast<f32>(image_width) / image_height);

    Material sphere_mat{vec3(0.1f, 0.1f, 0.1f), vec3(0.1f, 0.1f, 0.1f)};
    scene.add_object(Sphere(vec3(0, 0, 0), 0.5f, sphere_mat));
    scene.emit(10'000'000, 2);

    HitRecord rec;
    Material mat;
    for (u32 y = 0; y < image_height; y++) {
        for (u32 x = 0; x < image_width; x++) {
            const f32 s = (x + 0.5f) / static_cast<f32>(image_width);
            const f32 t = 1.0f - (y + 0.5f) / static_cast<f32>(image_height);
            Ray r = cam.get_ray(s, t);

            if (scene.hit(r, 0.001f, std::numeric_limits<f32>::max(), rec, mat)) {
                img.set_pixel(x, y, scene.get_color(rec.point, rec.normal, n));
            }
            printf("%i %i\n", x, y);
        }
    }
    img.write_png("output.png");

    return 0;
}
