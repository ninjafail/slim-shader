#include <lightwave.hpp>

namespace lightwave {

class CheckerboardTexture : public Texture {
    Color color0, color1;
    Point2 scale;

public:
    CheckerboardTexture(const Properties &properties) {
        color0 = properties.get<Color>("color0", Color(0));
        color1 = properties.get<Color>("color1", Color(1));
        scale  = properties.get<Point2>("scale", Point2(1));
    }

    Color evaluate(const Point2 &uv) const override {
        float x = uv.x() * scale.x();
        float y = uv.y() * scale.y();

        if ((int(x) + int(y)) % 2 == 0) {
            return color0;
        } else {
            return color1;
        }
    }

    std::string toString() const override {
        return tfm::format(
            "CheckerboardTexture[\n"
            "  color0 = %s\n"
            "  color1 = %s\n"
            "  scale = %s\n"
            "]",
            indent(color0),
            indent(color1),
            indent(scale));
    }
};

} // namespace lightwave

REGISTER_TEXTURE(CheckerboardTexture, "checkerboard")
