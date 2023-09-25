#include "components/mountain.h"
#include "components/particle_state.h"
#include "raylib.h"
#include "systems/game_logic.h"
#include "systems/input_systems.h"
#include "systems/physics.h"
#include "systems/render_systems.h"
#include <flecs.h>
#include <iostream>

void mainLoop(flecs::world &world);

int main() {
    std::cout << "surviving sarntal" << std::endl;

    flecs::world world;

#ifdef kinect
    std::cout << "Kinect is active" << std::endl;
    kinect_mode = true;
#endif

    graphics::initRenderSystem(world);

    world.import <PhysicSystems>();
    world.import <InputSystems>();

    world.set<Mountain>({});

    initGameLogic(world);

    world.set<AppInfo>({});

    mainLoop(world);

    graphics::destroy();
}

void mainLoop(flecs::world &world) {
    world.progress(0);

    SetTargetFPS(1000);

    auto app_info = world.get<AppInfo>();

    while (app_info->isRunning) {
        float dt = GetFrameTime();
        std::cout << "Frame Rate:" << 1 / dt << std::endl;
        world.progress(dt);
    }
}
