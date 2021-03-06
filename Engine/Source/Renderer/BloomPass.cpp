#include "Renderer/BloomPass.h"

#include "Renderer.h"

#include "TextureUnit.h"
#include "Texture.h"
#include "Framebuffer.h"

namespace Flux {
    BloomPass::BloomPass() : RenderPhase("Bloom")
    {
        shader.loadFromFile("res/Shaders/Quad.vert", "res/Shaders/Bloom.frag");

        requiredSet.addCapability(STENCIL_TEST, true);
        requiredSet.addCapability(DEPTH_TEST, false);
    }

    void BloomPass::Resize(const Size& windowSize)
    {
        Texture2D bloomTex;
        bloomTex.create();
        bloomTex.bind(TextureUnit::TEXTURE0);
        bloomTex.setData(windowSize.width, windowSize.height, GL_RGBA16F, GL_RGBA, GL_FLOAT, nullptr);
        bloomTex.setWrapping(CLAMP, CLAMP);
        bloomTex.setSampling(LINEAR, LINEAR, LINEAR);
        bloomTex.generateMipmaps();
        bloomTex.release();

        buffer.create();
        buffer.bind();
        // Textures are linearly sampled for first step of gaussian bloom blur
        buffer.addColorTexture(0, bloomTex);
        buffer.validate();
        buffer.release();

        blurPass.Resize(windowSize);
    }

    void BloomPass::render(RenderState& renderState, const Scene& scene)
    {
        renderState.require(requiredSet);
        nvtxRangePushA(getPassName().c_str());

        glStencilFunc(GL_ALWAYS, 0, 0xFF);

        const Framebuffer* sourceFramebuffer = RenderState::currentFramebuffer;

        shader.bind();

        source->bind(TextureUnit::TEXTURE);

        shader.uniform1i("tex", TextureUnit::TEXTURE);
        shader.uniform1f("threshold", 0);

        buffer.bind();

        renderState.drawQuad();

        // Blur the buffer
        blurPass.SetSource(&buffer.getTexture());
        blurPass.render(renderState, scene);

        // Add the bloom to the HDR buffer
        sourceFramebuffer->bind();

        std::vector<Texture2D> sources{ *source, buffer.getTexture() };
        std::vector<float> weights{ 1, 0.5f };
        addPass.SetTextures(sources);
        addPass.SetWeights(weights);
        addPass.render(renderState, scene);

        nvtxRangePop();
    }
}
