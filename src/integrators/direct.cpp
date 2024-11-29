#include <lightwave.hpp>

namespace lightwave {
class DirectIntegrator : public SamplingIntegrator {
    Color LiLightSample(const Intersection &its, Sampler &rng) {
        LightSample light = m_scene->sampleLight(rng);

        if (light.isInvalid()) {
            return Color(0);
        }
        DirectLightSample sample = light.light->sampleDirect(its.position, rng);
        Ray reverse_light_ray(its.position, sample.wi);

        // If light is occluded: return black
        // light is occluded if there is an intersection from the
        // surface to the light source
        Intersection light_its = m_scene->intersect(reverse_light_ray, rng);
        if (light_its && light_its.t < sample.distance)
            return Color(0);

        return sample.weight * its.evaluateBsdf(sample.wi).value /
               light.probability;
    }

public:
    DirectIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {}

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);

        // Start with the emission of the hit object/background
        Color li = its.evaluateEmission().value;
        // If no intersection was found: we add contribution of background
        if (!its) {
            return li;
        }

        // return color and this samples associated weight
        if (m_scene->hasLights()) {
            // If intersection was found: sample light
            li += LiLightSample(its, rng);
        }

        // Sample one bounce to get lights from emissive objects
        BsdfSample bsdf_sample = its.sampleBsdf(rng);

        Ray bsdf_ray(its.position, bsdf_sample.wi.normalized());
        Intersection bsdf_its = m_scene->intersect(bsdf_ray, rng);
        return li + bsdf_its.evaluateEmission().value * bsdf_sample.weight;
    }

    std::string toString() const override { return "DirectIntegrator[]"; }
};
} // namespace lightwave

REGISTER_INTEGRATOR(DirectIntegrator, "direct")
