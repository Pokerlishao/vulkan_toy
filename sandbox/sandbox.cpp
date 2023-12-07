#include "SDL.h"
#include "SDL_vulkan.h"
#include <iostream>
#include <vector>

#include "toy2d/tool.hpp"
#include "toy2d/toy2d.hpp"

int main(int argc, char** argv) {
    SDL_Init(SDL_INIT_EVERYTHING);

    SDL_Window* window = SDL_CreateWindow("sandbox",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          1024, 720,
                                          SDL_WINDOW_SHOWN|SDL_WINDOW_VULKAN);
    if (!window) {
        SDL_Log("create window failed");
        exit(2);
    }
    bool shouldClose = false;
    SDL_Event event;
    uint32_t count;



    //两次调用
    SDL_Vulkan_GetInstanceExtensions(window, &count, nullptr);
    std::vector<const char*> extensions(count);
    SDL_Vulkan_GetInstanceExtensions(window, &count, extensions.data());
    
    toy2d::Init(extensions, [&](vk::Instance instance) {
            VkSurfaceKHR surface;
            SDL_Vulkan_CreateSurface(window, instance, &surface);
            return surface;
            }, 1024, 720
        );

    auto renderer = toy2d::GetRenderer();

    int sw = 0;

    std::vector<int>x, y;
    std::vector<float> rot;

    toy2d::Texture * texture1 = toy2d::LoadTexture("G:\\code\\toy2d\\resources\\nahida.png");
    toy2d::Texture* texture2 = toy2d::LoadTexture("G:\\code\\toy2d\\resources\\furina.jpg");
    x.push_back(100);   y.push_back(100);    rot.push_back(0);
    x.push_back(600);   y.push_back(300);    rot.push_back(0);

    renderer->SetDrawColor(toy2d::Color{ 0, 1, 0 });
    while (!shouldClose) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                shouldClose = true;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_a) {
                    x[sw] -= 10;
                }
                if (event.key.keysym.sym == SDLK_d) {
                    x[sw] += 10;
                }
                if (event.key.keysym.sym == SDLK_w) {
                    y[sw] -= 10;
                }
                if (event.key.keysym.sym == SDLK_s) {
                    y[sw] += 10;
                }
                if (event.key.keysym.sym == SDLK_q) {
                    rot[sw] += 10;
                }
                if (event.key.keysym.sym == SDLK_e) {
                    rot[sw] -= 10;
                }
                if (event.key.keysym.sym == SDLK_0) {
                    renderer->SetDrawColor(toy2d::Color{ 1, 0, 0 });
                }
                if (event.key.keysym.sym == SDLK_1) {
                    renderer->SetDrawColor(toy2d::Color{ 0, 1, 0 });
                }
                if (event.key.keysym.sym == SDLK_2) {
                    renderer->SetDrawColor(toy2d::Color{ 0, 0, 1 });
                }
                if (event.key.keysym.sym == SDLK_g) {
                    sw = 1-sw;
                }
            }
        }
        renderer->StartRender();
        renderer->DrawTexture( x[0], y[0], rot[0], *texture1);
        renderer->DrawTexture( x[1], y[1], rot[1], *texture2);
        renderer->EndRender();
    }

    toy2d::DestroyTexture(texture1);
    toy2d::DestroyTexture(texture2);

    toy2d::Quit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
