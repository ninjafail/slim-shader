#include "fresnel.hpp"
#include <lightwave.hpp>

namespace lightwave {

class Dielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;

public:
    Dielectric(const Properties &properties) {
        m_ior           = properties.get<Texture>("ior");
        m_reflectance   = properties.get<Texture>("reflectance");
        m_transmittance = properties.get<Texture>("transmittance");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // the probability of a light sample picking exactly the direction `wi'
        // that results from reflecting or refracting `wo' is zero, hence we can
        // just ignore that case and always return black
        return BsdfEval::invalid();
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // Compute Fresnel term
        float ior       = m_ior->scalar(uv);
        float cos_theta = Frame::cosTheta(wo);
        float fresnel   = fresnelDielectric(cos_theta, ior);

        // Decide whether to reflect or refract
        float dec = rng.next();
        Vector wi;
        Color color;
        if (dec <= fresnel) {
            // Reflect
            wi    = reflect(wo, Vector(0, 0, 1));
            color = m_reflectance.get()->evaluate(uv);
        } else {
            // Refract
            if (cos_theta < 0) {
                ior = 1 / ior;
            }
            wi    = refract(wo, Vector(0, 0, 1), ior);
            color = m_transmittance.get()->evaluate(uv) / (sqr(ior));
            if (wi == Vector(0)) { // TIR occurs
                wi    = reflect(wo, Vector(0, 0, 1));
                color = m_reflectance.get()->evaluate(uv);
            }
        }
        return BsdfSample{ wi, color };
    }

    std::string toString() const override {
        return tfm::format(
            "Dielectric[\n"
            "  ior           = %s,\n"
            "  reflectance   = %s,\n"
            "  transmittance = %s\n"
            "]",
            indent(m_ior),
            indent(m_reflectance),
            indent(m_transmittance));
    }
};

} // namespace lightwave

REGISTER_BSDF(Dielectric, "dielectric")
