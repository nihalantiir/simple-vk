#pragma once

#include <volk.h>

#include <functional>
#include <string>
#include <vector>

struct SDL_Window;
union SDL_Event;

namespace core {

class Window {
public:
    using EventCallback = std::function<void(const SDL_Event&)>;

    Window(const std::string& title, int width, int height);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    // Pumps the SDL event queue, forwarding each raw event to onEvent (if
    // set) before updating internal state (quit/resize).
    void pollEvents(const EventCallback& onEvent = {});

    // Updates the title to "<title> - X.XX ms (YYY FPS)" at most twice a
    // second. Call once per frame with that frame's delta time.
    void updateTitle(float deltaTimeSeconds);

    VkSurfaceKHR createSurface(VkInstance instance) const;
    std::vector<const char*> getRequiredInstanceExtensions() const;

    // Current drawable size in pixels (accounts for HiDPI scaling).
    void getFramebufferSize(int& width, int& height) const;

    SDL_Window* handle() const { return window_; }
    bool shouldClose() const { return quitRequested_; }

    // Returns true exactly once after a resize event, then clears the flag.
    bool consumeResizedFlag();

private:
    SDL_Window* window_ = nullptr;
    bool quitRequested_ = false;
    bool resized_ = false;

    std::string baseTitle_;
    float titleUpdateTimer_ = 0.0f;
    int titleUpdateFrames_ = 0;
};

} // namespace core
