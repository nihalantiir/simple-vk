#pragma once

#include <glm/vec3.hpp>

namespace game {

// Placeholder for game/engine logic, deliberately decoupled from the
// Vulkan bootstrap (core/) and rendering (renderer/) code. Grow this into
// scene state, input handling, and gameplay systems; the render loop in
// main.cpp already calls update() once per frame.
class Game {
public:
    Game() = default;

    void update(float deltaTimeSeconds);

    const glm::vec3& cameraPosition() const { return cameraPosition_; }

private:
    glm::vec3 cameraPosition_{0.0f, 0.0f, 0.0f};
};

} // namespace game
