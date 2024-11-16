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
        // first normalize the uv coordinates to [0, 1] depending on the border 
        // mode
        float u = uv.x();
        float v = uv.y();
        switch (m_border) {
            case BorderMode::Clamp: 
                u = clamp(u, 0.f, 1.f);
                v = 1 - clamp(v, 0.f, 1.f);
                break;
            case BorderMode::Repeat: 
                u = fmod(u, 1.f);
                v = 1 - fmod(v, 1.f);
                break;
        }

        auto resolution = m_image->resolution();
        int x = u * (resolution.x());
        int y = v * (resolution.y());

        Color color = m_image->get(Point2i(x, y));

        return color;
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
