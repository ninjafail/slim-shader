#include <lightwave.hpp>

namespace lightwave {
class PathtracerIntegrator : public SamplingIntegrator {
    int depth;

public:
    PathtracerIntegrator(const Properties &properties)
        : SamplingIntegrator(properties) {
        depth = properties.get<int>("depth", 2);
    }

    Color Li(const Ray &ray, Sampler &rng) override {
        // Start with the emission of the hit object/background
        Color li(0);

        Ray cur_ray       = ray;
        Color path_weight = Color(1);

        float p_bsdf = 1.0f;

        for (int cur_depth = 1; cur_depth < depth; cur_depth++) {
            Intersection its = m_scene->intersect(cur_ray, rng);

            // If no intersection was found: we add contribution of background
            if (!its) {
                li += its.evaluateEmission().value * path_weight;
                break;
            }

            // Compute direct illumination
            Color light_contribution(0);
            float p_ne = 0.0f;

            if (m_scene->hasLights()) {
                LightSample light = m_scene->sampleLight(rng);

                if (!light.isInvalid() && light.probability >= Epsilon) {
                    DirectLightSample sample =
                        light.light->sampleDirect(its.position, rng);
                    Ray reverse_light_ray(its.position, sample.wi);

                    // If light is occluded: return black
                    // light is occluded if there is an intersection from the
                    // surface to the light source
                    Intersection light_its =
                        m_scene->intersect(reverse_light_ray, rng);
                    if (!light_its && light_its.t >= sample.distance) {
                        light_contribution =
                            sample.weight * its.evaluateBsdf(sample.wi).value;
                        p_ne = sample.pdf * light.probability;
                    }
                }
            }

            // Compute bsdf contribution
            Color bsdf_contribution = its.evaluateEmission().value;

            // Compute balance heuristics
            float weight_bsdf;
            float weight_ne;

            float pdf_sum = p_ne + p_bsdf;
            if (pdf_sum > 0 && p_ne != Infinity && p_bsdf != Infinity) {
                weight_bsdf = p_bsdf / pdf_sum;
                weight_ne   = p_ne / pdf_sum;
            } else if (p_ne == Infinity) {
                weight_bsdf = 0.0f;
                weight_ne   = 1.0f;
            } else {
                weight_bsdf = 1.0f;
                weight_ne   = 0.0f;
            }

            Color contribution = bsdf_contribution * weight_bsdf +
                                 light_contribution * weight_ne;

            li += contribution * path_weight;

            //-----------------------------------

            // Sample direction w_i to continue the path
            BsdfSample bsdf_sample = its.sampleBsdf(rng);
            if (bsdf_sample.isInvalid()) {
                break;
            }
            // Trace ray to find next point
            Ray bsdf_ray(its.position, bsdf_sample.wi.normalized());
            cur_ray = bsdf_ray;

            // Compute bsdf contribution
            p_bsdf = bsdf_sample.pdf;
            path_weight *= bsdf_sample.weight;
        }

        return li;
    }

    std::string toString() const override { return "PathtracerIntegrator[]"; }
};
} // namespace lightwave

REGISTER_INTEGRATOR(PathtracerIntegrator, "pathtracer")
