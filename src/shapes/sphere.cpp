#include <lightwave.hpp>

namespace lightwave {
class Sphere : public Shape {
    inline void populate(SurfaceEvent &surf, const Point &position) const {
        surf.position = position;
        surf.pdf      = 0; // for now (stated in the assignment)

        // map the position from [-1,-1,0]..[+1,+1,0] to [0,0]..[1,1] by
        // discarding the z component and rescaling
        surf.uv.x() = 0.5 + (std::atan2(position.x(), position.z()) / (2 * Pi));
        surf.uv.y() = 0.5 - (asin(position.y()) / Pi);

        // the normal is the vector from origin (0, 0, 0) to the intersection
        // point, it is already normalized bc sphere has radius 1
        surf.shadingNormal  = Vector(position);
        surf.geometryNormal = Vector(position);

        // define tangent with the cross product of the normal and (1, 0, 0) if
        // the intersection is on the y axis and with (0, 1, 0) otherwise
        if (position.x() == 0)
            surf.tangent = Vector(position).cross(Vector(1, 0, 0)).normalized();
        else
            surf.tangent = Vector(position).cross(Vector(0, 1, 0)).normalized();
    }

public:
    Sphere(const Properties &properties) {}

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        PROFILE("Sphere")

        Vector ori = Vector(ray.origin);
        Vector dir = ray.direction;

        // computations based on material provided in lecture
        float b = 2 * dir.dot(ori);
        float c = ori.dot(ori) - 1;

        float discriminant = pow(b, 2) - 4 * c;
        // if the discriminant is below 0 there is no intersection
        if (discriminant < 0)
            return false;
        float sqrt_discriminant = sqrt(discriminant);

        float t_0 = (-b - sqrt_discriminant) / 2;
        float t_1 = (-b + sqrt_discriminant) / 2;
        float t;

        // t has to be positive for the hit point not to be behind the origin
        if (t_0 > Epsilon) {
            t = t_0;
        } else if (t_1 > 0) {
            t = t_1;
        } else {
            return false;
        }
        // note that we never report an intersection closer than Epsilon (to
        // avoid self-intersections)! we also do not update the intersection if
        // a closer intersection already exists (i.e., its.t is lower than our
        // own t)
        if (t < Epsilon || t > its.t)
            return false;

        // compute the hitpoint
        const Point position = ray(t);

        // we have determined there was an intersection! we are now free to
        // change the intersection object and return true.
        its.t = t;
        populate(its, position);
        return true;
    }

    Bounds getBoundingBox() const override {
        return Bounds(Point{ -1, -1, -1 }, Point{ +1, +1, +1 });
    }

    Point getCentroid() const override { return Point(0); }

    AreaSample sampleArea(Sampler &rng) const override{ NOT_IMPLEMENTED }

    std::string toString() const override {
        return "Sphere[]";
    }
};
} // namespace lightwave
REGISTER_SHAPE(Sphere, "sphere")
