#include <lightwave.hpp>

#include "../core/plyparser.hpp"
#include "accel.hpp"

namespace lightwave {

/**
 * @brief A shape consisting of many (potentially millions) of triangles, which
 * share an index and vertex buffer. Since individual triangles are rarely
 * needed (and would pose an excessive amount of overhead), collections of
 * triangles are combined in a single shape.
 */
class TriangleMesh : public AccelerationStructure {
    /**
     * @brief The index buffer of the triangles.
     * The n-th element corresponds to the n-th triangle, and each component of
     * the element corresponds to one vertex index (into @c m_vertices ) of the
     * triangle. This list will always contain as many elements as there are
     * triangles.
     */
    std::vector<Vector3i> m_triangles;
    /**
     * @brief The vertex buffer of the triangles, indexed by m_triangles.
     * Note that multiple triangles can share vertices, hence there can also be
     * fewer than @code 3 * numTriangles @endcode vertices.
     */
    std::vector<Vertex> m_vertices;
    /// @brief The file this mesh was loaded from, for logging and debugging
    /// purposes.
    std::filesystem::path m_originalPath;
    /// @brief Whether to interpolate the normals from m_vertices, or report the
    /// geometric normal instead.
    bool m_smoothNormals;
    float area;

protected:
    int numberOfPrimitives() const override { return int(m_triangles.size()); }

    bool intersect(int primitiveIndex, const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        Point orig = ray.origin;
        Vector dir = ray.direction;

        Vector3i indices = m_triangles[primitiveIndex];

        Vertex v0 = m_vertices[indices[0]];
        Vertex v1 = m_vertices[indices[1]];
        Vertex v2 = m_vertices[indices[2]];

        Vector v0v1 = v1.position - v0.position;
        Vector v0v2 = v2.position - v0.position;
        Vector pvec = dir.cross(v0v2);
        float det   = v0v1.dot(pvec);

        if (fabs(det) < 0)
            return false;

        float invDet = 1 / det;

        Vector tvec = orig - v0.position;
        float u     = tvec.dot(pvec) * invDet;
        if (u < 0 || u > 1)
            return false;

        Vector qvec = tvec.cross(v0v1);
        float v     = dir.dot(qvec) * invDet;
        if (v < 0 || u + v > 1)
            return false;

        float t = v0v2.dot(qvec) * invDet;

        if (t < Epsilon || t > its.t)
            return false;

        its.t               = t;
        its.position        = ray(t);
        Vertex interpolated = Vertex::interpolate(Vector2(u, v), v0, v1, v2);
        its.uv              = interpolated.uv;

        its.geometryNormal = v0v1.cross(v0v2).normalized();
        if (m_smoothNormals) {
            its.shadingNormal = interpolated.normal.normalized();
        } else {
            its.shadingNormal = its.geometryNormal;
        }
        Vector bitangent;
        buildOrthonormalBasis(its.shadingNormal, its.tangent, bitangent);
        its.pdf = 0;

        return true;

        // hints:
        // * use m_triangles[primitiveIndex] to get the vertex indices of the
        // triangle that should be intersected
        // * if m_smoothNormals is true, interpolate the vertex normals from
        // m_vertices
        //   * make sure that your shading frame stays orthonormal!
        // * if m_smoothNormals is false, use the geometrical normal (can be
        // computed from the vertex positions)
    }

    Bounds getBoundingBox(int primitiveIndex) const override {
        Vector3i indices = m_triangles[primitiveIndex];

        Vertex v0 = m_vertices[indices[0]];
        Vertex v1 = m_vertices[indices[1]];
        Vertex v2 = m_vertices[indices[2]];

        float minX =
            min(v0.position.x(), min(v1.position.x(), v2.position.x()));
        float minY =
            min(v0.position.y(), min(v1.position.y(), v2.position.y()));
        float minZ =
            min(v0.position.z(), min(v1.position.z(), v2.position.z()));

        float maxX =
            max(v0.position.x(), max(v1.position.x(), v2.position.x()));
        float maxY =
            max(v0.position.y(), max(v1.position.y(), v2.position.y()));
        float maxZ =
            max(v0.position.z(), max(v1.position.z(), v2.position.z()));

        return Bounds(Point{ minX, minY, minZ }, Point{ maxX, maxY, maxZ });
    }

    Point getCentroid(int primitiveIndex) const override {
        Vector3i indices = m_triangles[primitiveIndex];

        Vector v0 = Vector(m_vertices[indices[0]].position);
        Vector v1 = Vector(m_vertices[indices[1]].position);
        Vector v2 = Vector(m_vertices[indices[2]].position);

        Vector centroid = (v0 + v1 + v2) / 3;
        return centroid;
    }

public:
    TriangleMesh(const Properties &properties) {
        m_originalPath  = properties.get<std::filesystem::path>("filename");
        m_smoothNormals = properties.get<bool>("smooth", true);
        readPLY(m_originalPath, m_triangles, m_vertices);
        logger(EInfo,
               "loaded ply with %d triangles, %d vertices",
               m_triangles.size(),
               m_vertices.size());
        buildAccelerationStructure();
        area = 0;
        for (int i = 0; i < m_triangles.size(); i++) {
            Vector3i indices = m_triangles[i];

            Vertex v0 = m_vertices[indices[0]];
            Vertex v1 = m_vertices[indices[1]];
            Vertex v2 = m_vertices[indices[2]];

            Vector v0v1 = v1.position - v0.position;
            Vector v0v2 = v2.position - v0.position;

            area += 0.5 * v0v1.cross(v0v2).length();
        }
    }

    bool intersect(const Ray &ray, Intersection &its,
                   Sampler &rng) const override {
        PROFILE("Triangle mesh")
        return AccelerationStructure::intersect(ray, its, rng);
    }

    AreaSample sampleArea(Sampler &rng) const override {
        // only implement this if you need triangle mesh area light sampling for
        // your rendering competition
        // TODO: this does not work for arbitrary meshes, only for rectangles,
        // but since we only have rectangular area lights, this is fine
        Point2 rnd = rng.next2D();
        Point position{
            2 * rnd.x() - 1, 2 * rnd.y() - 1, 0
        }; // stretch the random point to [-1,-1]..[+1,+1] and set z=0

        AreaSample surf;
        surf.position = position;

        surf.uv.x() = position.x();
        surf.uv.y() = position.y();
        
        // the tangent always points in positive x direction
        surf.tangent = Vector(1, 0, 0);
        // and accordingly, the normal always points in the positive z direction
        surf.shadingNormal  = Vector(0, 0, 1);
        surf.geometryNormal = Vector(0, 0, 1);

        // since we sample the area uniformly, the pdf is given by 1/surfaceArea
        surf.pdf = 1.0f / area;

        return surf;
    }

    std::string toString() const override {
        return tfm::format(
            "Mesh[\n"
            "  vertices = %d,\n"
            "  triangles = %d,\n"
            "  filename = \"%s\"\n"
            "]",
            m_vertices.size(),
            m_triangles.size(),
            m_originalPath.generic_string());
    }
};

} // namespace lightwave

REGISTER_SHAPE(TriangleMesh, "mesh")
