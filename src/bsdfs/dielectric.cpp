#include "fresnel.hpp"
#include <lightwave.hpp>

namespace lightwave {

class Dielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;

public:
    Dielectric(const Properties &properties) {
        // index of refraction
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
        Vector normal   = Vector(0, 0, 1);
        // Potentially flip the orientation for snells law
        if (cos_theta < 0) {
            ior       = 1 / ior;
            normal    = -normal;
            cos_theta = -cos_theta;
        }
        float fresnel = fresnelDielectric(cos_theta, ior);
        float pdf;

        // Decide whether to reflect or refract
        float dec = rng.next();
        Vector wi = refract(wo, normal, ior);
        Color color;
        if (dec <= fresnel || wi == Vector(0)) {
            // Reflect
            wi    = reflect(wo, normal);
            color = m_reflectance->evaluate(uv);
            pdf   = fresnel;
        } else {
            // Refract
            wi    = refract(wo, normal, ior);
            color = m_transmittance->evaluate(uv) / (sqr(ior));
            pdf   = 1 - fresnel;
        }
        return BsdfSample{ .wi = wi.normalized(), .weight = color, .pdf = pdf };
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
