#include <lightwave.hpp>

namespace lightwave {
class PathtracerIntegrator : public SamplingIntegrator {
    int depth;

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
    PathtracerIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        depth = properties.get<int>("depth", 2);
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        Intersection its = m_scene->intersect(ray, rng);
        // Start with the emission of the hit object/background
        Color li = its.evaluateEmission().value;
        // If no intersection was found: we add contribution of background
        if (!its) {
            return li;
        }

        Intersection cur_its = its;
        Color cur_weigth     = Color(1);
        for (int cur_depth = 1; cur_depth < depth; cur_depth++) {
            Color cur_li = Color(0);

            // compute direct illumination
            if (m_scene->hasLights()) {
                cur_li += LiLightSample(cur_its, rng);
            }

            // Sample direction w_i to continue the path
            BsdfSample bsdf_sample = cur_its.sampleBsdf(rng);
            if (bsdf_sample.isInvalid()) {
                break;
            }
            // Trace ray to find next point
            Ray bsdf_ray(cur_its.position, bsdf_sample.wi.normalized());
            cur_its = m_scene->intersect(bsdf_ray, rng);
            cur_li += cur_its.evaluateEmission().value * bsdf_sample.weight;

            li += cur_li * cur_weigth;
            cur_weigth *= bsdf_sample.weight;

            if (!cur_its) {
                break;
            }
        }

        return li;
    }

    std::string toString() const override { return "PathtracerIntegrator[]"; }
};
} // namespace lightwave

REGISTER_INTEGRATOR(PathtracerIntegrator, "pathtracer")
