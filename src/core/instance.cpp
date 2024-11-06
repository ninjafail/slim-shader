#include <lightwave/core.hpp>
#include <lightwave/instance.hpp>
#include <lightwave/registry.hpp>
#include <lightwave/sampler.hpp>

namespace lightwave {

void Instance::transformFrame(SurfaceEvent &surf, const Vector &wo) const {
    // Transforms the Frame from object space to world space.
    // reproject the shading frame into an orthonormal basis
    // -> keep the direction of the tangent (but normalize it)

    Frame shadingFrame = surf.shadingFrame();
    surf.tangent       = shadingFrame.tangent.normalized();
    surf.geometryNormal =
        m_transform->applyNormal(shadingFrame.normal).normalized();
    surf.shadingNormal = surf.geometryNormal;

    // TODO: maybe change pdf
}

inline void validateIntersection(const Intersection &its) {
    // use the following macros to make debugginer easier:
    // * assert_condition(condition, { ... });
    // * assert_normalized(vector, { ... });
    // * assert_ortoghonal(vec1, vec2, { ... });
    // * assert_finite(value or vector or color, { ... });

    // each assert statement takes a block of code to execute when it fails
    // (useful for printing out variables to narrow done what failed)

    assert_finite(its.t, {
        logger(
            EError,
            "  your intersection produced a non-finite intersection distance");
        logger(EError, "  offending shape: %s", its.instance->shape());
    });
    assert_condition(its.t >= Epsilon, {
        logger(EError,
               "  your intersection is susceptible to self-intersections");
        logger(EError, "  offending shape: %s", its.instance->shape());
        logger(EError,
               "  returned t: %.3g (smaller than Epsilon = %.3g)",
               its.t,
               Epsilon);
    });
}

bool Instance::intersect(const Ray &worldRay, Intersection &its,
                         Sampler &rng) const {
    if (!m_transform) {
        // fast path, if no transform is needed
        const Ray localRay        = worldRay;
        const bool wasIntersected = m_shape->intersect(localRay, its, rng);
        if (wasIntersected) {
            its.instance = this;
            validateIntersection(its);
        }
        return wasIntersected;
    }

    const float previousT = its.t;
    Ray localRay;

    // * transform the ray (do not forget to normalize!)
    localRay = m_transform->inverse(worldRay).normalized();

    // hint: how does its.t need to change?
    // currently the old intersection point is from world space, comparing it
    // with the new intersection point in its local space will not work
    // -> transform the old intersection point to new local space
    if (its)
        its.t = (localRay.origin - this->m_transform->inverse(its.position))
                    .length();

    // calculate hit point
    const bool wasIntersected = m_shape->intersect(localRay, its, rng);
    if (!wasIntersected) {
        its.t = previousT;
        return false;
    }

    its.instance = this;
    validateIntersection(its);
    // -> transform the hit point to world space and get distance
    its.position = m_transform->apply(its.position);
    its.t        = (its.position - worldRay.origin).length();

    transformFrame(its, -localRay.direction);

    return wasIntersected;
}

Bounds Instance::getBoundingBox() const {
    if (!m_transform) {
        // fast path
        return m_shape->getBoundingBox();
    }

    const Bounds untransformedAABB = m_shape->getBoundingBox();
    if (untransformedAABB.isUnbounded()) {
        return Bounds::full();
    }

    Bounds result;
    for (int point = 0; point < 8; point++) {
        Point p = untransformedAABB.min();
        for (int dim = 0; dim < p.Dimension; dim++) {
            if ((point >> dim) & 1) {
                p[dim] = untransformedAABB.max()[dim];
            }
        }
        p = m_transform->apply(p);
        result.extend(p);
    }
    return result;
}

Point Instance::getCentroid() const {
    if (!m_transform) {
        // fast path
        return m_shape->getCentroid();
    }

    return m_transform->apply(m_shape->getCentroid());
}

AreaSample Instance::sampleArea(Sampler &rng) const {
    AreaSample sample = m_shape->sampleArea(rng);
    transformFrame(sample, Vector());
    return sample;
}

} // namespace lightwave

REGISTER_CLASS(Instance, "instance", "default")
