#include "SceneReader.hpp"
#include "camera.hpp"
#include "common.hpp"
#include "hittable.hpp"
#include "image.hpp"
#include "limits"
#include "material.hpp"
#include "pointlight.hpp"
#include "ray.hpp"
#include "scene.hpp"
#include <ctime>

int main(int argc, char **argv) {
    srand(time(nullptr));

    auto scenes = ReadFile("./models/Box/glTF/Box.gltf");
    auto scene = scenes[0];
    scene.set_camera(Camera(
        vec3(2.0f, 2.0f, 2.0f),
        vec3(0.0f, 0.0f, 0.0f),
        vec3(0.0f, 1.0f, 0.0f),
        40.0f,
        1.0
    ));

    // note to the future people reading this
    // it is expected that the image is black when no light is added
    scene.add_light(PointLight(vec3(2.5f, 3.5f, 3.0f), 500.0f));

    const u32 image_height = 512;
    const u32 image_width = scene.get_camera().aspect_ratio()*image_height;
    const u32 n = 64;
    const u32 photon_num = 1'000'000;
    const u32 max_bounces = 3;
    Image img(image_width, image_height);

    // this example shows that the colors are wrong; first hit of the photon is always white
    //
    // auto scene = Scene();
    // scene.set_camera(Camera(vec3(0, 2, 0), vec3(0, 0, 0), vec3(0, 0, 1), 60.0f, static_cast<f32>(image_width) / image_height));
    // Material white {vec3(0.8f, 0.8f, 0.8f), vec3(0.0f, 0.0f, 0.0f)};
    // Material red   {vec3(0.8f, 0.1f, 0.1f), vec3(0.0f, 0.0f, 0.0f)};
    // Material green {vec3(0.1f, 0.8f, 0.1f), vec3(0.0f, 0.0f, 0.0f)};
    //
    // // floor (z = -1)
    // scene.add_triangle(Triangle(vec3(-1,-1,-1), vec3( 1,-1,-1), vec3( 1, 1,-1), white));
    // scene.add_triangle(Triangle(vec3(-1,-1,-1), vec3( 1, 1,-1), vec3(-1, 1,-1), white));
    //
    // // ceiling (z = 1)
    // scene.add_triangle(Triangle(vec3(-1,-1, 1), vec3( 1, 1, 1), vec3( 1,-1, 1), white));
    // scene.add_triangle(Triangle(vec3(-1,-1, 1), vec3(-1, 1, 1), vec3( 1, 1, 1), white));
    //
    // // back wall (y = -1)
    // scene.add_triangle(Triangle(vec3(-1,-1,-1), vec3( 1,-1, 1), vec3( 1,-1,-1), white));
    // scene.add_triangle(Triangle(vec3(-1,-1,-1), vec3(-1,-1, 1), vec3( 1,-1, 1), white));
    //
    // // left wall (x = -1, red)
    // scene.add_triangle(Triangle(vec3(-1,-1,-1), vec3(-1, 1,-1), vec3(-1, 1, 1), red));
    // scene.add_triangle(Triangle(vec3(-1,-1,-1), vec3(-1, 1, 1), vec3(-1,-1, 1), red));
    //
    // // right wall (x = 1, green)
    // scene.add_triangle(Triangle(vec3( 1,-1,-1), vec3( 1, 1, 1), vec3( 1, 1,-1), green));
    // scene.add_triangle(Triangle(vec3( 1,-1,-1), vec3( 1,-1, 1), vec3( 1, 1, 1), green));
    //
    // // also move the light inside the box
    // scene.add_light(PointLight(vec3(0, 0, 0.9f), 10.0f));

    scene.emit(photon_num, max_bounces);

    HitRecord rec;
    Material mat;
    Camera cam = scene.get_camera();

    for (u32 y = 0; y < image_height; y++) {
        for (u32 x = 0; x < image_width; x++) {
            const f32 s = (x + 0.5f) / static_cast<f32>(image_width);
            const f32 t = 1.0f - (y + 0.5f) / static_cast<f32>(image_height);
            Ray r = cam.get_ray(s, t);

            if (scene.hit(r, 0.001f, std::numeric_limits<f32>::max(), rec, mat)) {
                img.set_pixel(x, y, scene.get_color(rec.point, rec.normal, n));
            }
        }
        printf("%i / %i\n", y, image_height);
    }
    img.write_png("output.png");

    return 0;
}
