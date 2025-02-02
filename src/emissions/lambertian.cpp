#include <lightwave.hpp>

namespace lightwave {

class Lambertian : public Emission {
    ref<Texture> m_emission;
    float intensity;

public:
    Lambertian(const Properties &properties) {
        m_emission = properties.get<Texture>("emission");
        intensity = properties.get("intensity", 1.0f);
    }

    EmissionEval evaluate(const Point2 &uv, const Vector &wo) const override {
        // Only emit from the front side
        if (wo.z() <= 0)
            return EmissionEval{ .value = Color(0) };
        Color color = m_emission->evaluate(uv);

        return EmissionEval{ .value = color * intensity };
    }

    std::string toString() const override {
        return tfm::format(
            "Lambertian[\n"
            "  emission = %s\n"
            "]",
            indent(m_emission));
    }
};

} // namespace lightwave

REGISTER_EMISSION(Lambertian, "lambertian")
