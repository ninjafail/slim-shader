#include <lightwave.hpp>

namespace lightwave {

class Diffuse : public Bsdf {
    ref<Texture> m_albedo;

public:
    Diffuse(const Properties &properties) {
        m_albedo = properties.get<Texture>("albedo");
    }

    BsdfEval evaluate(const Point2 &uv, const Vector &wo,
                      const Vector &wi) const override {
        // Check if we and wo are in same hemisphere
        if (!Frame::sameHemisphere(wo, wi)) {
            return BsdfEval::invalid();
        }
        Color color = m_albedo->evaluate(uv) * InvPi *
                      Frame::absCosTheta(wi.normalized());
        return BsdfEval{ .value = color };
    }

    BsdfSample sample(const Point2 &uv, const Vector &wo,
                      Sampler &rng) const override {
        // First sample ray direction
        Vector out_dir = squareToCosineHemisphere(rng.next2D());
        // sometimes we get a ray direction that is not in the same hemisphere
        // Remark: sample solution uses cosTheta(wo) to check hemisphere and 
        // flips whole vector
        if (!Frame::sameHemisphere(wo, out_dir)) {
            out_dir.z() *= -1;
        }
        // The functions indicate that cosineHemispherePdf = z * InvPi and
        // Frame::absCosTheta(vec) = vec.z(). Thus when dividing by the pdf,
        // the InvPi and Frame::absCosTheta(out_dir) cancel out, leaving us with
        // with the albedo value.
        return BsdfSample{ .wi     = out_dir.normalized(),
                           .weight = m_albedo->evaluate(uv) };
    }

    Color albedo(const Point2 &uv) const override {
        return m_albedo->evaluate(uv);
    }

    std::string toString() const override {
        return tfm::format(
            "Diffuse[\n"
            "  albedo = %s\n"
            "]",
            indent(m_albedo));
    }
};

} // namespace lightwave

REGISTER_BSDF(Diffuse, "diffuse")
