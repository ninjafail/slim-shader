#ifdef LW_WITH_OIDN

#include <OpenImageDenoise/oidn.hpp>
#include <lightwave.hpp>

namespace lightwave {

class Denoise : public Postprocess {
protected:
    ref<Image> m_normal;
    ref<Image> m_albedo;

public:
    Denoise(const Properties &properties) : Postprocess(properties) {
        m_normal = properties.get<Image>("normal", nullptr);
        m_albedo = properties.get<Image>("albedo", nullptr);
    }

    void execute() override {
        const Point2i input_resolution = m_input->resolution();
        const int width                = input_resolution.x();
        const int height               = input_resolution.y();
        m_output->initialize(input_resolution);

        // Copied from the Intel Open Image Denoise example
        // Create an Open Image Denoise device
        oidn::DeviceRef device = oidn::newDevice(); // CPU or GPU if available
        // oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
        device.commit();

        // Create a filter for denoising a beauty (color) image using optional
        // auxiliary images too This can be an expensive operation, so try no to
        // create a new filter for every image!
        oidn::FilterRef filter =
            device.newFilter("RT"); // generic ray tracing filter
        filter.setImage("color",
                        m_input->data(),
                        oidn::Format::Float3,
                        width,
                        height); // beauty
        if (m_albedo)
            filter.setImage("albedo",
                            m_albedo->data(),
                            oidn::Format::Float3,
                            width,
                            height); // auxiliary
        if (m_normal)
            filter.setImage("normal",
                            m_normal->data(),
                            oidn::Format::Float3,
                            width,
                            height); // auxiliary

        filter.setImage("output",
                        m_output->data(),
                        oidn::Format::Float3,
                        width,
                        height); // denoised
        filter.set("hdr", true); // beauty image is HDR
        filter.commit();

        // Filter the beauty image
        filter.execute();

        // Check for errors
        const char *errorMessage;
        if (device.getError(errorMessage) != oidn::Error::None)
            lightwave_throw(errorMessage);

        // Save the output image and stream it to tev
        m_output->save();
        Streaming stream{ *m_output };
        stream.update();
    }

    std::string toString() const override { return tfm::format("Denoising[]"); }
};
} // namespace lightwave

REGISTER_POSTPROCESS(Denoise, "denoising")

#endif // LW_WITH_OIDN

/** Should be included like this:
<integrator type="pathtracer" depth="10">
  <ref id="scene"/>
  <image id="denoise_test"/>
  <sampler type="independent" count="8"/>
</integrator>
<integrator type="aov" variable="normals">
  <ref id="scene"/>
  <image id="denoise_test_normal"/>
  <sampler type="independent" count="8"/>
</integrator>
<integrator type="albedo">
  <ref id="scene"/>
  <image id="denoise_test_albedo"/>
  <sampler type="independent" count="8"/>
</integrator>
<postprocess type="denoising">
  <ref name="input" id="denoise_test"/>
  <ref name="normal" id="denoise_test_normal"/>
  <ref name="albedo" id="denoise_test_albedo"/>
  <image id="denoise_test_output"/>
</postprocess>
 */