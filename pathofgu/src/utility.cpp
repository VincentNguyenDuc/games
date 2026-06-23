#include "utility.hpp"
#include "components/position.hpp"

#include <random>

static int chebyshev(const Position& a, const Position& b) {
    return std::max(std::abs(a.x - b.x), std::abs(a.y - b.y));
}

static int random_variance(int base) {
    static std::mt19937 gen(std::random_device{}());
    std::uniform_int_distribution<int> dist(-base / 4, base / 4);
    return base + dist(gen);
}
