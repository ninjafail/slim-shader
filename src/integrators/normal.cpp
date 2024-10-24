#include <lightwave.hpp>

namespace lightwave {
class NormalIntegrator : public SamplingIntegrator {
    bool m_remap;

public:
    NormalIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        m_remap = properties.get<bool>("remap");
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        auto intersection = m_scene->intersect(ray, rng);
        return Color(intersection.geometryNormal);
    }

    std::string toString() const override { return "NormalIntegrator[]"; }
};
} // namespace lightwave