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
    float x_ratio;
    float y_ratio;

public:
    Perspective(const Properties &properties) : Camera(properties) {
        const float fov = properties.get<float>("fov");

        // * precompute any expensive operations here (most importantly
        // trigonometric functions)
        // compute factor to map on plane z = 1, must transform deg to radians
        float tan_fov = tan((fov / 2.0f) * (Pi / 180));

        // * use m_resolution to find the aspect ratio of the image
        float width  = m_resolution.x();
        float height = m_resolution.y();

        // construct factor to include aspect ratio and fov
        std::string fovAxis = properties.get<std::string>("fovAxis");
        if (fovAxis == "y") {
            x_ratio = (width / height) * tan_fov;
            y_ratio = tan_fov;
        } else if (fovAxis == "x") {
            x_ratio = tan_fov;
            y_ratio = (height / width) * tan_fov;
        }
    }

    CameraSample sample(const Point2 &normalized, Sampler &rng) const override {
        // define vector pointing to the point on the z plane
        Vector direction(
            normalized.x() * x_ratio, normalized.y() * y_ratio, 1.0f);

        // normalize the local ray
        Ray local_ray(Vector(0.f, 0.f, 0.f), direction.normalized());

        // * use m_transform to transform the local camera coordinate system
        // into the world coordinate system
        Ray world_ray = m_transform->apply(local_ray);

        return CameraSample{ .ray = world_ray.normalized(), .weight = Color(1.0f) };
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
