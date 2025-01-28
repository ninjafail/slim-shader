#include <lightwave.hpp>

namespace lightwave {

class AreaLight final : public Light {
    ref<Instance> m_shape;

public:
    AreaLight(const Properties &properties) : Light(properties) {
        m_shape = properties.getChild<Instance>("shape");
    }

    DirectLightSample sampleDirect(const Point &origin,
                                   Sampler &rng) const override {

        // returns a sample of the object in world space
        AreaSample sample = m_shape->sampleArea(rng);
        Vector dir        = sample.position - origin;

        float distance = dir.length();

        Vector sampleLocal = sample.shadingFrame().toLocal(-dir).normalized();
        EmissionEval emission =
            m_shape->emission()->evaluate(sample.uv, sampleLocal);
        float cosine  = Frame::cosTheta(sampleLocal);
        float falloff = 1 / (distance * distance);

        return DirectLightSample{ .wi     = dir,
                                  .weight = emission.value * cosine * falloff /
                                            sample.pdf,
                                  .distance = distance };
    }

    bool canBeIntersected() const override { return true; }

    std::string toString() const override {
        return tfm::format(
            "AreaLight[\n"
            "]");
    }
};

} // namespace lightwave

REGISTER_LIGHT(AreaLight, "area")
