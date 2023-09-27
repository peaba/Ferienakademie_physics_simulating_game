#pragma once

#include "flecs.h"
#include "string"
#include <raylib.h>

namespace graphics {

constexpr char WINDOW_NAME[] = "Surviving Sarntal";
constexpr bool DEBUG = true;

// to be called once at app startup
void initRenderSystem(const flecs::world &world);

// prepares resources, call before render*
void prepareGame(const flecs::world &world);
void prepareMenu(const flecs::world &world);
void prepareHUD(const flecs::world &world);

// call per frame to render the current frame
void renderSystem(flecs::iter &iter);
void renderHUD(flecs::iter &iter);
void renderMenu(flecs::iter &iter);

void generateChunkMesh(const flecs::world &world);
Vector3 computeNormal(Vector3 p1, Vector3 p2, Vector3 p3);
void handleWindow(flecs::world &world);
void destroy();

} // namespace graphics
