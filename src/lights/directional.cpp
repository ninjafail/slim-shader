#include <lightwave.hpp>

namespace lightwave {

class DirectionalLight final : public Light {
    Vector direction;
    Color intensity;

public:
    DirectionalLight(const Properties &properties) : Light(properties) {
        direction = properties.get<Vector>("direction");
        intensity = properties.get<Color>("intensity");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {
        return DirectLightSample{ .wi       = direction.normalized(),
                                  .weight   = intensity,
                                  .distance = Infinity,
                                  .pdf      = Infinity };
    }

    bool canBeIntersected() const override { return false; }

    std::string toString() const override {
        return tfm::format(
            "DirectionalLight[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(DirectionalLight, "directional")
