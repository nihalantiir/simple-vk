#include "core/Swapchain.h"
#include "core/VulkanContext.h"
#include "core/Window.h"
#include "debug/DebugUi.h"
#include "game/Game.h"
#include "renderer/Renderer.h"

#include <SDL3/SDL.h>

#include <cstdlib>
#include <exception>
#include <iostream>

int main() {
    try {
        core::Window window("simple-vk", 1280, 720);
        core::VulkanContext context(window);
        core::Swapchain swapchain(context, window);
        renderer::Renderer renderer(context, swapchain, window);
        debug::DebugUi debugUi(context, swapchain, window, renderer);
        game::Game game;

        Uint64 lastTicks = SDL_GetPerformanceCounter();
        const Uint64 frequency = SDL_GetPerformanceFrequency();

        while (!window.shouldClose()) {
            window.pollEvents([&debugUi](const SDL_Event& event) { debugUi.processEvent(event); });

            int width = 0;
            int height = 0;
            window.getFramebufferSize(width, height);
            if (width == 0 || height == 0) {
                continue; // minimized: skip rendering until the window has a usable size
            }

            const Uint64 now = SDL_GetPerformanceCounter();
            const float deltaTime = static_cast<float>(now - lastTicks) / static_cast<float>(frequency);
            lastTicks = now;

            game.update(deltaTime);
            debugUi.beginFrame();
            renderer.drawFrame(&debugUi);
            window.updateTitle(deltaTime);
        }
    } catch (const std::exception& e) {
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
