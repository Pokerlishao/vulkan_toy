#include "toy2d/toy2d.hpp"
#include "toy2d/context.hpp"
#include "toy2d/tool.hpp"
#include "toy2d/shader.hpp"
#include "toy2d/descriptor_manager.hpp"
#include "toy2d/texture.hpp"

namespace toy2d {

    std::unique_ptr<Renderer> renderer_;

    void Init(const std::vector<const char*>& extensions, CreateSurfaceFunc func, int W, int H) {
        Context::Init(extensions, func);
        auto& ctx = Context::GetInstance();
        ctx.InitSwapchain(W, H);
        Shader::Init(ReadWholeFile("G:/code/toy2d/shader/vert.spv"), ReadWholeFile("G:/code/toy2d/shader/frag.spv"));
        ctx.InitRenderProcess();
        ctx.InitGraphicsPipeline();
        ctx.swapchain->InitFramebuffers();
        ctx.InitCommandPool();
        ctx.initSampler();

        int maxFlightCount = 2;
        DescriptorSetManager::Init(maxFlightCount);
        renderer_ = std::make_unique<Renderer>(maxFlightCount);
        renderer_->SetProject(W, 0, 0, H, -1, 1);

        //File read Path 

    }

    void Quit() {
        Context::GetInstance().device.waitIdle();
        renderer_.reset();
        Shader::Quit();
        DescriptorSetManager::Quit();
        Context::Quit();
    }

    Renderer* GetRenderer() {
        return renderer_.get();
    }

    Texture* LoadTexture(const std::string& filename) {
        return TextureManager::Instance().Load(filename);
    }

    void DestroyTexture(Texture* texture) {
        TextureManager::Instance().Destroy(texture);
    }
}