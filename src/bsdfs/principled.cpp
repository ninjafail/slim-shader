#include <lightwave.hpp>

#include "fresnel.hpp"
#include "microfacet.hpp"

namespace lightwave {

struct DiffuseLobe {
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        // Check if we and wo are in same hemisphere
        if (!Frame::sameHemisphere(wo, wi)) {
            return BsdfEval::invalid();
        }
        Color result = color * InvPi * Frame::absCosTheta(wi.normalized());
        return BsdfEval{ .value = result };

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        // First sample ray direction
        Vector out_dir = squareToCosineHemisphere(rng.next2D());
        // sometimes we get a ray direction that is not in the same hemisphere
        if (!Frame::sameHemisphere(wo, out_dir)) {
            out_dir.z() *= -1;
        }
        // The functions indicate that cosineHemispherePdf = z * InvPi and
        // Frame::absCosTheta(vec) = vec.z(). Thus when dividing by the pdf,
        // the InvPi and Frame::absCosTheta(out_dir) cancel out, leaving us with
        // with the albedo value.
        return BsdfSample{ .wi = out_dir, .weight = color };

        // hints:
        // * copy your diffuse bsdf evaluate here
        // * you do not need to query a texture, the albedo is given by `color`
    }
};

struct MetallicLobe {
    float alpha;
    Color color;

    BsdfEval evaluate(const Vector &wo, const Vector &wi) const {
        Vector wm       = (wi + wo).normalized();
        Color refl      = color;
        float dist      = microfacet::evaluateGGX(alpha, wm);
        float gi        = microfacet::smithG1(alpha, wm, wi);
        float go        = microfacet::smithG1(alpha, wm, wo);
        float cos_theta = Frame::absCosTheta(wo);

        Color result = (refl * dist * gi * go) / (4 * cos_theta);

        return BsdfEval{ result };

        // hints:
        // * copy your roughconductor bsdf evaluate here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
    }

    BsdfSample sample(const Vector &wo, Sampler &rng) const {
        Vector n  = microfacet::sampleGGXVNDF(alpha, wo, rng.next2D());
        Vector wi = reflect(wo, n);

        Vector wm = (wi + wo).normalized();
        float gi  = microfacet::smithG1(alpha, wm, wi);

        Color result = color * gi;

        return BsdfSample{ wi, result };

        // hints:
        // * copy your roughconductor bsdf sample here
        // * you do not need to query textures
        //   * the reflectance is given by `color'
        //   * the variable `alpha' is already provided for you
    }
};

class Principled : public Bsdf {
    ref<Texture> m_baseColor;
    ref<Texture> m_roughness;
    ref<Texture> m_metallic;
    ref<Texture> m_specular;

    struct Combination {
        float diffuseSelectionProb;
        DiffuseLobe diffuse;
        MetallicLobe metallic;
    };

    Combination combine(const Point2 &uv, const Vector &wo) const {
        const auto baseColor = m_baseColor->evaluate(uv);
        const auto alpha = std::max(float(1e-3), sqr(m_roughness->scalar(uv)));
        const auto specular = m_specular->scalar(uv);
        const auto metallic = m_metallic->scalar(uv);
        const auto F =
            specular * schlick((1 - metallic) * 0.08f, Frame::cosTheta(wo));

        const DiffuseLobe diffuseLobe = {
            .color = (1 - F) * (1 - metallic) * baseColor,
        };
        const MetallicLobe metallicLobe = {
            .alpha = alpha,
            .color = F * Color(1) + (1 - F) * metallic * baseColor,
        };

        const auto diffuseAlbedo = diffuseLobe.color.mean();
        const auto totalAlbedo =
            diffuseLobe.color.mean() + metallicLobe.color.mean();
        return {
            .diffuseSelectionProb =
                totalAlbedo > 0 ? diffuseAlbedo / totalAlbedo : 1.0f,
            .diffuse  = diffuseLobe,
            .metallic = metallicLobe,
        };
    }

public:
    Principled(const Properties &properties) {
        m_baseColor = properties.get<Texture>("baseColor");
        m_roughness = properties.get<Texture>("roughness");
        m_metallic  = properties.get<Texture>("metallic");
        m_specular  = properties.get<Texture>("specular");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        PROFILE("Principled")

        const auto combination = combine(uv, wo);

        auto diff_eval   = combination.diffuse.evaluate(wo, wi);
        auto metall_eval = combination.metallic.evaluate(wo, wi);

        Color color = diff_eval.value + metall_eval.value;

        return BsdfEval{ color };

        // hint: evaluate `combination.diffuse` and `combination.metallic` and
        // combine their results
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        PROFILE("Principled")

        const auto combination = combine(uv, wo);

        BsdfSample sample;
        Color weight;

        float dec          = rng.next();
        float diffuse_prob = combination.diffuseSelectionProb;
        if (dec <= combination.diffuseSelectionProb) {
            sample = combination.diffuse.sample(wo, rng);
            weight = sample.weight / diffuse_prob;
        } else {
            sample = combination.metallic.sample(wo, rng);
            weight = sample.weight / (1 - diffuse_prob);
        }

        return BsdfSample{ sample.wi, weight };

        // hint: sample either `combination.diffuse` (probability
        // `combination.diffuseSelectionProb`) or `combination.metallic`
    }

    std::string toString() const override {
        return tfm::format(
            "Principled[\n"
            "  baseColor = %s,\n"
            "  roughness = %s,\n"
            "  metallic  = %s,\n"
            "  specular  = %s,\n"
            "]",
            indent(m_baseColor),
            indent(m_roughness),
            indent(m_metallic),
            indent(m_specular));
    }
};

} // namespace lightwave

REGISTER_BSDF(Principled, "principled")
