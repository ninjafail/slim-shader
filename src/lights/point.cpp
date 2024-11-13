#include <lightwave.hpp>

namespace lightwave {

class PointLight final : public Light {
    Point position;
    Color power;

public:
    PointLight(const Properties &properties) : Light(properties) {
        position = properties.get<Point>("position");
        power    = properties.get<Color>("power");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        Color intensity = power / (4 * Pi);
        Vector dir      = position - origin;
        float distance  = dir.length();
        return DirectLightSample{ .wi       = dir.normalized(),
                                  .weight   = intensity,
                                  .distance = distance };
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "PointLight[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(PointLight, "point")
