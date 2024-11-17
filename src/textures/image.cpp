#include <lightwave.hpp>

namespace lightwave {

class ImageTexture : public Texture {
    enum class BorderMode {
        Clamp,
        Repeat,
    };

    enum class FilterMode {
        Nearest,
        Bilinear,
    };

    ref<Image> m_image;
    float m_exposure;
    BorderMode m_border;
    FilterMode m_filter;

public:
    ImageTexture(const Properties &properties) {
        if (properties.has("filename")) {
            m_image = std::make_shared<Image>(properties);
        } else {
            m_image = properties.getChild<Image>();
        }
        m_exposure = properties.get<float>("exposure", 1);

        // clang-format off
        m_border = properties.getEnum<BorderMode>("border", BorderMode::Repeat, {
            { "clamp", BorderMode::Clamp },
            { "repeat", BorderMode::Repeat },
        });

        m_filter = properties.getEnum<FilterMode>("filter", FilterMode::Bilinear, {
            { "nearest", FilterMode::Nearest },
            { "bilinear", FilterMode::Bilinear },
        });
        // clang-format on
    }

    Color evaluate(const Point2 &uv) const override {
        float u = uv.x();
        float v = 1 - uv.y();

        // first we transform the uv coordinates into image coordinates x, y
        auto resolution = m_image->resolution();
        float x_float   = u * (resolution.x() - 1);
        float y_float   = v * (resolution.y() - 1);

        if (m_filter == FilterMode::Nearest) {
            int x = floorf(x_float + 0.5);
            int y = floorf(y_float + 0.5);

            switch (m_border) {
            case BorderMode::Clamp:
                x = clamp(x, 0, resolution.x() - 1);
                y = clamp(y, 0, resolution.y() - 1);
                break;
            case BorderMode::Repeat:
                // mod can yield negative results, so some additional handling
                // is required
                x = (x % resolution.x() + resolution.x()) % resolution.x();
                y = (y % resolution.y() + resolution.y()) % resolution.y();
                break;
            }

            Color color = m_image->get(Point2i(x, y));
            return color;
        } else if (m_filter == FilterMode::Bilinear) {
            int x_0 = floorf(x_float);
            int y_0 = floorf(y_float);
            int x_1 = x_0 + 1;
            int y_1 = y_0 + 1;

            switch (m_border) {
            case BorderMode::Clamp:
                x_float = clamp(x_float, 0.0f, resolution.x() - 1.0f);
                y_float = clamp(y_float, 0.0f, resolution.y() - 1.0f);
                x_0     = clamp(x_0, 0, resolution.x() - 1);
                y_0     = clamp(y_0, 0, resolution.y() - 1);
                x_1     = clamp(x_1, 0, resolution.x() - 1);
                y_1     = clamp(y_1, 0, resolution.y() - 1);
                break;
            case BorderMode::Repeat:
                // mod can yield negative results, so some additional handling
                // is required
                x_float = fmod(fmod(x_float, resolution.x()) + resolution.x(),
                               resolution.x());
                y_float = fmod(fmod(y_float, resolution.y()) + resolution.y(),
                               resolution.y());
                y_0 = (y_0 % resolution.y() + resolution.y()) % resolution.y();
                x_0 = (x_0 % resolution.x() + resolution.x()) % resolution.x();
                y_0 = (y_0 % resolution.y() + resolution.y()) % resolution.y();
                x_1 = (x_1 % resolution.x() + resolution.x()) % resolution.x();
                y_1 = (y_1 % resolution.y() + resolution.y()) % resolution.y();
                break;
            }

            Color c_00 = m_image->get(Point2i(x_0, y_0));
            Color c_01 = m_image->get(Point2i(x_0, y_1));
            Color c_10 = m_image->get(Point2i(x_1, y_0));
            Color c_11 = m_image->get(Point2i(x_1, y_1));

            // linear interpolation of the x and y axis

            Color c_0 = lerp(c_00, c_01, y_float - y_0);
            Color c_1 = lerp(c_10, c_11, y_float - y_0);

            Color color = lerp(c_0, c_1, x_float - x_0);

            return color;
        }

        return Color(0);
    }

    std::string toString() const override {
        return tfm::format(
            "ImageTexture[\n"
            "  image = %s,\n"
            "  exposure = %f,\n"
            "]",
            indent(m_image),
            m_exposure);
    }
};

} // namespace lightwave

REGISTER_TEXTURE(ImageTexture, "image")
