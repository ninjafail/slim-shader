#include <lightwave.hpp>

namespace lightwave {

class PointLight final : public Light {
    Point position;
    Color power;
    Color intensity;

public:
    PointLight(const Properties &properties) : Light(properties) {
        position  = properties.get<Point>("position");
        power     = properties.get<Color>("power");
        intensity = power * Inv4Pi;
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        Vector dir     = position - origin;
        float distance = dir.length();
        float falloff  = 1 / (distance * distance);
        return DirectLightSample{ .wi       = dir.normalized(),
                                  .weight   = intensity * falloff,
                                  .distance = distance,
                                  .pdf      = Infinity };
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
