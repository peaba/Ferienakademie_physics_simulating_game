#include "physics.h"
#include "../components/render_components.h"
#include "flecs.h"
#include <algorithm>
#include <cmath>
#include <iostream>

using namespace physics;

physics::ClosestVertex physics::getClosestVertex(Position p, Radius r,
                                                 Mountain *m) {
    float_type x_min = p.x - r.value;
    float_type x_max = p.x + r.value;
    auto interval = m->getRelevantMountainSection(x_min, x_max);

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

    return ClosestVertex({closest_index, closest_distance});
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
    // R =  (  0   -1  )
    //      (  1    0  )
    Vector n = {-d.y, d.x};
    if (n.y < 0) {n =  n * -1.f;}
    float_type normalization = std::sqrt(n * n);
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

        if (it.entity(i).has<Exploding>()) {
            explodeRock(it.world(), it.entity(i), 50);
        }
    }
}

flecs::entity physics::makeRock(const flecs::world &world, Position p, Velocity v,
                       float_type radius) {
    return world.entity()
        .set<Position>(p)
        .set<Velocity>(v)
        .set<Radius>({radius})
        .add<Rock>()
        .set<graphics::CircleShapeRenderComponent>({radius});
}

void physics::rockCollision(Position &p1, Position &p2, Velocity &v1,
                            Velocity &v2, const Radius R1, const Radius R2) {
    float_type m1 = R1.value * R1.value;
    float_type m2 = R2.value * R2.value;
    float_type radius_sum = R1.value + R2.value;

    Vector vel_diff_vector = v1 - v2;
    Vector pos_diff_vector = p1 - p2;
    float_type distance_sq = pos_diff_vector * pos_diff_vector;
    Vector normal_vector = pos_diff_vector / (distance_sq + EPSILON);

    // Velocity Update
    float_type total_mass = m1 + m2;
    v1 -= pos_diff_vector * 2 * m2 * (vel_diff_vector * pos_diff_vector) /
          (distance_sq * total_mass + EPSILON);
    v2 += pos_diff_vector * 2 * m1 * (vel_diff_vector * pos_diff_vector) /
          (distance_sq * total_mass + EPSILON);

    // Position update using new velocities
    float_type overlap = radius_sum - pos_diff_vector.length() + EPSILON;
    float_type overlap1 = overlap * m2 / (m1 + m2);
    float_type overlap2 = overlap * m1 / (m1 + m2);

    p1 += normal_vector * overlap1;
    p2 -= normal_vector * overlap2;
}

void physics::rockRockInteractions(flecs::iter it, Position *positions,
                                   Velocity *velocities, Radius *radius) {
    for (int i = 0; i < it.count(); i++) {
        for (int j = i + 1; j < it.count(); j++) {
            if (isCollided(positions[i], positions[j], radius[i], radius[j])) {
                rockCollision(positions[i], positions[j], velocities[i],
                              velocities[j], radius[i],
                              radius[j]); // TODO: Optimization?

                if (it.entity(i).has<Exploding>()) {
                    std::cout << "eskalation" << std::endl;
                    explodeRock(it.world(), it.entity(i), 5);
                }
            }
        }
    }
}

void physics::updateState(flecs::iter it, Position *positions,
                          Velocity *velocities) {
    updateVelocity(it, velocities);
    updatePosition(it, positions, velocities);
}

void physics::updateVelocity(flecs::iter it, Velocity *velocities) {
    for (auto i : it) {
        velocities[i].y += GRAVITATIONAL_CONSTANT * it.delta_time();
    }
}

void physics::updatePosition(flecs::iter it, Position *positions,
                             Velocity *velocities) {
    for (auto i : it) {
        positions[i] += velocities[i] * it.delta_time();
    }
}

void physics::explodeRock(const flecs::world& world,
                          flecs::entity rock, const int number_of_rocks) {
    auto position = *rock.get<Position>();
    auto velocity = *rock.get<Velocity>();
    auto radius = rock.get<Radius>()->value;
    auto M = radius * radius;
    rock.destruct();

    float_type new_r = radius / std::sqrt(number_of_rocks);
    float_type delta_angle = 2.0 * PI / number_of_rocks;

    Position delta_direction, p;
    Velocity v;
    for (int i = 0; i < number_of_rocks; i++) {
        delta_direction = Position{ std::sin(delta_angle * i),
                                    std::cos(delta_angle * i)};
        p = (Position)(position + delta_direction * radius);
        v = (Velocity)(velocity + delta_direction * 10);
        makeRock(world, p, v, new_r);
    }
}

PhysicSystems::PhysicSystems(flecs::world &world) {
    world.module<PhysicSystems>();

    world.system<Position, Velocity>().with<Rock>().iter(updateState);

    world.system<Position, Velocity, Radius>().with<Rock>().iter(
        rockRockInteractions);

    world.system<Position, Velocity, Radius, Mountain>()
        .term_at(4)
        .singleton()
        .iter(terrainCollision);

    for (int i = 0; i < 1; i++) {
        Position p{300.f, 400.f + 25.f * (float)i};
        Velocity v{0., 0.};
        makeRock(world, p, v, 40.f).add<Exploding>();
    }
}
