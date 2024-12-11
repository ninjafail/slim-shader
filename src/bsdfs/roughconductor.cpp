#include "fresnel.hpp"
#include "microfacet.hpp"
#include <lightwave.hpp>

namespace lightwave {

class RoughConductor : public Bsdf {
    ref<Texture> m_reflectance;
    ref<Texture> m_roughness;

public:
    RoughConductor(const Properties &properties) {
        m_reflectance = properties.get<Texture>("reflectance");
        m_roughness   = properties.get<Texture>("roughness");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // Using the squared roughness parameter results in a more gradual
        // transition from specular to rough. For numerical stability, we avoid
        // extremely specular distributions (alpha values below 10^-3)
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        Vector wm       = (wi + wo).normalized();
        Color refl      = m_reflectance.get()->evaluate(uv);
        float dist      = microfacet::evaluateGGX(alpha, wm);
        float gi        = microfacet::smithG1(alpha, wm, wi);
        float go        = microfacet::smithG1(alpha, wm, wo);
        float cos_theta = Frame::absCosTheta(wo);

        Color color = (refl * dist * gi * go) / (4 * cos_theta);

        return BsdfEval{ color };

        // hints:
        // * the microfacet normal can be computed from `wi' and `wo'
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));

        Vector n  = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());
        Vector wi = reflect(wo, n);

        Vector wm = (wi + wo).normalized();
        float gi  = microfacet::smithG1(alpha, wm, wi);

        Color color = m_reflectance.get()->evaluate(uv) * gi;

        return BsdfSample{ wi, color };

        // hints:
        // * do not forget to cancel out as many terms from your equations as
        // possible!
        //   (the resulting sample weight is only a product of two factors)
    }

    std::string toString() const override {
        return tfm::format(
            "RoughConductor[\n"
            "  reflectance = %s,\n"
            "  roughness = %s\n"
            "]",
            indent(m_reflectance),
            indent(m_roughness));
    }
};

} // namespace lightwave

REGISTER_BSDF(RoughConductor, "roughconductor")
