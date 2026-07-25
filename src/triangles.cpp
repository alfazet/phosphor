#include "triangles.hpp"

// https://raytracing.github.io/books/RayTracingTheNextWeek.html#boundingvolumehierarchies

Triangles::Triangles() : left(nullptr), right(nullptr) {}

bool box_compare(Triangle &a, Triangle &b, u32 index) {
    return a.get_bounding_box().index(index).start < b.get_bounding_box().index(index).start;
}

bool box_compare0(Triangle &a, Triangle &b) { return box_compare(a, b, 0); }

bool box_compare1(Triangle &a, Triangle &b) { return box_compare(a, b, 1); }

bool box_compare2(Triangle &a, Triangle &b) { return box_compare(a, b, 2); }

Triangles::Triangles(std::shared_ptr<std::vector<Triangle>> triangles, u32 start, u32 end) {
    objects = triangles;
    this->start = start;
    this->end = end;
    this->left = nullptr;
    this->right = nullptr;
    split();
}

void Triangles::split() {
    u32 span = end - start;
    if (span == 1) {
        boundingBox = (*objects)[start].get_bounding_box();
        return;
    }

    u32 axis = random() % 3;
    auto comparator = (axis == 0) ? box_compare0 : (axis == 1) ? box_compare1 : box_compare2;

    std::sort(std::begin(*objects) + start, std::begin(*objects) + end, comparator);

    auto mid = start + (end - start) / 2;
    left = std::make_shared<Triangles>(objects, start, mid);
    right = std::make_shared<Triangles>(objects, mid, end);

    boundingBox = BoundingBox(left->boundingBox, right->boundingBox);
}

bool Triangles::hit(const Ray &r, interval t, HitRecord &rec, Material &mat_out, vec2 &uv,
                    const std::vector<Texture> &textures) const {
    if (!boundingBox.hit(r, t) || t.start > t.end)
        return false;
    u32 span = end - start;
    if (span == 1) {
        const Triangle &tri = (*objects)[start];
        if (tri.hit(r, t, rec, textures)) {
            mat_out = tri.mat;
            uv = compute_bary(rec.bary, tri.uv0, tri.uv1, tri.uv2);
            return true;
        }
        return false;
    }

    bool hit_left = left->hit(r, t, rec, mat_out, uv, textures);
    bool hit_right = right->hit(r, interval(t.start, hit_left ? rec.t : t.end), rec, mat_out, uv, textures);

    return hit_left || hit_right;
}
