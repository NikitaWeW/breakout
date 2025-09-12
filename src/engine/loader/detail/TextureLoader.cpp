#include "Loaders.hpp"
#include "stb_image.h"

static unsigned rand(unsigned &state) {
	state = state * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}
static float randZeroOne(unsigned &state) {
    return float(rand(state)) * (1.0 / float(0xffffffffu));
}

ecs::entity engine::loader::detail::TextureLoader::load(ecs::registry &reg, std::string_view path)
{
    int width = 0, height = 0, numChannels = 0;
    float *buff = stbi_loadf(path.data(), &width, &height, &numChannels, 0);
    if(!buff)
    {
        ENGINE_OUT << "failed to load texture: \"" << path << "\"!: " << stbi_failure_reason() << '\n';
        ENGINE_ASSERT(false, "failed to load a texture");
    }
    ENGINE_ASSERT(width > 0 && height > 0, "failed to load a texture");
    engine::Texture texture;
    texture.data = engine::Bitmap{width, height, numChannels, buff};
    stbi_image_free(buff);

    const int pixelCount = glm::min(width * height, 10);
    unsigned seed = width * height;
    texture.grayscale = true;
    for (int i = 0; i < pixelCount; ++i) {
        unsigned x = randZeroOne(seed) * width;
        unsigned y = randZeroOne(seed) * height;
        glm::vec4 pixel = texture.data.getPixel(x, y);
        if (pixel.r != pixel.g || pixel.r != pixel.b) {
            texture.grayscale = false;
            break;
        }
    }

    texture.type = "unknown";

    return reg.create(std::move(texture));
}