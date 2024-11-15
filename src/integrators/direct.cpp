#include <lightwave.hpp>

namespace lightwave {
class DirectIntegrator : public SamplingIntegrator {
public:
    DirectIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {}

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);

        // If no intersection was found: we add contribution of background
        if (!its) {
            return its.evaluateEmission().value;
        }

        // If intersection was found: sample light
        LightSample light        = m_scene->sampleLight(rng);
        DirectLightSample sample = light.light->sampleDirect(its.position, rng);
        Ray reverse_light_ray(its.position, sample.wi);

        // If light is occluded: return black
        // light is occluded if there is no intersection from the surface to
        // the light source
        Intersection light_its = m_scene->intersect(reverse_light_ray, rng);
        if (light_its && light_its.t < sample.distance) {
            return Color(0);
        }

        return sample.weight * its.shadingNormal.dot(sample.wi) *
               its.evaluateBsdf(sample.wi).value;
    }

    std::string toString() const override { return "DirectIntegrator[]"; }
};
} // namespace lightwave

REGISTER_INTEGRATOR(DirectIntegrator, "direct")
