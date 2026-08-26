#pragma once

#include <glm/vec3.hpp>

namespace game {

// Placeholder for game/engine logic, decoupled from core/ and renderer/.
// main.cpp calls update() once per frame.
class Game {
public:
    Game() = default;

    void update(float deltaTimeSeconds);

    const glm::vec3& cameraPosition() const { return cameraPosition_; }

private:
    glm::vec3 cameraPosition_{0.0f, 0.0f, 0.0f};
};

} // namespace game
