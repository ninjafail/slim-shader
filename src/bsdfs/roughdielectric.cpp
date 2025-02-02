#include "fresnel.hpp"
#include "microfacet.hpp"
#include <lightwave.hpp>

namespace lightwave {

class RoughDielectric : public Bsdf {
    ref<Texture> m_ior;
    ref<Texture> m_reflectance;
    ref<Texture> m_transmittance;
    ref<Texture> m_roughness;

public:
    RoughDielectric(const Properties &properties) {
        // index of refraction
        m_ior           = properties.get<Texture>("ior");
        m_reflectance   = properties.get<Texture>("reflectance");
        m_transmittance = properties.get<Texture>("transmittance");
        m_roughness     = properties.get<Texture>("roughness");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        float ior = m_ior->scalar(uv);

        Vector h        = (wi + wo).normalized();
        float f         = fresnelDielectric(wi.dot(h), ior);
        float d         = microfacet::evaluateGGX(alpha, h);
        float gi        = microfacet::smithG1(alpha, h, wi);
        float go        = microfacet::smithG1(alpha, h, wo);
        float cos_theta = Frame::absCosTheta(wo);

        Color refl = (f * m_reflectance.get()->evaluate(uv) * d * gi * go) /
                     (4 * cos_theta);

        h         = (wo + ior * wi).normalized();
        f         = fresnelDielectric(wi.dot(h), ior);
        d         = microfacet::evaluateGGX(alpha, h);
        gi        = microfacet::smithG1(alpha, h, wi);
        go        = microfacet::smithG1(alpha, h, wo);
        cos_theta = Frame::absCosTheta(wo);

        // TODO : i do not think this is the correct computation ...
        Color trans =
            ((1 - f) * m_transmittance.get()->evaluate(uv) * d * gi * go) /
            (4 * cos_theta);

        return BsdfEval{ refl + trans };
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {

        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        // Compute Fresnel term
        float ior       = m_ior->scalar(uv);
        float cos_theta = Frame::cosTheta(wo);
        Vector normal   = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());
        // Potentially flip the orientation for snells law
        if (cos_theta < 0) {
            ior       = 1 / ior;
            normal    = -normal;
            cos_theta = -cos_theta;
        }
        float fresnel = fresnelDielectric(cos_theta, ior);

        // Decide whether to reflect or refract
        float dec = rng.next();
        Vector wi = refract(wo, normal, ior);
        Color color;
        float pdf;

        if (dec <= fresnel || wi == Vector(0)) {
            // Reflect
            wi    = reflect(wo, normal);
            color = m_reflectance->evaluate(uv);
            pdf   = microfacet::detReflection(normal, wo) * fresnel;
        } else {
            // Refract
            // wi = refract(wo, normal, ior);
            color = m_transmittance->evaluate(uv) / (sqr(ior));
            pdf   = microfacet::detReflection(normal, wo) * (1.0f - fresnel);
        }
        return BsdfSample{ .wi = wi.normalized(), .weight = color, .pdf = pdf };
    }

    std::string toString() const override {
        return tfm::format(
            "RoughDielectric[\n"
            "  ior           = %s,\n"
            "  reflectance   = %s,\n"
            "  transmittance = %s,\n"
            "  roughness     = %s\n"
            "]",
            indent(m_ior),
            indent(m_reflectance),
            indent(m_transmittance),
            indent(m_roughness));
    }
};

} // namespace lightwave

REGISTER_BSDF(RoughDielectric, "roughdielectric")