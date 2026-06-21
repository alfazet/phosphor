#include <Material.hpp>
#include <camera.hpp>
#include <common.hpp>
#include <ctime>
#include <image.hpp>
#include <limits>
#include <pointlight.hpp>
#include <ray.hpp>
#include <scene.hpp>
#include <sphere.hpp>

int main(int argc, char **argv) {
    srand(time(nullptr));

    const int image_width = 100;
    const int image_height = 100;
    const int n = 50;
    Image img(image_width, image_height);
    Camera cam(vec3(0, 0, 0), vec3(0, 1, 0), vec3(0, 0, 1), 60.0f, image_width/image_height);

    Scene scene;
    scene.AddLight(Pointlight(vec3(0, 1, 2), 1000.0f));

    // Material sphere_mat{vec3(0.8f, 0.8f, 0.8f), vec3(0.1f, 0.1f, 0.1f)};
    Material sphere_mat{vec3(0.1f, 0.1f, 0.1f), vec3(0.1f, 0.1f, 0.1f)};
    scene.AddObject(Sphere(vec3(0, 1, 0), 0.5f, sphere_mat));

    scene.Emit(10000000, 2);

    // for (auto i : scene.photons()) {
    //     printf("[%f,%f,%f] [%f,%f,%f] [%f,%f]\n", i.pos.x, i.pos.y, i.pos.z, i.power.x, i.power.y, i.power.z, i.phi, i.theta);
    // }

    HitRecord rec;
    Material mat;
    for (u32 y = 0; y < image_height; ++y) {
        for (u32 x = 0; x < image_width; ++x) {
            const f32 s = (x + 0.5f) / static_cast<f32>(image_width);
            const f32 t = 1.0f - (y + 0.5f) / static_cast<f32>(image_height);
            Ray r = cam.get_ray(s, t);

            if (scene.hit(r, 0.001f, std::numeric_limits<f32>::max(), rec, mat)) {
                img.set_pixel(x, y, scene.GetColor(rec.point, n));
            }
            printf("%i %i\n", x, y);
        }
    }
    img.write_png("output.png");
    return 0;
}