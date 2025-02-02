#include <lightwave.hpp>

namespace lightwave {
class AlbedoIntegrator : public SamplingIntegrator {
    bool m_remap = true;

public:
    AlbedoIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {}

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection intersection = m_scene->intersect(ray, rng);
        if (!intersection)
            return Color(0.5f);

        if (intersection.instance->bsdf() == nullptr &&
            intersection.instance->emission() != nullptr) {
            auto emission = intersection.evaluateEmission();
            if (emission)
                return emission.value;
            else
                return Color(0);
        }
        if (intersection.instance->bsdf() == nullptr) {
            return Color(0);
        }

        Color albedo = intersection.instance->bsdf()->albedo(intersection.uv);
        return albedo;
    }

    std::string toString() const override { return "AlbedoIntegrator[]"; }
};
} // namespace lightwave

REGISTER_INTEGRATOR(AlbedoIntegrator, "albedo")