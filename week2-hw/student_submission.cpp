#include <thread>
#include <mutex>
#include <iostream>
#include <cfloat>
#include <random>

struct Color {
    uint8_t r, g, b;
    Color() : r{0}, g{0}, b{0} {}
    Color(uint8_t rr, uint8_t gg, uint8_t bb) : r(rr), g(gg), b(bb) {}
};

struct Vector3 {
    float x, y, z;

    Vector3() : x{0}, y{0}, z{0} {}
    Vector3(float xx, float yy, float zz) : x(xx), y(yy), z(zz) {}

    Vector3 operator-() const { return Vector3(-this->x, -this->y, -this->z); }

    Vector3 operator+(const Vector3& v) const { return Vector3(this->x + v.x, this->y + v.y, this->z + v.z); }

    Vector3 operator-(const Vector3& v) const { return Vector3(this->x - v.x, this->y - v.y, this->z - v.z); }

    Vector3 operator*(const float& s) const { return Vector3(this->x * s, this->y * s, this->z * s); }

    Vector3 operator/(const float& s) const { return Vector3(this->x / s, this->y / s, this->z / s); }

    Vector3 operator+=(const Vector3& v) {
        this->x += v.x;
        this->y += v.y;
        this->z += v.z;
        return *this;
    }

    Vector3 operator-=(const Vector3& v) { return *this += -v; }

    Vector3 operator*=(const float& s) {
        this->x *= s;
        this->y *= s;
        this->z *= s;
        return *this;
    }

    Vector3 operator/=(const float& s) { return *this *= 1/s; }
};

struct Checksum {
    uint32_t r, g, b;

    Checksum() : r{0}, g{0}, b{0} {}
    Checksum(uint32_t rr, uint32_t gg, uint32_t bb) : r(rr), g(gg), b(bb) {}

    Checksum operator+(const Checksum& v) const { return Checksum(this->r + v.r, this->b + v.b, this->g + v.g); }

    Checksum operator+=(const Checksum& v) {
        this->r += v.r;
        this->g += v.g;
        this->b += v.b;
        return *this;
    }
};

inline float dot(const Vector3& a, const Vector3& b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline Vector3 cross(const Vector3& a, const Vector3& b) { return Vector3(a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x); }
inline float length(const Vector3& vec3) { return sqrtf(vec3.x * vec3.x + vec3.y * vec3.y + vec3.z * vec3.z); }
inline Vector3 unit_vector(const Vector3& vec3) { return vec3 / length(vec3); }
inline Vector3 reflect(const Vector3 vec3, const Vector3 normal) { return vec3 - normal * 2 * dot(vec3, normal); }

struct Ray {
    Vector3 origin_point;
    Vector3 direction;

    Ray() {}
    Ray(Vector3 origin, Vector3 dir) : origin_point(origin), direction(dir) {}
};

inline Vector3 ray_at(const Ray& ray, float t) { return ray.origin_point + ray.direction * t; }

inline uint8_t clamp(uint8_t num, uint8_t min, uint8_t max) {
    uint8_t temp1 = num > min ? num : min;
    uint8_t temp2 = temp1 < max ? temp1 : max;
    return temp2;
}

inline float clamp(float num, float min, float max) {
    float temp1 = num > min ? num : min;
    float temp2 = temp1 < max ? temp1 : max;
    return temp2;
}

inline float random_float_srand() { return rand() / (2147483648.0f); }
inline float random_float_srand(float min, float max) { return min + (max-min) * random_float_srand(); }

inline float random_float() {
    static thread_local std::mt19937 generator;
    static std::uniform_real_distribution<float> distribution(0.0, 1.0);
    return distribution(generator);
}
inline float random_float(float min, float max) { return min + (max-min) * random_float(); }
inline Vector3 random_vector3() { return Vector3(random_float(), random_float(), random_float()); }
inline Vector3 random_vector3(float min, float max) { return Vector3(random_float(min,max), random_float(min,max), random_float(min,max)); }

struct Camera {
    const float aspect_ratio;
    float lens_radius;
    Vector3 origin, lower_left_corner, horizontal, vertical;
    Vector3 u, v, w;

    Camera(Vector3 position, Vector3 look_at, Vector3 up, float aspect_r, float vertical_fov, float aperture, float focus_dist) : aspect_ratio(aspect_r) {
        const auto viewport_height = 2.0f * tan(vertical_fov * 0.00872665);
        const auto viewport_width = aspect_ratio * viewport_height;

        w = unit_vector(position - look_at);
        u = unit_vector(cross(up, w));
        v = unit_vector(cross(w, u));

        origin = position;
        horizontal = u * viewport_width * focus_dist;
        vertical = v * viewport_height * focus_dist;
        lower_left_corner = origin - horizontal/2 - vertical/2 - w * focus_dist;

        lens_radius = aperture / 2;
    }
};

inline Vector3 random_in_unit_disk() {
    while (true) {
        const auto p = Vector3(random_float(-1,1), random_float(-1,1), 0);
        if (dot(p, p) >= 1) continue;
        return p;
    }
}

inline Ray get_camera_ray(const Camera& cam, float u, float v) {
    const Vector3 rd = random_in_unit_disk() * cam.lens_radius;
    const Vector3 offset = cam.u * rd.x + cam.v * rd.y;

    return Ray(cam.origin + offset, cam.lower_left_corner + cam.horizontal * u + cam.vertical * v - cam.origin - offset);
}

#define IMAGE_WIDTH 1920
#define IMAGE_HEIGHT 1080
#define NUM_SAMPLES 80
#define SAMPLE_DEPTH 50
#define NUM_SPHERES 14

std::mutex mutex;

inline Color compute_color(Checksum &checksum, Vector3 pixel_color)
{
    const auto r = pixel_color.x / NUM_SAMPLES;
    const auto g = pixel_color.y / NUM_SAMPLES;
    const auto b = pixel_color.z / NUM_SAMPLES;

    // Divide the color by the number of NUM_SAMPLES.

    // Write the translated [0,255] value of each color component.
    const auto pixel_r = static_cast<uint8_t>(256 * clamp(r, 0.0, 0.999));
    const auto pixel_g = static_cast<uint8_t>(256 * clamp(g, 0.0, 0.999));
    const auto pixel_b = static_cast<uint8_t>(256 * clamp(b, 0.0, 0.999));

    mutex.lock();
    checksum.r += pixel_r;
    checksum.g += pixel_g;
    checksum.b += pixel_b;
    mutex.unlock();

    return {pixel_r, pixel_g, pixel_b};
}

struct Material
{
    Vector3 albedo;
    float fuzziness;
};

struct Sphere
{
    Vector3 center;
    float radius;
    Material material;

    Sphere(Vector3 cen, float r, Material mat) : center(cen), radius(r), material(mat) {}
};

struct Hit
{
    Vector3 point;
    Vector3 normal;
    float t;
    bool front_face;
    Material material;
};

Vector3 random_in_unit_sphere()
{
    while (true)
    {
        auto p = random_vector3(-1, 1);
        if (dot(p, p) >= 1)
            continue;
        return p;
    }
}

inline bool metal_scater(const Material &material, const Ray &incoming_ray, const Hit &hit, Vector3 &attenuation, Ray &outgoing_ray)
{
    auto reflected_vec = reflect(unit_vector(incoming_ray.direction), hit.normal);
    outgoing_ray = Ray(hit.point, reflected_vec + random_in_unit_sphere() * material.fuzziness);
    attenuation = material.albedo;
    return (dot(outgoing_ray.direction, hit.normal) > 0);
}

inline void set_face_normal(Hit &hit, const Ray &ray, const Vector3 &outward_normal)
{
    hit.front_face = dot(ray.direction, outward_normal) < 0;
    hit.normal = hit.front_face ? outward_normal : -outward_normal;
}

inline bool sphere_hit(const Sphere &sphere, const Ray &ray, float t_min, float t_max, Hit &hit)
{
    const auto diff = ray.origin_point - sphere.center;

    const auto a = dot(ray.direction, ray.direction);
    const auto b = dot(diff, ray.direction);
    const auto c = dot(diff, diff) - sphere.radius * sphere.radius;

    const auto discriminant = b * b - a * c;

    if (discriminant > 0)
    {
        const float discriminant_sqrt = sqrtf(discriminant);
        const auto first_root = (-b - discriminant_sqrt) / a;

        if (first_root > t_min && first_root < t_max)
        {
            hit.t = first_root;
            hit.point = ray_at(ray, hit.t);
            const auto outward_normal = (hit.point - sphere.center) / sphere.radius;
            set_face_normal(hit, ray, outward_normal);
            return true;
        }

        const auto second_root = (-b + discriminant_sqrt) / a;
        if (second_root > t_min && second_root < t_max)
        {
            hit.t = second_root;
            hit.point = ray_at(ray, hit.t);
            const auto outward_normal = (hit.point - sphere.center) / sphere.radius;
            set_face_normal(hit, ray, outward_normal);
            return true;
        }
    }
    return false;
}

inline void readInput()
{
    unsigned int seed = 0;
    std::cout << "READY" << std::endl;
    std::cin >> seed;


    // Set the pseudo random number generator seed
    srand(seed);
}

inline void writeOutput(Checksum checksum)
{
    std::cout << "red checksum is : " << (float)checksum.r << std::endl;
    std::cout << "green checksum is : " << (float)checksum.g << std::endl;
    std::cout << "blue checksum is : " << (float)checksum.b << std::endl;

    // This stops the timer.
    std::cout << std::endl
              << "DONE" << std::endl;
}

inline void create_random_scene(std::vector<Sphere> &spheres)
{
    Material mat_ground;
    mat_ground.albedo = Vector3(0.5, 0.5, 0.5);
    mat_ground.fuzziness = 1.0;

    for (int i = 0; i < NUM_SPHERES; i++)
    {
        Material mat;
        mat.albedo = Vector3(random_float_srand(0, 1), random_float_srand(0, 1), random_float_srand(0, 1));
        mat.fuzziness = random_float_srand(0, 1);
        spheres.emplace_back(Vector3(random_float_srand(-6, 6), 0, random_float_srand(-6, 6)), 0.5f, mat);
    }

    spheres.emplace_back(Vector3(0, -100.5f, -1), 100, mat_ground);
}

/*
** Checks if the given ray hits a sphere surface and returns.
** Also returns hit data which contains material information.
*/
inline bool check_sphere_hit(const std::vector<Sphere> &spheres, const Ray &ray, float t_min, float t_max, Hit &hit)
{
    Hit closest_hit;
    bool has_hit = false;
    auto closest_hit_distance = t_max;
    Material material;

    for (const auto & sphere : spheres)
    {
        if (sphere_hit(sphere, ray, t_min, closest_hit_distance, closest_hit))
        {
            has_hit = true;
            closest_hit_distance = closest_hit.t;
            material = sphere.material;
        }
    }

    if (has_hit)
    {
        hit = closest_hit;
        hit.material = material;
    }

    return has_hit;
}

/*
** Traces a ray, returns color for the corresponding pixel.
*/
inline Vector3 trace_ray(const Ray &ray, const std::vector<Sphere> &spheres, int depth)
{
    if (depth < 1)
    {
        return Vector3(0, 0, 0);
    }

    Hit hit;
    if (check_sphere_hit(spheres, ray, 0.001f, FLT_MAX, hit))
    {
        Ray outgoing_ray;
        Vector3 attenuation;

        if (metal_scater(hit.material, ray, hit, attenuation, outgoing_ray))
        {
            const auto ray_color = trace_ray(outgoing_ray, spheres, depth - 1);
            return Vector3(ray_color.x * attenuation.x, ray_color.y * attenuation.y, ray_color.z * attenuation.z);
        }

        return Vector3(0, 0, 0);
    }

    Vector3 unit_direction = unit_vector(ray.direction);
    const float t = 0.5f * (unit_direction.y + 1.0f);
    return Vector3(1.0f, 1.0f, 1.0f) * (1.0f - t) + Vector3(0.5f, 0.7f, 1.0f) * t;
}

inline void thread_work(
        const uint8_t &thread_id,
        uint8_t *image_data,
        const uint8_t n_threads,
        const Camera &camera,
        const std::vector<Sphere> &spheres,
        Checksum &checksum)
{
    for (uint32_t i = thread_id; i < IMAGE_WIDTH * IMAGE_HEIGHT; i += n_threads)
    {
        uint16_t x = i % IMAGE_WIDTH;
        uint16_t y = i / IMAGE_WIDTH;
        Vector3 pixel_color(0, 0, 0);
        for (uint8_t s = 0; s < NUM_SAMPLES; s++)
        {
            const auto u = (x + random_float()) / (IMAGE_WIDTH - 1);
            const auto v = (y + random_float()) / (IMAGE_HEIGHT - 1);
            const auto r = get_camera_ray(camera, u, v);
            pixel_color += trace_ray(r, spheres, SAMPLE_DEPTH);
        }
        auto output_color = compute_color(checksum, pixel_color);

        int pos = ((IMAGE_HEIGHT - 1 - y) * IMAGE_WIDTH + x) * 3;
        image_data[pos] = output_color.r;
        image_data[pos + 1] = output_color.g;
        image_data[pos + 2] = output_color.b;
    }
}

int main()
{
    const uint8_t n_threads = std::max(uint8_t(1), static_cast<uint8_t>(std::thread::hardware_concurrency()));
    std::thread threads[n_threads];
    auto image_data = static_cast<uint8_t *>(malloc(IMAGE_WIDTH * IMAGE_HEIGHT * sizeof(uint8_t) * 3));

    // checksums for each color individually
    Checksum checksum(0, 0, 0);

    // Calculating the aspect ratio and creating the camera for the rendering
    const auto aspect_ratio = (float)IMAGE_WIDTH / IMAGE_HEIGHT;
    const Camera camera(Vector3(0, 1, 1), Vector3(0, 0, -1), Vector3(0, 1, 0), aspect_ratio, 90, 0.0f, 1.5f);

    std::vector<Sphere> spheres;
    readInput();
    create_random_scene(spheres);

    for (auto i = 0; i < n_threads; ++i)
    {
        threads[i] = std::thread(
                thread_work,
                i,
                image_data,
                n_threads,
                std::cref(camera),
                std::cref(spheres),
                std::ref(checksum));
    }

    for (auto &thread : threads)
    {
        thread.join();
    }

    writeOutput(checksum);
    return 0;
}
