#include <lightwave.hpp>

namespace lightwave {
class NormalIntegrator : public SamplingIntegrator {
    bool m_remap = true;

public:
    NormalIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        m_remap = properties.get<bool>("remap");
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        auto intersection = m_scene->intersect(ray, rng);
        Vector normal     = intersection.geometryNormal;
        if (m_remap)
            return (Color(normal) + Color(1)) / 2;
        return Color(normal);
    }

    std::string toString() const override { return "NormalIntegrator[]"; }
};
} // namespace lightwave

REGISTER_INTEGRATOR(NormalIntegrator, "normals")