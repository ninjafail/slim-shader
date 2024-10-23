#include <lightwave.hpp>

namespace lightwave {

/**
 * @brief A perspective camera with a given field of view angle and transform.
 *
 * In local coordinates (before applying m_transform), the camera looks in
 * positive z direction [0,0,1]. Pixels on the left side of the image ( @code
 * normalized.x < 0 @endcode ) are directed in negative x direction ( @code
 * ray.direction.x < 0 ), and pixels at the bottom of the image ( @code
 * normalized.y < 0 @endcode ) are directed in negative y direction ( @code
 * ray.direction.y < 0 ).
 */
class Perspective : public Camera {
protected:
    float z;
    float x_ratio = 1;
    float y_ratio = 1;

public:
    Perspective(const Properties &properties) : Camera(properties) {
        const float fov = properties.get<float>("fov");

        // * precompute any expensive operations here (most importantly
        // trigonometric functions)
        // calculate the length of the z vector, must transform fov to radians
        z = 1 / tan((fov / 2) * (Pi / 180));
        // hints:
        // * use m_resolution to find the aspect ratio of the image
        float y = m_resolution.y();
        float x = m_resolution.x();
        // TODO: truly understand why we need the if case
        if (z >= 1)
            x_ratio = x / y;
        else
            y_ratio = y / x;
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
        // define vector pointing to the point on the z plane
        Vector direction(normalized.x() * x_ratio, normalized.y() * y_ratio, z);

        // normalize the local ray
        Ray local_ray(Vector(0.f, 0.f, 0.f), direction.normalized());

        // * use m_transform to transform the local camera coordinate system
        // into the world coordinate system
        Ray world_ray = m_transform->apply(local_ray);

        return CameraSample{ .ray = world_ray, .weight = Color(1.0f) };
    }

    std::string toString() const override {
        return tfm::format(
            "Perspective[\n"
            "  width = %d,\n"
            "  height = %d,\n"
            "  transform = %s,\n"
            "]",
            m_resolution.x(),
            m_resolution.y(),
            indent(m_transform));
    }
};

} // namespace lightwave

REGISTER_CAMERA(Perspective, "perspective")
