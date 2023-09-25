#include "physics.h"
#include "../components/render_components.h"
#include "flecs.h"
#include <cmath>
#include <iostream>

using namespace physics;

physics::Vertex physics::getClosestVertex(Position p, Radius r, Mountain *m) {
    float_type x_min = p.x - r.value;
    float_type x_max = p.x + r.value;
    auto interval = Mountain::getRelevantMountainSection(x_min, x_max);

    auto closest_index = interval.start_index;
    float_type closest_distance =
        m->getVertex(interval.start_index).distanceTo(p);

    for (auto j = interval.start_index; j < interval.end_index; j++) {
        auto mountain_vertex = m->getVertex(j);
        auto current_dist = mountain_vertex.distanceTo(p);

        if (current_dist < closest_distance) {
            closest_distance = current_dist;
            closest_index = j;
        }
    }

    return Vertex({closest_index, closest_distance});
}

Vector physics::getNormal(std::size_t idx, Position rock_pos, Mountain *m) {
    // determine closer vertex
    Position vertex_other =
        m->getVertex((idx - 1) % Mountain::NUMBER_OF_VERTICES);
    Position vertex_right =
        m->getVertex((idx + 1) % Mountain::NUMBER_OF_VERTICES);
    if (rock_pos.distanceTo(vertex_other) > rock_pos.distanceTo(vertex_right)) {
        // else already correct
        vertex_other = vertex_right;
    }
    // calc distances
    Position vertex = m->getVertex(idx);
    Vector d = vertex - vertex_other;
    // compute normal from distances via rotation
    // signbit is used to let normal vector always point in positive y direction
    float_type sgn_n_x = ((float_type)std::signbit(-d.y) - 0.5f) * 2.f;
    // R =  (  0   -1  )
    //      (  1    0  )
    Vector n = {sgn_n_x * -d.y, sgn_n_x * d.x};
    float_type normalization = std::sqrt(n * n);

    if (n.y < 0) {
        n = n * (-1);
    }
    return n / normalization;
}

bool physics::isCollided(Position p1, Position p2, Radius r1, Radius r2) {
    return (p1.distanceTo(p2) <= r1.value + r2.value);
}

void physics::terrainCollision(flecs::iter it, Position *positions,
                               Velocity *velocities, Radius *r, Mountain *m) {
    for (auto i : it) {
        auto closest_vertex = getClosestVertex(positions[i], r[i], m);
        auto vertex_position = m->getVertex(closest_vertex.index);
        if (closest_vertex.distance > r[i].value) {
            continue;
        }
        auto normal_vector = getNormal(closest_vertex.index, positions[i], m);
        velocities[i] = (Velocity)velocities[i].reflectOnNormal(normal_vector);

        float_type vertex_normal = vertex_position * normal_vector;
        float_type position_normal = positions[i] * normal_vector;
        float_type velocity_normal = velocities[i] * normal_vector;

        float_type terrain_exit_time =
            (r[i].value + vertex_normal - position_normal) / velocity_normal;

        positions[i] += velocities[i] * terrain_exit_time + EPSILON;
    }
}

void physics::makeRock(const flecs::world &world, Position p, Velocity v,
                       float_type radius) {
    world.entity()
        .set<Position>(p)
        .set<Velocity>(v)
        .set<Radius>({radius})
        .add<Rock>()
        .set<graphics::CircleShapeRenderComponent>({radius});
}

void physics::rockCollision(Position &p1, Position &p2, Velocity &v1,
                            Velocity &v2, const Radius R1, const Radius R2,
                            float dt) {
    float_type m1 = R1.value * R1.value;
    float_type m2 = R2.value * R2.value;

    p1 -= v1 * dt;
    p2 -= v2 * dt;

    Vector vel_diff_vector = v1 - v2;
    Vector pos_diff_vector = p1 - p2;
    float_type distance_sq = pos_diff_vector * pos_diff_vector;

    // Velocity Update
    float_type total_mass = m1 + m2;
    v1 -= pos_diff_vector * 2 * m2 * (vel_diff_vector * pos_diff_vector) /
          (distance_sq * total_mass + EPSILON);
    v2 += pos_diff_vector * 2 * m1 * (vel_diff_vector * pos_diff_vector) /
          (distance_sq * total_mass + EPSILON);
}

void physics::rockRockInteractions(flecs::iter it, Position *positions,
                                   Velocity *velocities, Radius *radius) {
    for (int i = 0; i < it.count(); i++) {
        for (int j = i + 1; j < it.count(); j++) {
            if (isCollided(positions[i], positions[j], radius[i], radius[j])) {
                rockCollision(positions[i], positions[j], velocities[i],
                              velocities[j], radius[i], radius[j],
                              it.delta_time()); // TODO: Optimization?
            }
        }
    }
}

void physics::updateRockState(flecs::iter it, Position *positions,
                              Velocity *velocities) {
    updateRockVelocity(it, velocities);
    updateRockPosition(it, positions, velocities);
}

void physics::updateRockVelocity(flecs::iter it, Velocity *velocities) {
    for (auto i : it) {
        velocities[i].y += GRAVITATIONAL_CONSTANT * it.delta_time();
        if (velocities[i].length() > VELOCITY_CAP) {
            velocities[i] = (Velocity) (velocities[i] * VELOCITY_CAP / velocities[i].length());
        }
    }
}

void physics::updateRockPosition(flecs::iter it, Position *positions,
                                 Velocity *velocities) {
    for (auto i : it) {
        positions[i] += velocities[i] * it.delta_time();
    }
}

void physics::updatePlayerState(flecs::iter it, Position *positions,
                                Velocity *velocities,
                                PlayerMovement *player_movements,
                                InputEntity *input_entities, Height *heights,
                                Width *widths) {

    updatePlayerVelocity(it, positions, velocities, player_movements,
                         input_entities, heights);
    updatePlayerPosition(it, positions, velocities, player_movements);
}

void physics::updatePlayerVelocity(flecs::iter it, Position *positions,
                                   Velocity *velocities,
                                   PlayerMovement *player_movements,
                                   InputEntity *input_entities,
                                   Height *heights) {

    checkJumpEvent(velocities, player_movements, input_entities);
    checkDuckEvent(velocities, player_movements, input_entities, heights);
    checkXMovement(velocities, player_movements, input_entities);
    checkAerialState(it, velocities, player_movements, input_entities);
    checkDirection(velocities, player_movements, input_entities);
}

void physics::updatePlayerPosition(flecs::iter it, Position *positions,
                                   Velocity *velocities,
                                   PlayerMovement *player_movements) {
    positions[0].x += velocities[0].x * it.delta_time();
    auto terrain_y = getYPosFromX(it.world(), positions[0].x);
    if (player_movements[0].current_state ==
        PlayerMovement::MovementState::IN_AIR) {
        auto air_y = positions[0].y + velocities[0].y * it.delta_time();
        if (air_y > terrain_y) {
            positions[0].y = air_y;
        } else {
            positions[0].y = terrain_y;
            player_movements[0].current_state =
                PlayerMovement::MovementState::MOVING;
            player_movements[0].can_jump_again = true;
        }
    } else {
        positions[0].y = terrain_y;
    }
}

void physics::checkJumpEvent(Velocity *velocities,
                             PlayerMovement *player_movements,
                             InputEntity *input_entities) {
    if (input_entities->getEvent(Event::JUMP)) {
        float factor = 1;
        if (player_movements[0].current_state ==
            PlayerMovement::MovementState::DUCKED) {
            factor = DUCK_SPEED_FACTOR;
        }
        if (player_movements[0].current_state !=
            PlayerMovement::MovementState::IN_AIR) {
            player_movements[0].last_jump = 0;
        }
        if (player_movements[0].last_jump < 1.5 &&
            player_movements[0].can_jump_again) {
            velocities[0].y = JUMP_VELOCITY_CONSTANT * factor;
            if (player_movements[0].current_state ==
                PlayerMovement::MovementState::IN_AIR) {
                player_movements[0].can_jump_again = false;
            }
            player_movements[0].current_state =
                PlayerMovement::MovementState::IN_AIR;
        }
    }
}

// TODO not ducking anymore
void physics::checkDuckEvent(Velocity *velocities,
                             PlayerMovement *player_movements,
                             InputEntity *input_entities, Height *heights) {
    if (input_entities->getEvent(Event::DUCK) &&
        player_movements[0].current_state !=
            PlayerMovement::MovementState::IN_AIR) {
        velocities[0].x *= DUCK_SPEED_FACTOR;
        player_movements[0].current_state =
            PlayerMovement::MovementState::DUCKED;
        // heights[0].h = HIKER_HEIGHT/2;
    }
}

void physics::checkXMovement(Velocity *velocities,
                             PlayerMovement *player_movements,
                             InputEntity *input_entities) {
    double x_factor = input_entities->getAxis(Axis::MOVEMENT_X);
    if (player_movements[0].current_state !=
        PlayerMovement::MovementState::IN_AIR) {
        if (x_factor == 0) {
            player_movements[0].current_state =
                PlayerMovement::MovementState::IDLE;
        } else {
            player_movements[0].current_state =
                PlayerMovement::MovementState::MOVING;
        }
    }
    velocities[0].x = NORMAL_SPEED * (float_type)x_factor;
}

void physics::checkAerialState(flecs::iter it, Velocity *velocities,
                               PlayerMovement *player_movements,
                               InputEntity *input_entities) {
    if (player_movements[0].current_state ==
        PlayerMovement::MovementState::IN_AIR) {
        player_movements[0].last_jump += it.delta_time();
        velocities[0].y += GRAVITATIONAL_CONSTANT * it.delta_time();
    }
}

void physics::checkDirection(Velocity *velocities,
                             PlayerMovement *player_movements,
                             InputEntity *input_entities) {
    if (velocities[0].x < 0) {
        player_movements[0].current_direction = PlayerMovement::Direction::LEFT;
    } else if (velocities[0].x > 0) {
        player_movements[0].current_direction =
            PlayerMovement::Direction::RIGHT;
    } else {
        player_movements[0].current_direction =
            PlayerMovement::Direction::NEUTRAL;
    }
}

float physics::getYPosFromX(const flecs::world &world, float x) {
    auto mountain = world.get_mut<Mountain>();
    auto interval = Mountain::getRelevantMountainSection(x, x);
    std::size_t closest_indices[] = {interval.start_index, interval.end_index};
    auto closest_left_distance =
        std::abs(mountain->getVertex(interval.start_index).x - x);
    auto closest_right_distance =
        std::abs(mountain->getVertex(interval.end_index).x - x);

    for (auto j = interval.start_index; j < interval.end_index; j++) {
        auto mountain_vertex = mountain->getVertex(j);
        auto current_dist = mountain_vertex.x - x;

        if (current_dist < 0 &&
            std::abs(current_dist) < closest_left_distance) {
            closest_indices[0] = j;
            closest_left_distance = std::abs(current_dist);
        } else if (current_dist > 0 &&
                   std::abs(current_dist) < closest_right_distance) {
            closest_indices[1] = j;
            closest_right_distance = std::abs(current_dist);
        }
    }

    auto vertex_left = mountain->getVertex(closest_indices[0]);
    auto vertex_right = mountain->getVertex(closest_indices[1]);

    return ((x - vertex_left.x) * vertex_right.y +
            (vertex_right.x - x) * vertex_left.y) /
               (vertex_right.x - vertex_left.x) +
           HIKER_HEIGHT;
}

void physics::checkPlayerIsHit(flecs::iter rock_it, Position *rock_positions,
                               Radius *radii) {
    rock_it.world()
        .filter_builder<Width, Height, Position, Health>()
        .with<Player>()
        .build()
        .iter([&](flecs::iter player_it, Width *widths, Height *heights,
                  Position *player_positions, Health *healths) {
            auto w = widths[0].w;
            auto h = heights[0].h;
            auto player_position = player_positions[0];
            bool is_hit = false;
            for (auto i : rock_it) {
                auto rock_position = rock_positions[i];
                auto x_centre_distance =
                    std::abs(player_position.x - rock_position.x);
                auto y_centre_distance =
                    std::abs(player_position.y - rock_position.y);
                auto r = radii[i].value;

                if (x_centre_distance > w / 2 + r ||
                    y_centre_distance > h / 2 + r) {
                } else if (x_centre_distance <= w / 2 ||
                           y_centre_distance <= h / 2) {
                    is_hit = true;
                } else {

                    auto corner_distance_sq =
                        std::pow(x_centre_distance - w / 2, 2) +
                        std::pow(y_centre_distance - h / 2, 2);

                    if (corner_distance_sq <= std::pow(r, 2)) {
                        is_hit = true;
                    }
                }
                if (is_hit) {
                    int rock_dmg =
                        std::abs(49 * (radii[i].value - MIN_ROCK_SIZE)) /
                            (MAX_ROCK_SIZE - MIN_ROCK_SIZE) +
                        1;
                    healths[0].hp -= rock_dmg;
                    if (healths[0].hp <= 0) {
                        // TODO end animation or sth.
                        std::cout << "Player unalive" << std::endl;
                        rock_it.world().get_mut<AppInfo>()->isRunning = false;
                    }
                }
            }
        });
}

PhysicSystems::PhysicSystems(flecs::world &world) {
    world.module<PhysicSystems>();

    world.system<Position, Velocity>().with<Rock>().multi_threaded(true).iter(
        updateRockState);

    world.system<Position, Velocity, Radius>().with<Rock>().iter(
        rockRockInteractions);

    world.system<Position, Radius>().with<Rock>().iter(checkPlayerIsHit);

    world.system<Position, Velocity, Radius, Mountain>()
        .term_at(4)
        .singleton()
        .iter(terrainCollision);

    world
        .system<Position, Velocity, PlayerMovement, InputEntity, Height,
                Width>()
        .with<Player>()
        .iter(updatePlayerState);

    // for (int i = 0; i < 20; i++) {
    //     Position p{300.f + 200.f, 25.f * (float)i};
    //     Velocity v{0., 0.};
    //     makeRock(world, p, v, 10.f);
    // }
}
