#pragma once

#include <volk.h>

#include <string>
#include <vector>

struct SDL_Window;

namespace core {

// Wraps SDL3 window creation and the SDL/Vulkan integration points: the
// required instance extension list, surface creation, and event polling.
// This is the only class that knows about SDL; everything else deals in
// plain Vulkan handles.
class Window {
public:
    Window(const std::string& title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Pumps the SDL event queue and updates internal state (quit/resize).
    void pollEvents();

    VkSurfaceKHR createSurface(VkInstance instance) const;
    std::vector<const char*> getRequiredInstanceExtensions() const;

    // Current drawable size in pixels (accounts for HiDPI scaling).
    void getFramebufferSize(int& width, int& height) const;

    bool shouldClose() const { return quitRequested_; }

    // Returns true exactly once after a resize event, then clears the flag.
    bool consumeResizedFlag();

private:
    SDL_Window* window_ = nullptr;
    bool quitRequested_ = false;
    bool resized_ = false;
};

} // namespace core
