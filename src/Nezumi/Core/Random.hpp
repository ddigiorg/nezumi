// =============================================================================
// Random.hpp
//
// Pseudo Random Number Generator (PRNG) object.
// =============================================================================

#pragma once
#include "Nezumi/Core/RandomConfig.hpp"
#include <random>
#include <algorithm>
#include <sstream>
#include <string>
#include <type_traits>
#include <stdexcept>

namespace Nezumi {

class Random {
public:

    // -------------------------------------------------------------------------
    // Constructor
    // -------------------------------------------------------------------------

    // Seed from the OS entropy source
    Random() {
        std::random_device rd;
        std::seed_seq seq{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
        engine_.seed(seq);
    }

    // Seed deterministically (for testing / reproducibility)
    Random(uint64_t seed) {
        seed_deterministic_(seed);
    }

    Random(const RandomConfig& config) {
        seed_deterministic_(config.seed);
    }

    // Non-copyable
    Random(const Random&) = default;
    Random& operator=(const Random&) = default;

    // -------------------------------------------------------------------------
    // Re-seeding
    // -------------------------------------------------------------------------

    // Re-seed from OS
    void re_seed() {
        std::random_device rd;
        std::seed_seq seq{ rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd() };
        engine_.seed(seq);
    }

    // Re-seed deterministically
    void re_seed(uint64_t seed) { seed_deterministic_(seed); }

    // -------------------------------------------------------------------------
    // Get random values
    // -------------------------------------------------------------------------

    // Get random boolean value with optional probability of returning true
    //   rng.get_bool(0.5);
    bool get_bool(double probability = 0.5) {
        std::bernoulli_distribution dist(probability);
        return dist(engine_);
    }

    // Get random integer value in range [min, max] inclusive
    //   rng.get<int>(-1, 1);
    template<typename T>
    typename std::enable_if<std::is_integral<T>::value, T>::type
    get(T min = 0, T max = 1) {
        std::uniform_int_distribution<T> dist(min, max);
        return dist(engine_);
    }

    // Get random float or double value in range [min, max] inclusive
    //   rng.get<float>(1.0f, 1.0f);
    template<typename T>
    typename std::enable_if<std::is_floating_point<T>::value, T>::type
    get(T min = 0.0, T max = 1.0) {
        std::uniform_real_distribution<T> dist(min, max);
        return dist(engine_);
    }

    // Get weighted random index from weights
    //   std::vector<int> weights = { 70, 20, 10 };
    //   rng.get_weighted_index(weights); // returns 70% 0, 20% 1, or 10% 2
    template<typename T>
    std::size_t get_weighted_index(std::initializer_list<T> weights) {
        std::discrete_distribution<std::size_t> dist(weights);
        return dist(engine_);
    }

    // Get weighted random index from iterators
    //   std::vector<int> weights = { 70, 20, 10 };
    //   rng.get_weighted_index(weights.begin(), weights.end());
    template<typename Iterator>
    std::size_t get_weighted_index(Iterator first, Iterator last) {
        std::discrete_distribution<std::size_t> dist(first, last);
        return dist(engine_);
    }

    // Get random element value from a container
    //   std::vector<int> container = { 10, 20, 30 };
    //   rng.pick_from(container);
    template<typename Container>
    auto& pick_from(Container& container) {
        if (container.empty())
            throw std::invalid_argument("Container is empty");
        auto it = container.begin();
        std::advance(it, get<std::size_t>(0, container.size() - 1));
        return *it;
    }

    // Get random element value from a read-only container
    //   const std::vector<int> container = { 10, 20, 30 };
    //   rng.pick_from(container);
    template<typename Container>
    const auto& pick_from(const Container& container) {
        if (container.empty())
            throw std::invalid_argument("Container is empty");
        auto it = container.begin();
        std::advance(it, get<std::size_t>(0, container.size() - 1));
        return *it;
    }

    // Shuffle a container in place
    //   std::vector<int> container = { 10, 20, 30 };
    //   rng.shuffle(container);
    template<typename Container>
    void shuffle(Container& container) {
        std::shuffle(container.begin(), container.end(), engine_);
    }

    // -------------------------------------------------------------------------
    // Utility
    // -------------------------------------------------------------------------

    // Snapshot for game saves and replays
    std::string snapshot() const {
        std::ostringstream oss;
        oss << engine_;
        return oss.str();
    }

    // Restore for game saves and replays
    void restore(const std::string& state) {
        std::istringstream iss(state);
        iss >> engine_;
    }

    // Raw engine, for std:: algorithms that take a URBG directly
    std::mt19937_64& engine() { return engine_; }

private:

    // -------------------------------------------------------------------------
    // Seed Deterministic
    // -------------------------------------------------------------------------

    void seed_deterministic_(uint64_t seed) {
        // Mix to avoid a weak all-zero state
        seed ^= seed << 33;
        seed ^= seed >> 17;
        seed ^= seed << 23;
        engine_.seed(seed);
    }

    // -------------------------------------------------------------------------
    // Members
    // -------------------------------------------------------------------------

    std::mt19937_64 engine_;
};

} // namespace Nezumi
