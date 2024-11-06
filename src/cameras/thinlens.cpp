#include <lightwave.hpp>

namespace lightwave {

class Thinlens : public Camera {
protected:
    float lens_radius, focal_distance;
    // ----------
    float x_ratio;
    float y_ratio;

public:
    Thinlens(const Properties &properties) : Camera(properties) {
        lens_radius    = properties.get<float>("lensRadius");
        focal_distance = properties.get<float>("focalDistance");

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

        if (lens_radius > 0) {
            // sample point on lens
            Point2 p_lens =
                lens_radius *
                Vector2(squareToUniformDiskConcentric(rng.next2D()));

            // compute point on plane of focus
            float ft      = focal_distance / local_ray.direction.z();
            Point p_focus = local_ray(ft);

            // update ray for effect of lens
            local_ray.origin    = Point(p_lens.x(), p_lens.y(), 0);
            local_ray.direction = (p_focus - local_ray.origin).normalized();

            Ray world_ray = m_transform->apply(local_ray);

            return CameraSample{ .ray = local_ray, .weight = Color(1.0f) };
        }

        Ray world_ray = m_transform->apply(local_ray);

        return CameraSample{ .ray = world_ray, .weight = Color(1.0f) };
    }

    std::string toString() const override {
        return tfm::format(
            "Thinlens[\n"
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

REGISTER_CAMERA(Thinlens, "thinlens")
