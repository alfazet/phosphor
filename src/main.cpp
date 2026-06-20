#include <camera.hpp>
#include <common.hpp>
#include <image.hpp>
#include <ray.hpp>
#include <sphere.hpp>

int main(int argc, char **argv) {

    int image_width = 160;
    int image_height = 90;
    Image img(image_width, image_height);
    Camera cam(vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1), 60.0f, 16.0f / 9.0f);

    Sphere sphere(vec3(0, 1, 0), 0.5f);
    HitRecord rec;

    for (u32 y = 0; y < image_height; ++y) {
        for (u32 x = 0; x < image_width; ++x) {
            const f32 s = (x + 0.5f) / static_cast<f32>(image_width);
            const f32 t = 1.0f - (y + 0.5f) / static_cast<f32>(image_height);
            Ray r = cam.get_ray(s, t);
            if (sphere.hit(r, 0.001f, std::numeric_limits<f32>::max(), rec)) {
                img.set_pixel(x, y, vec3(0, 0, 1));
            }
        }
    }
    img.write_png("output.png");
    return 0;
}
