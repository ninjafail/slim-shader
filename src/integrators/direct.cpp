#include <lightwave.hpp>

namespace lightwave {
class DirectIntegrator : public SamplingIntegrator {
public:
    DirectIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {}

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection intersection = m_scene->intersect(ray, rng);

        // If no intersection was found: we add contribution of background
        if (intersection.background) {
            return intersection.evaluateEmission().value;
        }

        // If intersection was found: sample light
        LightSample light = m_scene->sampleLight(rng);
        DirectLightSample sample =
            light.light->sampleDirect(intersection.position, rng);

        Ray light_ray(intersection.position, -sample.wi);

        if (m_scene->intersect(light_ray, rng).t < sample.distance) {
            return Color(0);
        }

        return sample.weight * intersection.evaluateBsdf(ray.direction).value;
    }

    std::string toString() const override { return "DirectIntegrator[]"; }
};
} // namespace lightwave

REGISTER_INTEGRATOR(DirectIntegrator, "direct")
