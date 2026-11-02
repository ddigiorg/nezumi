// =============================================================================
// tests/test_particles/main.cpp
//
// Headless checks for RingBuffer (FIFO order, overwrite of the oldest when
// full, pop/clear) and ParticlePool (aging, expiry, gravity, drag, capacity).
// =============================================================================

#include "Nezumi/Object/RingBuffer.hpp"
#include "Nezumi/Render/2D/Particles/ParticlePool.hpp"
#include <cmath>   // abs
#include <cstdio>  // printf
#include <cstdlib> // EXIT_FAILURE, EXIT_SUCCESS
#include <vector>  // vector

using namespace Nezumi;
using namespace Nezumi::Render;

static int g_failures = 0;

#define CHECK(cond)                                                          \
    do {                                                                     \
        if (!(cond)) {                                                       \
            std::printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);    \
            g_failures++;                                                    \
        }                                                                    \
    } while (0)

static bool near(float a, float b) { return std::abs(a - b) < 1e-4f; }

static void test_ring_buffer() {
    std::printf("ring buffer keeps FIFO order and overwrites the oldest\n");
    RingBuffer<int> ring(3);
    CHECK(ring.empty() && ring.capacity() == 3);

    ring.push(1); ring.push(2); ring.push(3);
    CHECK(ring.full() && ring.front() == 1 && ring.back() == 3);

    ring.push(4); // drops 1
    CHECK(ring.size() == 3 && ring[0] == 2 && ring[1] == 3 && ring[2] == 4);

    ring.pop_front();
    CHECK(ring.size() == 2 && ring.front() == 3);

    ring.push(5); ring.push(6); // wraps, drops 3
    std::vector<int> seen;
    ring.for_each([&](int v) { seen.push_back(v); });
    CHECK((seen == std::vector<int>{ 4, 5, 6 }));

    ring.clear();
    CHECK(ring.empty());
    ring.push(7);
    CHECK(ring.size() == 1 && ring.front() == 7);
}

static void test_particles_age_and_expire() {
    std::printf("particles age, move and expire\n");
    ParticlePool pool({ .capacity = 8, .gravity = { 0.0f, 0.0f } });

    pool.spawn({ .velocity = { 1.0f, 0.0f }, .lifetime = 0.5f });
    pool.spawn({ .velocity = { 0.0f, 2.0f }, .lifetime = 1.0f });

    pool.update(0.25f);
    CHECK(pool.count() == 2);

    std::vector<Particle> live;
    pool.for_each([&](const Particle& p) { live.push_back(p); });
    CHECK(live.size() == 2 && near(live[0].position.x, 0.25f) && near(live[1].position.y, 0.5f));

    pool.update(0.25f); // the first one dies
    CHECK(pool.count() == 1);
    pool.update(0.5f);  // and the second
    CHECK(pool.count() == 0);
}

static void test_particles_gravity_and_drag() {
    std::printf("gravity applies only to flagged particles; drag slows all\n");
    ParticlePool pool({ .capacity = 8, .gravity = { 0.0f, -10.0f }, .drag = 0.5f });

    pool.spawn({ .velocity = { 2.0f, 0.0f }, .lifetime = 10.0f, .flags = ParticleFlags::Gravity });
    pool.spawn({ .velocity = { 2.0f, 0.0f }, .lifetime = 10.0f });
    pool.update(0.1f);

    std::vector<Particle> live;
    pool.for_each([&](const Particle& p) { live.push_back(p); });
    CHECK(live.size() == 2);
    CHECK(near(live[0].velocity.y, -1.0f * 0.95f)); // gravity, then drag
    CHECK(near(live[1].velocity.y, 0.0f));
    CHECK(near(live[1].velocity.x, 2.0f * 0.95f));
}

static void test_particles_full_pool() {
    std::printf("a full pool overwrites its oldest particle\n");
    ParticlePool pool({ .capacity = 4 });
    for (int i = 0; i < 6; i++)
        pool.spawn({ .position = { static_cast<float>(i), 0.0f }, .lifetime = 10.0f });

    pool.update(0.0f);
    CHECK(pool.count() == 4);

    std::vector<float> xs;
    pool.for_each([&](const Particle& p) { xs.push_back(p.position.x); });
    CHECK((xs == std::vector<float>{ 2.0f, 3.0f, 4.0f, 5.0f }));
}

int main() {
    test_ring_buffer();
    test_particles_age_and_expire();
    test_particles_gravity_and_drag();
    test_particles_full_pool();

    if (g_failures) {
        std::printf("%d check(s) failed\n", g_failures);
        return EXIT_FAILURE;
    }
    std::printf("all passed\n");
    return EXIT_SUCCESS;
}
